/******************************************************************************
*  Copyright 2017 - 2018, Opulinks Technology Ltd.
*  ---------------------------------------------------------------------------
*  Statement:
*  ----------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Opulinks Technology Ltd. (C) 2018
******************************************************************************/

/**
 * @file blewifi_ctrl_http_ota.c
 * @author Luke Fang
 * @date 11 Sep 2018
 * @brief File creates the blewifi http ota task architecture.
 *
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "cmsis_os.h"
#include "sys_os_config.h"
#include "blewifi_configuration.h"
#include "blewifi_ctrl_http_ota.h"
#include "blewifi_http_ota.h"
#include "blewifi_data.h"
#include "blewifi_common.h"
#include "blewifi_ctrl.h"
#include "blewifi_wifi_api.h"

#if (WIFI_OTA_FUNCTION_EN == 1)
osThreadId   BleWifCtrliHttpOtaTaskId;
osMessageQId BleWifiCtrlHttpOtaQueueId;

osTimerId g_tOtaSchedTimer = NULL;
uint32_t g_u32OtaSchedTimerSeq = 0;

static void blewifi_ctrl_ota_sched_timeout_handle(uint32_t evt_type, void *data, int len);

void blewifi_ctrl_http_ota_task_evt_handler(uint32_t evt_type, void *data, int len)
{
    switch (evt_type)
    {
        case BLEWIFI_CTRL_HTTP_OTA_MSG_TRIG:
            BLEWIFI_INFO("BLEWIFI: MSG BLEWIFI_CTRL_HTTP_OTA_MSG_TRIG \r\n");
            BleWifi_Ctrl_MsgSend(BLEWIFI_CTRL_MSG_OTHER_OTA_ON, NULL, 0);
            BleWifi_Wifi_SetDTIM(0);
            if (ota_download_by_http(data) != 0)
            {
                BleWifi_Wifi_SetDTIM(BleWifi_Ctrl_DtimTimeGet());
                BleWifi_Wifi_OtaTrigRsp(BLEWIFI_WIFI_OTA_FAILURE);
                BleWifi_Ctrl_MsgSend(BLEWIFI_CTRL_MSG_OTHER_OTA_OFF_FAIL, NULL, 0);
            }
            else
            {
                BleWifi_Wifi_SetDTIM(BleWifi_Ctrl_DtimTimeGet());
                BleWifi_Wifi_OtaTrigRsp(BLEWIFI_WIFI_OTA_SUCCESS);
                BleWifi_Ctrl_MsgSend(BLEWIFI_CTRL_MSG_OTHER_OTA_OFF, NULL, 0);
            }
            break;
        case BLEWIFI_CTRL_HTTP_OTA_MSG_DEVICE_VERSION:
        {
            BLEWIFI_INFO("BLEWIFI: MSG BLEWIFI_CTRL_HTTP_OTA_MSG_DEVICE_VERSION \r\n");
            uint16_t pid;
            uint16_t cid;
            uint16_t fid;

            ota_get_version(&pid, &cid, &fid);

            printf("BLEWIFI_CTRL_HTTP_OTA_MSG_DEVICE_VERSION pid = %d, cid = %d, fid = %d\n", pid, cid, fid);
            BleWifi_Wifi_OtaDeviceVersionRsp(fid);
        }
        break;
        case BLEWIFI_CTRL_HTTP_OTA_MSG_SERVER_VERSION:
        {
            BLEWIFI_INFO("BLEWIFI: MSG BLEWIFI_CTRL_HTTP_OTA_MSG_SERVER_VERSION \r\n");
            uint16_t fid;

            if (ota_download_by_http_get_server_version(WIFI_OTA_HTTP_URL, &fid) != 0)
            {
                fid = 0;
                BleWifi_Wifi_OtaServerVersionRsp(fid);
            }
            else
            {
                printf("ota_download_by_http_get_server_version - fid = %d", fid);
                BleWifi_Wifi_OtaServerVersionRsp(fid);
            }
        }
        break;
        case BLEWIFI_CTRL_HTTP_OTA_MSG_SCHED_TIMEOUT:
            blewifi_ctrl_ota_sched_timeout_handle(evt_type, data, len);
            break;

        default:
            break;
    }
}

void blewifi_http_ota_task(void *args)
{
    osEvent rxEvent;
    xBleWifiCtrlHttpOtaMessage_t *rxMsg;

    for(;;)
    {
        /* Wait event */
        rxEvent = osMessageGet(BleWifiCtrlHttpOtaQueueId, osWaitForever);
        if(rxEvent.status != osEventMessage)
            continue;

        rxMsg = (xBleWifiCtrlHttpOtaMessage_t *)rxEvent.value.p;
        blewifi_ctrl_http_ota_task_evt_handler(rxMsg->event, rxMsg->ucaMessage, rxMsg->length);

        /* Release buffer */
        if (rxMsg != NULL)
            free(rxMsg);
    }
}

void blewifi_ctrl_http_ota_task_create(void)
{
    osThreadDef_t task_def;
    osMessageQDef_t blewifi_queue_def;

    /* Create ble-wifi task */
    task_def.name = "blewifi ctrl http ota";
    task_def.stacksize = OS_TASK_STACK_SIZE_APP * 2;
    task_def.tpriority = OS_TASK_PRIORITY_APP;
    task_def.pthread = blewifi_http_ota_task;

    BleWifCtrliHttpOtaTaskId = osThreadCreate(&task_def, (void*)NULL);
    if(BleWifCtrliHttpOtaTaskId == NULL)
    {
        BLEWIFI_INFO("BLEWIFI: ctrl task create fail \r\n");
    }

    /* Create message queue*/
    blewifi_queue_def.item_sz = sizeof(xBleWifiCtrlHttpOtaMessage_t);
    blewifi_queue_def.queue_sz = BLEWIFI_CTRL_HTTP_OTA_QUEUE_SIZE;
    BleWifiCtrlHttpOtaQueueId = osMessageCreate(&blewifi_queue_def, NULL);
    if(BleWifiCtrlHttpOtaQueueId == NULL)
    {
		BLEWIFI_ERROR("ota: osMessageCreate fail\n");
    }
}

int blewifi_ctrl_http_ota_msg_send(int msg_type, uint8_t *data, int data_len)
{
    int iRet = -1;
    xBleWifiCtrlHttpOtaMessage_t *pMsg = NULL;

    /* Mem allocate */
    pMsg = malloc(sizeof(xBleWifiCtrlHttpOtaMessage_t) + data_len);
    if (pMsg == NULL)
    {
        BLEWIFI_ERROR("ota: malloc fail\n");
        goto done;
    }

    pMsg->event = msg_type;
    pMsg->length = data_len;
    if (data_len > 0)
    {
        memcpy(pMsg->ucaMessage, data, data_len);
    }

    if (osMessagePut(BleWifiCtrlHttpOtaQueueId, (uint32_t)pMsg, 0) != osOK)
    {
        BLEWIFI_ERROR("ota: osMessagePut fail\n");
        goto done;
    }

    iRet = 0;

done:
    if(iRet)
    {
        if (pMsg != NULL)
        {
            free(pMsg);
        }
    }

    return iRet;
}

static void blewifi_ctrl_ota_sched_timer_callback(void const *argu)
{
    blewifi_ctrl_http_ota_msg_send(BLEWIFI_CTRL_HTTP_OTA_MSG_SCHED_TIMEOUT, (uint8_t *)&g_u32OtaSchedTimerSeq, sizeof(g_u32OtaSchedTimerSeq));
}

static void blewifi_ctrl_ota_sched_timeout_handle(uint32_t evt_type, void *data, int len)
{
    uint32_t *pu32Seq = (uint32_t *)data;
    uint16_t server_fid;
	uint16_t pid, cid, fid;

    BLEWIFI_INFO("BLEWIFI: MSG BLEWIFI_CTRL_MSG_OTA_SCHED_TIMEOUT \r\n");

    if(*pu32Seq != g_u32OtaSchedTimerSeq)
    {
        BLEWIFI_WARN("ota timer id not match\n");
        goto done;
    }

    ota_download_by_http_get_server_version(WIFI_OTA_UPGRADE_URL, &server_fid);
    ota_get_version(&pid, &cid, &fid);

    printf("OTA server: fid=%d\r\n", server_fid);
    printf("OTA device: pid=%d, cid=%d, fid=%d\r\n", pid, cid, fid);

    if (fid < server_fid)
    {
        //trigger OTA upgrade
        BleWifi_Wifi_OtaTrigReq(WIFI_OTA_UPGRADE_URL);
    }

    blewifi_ctrl_ota_sched_start(0);

done:
    return;
}

void blewifi_ctrl_ota_sched_init(void)
{
    if(!g_tOtaSchedTimer)
    {
        osTimerDef_t tTimerButtonDef = {0};

        tTimerButtonDef.ptimer = blewifi_ctrl_ota_sched_timer_callback;
        g_tOtaSchedTimer = osTimerCreate(&tTimerButtonDef, osTimerOnce, NULL);

        if(!g_tOtaSchedTimer)
        {
            BLEWIFI_ERROR("ota: osTimerCreate fail\n");
        }
    }

    return;
}

void blewifi_ctrl_ota_sched_start(uint32_t u32Sec)
{
    struct tm tInfo = {0};
    uint32_t u32Time = 0; // current seconds of today
    uint32_t u32Ms = 0;

    osTimerStop(g_tOtaSchedTimer);

    BleWifi_SntpGet(&tInfo, g_s32TimeZoneSec);

    if(u32Sec)
    {
        u32Ms = u32Sec * 1000;
    }
    else
    {
        u32Time = (tInfo.tm_hour * 3600) + (tInfo.tm_min * 60) + tInfo.tm_sec;
    
        BLEWIFI_WARN("OTA sched time[%02u:%02u:%02u] wday[%u]: seconds_of_today[%u]\n", tInfo.tm_hour, tInfo.tm_min, tInfo.tm_sec, tInfo.tm_wday, u32Time);
        (void)u32Time;
    
        u32Ms = WIFI_OTA_SCHED_TIME * 60* 1000;
    }

    ++g_u32OtaSchedTimerSeq;
    osTimerStart(g_tOtaSchedTimer, u32Ms); // unit: ms

    printf("OTA SchedTimerSeq %d\r\n", g_u32OtaSchedTimerSeq);
    return;
}

#endif /* #if (WIFI_OTA_FUNCTION_EN == 1) */
