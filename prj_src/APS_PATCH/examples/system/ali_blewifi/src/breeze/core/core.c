/*
 * Copyright (C) 2015-2018 Alibaba Group Holding Limited
 */

#include <string.h>
#include "blewifi_ble_api.h"

#include "core.h"
#include "transport.h"
#include "auth.h"
#include "extcmd.h"
#include "common.h"
#include "ble_service.h"
#include "breeze_hal_ble.h"
#include "bzopt.h"
#include "ble_gap_if.h"

#include "utils.h"
#ifdef CONFIG_AIS_SECURE_ADV
#include "sha256.h"
#endif
#include <stdlib.h>
#include "infra_config.h"
//#include "blewifi_ctrl.h"

SHM_DATA core_t g_core;
extern auth_t g_auth;

#ifdef CONFIG_AIS_SECURE_ADV
#define AIS_SEQ_KV_KEY      "ais_adv_seq"
#define AIS_SEQ_UPDATE_FREQ (1 * 60 * 60) /* in second uint */
//static uint32_t g_seq = 0;
uint32_t g_seq = 0;
static os_timer_t g_secadv_timer;
#endif

//static ali_init_t const *g_ali_init;

/**
 * @brief Callback when there is device status to set.
 *
 * @param[in] buffer @n The data to be set.
 * @param[in] model @n Length of the data.
 * @return None.
 * @see None.
 * @note This API should be implemented by user and will be called by SDK.
 */
typedef void (*set_dev_status_cb)(uint8_t *buffer, uint32_t length);

/**
 * @brief Callback when there is device status to get.
 *
 * @param[out] buffer @n The data of device status.
 * @param[out] model @n Length of the data.
 * @return None.
 * @see None.
 * @note This API should be implemented by user and will be called by SDK.
 */
typedef void (*get_dev_status_cb)(uint8_t *buffer, uint32_t length);


/***** BLE STATUS ******/
typedef enum {
    CONNECTED,     // connect with phone success
    DISCONNECTED,  // lost connection with phone
    AUTHENTICATED,                            // success authentication, security key auth
    TX_DONE,                                  // send user payload data complete
    EVT_USER_BIND,                            // user binded, has AuthCode
    EVT_USER_UNBIND,                          // user unbind, no AuthCode
    EVT_USER_SIGNED,                          // user AuthCode sign pass
    NONE
} breeze_event_t;

static void combo_set_dev_status_handler(uint8_t *buffer, uint32_t length)
{
    printf("\nCOM_SET:%.*s\n\n", length, buffer);
    //BleWifi_Ctrl_MsgSend(BLEWIFI_CTRL_MSG_NETWORKING_STOP, NULL, 0);
}

static void combo_get_dev_status_handler(uint8_t *buffer, uint32_t length)
{
    printf("\nCOM_GET:%.*s\n\n", length, buffer);
    //BleWifi_Ctrl_MsgSend(BLEWIFI_CTRL_MSG_NETWORKING_STOP, NULL, 0);
}

static get_dev_status_cb m_query_handler = combo_get_dev_status_handler;
static set_dev_status_cb m_ctrl_handler = combo_set_dev_status_handler;

uint8_t g_ble_state = 0;
uint8_t g_bind_state;
extern int awss_clear_reset(void);
static void notify_status(breeze_event_t event)
{
 switch (event) {
        case CONNECTED:
            printf("\n\nConnected\n");
            g_ble_state = 1;
            break;

        case DISCONNECTED:
            printf("\n\nDisconnected\n");
            g_ble_state = 0;
//            aos_post_event(EV_BZ_COMBO, COMBO_EVT_CODE_RESTART_ADV, 0);
            break;

        case AUTHENTICATED:
            printf("\n\nAuthenticated\n");
            break;

        case TX_DONE:
            break;

        case EVT_USER_BIND:
            printf("\n\nBle bind\n");
            g_bind_state = 1;
            awss_clear_reset();
            break;

        case EVT_USER_UNBIND:
            printf("\n\nBle unbind\n");
            g_bind_state = 0;
            break;

        default:
            break;
    }
}

static void event_handler(ali_event_t *p_event)
{
//    uint32_t err_code;    
#if BZ_ENABLE_OTA
    bool b_notify_upper = false;
    breeze_otainfo_t m_disc_evt;
#endif

    switch (p_event->type) {
        case BZ_EVENT_CONNECTED:
            notify_status(CONNECTED);
#if BZ_ENABLE_OTA
            g_disconnect_by_ota = false;
#endif
            break;

        case BZ_EVENT_DISCONNECTED:
            core_reset();
            notify_status(DISCONNECTED);
#if BZ_ENABLE_OTA
            m_disc_evt.type = OTA_EVT;
            m_disc_evt.cmd_evt.m_evt.evt = ALI_OTA_ON_DISCONNECTED;
            m_disc_evt.cmd_evt.m_evt.d = 0;
            b_notify_upper = true;
            if(g_disconnect_by_ota == true){
                //do nothing here as expected
            }
#endif
            break;

        case BZ_EVENT_AUTHENTICATED:
            notify_status(AUTHENTICATED);
#if BZ_ENABLE_OTA
            m_disc_evt.type = OTA_EVT;
            m_disc_evt.cmd_evt.m_evt.evt = ALI_OTA_ON_AUTH_EVT;
            m_disc_evt.cmd_evt.m_evt.d = 1;
            b_notify_upper = true;
#endif
            break;

        case BZ_EVENT_TX_DONE:
            notify_status(TX_DONE);
#if BZ_ENABLE_OTA
            uint8_t cmd = *p_event->rx_data.p_data;
            if (cmd == BZ_CMD_OTA_CHECK_RESULT || cmd == BZ_CMD_ERR || cmd == BZ_CMD_OTA_PUB_SIZE) {
                m_disc_evt.type = OTA_EVT;
                m_disc_evt.cmd_evt.m_evt.evt = ALI_OTA_ON_TX_DONE;
                m_disc_evt.cmd_evt.m_evt.d = cmd;
                b_notify_upper = true;
            }

            /*there is a special case here to handle, for the last disconnected event caused by OTA, 
            * advertising will be confusing, which will be postponed till reboot*/
            if(cmd ==BZ_CMD_OTA_CHECK_RESULT){
                g_disconnect_by_ota = true;
            }
#endif
            break;

        case BZ_EVENT_RX_INFO:
            if(p_event->rx_data.p_data != NULL){
                struct rx_cmd_post_t *r_cmd  = (struct rx_cmd_post_t*) p_event->rx_data.p_data;
                uint8_t cmd = r_cmd ->cmd;	
                if(cmd == BZ_CMD_QUERY){
                    if (m_query_handler != NULL) {
                        m_query_handler(r_cmd->p_rx_buf, r_cmd->buf_sz);
                    }
                } else if(cmd == BZ_CMD_CTRL){
                    if (m_ctrl_handler != NULL) {
                        m_ctrl_handler (r_cmd->p_rx_buf, r_cmd->buf_sz);
                    }
                }else if((cmd & BZ_CMD_TYPE_MASK) == BZ_CMD_TYPE_OTA){
#if BZ_ENABLE_OTA
                    m_disc_evt.type = OTA_CMD;
                    m_disc_evt.cmd_evt.m_cmd.cmd = r_cmd->cmd;
                    m_disc_evt.cmd_evt.m_cmd.frame = r_cmd->frame_seq;
                    m_disc_evt.cmd_evt.m_cmd.len = r_cmd->buf_sz;
                    memcpy(m_disc_evt.cmd_evt.m_cmd.data, r_cmd->p_rx_buf, r_cmd->buf_sz);
                    b_notify_upper = true;
#endif
                }
            }
            break;

        case BZ_EVENT_APINFO:
#if BZ_ENABLE_COMBO_NET
//            if(m_apinfo_handler != NULL){
//                m_apinfo_handler(p_event->rx_data.p_data);
//	        }
#endif
            break;

        case BZ_EVENT_AC_AS:
            if (p_event->rx_data.p_data != NULL) {
                if ( (p_event->rx_data.p_data[0] == BZ_AC_AS_ADD) 
                    || (p_event->rx_data.p_data[0] == BZ_AC_AS_UPDATE)) {
                    notify_status(EVT_USER_BIND);
                } else if (p_event->rx_data.p_data[0] == BZ_AC_AS_DELETE) {
                    notify_status(EVT_USER_UNBIND);
                }
            }
            break;

        case BZ_EVENT_AUTH_SIGN:
            if (p_event->rx_data.p_data != NULL) {
                if (p_event->rx_data.p_data[0] == BZ_AUTH_SIGN_NO_CHECK_PASS) {
                    // do nothing, for future use
                } else if (p_event->rx_data.p_data[0] == BZ_AUTH_SIGN_CHECK_PASS) {
                    notify_status(EVT_USER_SIGNED);
                }
            }
            break;
        
#if BZ_ENABLE_OTA
        case BZ_EVENT_ERR_DISCONT:
            m_disc_evt.type = OTA_EVT;
            m_disc_evt.cmd_evt.m_evt.evt = ALI_OTA_ON_DISCONTINUE_ERR;
            m_disc_evt.cmd_evt.m_evt.d = 0;
            b_notify_upper = true;
	    break;
#endif
        default:
            break;
    }
#if BZ_ENABLE_OTA
    if(b_notify_upper && (m_ota_dev_handler != NULL)){
        m_ota_dev_handler(&m_disc_evt);
    }
#endif
}


void core_event_notify(uint8_t event_type, uint8_t *data, uint16_t length)
{
    ali_event_t event;

    event.type = event_type;
    event.rx_data.p_data = data;
    event.rx_data.length = length;
//    printf("\n[core_event_notify]event.type=%d\n\n", event.type);
//    g_core.event_handler(&event);
    event_handler(&event);
}


extern uint32_t tx_func_indicate(uint8_t cmd, uint8_t *p_data, uint16_t length);
/*
static uint32_t tx_func_indicate(uint8_t cmd, uint8_t *p_data, uint16_t length)
{
    return transport_tx(TX_INDICATION, cmd, p_data, length);
}

static uint32_t ais_init(ali_init_t const *p_init)
{
    ble_ais_init_t init_ais;

    memset(&init_ais, 0, sizeof(ble_ais_init_t));
    init_ais.mtu = p_init->max_mtu;
    return ble_ais_init(&init_ais);
}
*/
#ifdef CONFIG_AIS_SECURE_ADV
static void update_seq(void *arg1, void *arg2)
{
    os_kv_set(AIS_SEQ_KV_KEY, &g_seq, sizeof(g_seq), 1);
    os_timer_start(&g_secadv_timer);
}

static void init_seq_number(uint32_t *seq)
{
    int len = sizeof(uint32_t);

    if (!seq)
        return;

    if (os_kv_get(AIS_SEQ_KV_KEY, seq, &len) != 0) {
        *seq = 0;
        len  = sizeof(uint32_t);
        os_kv_set(AIS_SEQ_KV_KEY, seq, len, 1);
    }

    os_timer_new(&g_secadv_timer, update_seq, NULL, AIS_SEQ_UPDATE_FREQ);
    os_timer_start(&g_secadv_timer);
}

void set_adv_sequence(uint32_t seq)
{
    g_seq = seq;
    os_kv_set(AIS_SEQ_KV_KEY, &g_seq, sizeof(g_seq), 1);
}
#endif

ret_code_t core_init(ali_init_t const *p_init)
{
    // breeze core base infomation init
    memset(&g_core, 0, sizeof(core_t));
    g_core.event_handler = p_init->event_handler;
    memcpy(g_core.adv_mac, p_init->adv_mac, BZ_BT_MAC_LEN);
    g_core.product_id = p_init->model_id;
    // core device info init
    if((p_init->product_key.p_data != NULL) && (p_init->product_key.length > 0)) {
        g_core.product_key_len = p_init->product_key.length;
        memcpy(g_core.product_key, p_init->product_key.p_data, g_core.product_key_len);
    }
    if((p_init->product_secret.p_data != NULL) && (p_init->product_secret.length > 0)) {
        g_core.product_secret_len = p_init->product_secret.length;
        memcpy(g_core.product_secret, p_init->product_secret.p_data, g_core.product_secret_len);
    }
    if((p_init->device_name.p_data != NULL) && (p_init->device_name.length > 0)) {
        g_core.device_name_len = p_init->device_name.length;
        memcpy(g_core.device_name, p_init->device_name.p_data, g_core.device_name_len);
    }
    if((p_init->device_secret.p_data != NULL) && (p_init->device_secret.length > 0)) {
        g_core.device_secret_len = p_init->device_secret.length;
        memcpy(g_core.device_secret, p_init->device_secret.p_data, g_core.device_secret_len);
    }

#ifdef CONFIG_AIS_SECURE_ADV
    init_seq_number(&g_seq);
#endif

//    ais_init(p_init);
    transport_init(p_init);

#if BZ_ENABLE_AUTH
    ali_auth_init(p_init, tx_func_indicate);
#endif

    extcmd_init(p_init, tx_func_indicate);

    return BZ_SUCCESS;
}


void core_reset(void)
{
#if BZ_ENABLE_AUTH
    auth_reset();
#endif
    transport_reset();
    g_core.admin_checkin = 0;
    g_core.guest_checkin = 0;
}


extern uint32_t tx_func_notify(uint8_t cmd, uint8_t *p_data, uint16_t length);
void core_handle_err(uint8_t src, uint8_t code)
{
    uint8_t err;

    BREEZE_ERR("err at 0x%04x, code 0x%04x", src, code);
    switch (src & BZ_ERR_MASK) {
        case BZ_TRANS_ERR:
            if (code != BZ_EINTERNAL) {
                if (src == ALI_ERROR_SRC_TRANSPORT_FW_DATA_DISC) {
                    core_event_notify(BZ_EVENT_ERR_DISCONT, NULL, 0);
                }
//                err = transport_tx(TX_NOTIFICATION, BZ_CMD_ERR, NULL, 0);
                err = tx_func_notify(BZ_CMD_ERR, NULL, 0);
                if (err != BZ_SUCCESS) {
                    BREEZE_ERR("err at 0x%04x, code 0x%04x", ALI_ERROR_SRC_TRANSPORT_SEND, code);
                }
            }
            break;
#if BZ_ENABLE_AUTH
        case BZ_AUTH_ERR:
            printf("\nBZ_AUTH_ERR\n\n");
            auth_reset();
            if (code == BZ_ETIMEOUT) {
//                ble_disconnect(AIS_BT_REASON_REMOTE_USER_TERM_CONN);
                BleWifi_Ble_Disconnect();
            }
            break;
#endif
        case BZ_EXTCMD_ERR:
            printf("\nBZ_EXTCMD_ERR\n\n");
            break;
        default:
            printf("unknow bz err\r\n");
            break;
    }
}

void core_create_bz_adv_data(uint8_t sub_type, uint8_t sec_type, uint8_t bind_state)
{
    uint16_t idx;
    uint8_t version = 0;
    uint8_t fmsk = 0;
    char* p;
    char* ver_str;
    char *pSavedPtr = NULL;

    SET_U16_LE(g_core.adv_data, BZ_ALI_COMPANY_ID);
    idx = sizeof(uint16_t);

    // extract Breeze version from BZ_VERSION
    char t_ver_info[20] = { 0 };
    strncpy(t_ver_info, BZ_VERSION, sizeof(t_ver_info) - 1);
    p = strtok_r(t_ver_info, ".", &pSavedPtr);
    
    while(p!=NULL){
        ver_str = p;
        p = strtok_r(NULL, ".", &pSavedPtr);
    }
    version = (uint8_t)atoi(ver_str);
    g_core.adv_data[idx++] = (version<<BZ_SDK_VER_Pos) | (sub_type<<BZ_SUB_TYPE_Pos);

    // FMSK byte
    fmsk = BZ_BLUETOOTH_VER << BZ_FMSK_BLUETOOTH_VER_Pos;
#if BZ_ENABLE_OTA
    fmsk |= 1 << BZ_FMSK_OTA_Pos;
#endif
#if BZ_ENABLE_AUTH
    fmsk |= 1 << BZ_FMSK_SECURITY_Pos;
    if(sec_type == BZ_SEC_TYPE_DEVICE) {
        BREEZE_DEBUG("Breeze adv per device");
        fmsk |= 1 << BZ_FMSK_SECRET_TYPE_Pos;
        g_auth.auth_type = BZ_AUTH_TYPE_PER_DEV;
    } else if (sec_type == BZ_SEC_TYPE_PRODUCT) {
        BREEZE_DEBUG("Breeze adv per product");
        fmsk &= ~(1 << BZ_FMSK_SECRET_TYPE_Pos);
        g_auth.auth_type = BZ_AUTH_TYPE_PER_PK;
    } else {
        BREEZE_ERR("Breeze adv sec type err");
        g_auth.auth_type = BZ_AUTH_TYPE_NONE;
    }
#endif
#ifdef CONFIG_AIS_SECURE_ADV
    fmsk |= 1 << BZ_FMSK_SEC_ADV_Pos;
#endif
    if(bind_state && (version >= 6)){
        BREEZE_DEBUG("Breeze binded");
        fmsk |= 1 << BZ_FMSK_BIND_STATE_Pos;
    } else{
        BREEZE_DEBUG("Breeze unbind");
        fmsk &= ~(1 << BZ_FMSK_BIND_STATE_Pos);
    }   

    g_core.adv_data[idx++] = fmsk;

    SET_U32_LE(g_core.adv_data + idx, g_core.product_id);
    idx += sizeof(uint32_t);

    if (g_core.adv_mac[0]==0 && g_core.adv_mac[1]==0 && g_core.adv_mac[2]==0
        && g_core.adv_mac[3]==0 && g_core.adv_mac[4]==0 && g_core.adv_mac[5]==0) {
        LeGapGetBdAddr(g_core.adv_mac);
//        ble_get_mac(g_core.adv_mac);
    }
    memcpy(&g_core.adv_data[idx], g_core.adv_mac, 6);
    idx += 6;
    g_core.adv_data_len = idx;
}


ret_code_t core_get_bz_adv_data(uint8_t *p_data, uint16_t *length)
{
#ifdef CONFIG_AIS_SECURE_ADV
    if (*length < (g_core.adv_data_len + 4 + 4)) {
#else
    if (*length < g_core.adv_data_len) {
#endif
        return BZ_ENOMEM;
    }

#ifdef CONFIG_AIS_SECURE_ADV
    uint8_t  sign[4];
    uint32_t seq;

    seq = (++g_seq);
#if BZ_ENABLE_AUTH
    auth_calc_adv_sign(seq, sign);
#endif
    memcpy(p_data, g_core.adv_data, g_core.adv_data_len);
    memcpy(p_data + g_core.adv_data_len, sign, 4);
    memcpy(p_data + g_core.adv_data_len + 4, &seq, 4);
    *length = g_core.adv_data_len + 4 + 4;
#else
    memcpy(p_data, g_core.adv_data, g_core.adv_data_len);
    *length = g_core.adv_data_len;
#endif

    return BZ_SUCCESS;
}

