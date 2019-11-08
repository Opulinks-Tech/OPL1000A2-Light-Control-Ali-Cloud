/*
 * Copyright (C) 2015-2018 Alibaba Group Holding Limited
 */
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "transport.h"
#include "core.h"
#include "common.h"
#include "utils.h"
#include "ble_service.h"

#include "breeze_hal_sec.h"
#include "breeze_hal_os.h"

#define HEADER_SIZE 4
#define AES_BLK_SIZE 16

#define IS_ENC(data) ((data[0] & 0x10) != 0)
#define MSG_ID(data) (data[0] & 0xf)
#define CMD_TYPE(data) (data[1])
#define TOTAL_FRAME(data) ((data[2] >> 4) & 0x0f)
#define FRAME_SEQ(data) (data[2] & 0x0f)
#define FRAME_LEN(data) (data[3])

extern bool g_dn_complete;
transport_t g_transport;
struct rx_cmd_post_t rx_cmd_post;

void reset_tx(void)
{
    g_transport.tx.len = 0;
    g_transport.tx.bytes_sent = 0;
    g_transport.tx.msg_id = 0;
    g_transport.tx.cmd = 0;
    g_transport.tx.total_frame = 0;
    g_transport.tx.frame_seq = 0;
    g_transport.tx.pkt_req = 0;
    g_transport.tx.pkt_cfm = 0;
    memset(g_transport.tx.buff, 0, sizeof(g_transport.tx.buff));
    if (g_transport.timeout != 0) {
 //       os_timer_stop(&g_transport.tx.timer);
    }
}

void reset_rx(void)
{
    g_transport.rx.cmd = 0;
    g_transport.rx.total_frame = 0;
    g_transport.rx.frame_seq = 0;
    g_transport.rx.bytes_received = 0;
    memset(g_transport.rx.buff, 0, sizeof(g_transport.rx.buff));
    if (g_transport.timeout != 0) {
//        os_timer_stop(&g_transport.rx.timer);
    }
}

//static void on_tx_timeout(void *arg1, void *arg2)
//{
//    BREEZE_LOG_ERR("tx timeout\r\n");
//    reset_tx();
//}

//static void on_rx_timeout(void *arg1, void *arg2)
//{
//    BREEZE_LOG_ERR("rx timeout\r\n");
//    reset_rx();
//}

static bool is_valid_rx_command(uint8_t cmd) {
    if (cmd == BZ_CMD_CTRL ||
        cmd == BZ_CMD_QUERY ||
        cmd == BZ_CMD_EXT_DOWN ||
        cmd == BZ_CMD_AUTH_REQ ||
        cmd == BZ_CMD_AUTH_CFM ||
        cmd == BZ_CMD_OTA_VER_REQ ||
        cmd == BZ_CMD_OTA_REQ ||
        cmd == BZ_CMD_OTA_SIZE ||
        cmd == BZ_CMD_OTA_DONE ||
        cmd == BZ_CMD_OTA_DATA) {
        return true;
    }
    return false;
}

static bool is_valid_tx_command(uint8_t cmd) {
    if (cmd == BZ_CMD_STATUS ||
        cmd == BZ_CMD_REPLY ||
        cmd == BZ_CMD_EXT_UP ||
        cmd == BZ_CMD_AUTH_RAND ||
        cmd == BZ_CMD_AUTH_RSP ||
        cmd == BZ_CMD_AUTH_KEY ||
        cmd == BZ_CMD_OTA_VER_RSP ||
        cmd == BZ_CMD_OTA_RSP ||
        cmd == BZ_CMD_OTA_PUB_SIZE ||
        cmd == BZ_CMD_OTA_CHECK_RESULT ||
        cmd == BZ_CMD_OTA_UPDATE_PROCESS ||
        cmd == BZ_CMD_ERR) {
        return true;
    }
    return false;
}

static void do_encrypt(uint8_t *data, uint16_t len)
{
    uint16_t bytes_to_pad, blk_num = len >> 4;
//    uint8_t *decrypt_buf;
    uint8_t encrypt_data[ENCRYPT_DATA_SIZE];

    bytes_to_pad = (AES_BLK_SIZE - len % AES_BLK_SIZE) % AES_BLK_SIZE;
#ifdef ALI_OPL_DBG	
		printf("bytes_to_pad=%d\r\n", bytes_to_pad);
#endif	
    if (bytes_to_pad) {
        memset(data + len, 0, bytes_to_pad);
        g_transport.tx.zeroes_padded = bytes_to_pad;
        blk_num++;
        g_transport.tx.buff[3] += bytes_to_pad;
    }
    ais_aes128_cbc_encrypt(g_transport.p_aes_ctx, data, blk_num, encrypt_data);
    memcpy(data, encrypt_data, blk_num << 4);
}

static void do_decrypt(uint8_t *data, uint16_t len)
{
    uint16_t blk_num = len >> 4;
//    uint8_t *buffer;
    uint8_t decrypt_data[ENCRYPT_DATA_SIZE];

    ais_aes128_cbc_decrypt(g_transport.p_aes_ctx, data, blk_num, decrypt_data);
    memcpy(data, decrypt_data, len);
}

static uint32_t build_packet(uint8_t *data, uint16_t len)
{
    uint32_t ret = BZ_SUCCESS;

    g_transport.tx.zeroes_padded = 0;
    g_transport.tx.buff[0] = ((BZ_TRANSPORT_VER & 0x7) << 5) |
                             ((g_transport.tx.encrypted & 0x1) << 4) |
                             (g_transport.tx.msg_id & 0xF);
    g_transport.tx.buff[1] = g_transport.tx.cmd;
    g_transport.tx.buff[2] = ((g_transport.tx.total_frame & 0x0F) << 4) |
                             (g_transport.tx.frame_seq & 0x0F);
    g_transport.tx.buff[3] = len;

    /* Payload */
    if (len != 0) {
        memcpy(g_transport.tx.buff + HEADER_SIZE, data, len);
        if (g_transport.tx.encrypted != 0) {
            do_encrypt(g_transport.tx.buff + HEADER_SIZE, len);
        }
    }

    if(g_dn_complete == false){
        g_transport.tx.buff[0] &= (~(0x01 <<4));
    }

    if (g_dn_complete == true){
        g_transport.tx.buff[3] = len;
    }

    return ret;
}

static uint16_t tx_bytes_left(void)
{
    return (g_transport.tx.len - g_transport.tx.bytes_sent);
}

static bool rx_frames_left(void)
{
    return (g_transport.rx.total_frame != g_transport.rx.frame_seq);
}
extern bool g_noti_flag;
extern bool g_Indi_flag;
static ret_code_t send_fragment(uint8_t tx_type)
{
    ret_code_t ret = BZ_SUCCESS;
    uint16_t len, pkt_len, bytes_left;
    uint16_t pkt_payload_len = g_transport.max_pkt_size - HEADER_SIZE;
    uint16_t pkt_sent = 0;

    bytes_left = tx_bytes_left();
    if (g_transport.tx.encrypted != 0) {
        pkt_payload_len &= ~(AES_BLK_SIZE - 1);
    }

    do {
        len = MIN(bytes_left, pkt_payload_len);
        build_packet(g_transport.tx.data + g_transport.tx.bytes_sent, len);
        pkt_len = len + g_transport.tx.zeroes_padded + HEADER_SIZE;
#ifdef ALI_OPL_DBG			
				printf("pkt_len=%d\r\n", pkt_len);
				printf("WriteToDevice: ");
				for(int i=0;i<pkt_len;i++){
					printf("0x%02X ", g_transport.tx.buff[i]);
				}
				printf("\r\n");
#endif				
//        ret = g_transport.tx.active_func(g_transport.tx.buff, pkt_len);
        ret = g_transport.tx.active_func(g_transport.tx.conn_hdl, g_transport.tx.hdl, pkt_len, g_transport.tx.buff); //Modify
        if (ret == BZ_SUCCESS) {
            g_transport.tx.pkt_req++;
            g_transport.tx.frame_seq++;
            g_transport.tx.bytes_sent += len;
            bytes_left = tx_bytes_left();
#ifdef ALI_OPL_DBG					
						printf("bytes_left=%d\r\n", bytes_left);
#endif					
            pkt_sent++;
        }
 //       if (ret != BZ_SUCCESS ||
 //           g_transport.tx.active_func == ble_ais_send_indication) {
				if (ret != BZ_SUCCESS || (tx_type == TX_INDICATION && true==g_Indi_flag)) {			
            break;
        }
    }  while (bytes_left > 0);

    if ((bytes_left != 0) && (g_transport.timeout != 0)) {
//        os_timer_start(&g_transport.tx.timer);
    }
 //   if (g_transport.tx.active_func == ble_ais_send_notification) {
		if (tx_type == TX_NOTIFICATION && true==g_noti_flag) {
        transport_txdone(tx_type, pkt_sent);
    }
    return ret;
}

static void trans_rx_dispatcher(void)
{
    if (!is_valid_rx_command(g_transport.rx.cmd)) {
        return;
    }

    if((g_transport.rx.cmd & BZ_CMD_TYPE_MASK) == BZ_CMD_AUTH){
#if BZ_ENABLE_AUTH
        auth_rx_command(g_transport.rx.cmd, g_transport.rx.buff, g_transport.rx.bytes_received);
#endif
    } else if(g_transport.rx.cmd == BZ_CMD_EXT_DOWN){
#if BZ_ENABLE_COMBO_NET
        extcmd_rx_command(g_transport.rx.cmd, g_transport.rx.buff, g_transport.rx.bytes_received);
#endif
    } else {
        rx_cmd_post.cmd = g_transport.rx.cmd;
        rx_cmd_post.frame_seq = g_transport.rx.frame_seq + 1;
        rx_cmd_post.p_rx_buf =  g_transport.rx.buff;
        rx_cmd_post.buf_sz = g_transport.rx.bytes_received;
//        event_notify(BZ_CMD_CTX_INFO, (uint8_t *)&rx_cmd_post, sizeof(rx_cmd_post));
    }
}

ret_code_t transport_init(ali_init_t const *p_init)
{
    /* Initialize context */
    memset(&g_transport, 0, sizeof(transport_t));
    g_transport.max_pkt_size = GATT_MTU_SIZE_DEFAULT - 3;
    g_transport.timeout = p_init->transport_timeout;

    if (g_transport.timeout != 0) {
//        os_timer_new(&g_transport.tx.timer, on_tx_timeout, &g_transport, g_transport.timeout);
//        os_timer_new(&g_transport.rx.timer, on_rx_timeout, &g_transport, g_transport.timeout);
    }
    return BZ_SUCCESS;
}

void transport_reset(void)
{
    reset_tx();
    reset_rx();
    auth_reset(); //Kevin add it!
    ais_aes128_destroy(g_transport.p_aes_ctx);
    g_transport.p_aes_ctx = NULL;
    g_dn_complete = false;
}

ret_code_t transport_tx(uint8_t tx_type, uint8_t cmd,
                        uint8_t const *const p_data, uint16_t length, transport_tx_func_t active_func, int conn_hdl, int hdl)
{
    uint16_t pkt_payload_len;

    if (p_data == NULL && length != 0) {
        return BZ_ENULL;
    }

    if (g_transport.p_key != NULL &&
        (cmd == BZ_CMD_STATUS || cmd == BZ_CMD_REPLY || cmd == BZ_CMD_EXT_UP ||
         ((cmd & BZ_CMD_TYPE_MASK) == BZ_CMD_AUTH && cmd != BZ_CMD_AUTH_RAND))) {
        g_transport.tx.encrypted = 1;
        pkt_payload_len = (g_transport.max_pkt_size - HEADER_SIZE) & ~(AES_BLK_SIZE - 1);
    } else {
        g_transport.tx.encrypted = 0;
        pkt_payload_len = g_transport.max_pkt_size - HEADER_SIZE;
    }

    if (tx_bytes_left() != 0 ||
        g_transport.tx.pkt_req != g_transport.tx.pkt_cfm) {
        return BZ_EBUSY;
    }

    g_transport.tx.data = (uint8_t *)p_data;
    g_transport.tx.len = length;
    g_transport.tx.bytes_sent = 0;
    g_transport.tx.cmd = cmd;
    g_transport.tx.frame_seq = 0;
    g_transport.tx.pkt_req = 0;
    g_transport.tx.pkt_cfm = 0;

    if (cmd == BZ_CMD_REPLY || cmd == BZ_CMD_EXT_UP) {
        g_transport.tx.msg_id = g_transport.rx.msg_id;
    }

    if (cmd == BZ_CMD_STATUS) {
        g_transport.tx.msg_id = 0;
    }

    g_transport.tx.total_frame = length / pkt_payload_len;
    if (g_transport.tx.total_frame * pkt_payload_len == length && length != 0) {
        g_transport.tx.total_frame--;
    }   
   
    if (tx_type == TX_NOTIFICATION) {
    g_transport.tx.active_func = active_func;
		g_transport.tx.hdl=hdl;
		g_transport.tx.conn_hdl=conn_hdl;	
    } else if(tx_type == TX_INDICATION){
    g_transport.tx.active_func = active_func;
		g_transport.tx.hdl=hdl;
		g_transport.tx.conn_hdl=conn_hdl;	
    }
/*    if (tx_type == TX_NOTIFICATION) {
        g_transport.tx.active_func = ble_ais_send_notification;
    } else {
        g_transport.tx.active_func = ble_ais_send_indication;
    }
*/



    send_fragment(tx_type);
    return BZ_SUCCESS;
}

void transport_rx(uint8_t *p_data, uint16_t length)
{
    uint16_t len, buff_left;
//    uint32_t err_code;

    if (length == 0) {
        return;
    } else if ((length - HEADER_SIZE + g_transport.rx.bytes_received) > RX_BUFF_LEN) {
        core_handle_err(ALI_ERROR_SRC_TRANSPORT_RX_BUFF_SIZE, BZ_EDATASIZE);
        reset_rx();
        return;
    }

    if (!rx_frames_left()) {
        if (FRAME_SEQ(p_data) != 0) {
            core_handle_err(ALI_ERROR_SRC_TRANSPORT_1ST_FRAME, BZ_EINVALIDDATA);
            reset_rx();
            return;
        }

        g_transport.rx.msg_id = MSG_ID(p_data);
        g_transport.rx.cmd = CMD_TYPE(p_data);
        g_transport.rx.total_frame = TOTAL_FRAME(p_data);
        g_transport.rx.frame_seq = 0;
        g_transport.rx.bytes_received = 0;
    } else {
        if ((g_transport.rx.msg_id != MSG_ID(p_data)) ||
            (g_transport.rx.cmd != CMD_TYPE(p_data)) ||
            (g_transport.rx.total_frame != TOTAL_FRAME(p_data)) ||
            (((g_transport.rx.frame_seq + 1) & 0xF) != FRAME_SEQ(p_data) &&
             g_transport.rx.cmd != BZ_CMD_OTA_DATA)) {
            core_handle_err(ALI_ERROR_SRC_TRANSPORT_OTHER_FRAMES, BZ_EINVALIDDATA);
            reset_rx();
            return;
        } else if (((g_transport.rx.frame_seq + 1) & 0xF) != FRAME_SEQ(p_data) &&
                    g_transport.rx.cmd == BZ_CMD_OTA_DATA) {
            core_handle_err(ALI_ERROR_SRC_TRANSPORT_FW_DATA_DISC, BZ_EINVALIDDATA);
            reset_rx();
            return;
        } else {
            g_transport.rx.frame_seq = FRAME_SEQ(p_data);
        }
    }

    if (IS_ENC(p_data) != 0) {
        if ((length - HEADER_SIZE) % 16 != 0) {
            core_handle_err(ALI_ERROR_SRC_TRANSPORT_ENCRYPTED, BZ_EINVALIDDATA);
            reset_rx();
            return;
        }
        if (g_transport.p_key == NULL) {
            core_handle_err(ALI_ERROR_SRC_TRANSPORT_ENCRYPTED, BZ_EFORBIDDEN);
            reset_rx();
            return;
        }
    }

    if ((length != HEADER_SIZE + FRAME_LEN(p_data) && IS_ENC(p_data) == 0)
        || (length < HEADER_SIZE + FRAME_LEN(p_data) && IS_ENC(p_data) != 0)) {
        core_handle_err(ALI_ERROR_SRC_TRANSPORT_OTHER_FRAMES, BZ_EDATASIZE);
        reset_rx();
        return;
    }

    buff_left = RX_BUFF_LEN - g_transport.rx.bytes_received;
    if ((len = MIN(buff_left, FRAME_LEN(p_data))) > 0) {
        if (IS_ENC(p_data) != 0) {
            do_decrypt(p_data + HEADER_SIZE, length - HEADER_SIZE);
        }
        memcpy(g_transport.rx.buff + g_transport.rx.bytes_received, p_data + HEADER_SIZE, len);
        g_transport.rx.bytes_received += len;
    }
    if (!rx_frames_left()) {
        trans_rx_dispatcher();
        reset_rx();
    } else {
        if (g_transport.timeout != 0) {
//            os_timer_start(&g_transport.rx.timer);
        }
    }
}

void transport_txdone(uint8_t tx_type, uint16_t pkt_sent)
{
//    uint32_t err_code = BZ_SUCCESS;
    uint16_t bytes_left;

    g_transport.tx.pkt_cfm += pkt_sent;
    bytes_left = tx_bytes_left();
#ifdef ALI_OPL_DBG	
		printf("g_transport.tx.pkt_cfm=%d\r\n", g_transport.tx.pkt_cfm);
#endif		
    if (bytes_left != 0) {
//        send_fragment();
     send_fragment(tx_type);
    } else if (g_transport.tx.pkt_req == g_transport.tx.pkt_cfm &&
               g_transport.tx.pkt_req != 0) {
        if (!is_valid_tx_command(g_transport.tx.cmd)) {
            return;
        }
//        event_notify(BZ_EVENT_TX_DONE, &g_transport.tx.cmd, sizeof(g_transport.tx.cmd));
        reset_tx();
#if BZ_ENABLE_AUTH
        auth_tx_done();
#endif
    } else if (g_transport.tx.pkt_req < g_transport.tx.pkt_cfm) {
        reset_tx();
        core_handle_err(ALI_ERROR_SRC_TRANSPORT_PKT_CFM_SENT, BZ_EINTERNAL);
    }
}

uint32_t transport_update_key(uint8_t *key)
{
    char *iv = "0123456789ABCDEF";

    g_transport.p_key = key;
    if (g_transport.p_aes_ctx) {
        ais_aes128_destroy(g_transport.p_aes_ctx);
        g_transport.p_aes_ctx = NULL;
    }

    g_transport.p_aes_ctx = ais_aes128_init(g_transport.p_key, (const uint8_t*)iv);
    return BZ_SUCCESS;
}
