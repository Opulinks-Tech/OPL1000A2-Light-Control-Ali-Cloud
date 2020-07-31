/*
 * Copyright (C) 2015-2018 Alibaba Group Holding Limited
 */
#include "dev_bind_internal.h"

#if defined(__cplusplus)  /* If this is a C++ compiler, use C linkage */
extern "C" {
#endif

#define AWSS_RESET_PKT_LEN         (256)
#define AWSS_RESET_TOPIC_LEN       (128)
#define AWSS_RESET_MSG_ID_LEN      (16)

#define TOPIC_RESET_REPORT         "/sys/%s/%s/thing/reset"
#define TOPIC_RESET_REPORT_REPLY   "/sys/%s/%s/thing/reset_reply"
#define METHOD_RESET_REPORT        "thing.reset"

#define AWSS_RESET_REQ_FMT         "{\"id\":%s, \"version\":\"1.0\", \"method\":\"%s\", \"params\":{\"resetKey\":{\"devReset\":\"%d\"}}}"

#define AWSS_KV_RST                "awss.rst"
#define AWSS_KV_RST_TYPE           "awss.rst.type"

#ifdef DEV_BIND_ENABLED
extern int awss_start_bind(void);
#endif
iotx_vendor_dev_reset_type_t g_reset_type = IOTX_VENDOR_DEV_RESET_TYPE_UNBIND_SHADOW_CLEAR;
static uint8_t awss_report_reset_suc = 0;
static uint16_t awss_report_reset_id = 0;
static void *report_reset_timer = NULL;

int awss_report_reset_to_cloud(iotx_vendor_dev_reset_type_t *reset_type);
extern int HAL_Kv_Del(const char *key);

int awss_handle_reset_cloud_reply(void)
{
    awss_report_reset_suc = 1;
    HAL_Kv_Del(AWSS_KV_RST);
    HAL_Kv_Del(AWSS_KV_RST_TYPE);

    if (report_reset_timer)
    {
        HAL_Timer_Stop(report_reset_timer);
        HAL_Timer_Delete(report_reset_timer);
        report_reset_timer = NULL;
    }

//    AWSS_RST_UPDATE_STATIS(AWSS_RST_STATIS_SUC);

    iotx_event_post(IOTX_RESET);  // for old version of event
    do {  // for new version of event
        void *cb = NULL;
        cb = (void *)iotx_event_callback(ITE_AWSS_STATUS);
        if (cb == NULL) break;
        ((int (*)(int))cb)(IOTX_RESET);
    } while (0);

//    AWSS_RST_DISP_STATIS();

    return 0;
}

void awss_report_reset_reply(void *pcontext, void *pclient, void *mesg)
{
//    char rst = 0;

    iotx_mqtt_event_msg_pt msg = (iotx_mqtt_event_msg_pt)mesg;

    switch (msg->event_type) {
        case IOTX_MQTT_EVENT_PUBLISH_RECEIVED:
            break;
        default:
            return;
    }

    devrst_debug("[RST]", "%s\r\n", __func__);

    awss_handle_reset_cloud_reply();

#if 0
    awss_report_reset_suc = 1;
    HAL_Kv_Set(AWSS_KV_RST, &rst, sizeof(rst), 0);

    if(report_reset_timer)
    {
        HAL_Timer_Stop(report_reset_timer);
        HAL_Timer_Delete(report_reset_timer);
        report_reset_timer = NULL;
    }
    else
    {
        timer_dbg("[%s %d] report_reset_timer is null\n", __func__, __LINE__);
    }

    iotx_event_post(IOTX_RESET);

#ifdef INFRA_EVENT
    iotx_event_post(IOTX_RESET);  /* for old version of event */
    do {  /* for new version of event */
        void *cb = NULL;
        cb = (void *)iotx_event_callback(ITE_AWSS_STATUS);
        if (cb == NULL) {
            break;
        }
        ((int (*)(int))cb)(IOTX_RESET);
    } while (0);
#endif

#ifdef DEV_BIND_ENABLED
    //awss_start_bind();
#endif

#endif
}

void awss_awss_report_reset_to_cloud_timer(void)
{
	hal_ali_netlink_Task_MsgSend(ALI_NETLINK_AWSS_REPORT_RESET_TO_CLOUD, (void*)&g_reset_type, 1);
}

int awss_report_reset_to_cloud(iotx_vendor_dev_reset_type_t *reset_type)
{
    int ret = -1;
    int final_len = 0;
    char *topic = NULL;
    char *packet = NULL;
    int packet_len = AWSS_RESET_PKT_LEN;
    int topic_len = AWSS_RESET_TOPIC_LEN;

    if (awss_report_reset_suc) {
        return 0;
    }

    g_reset_type = *reset_type;


    if (report_reset_timer == NULL) {
//        report_reset_timer = HAL_Timer_Create("report_rst", (void (*)(void *))awss_report_reset_to_cloud, NULL);
        report_reset_timer = HAL_Timer_Create("report_rst", (void (*)(void *))awss_awss_report_reset_to_cloud_timer, &g_reset_type);
    }
    HAL_Timer_Stop(report_reset_timer);
    HAL_Timer_Start(report_reset_timer, 3000);
    do {
        char pk[IOTX_PRODUCT_KEY_LEN + 1] = {0};
        char dn[IOTX_DEVICE_NAME_LEN + 1] = {0};

        HAL_GetProductKey(pk);
        HAL_GetDeviceName(dn);

        topic = (char *)devrst_malloc(topic_len + 1);
        if (topic == NULL) {
            goto REPORT_RST_ERR;
        }
        memset(topic, 0, topic_len + 1);

        HAL_Snprintf(topic, topic_len, TOPIC_RESET_REPORT_REPLY, pk, dn);

        ret = IOT_MQTT_Subscribe(NULL, topic, IOTX_MQTT_QOS0,
                                      (iotx_mqtt_event_handle_func_fpt)awss_report_reset_reply, NULL);
        if (ret < 0) {
            goto REPORT_RST_ERR;
        }

        memset(topic, 0, topic_len + 1);
        HAL_Snprintf(topic, topic_len, TOPIC_RESET_REPORT, pk, dn);
    } while (0);

    packet = devrst_malloc(packet_len + 1);
    if (packet == NULL) {
        ret = -1;
        goto REPORT_RST_ERR;
    }
    memset(packet, 0, packet_len + 1);

    do {
        char id_str[AWSS_RESET_MSG_ID_LEN + 1] = {0};
        HAL_Snprintf(id_str, AWSS_RESET_MSG_ID_LEN, "\"%u\"", awss_report_reset_id ++);
        final_len = HAL_Snprintf(packet, packet_len, AWSS_RESET_REQ_FMT, id_str, METHOD_RESET_REPORT, g_reset_type);
    } while (0);

    devrst_debug("[RST]", "reset_type=%d, report reset:%s\r\n", g_reset_type, packet);

    ret = IOT_MQTT_Publish_Simple(NULL, topic, IOTX_MQTT_QOS0, packet, final_len);
    devrst_debug("[RST]", "report reset result:%d\r\n", ret);

REPORT_RST_ERR:
    if (packet) {
        devrst_free(packet);
    }
    if (topic) {
        devrst_free(topic);
    }
    return ret;
}

int awss_report_reset(iotx_vendor_dev_reset_type_t *reset_type)
{
    char rst = 0x01;
    int ret;
    awss_report_reset_suc = 0;
    iotx_vendor_dev_reset_type_t l_reset_type = IOTX_VENDOR_DEV_RESET_TYPE_UNBIND_SHADOW_CLEAR;

    if (reset_type == NULL)
    {
        log_warning("RST", "reset_type is NULL use default:%d", l_reset_type);
    }
    else
    {
        l_reset_type = *((iotx_vendor_dev_reset_type_t *)reset_type);
    }

//    if (l_reset_type < IOTX_VENDOR_DEV_RESET_TYPE_UNBIND_ONLY || l_reset_type > IOTX_VENDOR_DEV_RESET_TYPE_UNBIND_ALL_CLEAR)
    if (l_reset_type > IOTX_VENDOR_DEV_RESET_TYPE_UNBIND_ALL_CLEAR)
    {
        l_reset_type = IOTX_VENDOR_DEV_RESET_TYPE_UNBIND_SHADOW_CLEAR;
    }

    ret = HAL_Kv_Set(AWSS_KV_RST, &rst, sizeof(rst), 0);
    if (ret < 0) {
//        dump_dev_bind_status(STATE_SYS_DEPEND_KV_SET, "set reset kv flag fail");
    }

    ret = HAL_Kv_Set(AWSS_KV_RST_TYPE, reset_type, sizeof(iotx_vendor_dev_reset_type_t), 0);
    if (ret < 0) {
//        dump_dev_bind_status(STATE_SYS_DEPEND_KV_SET, "set reset_type kv flag fail");
    }

    ret = awss_report_reset_to_cloud(&l_reset_type);

    log_info("RST", "awss report reset cloud ret:%d", ret);

    return ret;

#if 0
    //char rst = 0x01;

    awss_report_reset_suc = 0;

    //HAL_Kv_Set(AWSS_KV_RST, &rst, sizeof(rst), 0);

    return awss_report_reset_to_cloud();
#endif
}

int awss_check_reset(iotx_vendor_dev_reset_type_t *reset_type)
{
    int len = 1;
    char rst = 0;

    int ret = HAL_Kv_Get(AWSS_KV_RST, &rst, &len);

    if (rst != 0x01)
    { // reset flag is not set
        return 0;
    }
    else
    {
        log_info("[RST]", "has reset flag in kv");
    }

    len = sizeof(iotx_vendor_dev_reset_type_t);
    ret = HAL_Kv_Get(AWSS_KV_RST_TYPE, &g_reset_type, &len);
    if (ret != 0 || len == 0)
    {
        log_warning("[RST]", "no rst type in kv");
        g_reset_type = IOTX_VENDOR_DEV_RESET_TYPE_UNBIND_SHADOW_CLEAR;
    }

    log_info("[RST]", "need report rst,type=%d", g_reset_type);
    awss_report_reset_suc = 0;

    if (reset_type)
    {
        *reset_type = g_reset_type;
    }

    return 1;
}

// return: 0 - reset success, -1 - reset fail
int awss_clear_reset(void)
{
    int ret = 0;
    if (HAL_Kv_Del(AWSS_KV_RST) != 0) {
        log_err("[RST]", "KV_RST del fail");
        ret = -1;
    }
    if (HAL_Kv_Del(AWSS_KV_RST_TYPE) != 0) {
        log_err("[RST]", "KV_RST_TYPE del fail");
        ret = -1;
    }

    if (report_reset_timer)
    {
        HAL_Timer_Stop(report_reset_timer);
        HAL_Timer_Delete(report_reset_timer);
        report_reset_timer = NULL;
    }
    return ret;
}

int awss_stop_report_reset()
{
    if (report_reset_timer == NULL) {
        return 0;
    }

    HAL_Timer_Stop(report_reset_timer);
    HAL_Timer_Delete(report_reset_timer);
    report_reset_timer = NULL;

    return 0;
}

int iotx_sdk_reset_cloud(iotx_vendor_dev_reset_type_t *reset_type)
{
    return awss_report_reset(reset_type);
}

int iotx_sdk_reset_local(void)
{
    int ret = 0;

#ifdef BUILD_AOS
    netmgr_clear_ap_config();
#endif

    ret = iotx_guider_clear_dynamic_url();

#if defined(SUPPORT_TLS) && defined(BUILD_AOS)
    ret |= HAL_SSL_Del_KV_Session_Ticket();
#endif

    return ret;
}

int iotx_sdk_reset(iotx_vendor_dev_reset_type_t *reset_type)
{
    iotx_sdk_reset_cloud(reset_type);
    iotx_sdk_reset_local();

    return 0;
}



#if defined(__cplusplus)  /* If this is a C++ compiler, use C linkage */
}
#endif

