/******************************************************************************
*  Copyright 2017 - 2018, Opulinks Technology Ltd.
*  ----------------------------------------------------------------------------
*  Statement:
*  ----------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Opulinks Technology Ltd. (C) 2018
******************************************************************************/

/**
 * @file blewifi_ctrl.c
 * @author Vincent Chen, Michael Liao
 * @date 20 Feb 2018
 * @brief File creates the blewifi ctrl task architecture.
 *
 */

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "cmsis_os.h"
#include "event_groups.h"
#include "sys_os_config.h"
#include "sys_os_config_patch.h"
#include "at_cmd_common.h"
#include "controller_wifi.h"

#include "blewifi_common.h"
#include "blewifi_configuration.h"
#include "blewifi_ctrl.h"
#include "blewifi_wifi_api.h"
#include "blewifi_ble_api.h"
#include "blewifi_data.h"
#include "blewifi_app.h"
#include "mw_ota_def.h"
#include "mw_ota.h"
#include "hal_system.h"
#include "mw_fim_default_group03.h"
#include "mw_fim_default_group03_patch.h"
#include "mw_fim_default_group11_project.h"
#include "mw_fim_default_group13_project.h"
#include "mw_fim_default_group14_project.h"
#include "ali_linkkitsdk_decl.h"
#include "light_control.h"
#include "ps_public.h"
#include "infra_config.h"
#include "awss_dev_reset.h"
#include "blewifi_ctrl_http_ota.h"
#include "mqtt_wrapper.h"

#ifdef BLEWIFI_SCHED_EXT
#include "mw_fim_default_group16_project.h"
#endif

#ifdef ALI_REPORT_TOKEN_AFTER_UNBIND
#include "dev_bind_api.h"
#endif

#ifdef ADA_REMOTE_CTRL 
#include "ada_lightbulb.h"
#endif

#ifdef ALI_BLE_WIFI_PROVISION
#include "awss_notify.h"
#include "awss_cmp.h"
#include "wifi_api.h"
#include "blewifi_server_app.h"
#include "breeze_export.h"
#include "..\src\breeze\include\common.h"
#include "infra_compat.h"
#include "passwd.h"
#include "mw_fim_default_group17_project.h"
#include "breeze_hal_ble.h"
#include "core.h"
#include "ble_gap_if.h"

SHM_DATA breeze_apinfo_t g_apInfo;
extern uint32_t tx_func_indicate(uint8_t cmd, uint8_t *p_data, uint16_t length);

extern bool g_noti_flag;
extern bool g_Indi_flag;
extern uint8_t g_led_mp_mode_flag;
#endif
#define BLEWIFI_CTRL_RESET_DELAY    (3000)  // ms

#define NETWORK_START_LED       1
#define NETWORK_STOP_LED        2
#define NETWORK_STOP_TOUT_LED   3

osThreadId   g_tAppCtrlTaskId;
osMessageQId g_tAppCtrlQueueId;
osTimerId    g_tAppCtrlAutoConnectTriggerTimer;
osTimerId    g_tAppCtrlSysTimer;
osTimerId    g_tAppButtonBleAdvTimerId;
osTimerId    g_tAppCtrlNetworkingLedTimerId;
EventGroupHandle_t g_tAppCtrlEventGroup;

uint8_t g_ulAppCtrlSysMode;
uint8_t g_ubAppCtrlSysStatus;

uint32_t g_NetLedTransState = NETWORK_START_LED;
uint32_t g_NetLedTimeCounter = 0;

uint8_t g_ubAppCtrlRequestRetryTimes;
uint32_t g_ulAppCtrlAutoConnectInterval;
uint32_t g_ulAppCtrlWifiDtimTime;
#ifdef ALI_BLE_WIFI_PROVISION
//uint8_t g_Ali_wifi_provision =0;
#endif
T_MwFim_GP11_WifiConnectSettings g_tAppCtrlWifiConnectSettings;

T_MwFim_GP13_Dev_Sched g_taDevSched[MW_FIM_GP13_DEV_SCHED_NUM] = {0};

#ifdef BLEWIFI_SCHED_EXT
T_MwFim_GP16_Dev_Sched_Ext g_taDevSchedExt[MW_FIM_GP13_DEV_SCHED_NUM] = {0};
#endif

T_BleWifi_Ctrl_SortDevSched g_taSortDevSched[MW_FIM_GP13_DEV_SCHED_NUM] = {0};

osTimerId g_tDevSchedTimer = NULL;
uint32_t g_u32DevSchedTimerSeq = 0;
uint8_t g_u8SortDevSchedIdx = 0;

#ifdef BLEWIFI_REFINE_INIT_FLOW
#else
volatile T_MwFim_GP14_Boot_Status g_tBootStatus = {0};
#endif

T_MwFim_GP17_AliyunInfo g_tAliyunInfo = {0};

#if (SNTP_FUNCTION_EN == 1)
osTimerId g_tAppCtrlSntpTimerId;
#endif

uint32_t g_u32AppAliWifiScanCount = 0;


static void BleWifi_Ctrl_TaskEvtHandler_BleInitComplete(uint32_t evt_type, void *data, int len);
static void BleWifi_Ctrl_TaskEvtHandler_BleAdvertisingCfm(uint32_t evt_type, void *data, int len);
static void BleWifi_Ctrl_TaskEvtHandler_BleAdvertisingExitCfm(uint32_t evt_type, void *data, int len);
static void BleWifi_Ctrl_TaskEvtHandler_BleAdvertisingTimeChangeCfm(uint32_t evt_type, void *data, int len);
static void BleWifi_Ctrl_TaskEvtHandler_BleConnectionComplete(uint32_t evt_type, void *data, int len);
static void BleWifi_Ctrl_TaskEvtHandler_BleConnectionFail(uint32_t evt_type, void *data, int len);
static void BleWifi_Ctrl_TaskEvtHandler_BleDisconnect(uint32_t evt_type, void *data, int len);
static void BleWifi_Ctrl_TaskEvtHandler_BleDataInd(uint32_t evt_type, void *data, int len);
static void BleWifi_Ctrl_TaskEvtHandler_WifiInitComplete(uint32_t evt_type, void *data, int len);
static void BleWifi_Ctrl_TaskEvtHandler_WifiScanDoneInd(uint32_t evt_type, void *data, int len);
static void BleWifi_Ctrl_TaskEvtHandler_WifiConnectionInd(uint32_t evt_type, void *data, int len);
static void BleWifi_Ctrl_TaskEvtHandler_WifiDisconnectionInd(uint32_t evt_type, void *data, int len);
static void BleWifi_Ctrl_TaskEvtHandler_WifiGotIpInd(uint32_t evt_type, void *data, int len);
static void BleWifi_Ctrl_TaskEvtHandler_WifiAutoConnectInd(uint32_t evt_type, void *data, int len);
#ifdef ALI_BLE_WIFI_PROVISION
static void BleWifi_Ctrl_TaskEvtHandler_AliWifiConnect(uint32_t evt_type, void *data, int len);
#endif
static void BleWifi_Ctrl_TaskEvtHandler_DevSchedSetAll(uint32_t evt_type, void *data, int len);
static void BleWifi_Ctrl_TaskEvtHandler_DevSchedTimeout(uint32_t evt_type, void *data, int len);
static void BleWifi_Ctrl_TaskEvtHandler_DevSchedStart(uint32_t evt_type, void *data, int len);

#ifdef BLEWIFI_LIGHT_CTRL
static void BleWifi_Ctrl_TaskEvtHandler_LightCtrlTimeout(uint32_t evt_type, void *data, int len);
#endif

#ifdef BLEWIFI_LIGHT_DRV
static void BleWifi_Ctrl_TaskEvtHandler_LightDrvTimeout(uint32_t evt_type, void *data, int len);
#endif

static void BleWifi_Ctrl_TaskEvtHandler_OtherOtaOn(uint32_t evt_type, void *data, int len);
static void BleWifi_Ctrl_TaskEvtHandler_OtherOtaOff(uint32_t evt_type, void *data, int len);
static void BleWifi_Ctrl_TaskEvtHandler_OtherOtaOffFail(uint32_t evt_type, void *data, int len);
static void BleWifi_Ctrl_TaskEvtHandler_OtherSysTimer(uint32_t evt_type, void *data, int len);
static void BleWifi_Ctrl_TaskEvtHandler_NetworkingStart(uint32_t evt_type, void *data, int len);
static void BleWifi_Ctrl_TaskEvtHandler_NetworkingStop(uint32_t evt_type, void *data, int len);
static void BleWifi_Ctrl_TaskEvtHandler_NetworkingLedTimeOut(uint32_t evt_type, void *data, int len);
static void BleWifi_Ctrl_TaskEvtHandler_ButtonBleAdvTimeOut(uint32_t evt_type, void *data, int len);

#ifdef BLEWIFI_REFINE_INIT_FLOW
static void BleWifi_Ctrl_TaskEvtHandler_BleAdvStart(uint32_t evt_type, void *data, int len);
#endif

#if (SNTP_FUNCTION_EN == 1)
//static void BleWifi_Ctrl_TaskEvtHandler_SntpStart(uint32_t evt_type, void *data, int len);
static void BleWifi_Ctrl_TaskEvtHandler_SntpTimeOut(uint32_t evt_type, void *data, int len);
#endif

static T_BleWifi_Ctrl_EvtHandlerTbl g_tCtrlEvtHandlerTbl[] =
{
    {BLEWIFI_CTRL_MSG_BLE_INIT_COMPLETE,                BleWifi_Ctrl_TaskEvtHandler_BleInitComplete},
    {BLEWIFI_CTRL_MSG_BLE_ADVERTISING_CFM,              BleWifi_Ctrl_TaskEvtHandler_BleAdvertisingCfm},
    {BLEWIFI_CTRL_MSG_BLE_ADVERTISING_EXIT_CFM,         BleWifi_Ctrl_TaskEvtHandler_BleAdvertisingExitCfm},
    {BLEWIFI_CTRL_MSG_BLE_ADVERTISING_TIME_CHANGE_CFM,  BleWifi_Ctrl_TaskEvtHandler_BleAdvertisingTimeChangeCfm},
    {BLEWIFI_CTRL_MSG_BLE_CONNECTION_COMPLETE,          BleWifi_Ctrl_TaskEvtHandler_BleConnectionComplete},
    {BLEWIFI_CTRL_MSG_BLE_CONNECTION_FAIL,              BleWifi_Ctrl_TaskEvtHandler_BleConnectionFail},
    {BLEWIFI_CTRL_MSG_BLE_DISCONNECT,                   BleWifi_Ctrl_TaskEvtHandler_BleDisconnect},
    {BLEWIFI_CTRL_MSG_BLE_DATA_IND,                     BleWifi_Ctrl_TaskEvtHandler_BleDataInd},
    
    {BLEWIFI_CTRL_MSG_WIFI_INIT_COMPLETE,               BleWifi_Ctrl_TaskEvtHandler_WifiInitComplete},
    {BLEWIFI_CTRL_MSG_WIFI_SCAN_DONE_IND,               BleWifi_Ctrl_TaskEvtHandler_WifiScanDoneInd},
    {BLEWIFI_CTRL_MSG_WIFI_CONNECTION_IND,              BleWifi_Ctrl_TaskEvtHandler_WifiConnectionInd},
    {BLEWIFI_CTRL_MSG_WIFI_DISCONNECTION_IND,           BleWifi_Ctrl_TaskEvtHandler_WifiDisconnectionInd},
    {BLEWIFI_CTRL_MSG_WIFI_GOT_IP_IND,                  BleWifi_Ctrl_TaskEvtHandler_WifiGotIpInd},
    {BLEWIFI_CTRL_MSG_WIFI_AUTO_CONNECT_IND,            BleWifi_Ctrl_TaskEvtHandler_WifiAutoConnectInd},
#ifdef ALI_BLE_WIFI_PROVISION
    {BLEWIFI_CTRL_MSG_ALI_WIFI_CONNECT,                 BleWifi_Ctrl_TaskEvtHandler_AliWifiConnect},
#endif
    {BLEWIFI_CTRL_MSG_DEV_SCHED_SET_ALL,                BleWifi_Ctrl_TaskEvtHandler_DevSchedSetAll},
    {BLEWIFI_CTRL_MSG_DEV_SCHED_TIMEOUT,                BleWifi_Ctrl_TaskEvtHandler_DevSchedTimeout},
    {BLEWIFI_CTRL_MSG_DEV_SCHED_START,                  BleWifi_Ctrl_TaskEvtHandler_DevSchedStart},

    #ifdef BLEWIFI_LIGHT_CTRL
    {BLEWIFI_CTRL_MSG_LIGHT_CTRL_TIMEOUT,               BleWifi_Ctrl_TaskEvtHandler_LightCtrlTimeout},
    #endif

    #ifdef BLEWIFI_LIGHT_DRV
    {BLEWIFI_CTRL_MSG_LIGHT_DRV_TIMEOUT,                BleWifi_Ctrl_TaskEvtHandler_LightDrvTimeout},
    #endif
		
    {BLEWIFI_CTRL_MSG_OTHER_OTA_ON,                     BleWifi_Ctrl_TaskEvtHandler_OtherOtaOn},
    {BLEWIFI_CTRL_MSG_OTHER_OTA_OFF,                    BleWifi_Ctrl_TaskEvtHandler_OtherOtaOff},
    {BLEWIFI_CTRL_MSG_OTHER_OTA_OFF_FAIL,               BleWifi_Ctrl_TaskEvtHandler_OtherOtaOffFail},
    {BLEWIFI_CTRL_MSG_OTHER_SYS_TIMER,                  BleWifi_Ctrl_TaskEvtHandler_OtherSysTimer},
    {BLEWIFI_CTRL_MSG_NETWORKING_START,                 BleWifi_Ctrl_TaskEvtHandler_NetworkingStart},
    {BLEWIFI_CTRL_MSG_NETWORKING_STOP,                  BleWifi_Ctrl_TaskEvtHandler_NetworkingStop},
    {BLEWIFI_CTRL_MSG_NETWORKING_LED_TIMEOUT,           BleWifi_Ctrl_TaskEvtHandler_NetworkingLedTimeOut},

    #ifdef BLEWIFI_REFINE_INIT_FLOW
    {BLEWIFI_CTRL_MSG_BLE_ADV_START,                    BleWifi_Ctrl_TaskEvtHandler_BleAdvStart},
    #endif
    
    {BLEWIFI_CTRL_MSG_BUTTON_BLE_ADV_TIMEOUT,           BleWifi_Ctrl_TaskEvtHandler_ButtonBleAdvTimeOut},

#if (SNTP_FUNCTION_EN == 1)
    //{BLEWIFI_CTRL_MSG_SNTP_START,                       BleWifi_Ctrl_TaskEvtHandler_SntpStart},
    {BLEWIFI_CTRL_MSG_SNTP_TIMEOUT,                     BleWifi_Ctrl_TaskEvtHandler_SntpTimeOut},
#endif

    {0xFFFFFFFF,                                        NULL}
};

void BleWifi_Ctrl_SysModeSet(uint8_t mode)
{
    g_ulAppCtrlSysMode = mode;
}

uint8_t BleWifi_Ctrl_SysModeGet(void)
{
    return g_ulAppCtrlSysMode;
}

#ifdef ALI_BLE_WIFI_PROVISION
extern int iotx_sdk_reset(iotx_vendor_dev_reset_type_t *reset_type);
extern int g_nRegion_Id;
extern BLE_APP_DATA_T gTheBle;
extern uint32_t g_seq;
extern uint8_t g_bind_state;
extern int ble_advertising_start(ais_adv_init_t *adv);
extern int awss_clear_reset(void);

void linkkit_event_monitor(int event)
{
    switch (event) {
        case IOTX_CONN_REPORT_TOKEN_SUC:
//#ifdef EN_COMBO_NET
            {
                uint8_t rsp[] = { 0x01, 0x01, 0x03 };
//                if(ble_connected){
                    tx_func_indicate(BZ_CMD_STATUS, rsp, sizeof(rsp));
//                    breeze_post(rsp, sizeof(rsp));
//                }
            }
//#endif
#if 0
            if (true == BleWifi_Ctrl_EventStatusGet(BLEWIFI_CTRL_EVENT_BIT_BLE))
            {
                //BleWifi_Ble_Disconnect();                

                ais_adv_init_t adv_data = {
                    .flag = (ais_adv_flag_t)(AIS_AD_GENERAL | AIS_AD_NO_BREDR),
                    .name = { .ntype = AIS_ADV_NAME_FULL, .name = "FY" },
                };
                
                uint8_t sub_type = 4;
                uint8_t sec_type = 1;
                uint8_t bind_state = 1;
                core_create_bz_adv_data(sub_type, sec_type, bind_state);
                adv_data.vdata.len = sizeof(adv_data.vdata.data);
                if (core_get_bz_adv_data(adv_data.vdata.data, &(adv_data.vdata.len))) {
                    
                    printf("%s %d core_get_bz_adv_data fail.\r\n", __func__, __LINE__);
                }                
                
                ble_advertising_start(&adv_data);
                
                int ret = LeGapSetAdvData(gTheBle.adv_data.len, gTheBle.adv_data.buf);
                if(SYS_ERR_SUCCESS != ret)
                {
                    printf("%s %d LeGapSetAdvData fail.\r\n", __func__, __LINE__);
                }
                
                //BleWifi_Ctrl_NetworkingStop();                                              
                
            }
#endif
            g_seq = 0;
            g_bind_state = 0;
            iotx_guider_set_dynamic_region(g_nRegion_Id);

        #if 1
            if(g_tAliyunInfo.ulRegionID != g_nRegion_Id)
            {
                g_tAliyunInfo.ulRegionID = g_nRegion_Id;

                if(MwFim_FileWrite(MW_FIM_IDX_GP17_PROJECT_ALIYUN_INFO, 0, MW_FIM_GP17_ALIYUN_INFO_SIZE, (uint8_t*)&g_tAliyunInfo) != MW_FIM_OK)
                {
                    BLEWIFI_ERROR("MwFim_FileWrite fail for region_id[%d]\r\n", g_nRegion_Id);
                }
            }
        #else
            T_MwFim_GP17_AliyunInfo AliyunInfo;
            
            AliyunInfo.ulRegionID = g_nRegion_Id;
            MwFim_FileWrite(MW_FIM_IDX_GP17_PROJECT_ALIYUN_INFO, 0, MW_FIM_GP17_ALIYUN_INFO_SIZE, (uint8_t*)&AliyunInfo);
        #endif


            BLEWIFI_WARN("---- report token success ----\r\n");
            if (true == BleWifi_Ctrl_EventStatusGet(BLEWIFI_CTRL_EVENT_BIT_ALI_STOP_BLE))
            {
                BleWifi_Ctrl_MsgSend(BLEWIFI_CTRL_MSG_NETWORKING_STOP, NULL, 0);
                BleWifi_Ctrl_EventStatusSet(BLEWIFI_CTRL_EVENT_BIT_ALI_STOP_BLE, false);
            }

            awss_clear_reset();
            break;

        case IOTX_CONN_CLOUD_SUC:
        {
            BLEWIFI_WARN("[%s %d] IOTX_CONN_CLOUD_SUC\n", __func__, __LINE__);

            if(true == BleWifi_Ctrl_EventStatusGet(BLEWIFI_CTRL_EVENT_BIT_PREPARE_ALI_BOOT_RESET))
            {
                BLEWIFI_WARN("[%s %d] IOTX_CONN_CLOUD_SUC: awss_report_reset for ALI_BOOT_RESET\n", __func__, __LINE__);

                #ifdef ALI_REPORT_TOKEN_AFTER_UNBIND
                awss_report_cloud();
                #else
                iotx_vendor_dev_reset_type_t reset_type = (iotx_vendor_dev_reset_type_t)2;
                iotx_sdk_reset(&reset_type);
//                awss_report_reset();
                #endif
            }
            else if(true == BleWifi_Ctrl_EventStatusGet(BLEWIFI_CTRL_EVENT_BIT_WAIT_ALI_RESET))
            {
                BLEWIFI_WARN("[%s %d] IOTX_CONN_CLOUD_SUC: awss_report_reset for WAIT_ALI_RESET\n", __func__, __LINE__);

                #ifdef ALI_REPORT_TOKEN_AFTER_UNBIND
                awss_report_cloud();
                #else
                iotx_vendor_dev_reset_type_t reset_type = (iotx_vendor_dev_reset_type_t)2;
                iotx_sdk_reset(&reset_type);
//                awss_report_reset();
                #endif
            }

            break;
        }

        case IOTX_CONN_CLOUD_FAIL:
        {

            break;
        }

        case IOTX_RESET:
        {
        #if 0
            BLEWIFI_WARN("[%s %d] IOTX_RESET\n", __func__, __LINE__);

            if(true == BleWifi_Ctrl_EventStatusGet(BLEWIFI_CTRL_EVENT_BIT_PREPARE_ALI_BOOT_RESET))
            {
                BLEWIFI_WARN("[%s %d] disable ALI_BOOT_RESET\n", __func__, __LINE__);

                BleWifi_Ctrl_EventStatusSet(BLEWIFI_CTRL_EVENT_BIT_PREPARE_ALI_BOOT_RESET, false);
            }
            else if(true == BleWifi_Ctrl_EventStatusGet(BLEWIFI_CTRL_EVENT_BIT_WAIT_ALI_RESET))
            {
                BLEWIFI_WARN("[%s %d] disable WAIT_ALI_RESET\n", __func__, __LINE__);

                BleWifi_Ctrl_EventStatusSet(BLEWIFI_CTRL_EVENT_BIT_WAIT_ALI_RESET, false);
                #ifdef ALI_REPORT_TOKEN_AFTER_UNBIND
                awss_report_cloud();
                #endif
            }
            
            #ifdef ALI_UNBIND_REFINE
            HAL_SetReportReset(0);
            BleWifi_Ctrl_EventStatusSet(BLEWIFI_CTRL_EVENT_BIT_UNBIND, true);
            #endif
        #endif
            break;
        }
        
        default:
            break;
    }
}
#endif

void BleWifi_Ctrl_EventStatusSet(uint32_t dwEventBit, uint8_t status)
{
// ISR mode is not supported.
#if 0
    BaseType_t xHigherPriorityTaskWoken, xResult;
    
    // check if it is ISR mode or not
    if (0 != __get_IPSR())
    {
        if (true == status)
        {
            // xHigherPriorityTaskWoken must be initialised to pdFALSE.
    		xHigherPriorityTaskWoken = pdFALSE;

            // Set bit in xEventGroup.
            xResult = xEventGroupSetBitsFromISR(g_tAppCtrlEventGroup, dwEventBit, &xHigherPriorityTaskWoken);
            if( xResult == pdPASS )
    		{
    			// If xHigherPriorityTaskWoken is now set to pdTRUE then a context
    			// switch should be requested.  The macro used is port specific and
    			// will be either portYIELD_FROM_ISR() or portEND_SWITCHING_ISR() -
    			// refer to the documentation page for the port being used.
    			portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    		}
        }
        else
            xEventGroupClearBitsFromISR(g_tAppCtrlEventGroup, dwEventBit);
    }
    // Taske mode
    else
#endif
    {
        if (true == status)
            xEventGroupSetBits(g_tAppCtrlEventGroup, dwEventBit);
        else
            xEventGroupClearBits(g_tAppCtrlEventGroup, dwEventBit);
    }
}

uint8_t BleWifi_Ctrl_EventStatusGet(uint32_t dwEventBit)
{
    EventBits_t tRetBit;

    tRetBit = xEventGroupGetBits(g_tAppCtrlEventGroup);
    if (dwEventBit == (dwEventBit & tRetBit))
        return true;

    return false;
}

uint8_t BleWifi_Ctrl_EventStatusWait(uint32_t dwEventBit, uint32_t millisec)
{
    EventBits_t tRetBit;

    tRetBit = xEventGroupWaitBits(g_tAppCtrlEventGroup,
                                  dwEventBit,
                                  pdFALSE,
                                  pdFALSE,
                                  millisec);
    if (dwEventBit == (dwEventBit & tRetBit))
        return true;

    return false;
}

void BleWifi_Ctrl_DtimTimeSet(uint32_t value)
{
    g_ulAppCtrlWifiDtimTime = value;
    BleWifi_Wifi_SetDTIM(g_ulAppCtrlWifiDtimTime);
}

uint32_t BleWifi_Ctrl_DtimTimeGet(void)
{
    return g_ulAppCtrlWifiDtimTime;
}

void BleWifi_Ctrl_DoAutoConnect(void)
{
    uint8_t data[2];

    // if the count of auto-connection list is empty, don't do the auto-connect
    if (0 == BleWifi_Wifi_AutoConnectListNum())
        return;

    // if request connect by Peer side, don't do the auto-connection
    if (g_ubAppCtrlRequestRetryTimes <= g_tAppCtrlWifiConnectSettings.ubConnectRetry)
        return;

    // BLE is disconnect and Wifi is disconnect, too.
    if ((false == BleWifi_Ctrl_EventStatusGet(BLEWIFI_CTRL_EVENT_BIT_BLE)) && (false == BleWifi_Ctrl_EventStatusGet(BLEWIFI_CTRL_EVENT_BIT_WIFI)))
    {
        // start to scan
        // after scan, do the auto-connect
        if (g_ubAppCtrlRequestRetryTimes == BLEWIFI_CTRL_AUTO_CONN_STATE_IDLE)
        {
            data[0] = 1;    // Enable to scan AP whose SSID is hidden
            data[1] = 2;    // mixed mode
            BleWifi_Wifi_DoScan(data, 2, 0);

            g_ubAppCtrlRequestRetryTimes = BLEWIFI_CTRL_AUTO_CONN_STATE_SCAN;
        }
    }
}

void BleWifi_Ctrl_AutoConnectTrigger(void const *argu)
{
    BleWifi_Ctrl_MsgSend(BLEWIFI_CTRL_MSG_WIFI_AUTO_CONNECT_IND, NULL, 0);
}

#ifdef BLEWIFI_REFINE_INIT_FLOW
#else
void BleWifi_Ctrl_BootCntClear(void)
{
    if(g_tBootStatus.u8Cnt)
    {
        g_tBootStatus.u8Cnt = 0;
    
        BLEWIFI_WARN("[%s %d] clear boot_cnt\n", __func__, __LINE__);
    
        if (MW_FIM_OK != MwFim_FileWrite(MW_FIM_IDX_GP14_PROJECT_BOOT_STATUS, 0, MW_FIM_GP14_BOOT_STATUS_SIZE, (uint8_t *)&g_tBootStatus))
        {
            BLEWIFI_ERROR("BootCntClear: MwFim_FileWrite fail\n");
        }
    }

    return;
}
#endif

void BleWifi_Ctrl_SysStatusChange(void)
{
    T_MwFim_SysMode tSysMode;
    T_MwFim_GP11_PowerSaving tPowerSaving;

    // get the settings of system mode
    if (MW_FIM_OK != MwFim_FileRead(MW_FIM_IDX_GP03_PATCH_SYS_MODE, 0, MW_FIM_SYS_MODE_SIZE, (uint8_t*)&tSysMode))
    {
        // if fail, get the default value
        memcpy(&tSysMode, &g_tMwFimDefaultSysMode, MW_FIM_SYS_MODE_SIZE);
    }
		
    // get the settings of power saving
    if (MW_FIM_OK != MwFim_FileRead(MW_FIM_IDX_GP11_PROJECT_POWER_SAVING, 0, MW_FIM_GP11_POWER_SAVING_SIZE, (uint8_t*)&tPowerSaving))
    {
        // if fail, get the default value
        memcpy(&tPowerSaving, &g_tMwFimDefaultGp11PowerSaving, MW_FIM_GP11_POWER_SAVING_SIZE);
    }
		
    // change from init to normal
    if (g_ubAppCtrlSysStatus == BLEWIFI_CTRL_SYS_INIT)
    {
        g_ubAppCtrlSysStatus = BLEWIFI_CTRL_SYS_NORMAL;

        /* Power saving settings */
        if (tSysMode.ubSysMode == MW_FIM_SYS_MODE_USER)
            ps_smart_sleep(tPowerSaving.ubPowerSaving);
				
//        // start the sys timer
//        osTimerStop(g_tAppCtrlSysTimer);
//        osTimerStart(g_tAppCtrlSysTimer, BLEWIFI_COM_SYS_TIME_NORMAL);

        #ifdef BLEWIFI_REFINE_INIT_FLOW
        #else
        BleWifi_Ctrl_BootCntClear();
        #endif
    }
    // change from normal to ble off
    else if (g_ubAppCtrlSysStatus == BLEWIFI_CTRL_SYS_NORMAL)
    {
        g_ubAppCtrlSysStatus = BLEWIFI_CTRL_SYS_BLE_OFF;

//        // change the advertising time
//        BleWifi_Ble_AdvertisingTimeChange(BLEWIFI_BLE_ADVERTISEMENT_INTERVAL_PS_MIN, BLEWIFI_BLE_ADVERTISEMENT_INTERVAL_PS_MAX);
    }
}

uint8_t BleWifi_Ctrl_SysStatusGet(void)
{
    return g_ubAppCtrlSysStatus;
}

void BleWifi_Ctrl_SysTimeout(void const *argu)
{
    BleWifi_Ctrl_MsgSend(BLEWIFI_CTRL_MSG_OTHER_SYS_TIMER, NULL, 0);
}

#ifdef ALI_BLE_WIFI_PROVISION

#if 1
int BleWifi_Wifi_Get_BSsid(void)
{
    int iRet = -1;
    wifi_scan_info_t *ap_list = NULL;
    uint16_t apCount = 0;
    int32_t i = 0;
	
    wifi_scan_get_ap_num(&apCount);
	
    if(apCount == 0)
    {
        BLEWIFI_ERROR("No AP found\r\n");
        goto err;
    }
    BLEWIFI_ERROR("ap num = %d\n", apCount);
    ap_list = (wifi_scan_info_t *)malloc(sizeof(wifi_scan_info_t) * apCount);
	
    if (!ap_list) {
        BLEWIFI_ERROR("Get_BSsid: malloc fail\n");
        goto err;
    }
	
    wifi_scan_get_ap_records(&apCount, ap_list);

    /* build blewifi ap list */
    for (i = 0; i < apCount; ++i)
    {
        if(!memcmp(ap_list[i].ssid, g_apInfo.ssid, sizeof(ap_list[i].ssid) ))
        {
            memcpy(g_apInfo.bssid, ap_list[i].bssid, 6);
            iRet = 0;
            break;
        }
    }

err:
    if (ap_list)
        free(ap_list);
    
    return iRet;


}
#else
int BleWifi_Wifi_Get_BSsid(void)
{
    wifi_scan_info_t *ap_list = NULL;
    uint16_t apCount = 0;
    int8_t ubAppErr = 0;
    int32_t i = 0;
	
    wifi_scan_get_ap_num(&apCount);
	
    if(apCount == 0)
    {
        BLEWIFI_ERROR("No AP found\r\n");
        goto err;
	}
	BLEWIFI_ERROR("ap num = %d\n", apCount);
	ap_list = (wifi_scan_info_t *)malloc(sizeof(wifi_scan_info_t) * apCount);
	
	if (!ap_list) {
        BLEWIFI_ERROR("Get_BSsid: malloc fail\n");
		ubAppErr = -1;
		goto err;
	}
	
    wifi_scan_get_ap_records(&apCount, ap_list);
	
	
    /* build blewifi ap list */
	for (i = 0; i < apCount; ++i)
	{
        if(!memcmp(ap_list[i].ssid, g_apInfo.ssid, sizeof(ap_list[i].ssid) ))
		{
            memcpy(g_apInfo.bssid, ap_list[i].bssid, 6);
            break;
        }
    }
err:
    if (ap_list)
        free(ap_list);
    
    return ubAppErr;


}
#endif

void Ali_WiFi_Connect(uint8_t ssid_len, uint8_t bssid_len, uint8_t pwd_len)
{

	uint8_t *conn_data;
	uint8_t conn_data_len=bssid_len+pwd_len+2;

        BleWifi_Ctrl_EventStatusSet(BLEWIFI_CTRL_EVENT_BIT_ALI_WIFI_PRO, true);
 
	conn_data = malloc(sizeof(uint8_t)*conn_data_len);
        if(conn_data==NULL)
        {
             BLEWIFI_ERROR("WiFi_Connect: malloc fail\n");
             return;
        }
	memset(conn_data, 0, sizeof(uint8_t)*conn_data_len);
	memcpy(conn_data, g_apInfo.bssid, bssid_len);
	*(conn_data+bssid_len) = 0;
	*(conn_data+bssid_len+1) = pwd_len;
	memcpy((conn_data+bssid_len+2), g_apInfo.pw, pwd_len);
	BleWifi_Wifi_DoConnect(conn_data, conn_data_len);
	free(conn_data);
}
#endif

static void BleWifi_Ctrl_TaskEvtHandler_BleInitComplete(uint32_t evt_type, void *data, int len)
{
    BLEWIFI_INFO("BLEWIFI: MSG BLEWIFI_CTRL_MSG_BLE_INIT_COMPLETE \r\n");
    g_noti_flag=false;
    g_Indi_flag=false;
    /* BLE Init Step 2: Do BLE Advertising*/
    //BleWifi_Ble_StartAdvertising();

    #ifdef BLEWIFI_REFINE_INIT_FLOW
    #else
    if(g_tBootStatus.u8Cnt >= BLEWIFI_CTRL_BOOT_CNT_FOR_ALI_RESET)
    {
        BLEWIFI_WARN("[%s %d] boot_cnt[%u]: BleWifi_Ctrl_NetworkingStart\n", __func__, __LINE__, g_tBootStatus.u8Cnt);

        BleWifi_Ctrl_NetworkingStart();

        BleWifi_Ctrl_BootCntClear();
    }
    #endif
}

static void BleWifi_Ctrl_TaskEvtHandler_BleAdvertisingCfm(uint32_t evt_type, void *data, int len)
{
    BLEWIFI_INFO("BLEWIFI: MSG BLEWIFI_CTRL_MSG_BLE_ADVERTISING_CFM \r\n");
    
    /* BLE Init Step 3: BLE is ready for peer BLE device's connection trigger */
}

static void BleWifi_Ctrl_TaskEvtHandler_BleAdvertisingExitCfm(uint32_t evt_type, void *data, int len)
{
    BLEWIFI_INFO("BLEWIFI: MSG BLEWIFI_CTRL_MSG_BLE_ADVERTISING_EXIT_CFM \r\n");
}

static void BleWifi_Ctrl_TaskEvtHandler_BleAdvertisingTimeChangeCfm(uint32_t evt_type, void *data, int len)
{
    BLEWIFI_INFO("BLEWIFI: MSG BLEWIFI_CTRL_MSG_BLE_ADVERTISING_TIME_CHANGE_CFM \r\n");
/*
    if (false == BleWifi_Ctrl_EventStatusGet(BLEWIFI_CTRL_EVENT_BIT_NETWORK))
    {
        BleWifi_Ble_StartAdvertising();
    }
*/
}

static void BleWifi_Ctrl_TaskEvtHandler_BleConnectionComplete(uint32_t evt_type, void *data, int len)
{
    BLEWIFI_INFO("BLEWIFI: MSG BLEWIFI_CTRL_MSG_BLE_CONNECTION_COMPLETE \r\n");
    BleWifi_Ctrl_EventStatusSet(BLEWIFI_CTRL_EVENT_BIT_BLE, true);
#ifdef ALI_BLE_WIFI_PROVISION
    #ifdef BLEWIFI_REFINE_INIT_FLOW
    #else
    BleWifi_Ctrl_EventStatusSet(BLEWIFI_CTRL_EVENT_BIT_LINK_CONN, false);
    BleWifi_Ctrl_EventStatusSet(BLEWIFI_CTRL_EVENT_BIT_UNBIND, true);
    #endif
#endif
    /* BLE Init Step 4: BLE said it's connected with a peer BLE device */
}

static void BleWifi_Ctrl_TaskEvtHandler_BleConnectionFail(uint32_t evt_type, void *data, int len)
{
    BLEWIFI_INFO("BLEWIFI: MSG BLEWIFI_CTRL_MSG_BLE_CONNECTION_FAIL \r\n");

    #ifdef BLEWIFI_REFINE_INIT_FLOW
    if(true == BleWifi_Ctrl_EventStatusGet(BLEWIFI_CTRL_EVENT_BIT_NETWORK_BLE_ADV))
    #else
    if(true == BleWifi_Ctrl_EventStatusGet(BLEWIFI_CTRL_EVENT_BIT_NETWORK))
    #endif
    {
        BleWifi_Ble_StartAdvertising();
    }
}

static void BleWifi_Ctrl_TaskEvtHandler_BleDisconnect(uint32_t evt_type, void *data, int len)
{
    BLEWIFI_INFO("BLEWIFI: MSG BLEWIFI_CTRL_MSG_BLE_DISCONNECT \r\n");

    #ifdef BLEWIFI_REFINE_INIT_FLOW
    if (false == BleWifi_Ctrl_EventStatusGet(BLEWIFI_CTRL_EVENT_BIT_NETWORK_BLE_ADV))
    #else
    if (false == BleWifi_Ctrl_EventStatusGet(BLEWIFI_CTRL_EVENT_BIT_NETWORK))
    #endif
    {
        // When ble statu is not at networking, then change ble time
        /* When button press timer is finish, then change ble time from 0.5 second to 10 second */
        //BleWifi_Ble_AdvertisingTimeChange(BLEWIFI_BLE_ADVERTISEMENT_INTERVAL_MIN, BLEWIFI_BLE_ADVERTISEMENT_INTERVAL_MAX);
    }
    else
    {
        // When ble statu is at networking, then ble disconnect. Ble adv again.
        BleWifi_Ble_StartAdvertising();
    }

    BleWifi_Ctrl_EventStatusSet(BLEWIFI_CTRL_EVENT_BIT_BLE, false);
    
    /* start to do auto-connection. */
    g_ulAppCtrlAutoConnectInterval = g_tAppCtrlWifiConnectSettings.ulAutoConnectIntervalInit;
    BleWifi_Ctrl_DoAutoConnect();
    
    /* stop the OTA behavior */
    if (gTheOta)
    {
        MwOta_DataGiveUp();
        free(gTheOta);
        gTheOta = 0;
    
        BleWifi_Ctrl_MsgSend(BLEWIFI_CTRL_MSG_OTHER_OTA_OFF_FAIL, NULL, 0);
    }
}

static void BleWifi_Ctrl_TaskEvtHandler_BleDataInd(uint32_t evt_type, void *data, int len)
{
    BLEWIFI_INFO("BLEWIFI: MSG BLEWIFI_CTRL_MSG_BLE_DATA_IND \r\n");
    BleWifi_Ble_DataRecvHandler(data, len);
}

static void BleWifi_Ctrl_TaskEvtHandler_WifiInitComplete(uint32_t evt_type, void *data, int len)
{
    BLEWIFI_INFO("BLEWIFI: MSG BLEWIFI_CTRL_MSG_WIFI_INIT_COMPLETE \r\n");
    
    /* When device power on, start to do auto-connection. */
    g_ulAppCtrlAutoConnectInterval = g_tAppCtrlWifiConnectSettings.ulAutoConnectIntervalInit;
    BleWifi_Ctrl_DoAutoConnect();
}

static void BleWifi_Ctrl_TaskEvtHandler_WifiScanDoneInd(uint32_t evt_type, void *data, int len)
{
    BLEWIFI_INFO("BLEWIFI: MSG BLEWIFI_CTRL_MSG_WIFI_SCAN_DONE_IND \r\n");
    // scan by auto-connect
    if (g_ubAppCtrlRequestRetryTimes == BLEWIFI_CTRL_AUTO_CONN_STATE_SCAN)
    {
        BleWifi_Wifi_UpdateScanInfoToAutoConnList();
        BleWifi_Wifi_DoAutoConnect();
        g_ulAppCtrlAutoConnectInterval = g_ulAppCtrlAutoConnectInterval + g_tAppCtrlWifiConnectSettings.ulAutoConnectIntervalDiff;
        if (g_ulAppCtrlAutoConnectInterval > g_tAppCtrlWifiConnectSettings.ulAutoConnectIntervalMax)
            g_ulAppCtrlAutoConnectInterval = g_tAppCtrlWifiConnectSettings.ulAutoConnectIntervalMax;
    
        g_ubAppCtrlRequestRetryTimes = BLEWIFI_CTRL_AUTO_CONN_STATE_IDLE;
    }
    // scan by user
    else
    {
#ifdef ALI_BLE_WIFI_PROVISION
        //if(g_Ali_wifi_provision == 0)
        if(false == BleWifi_Ctrl_EventStatusGet(BLEWIFI_CTRL_EVENT_BIT_ALI_WIFI_PRO_1))
        {
#endif
            BleWifi_Wifi_SendScanReport();
            BleWifi_Ble_SendResponse(BLEWIFI_RSP_SCAN_END, 0);
#ifdef ALI_BLE_WIFI_PROVISION
        }
        else
        {
        #if 1
            if(!BleWifi_Wifi_Get_BSsid())
            {
                Ali_WiFi_Connect(strlen(g_apInfo.ssid),  6, strlen(g_apInfo.pw));
            }
            else
            {
                if(g_u32AppAliWifiScanCount < 5)
                {
                    uint8_t tmpdata[2] = {0};
    
                    tmpdata[0] = 1;	// Enable to scan AP whose SSID is hidden
                    tmpdata[1] = 2;	// mixed mode
    
                    BleWifi_Wifi_DoScan(tmpdata, 2, 1);

                    ++g_u32AppAliWifiScanCount;
                }
                else
                {
                    // disc before next time auto-conn
                    if(true == BleWifi_Ctrl_EventStatusGet(BLEWIFI_CTRL_EVENT_BIT_WIFI))
                    {
                        BleWifi_Wifi_DoDisconnect();
                    }
                }
            }
        #else
            BleWifi_Wifi_Get_BSsid();
            Ali_WiFi_Connect(strlen(g_apInfo.ssid),  6, strlen(g_apInfo.pw));
        #endif
        }
#endif
    }
}

static void BleWifi_Ctrl_TaskEvtHandler_WifiConnectionInd(uint32_t evt_type, void *data, int len)
{
    BLEWIFI_INFO("BLEWIFI: MSG BLEWIFI_CTRL_MSG_WIFI_CONNECTION_IND \r\n");
    BleWifi_Ctrl_EventStatusSet(BLEWIFI_CTRL_EVENT_BIT_WIFI, true);
    
    // return to the idle of the connection retry
    g_ubAppCtrlRequestRetryTimes = BLEWIFI_CTRL_AUTO_CONN_STATE_IDLE;
    g_ulAppCtrlAutoConnectInterval = g_tAppCtrlWifiConnectSettings.ulAutoConnectIntervalInit;
    BleWifi_Ble_SendResponse(BLEWIFI_RSP_CONNECT, BLEWIFI_WIFI_CONNECTED_DONE);
}

static void BleWifi_Ctrl_TaskEvtHandler_WifiDisconnectionInd(uint32_t evt_type, void *data, int len)
{
    BLEWIFI_INFO("BLEWIFI: MSG BLEWIFI_CTRL_MSG_WIFI_DISCONNECTION_IND \r\n");
    BleWifi_Ctrl_EventStatusSet(BLEWIFI_CTRL_EVENT_BIT_WIFI, false);
    BleWifi_Ctrl_EventStatusSet(BLEWIFI_CTRL_EVENT_BIT_GOT_IP, false);
    BleWifi_Wifi_SetDTIM(0);

    // continue the connection retry
    if (g_ubAppCtrlRequestRetryTimes < g_tAppCtrlWifiConnectSettings.ubConnectRetry)
    {
        BleWifi_Wifi_ReqConnectRetry();
        g_ubAppCtrlRequestRetryTimes++;
    }
    // stop the connection retry
    else if (g_ubAppCtrlRequestRetryTimes == g_tAppCtrlWifiConnectSettings.ubConnectRetry)
    {
        // return to the idle of the connection retry
        g_ubAppCtrlRequestRetryTimes = BLEWIFI_CTRL_AUTO_CONN_STATE_IDLE;
        g_ulAppCtrlAutoConnectInterval = g_tAppCtrlWifiConnectSettings.ulAutoConnectIntervalInit;
        BleWifi_Ble_SendResponse(BLEWIFI_RSP_CONNECT, BLEWIFI_WIFI_CONNECTED_FAIL);
    
#ifdef ALI_BLE_WIFI_PROVISION
        if (true == BleWifi_Ctrl_EventStatusGet(BLEWIFI_CTRL_EVENT_BIT_ALI_WIFI_PRO))
        {
            BleWifi_Ctrl_EventStatusSet(BLEWIFI_CTRL_EVENT_BIT_ALI_WIFI_PRO, false);
        }
#endif
        /* do auto-connection. */
        if (false == BleWifi_Ctrl_EventStatusGet(BLEWIFI_CTRL_EVENT_BIT_BLE))
        {
            osTimerStop(g_tAppCtrlAutoConnectTriggerTimer);
            osTimerStart(g_tAppCtrlAutoConnectTriggerTimer, g_ulAppCtrlAutoConnectInterval);
        }
    }
    else
    {
        BleWifi_Ble_SendResponse(BLEWIFI_RSP_DISCONNECT, BLEWIFI_WIFI_DISCONNECTED_DONE);
    
        /* do auto-connection. */
        if (false == BleWifi_Ctrl_EventStatusGet(BLEWIFI_CTRL_EVENT_BIT_BLE))
        {
            osTimerStop(g_tAppCtrlAutoConnectTriggerTimer);
            osTimerStart(g_tAppCtrlAutoConnectTriggerTimer, g_ulAppCtrlAutoConnectInterval);
        }
    }
}

static void BleWifi_Ctrl_TaskEvtHandler_WifiGotIpInd(uint32_t evt_type, void *data, int len)
{
    BLEWIFI_INFO("BLEWIFI: MSG BLEWIFI_CTRL_MSG_WIFI_GOT_IP_IND \r\n");
    CtrlWifi_PsStateForce(STA_PS_AWAKE_MODE, 0); // always wake-up

#if (SNTP_FUNCTION_EN == 1)
    BLEWIFI_INFO("SntpStart\n");
    osTimerStop(g_tAppCtrlSntpTimerId);
    osTimerStart(g_tAppCtrlSntpTimerId, 60000);
#endif

    BleWifi_Wifi_UpdateBeaconInfo();
    BleWifi_Wifi_SetDTIM(BleWifi_Ctrl_DtimTimeGet());
    BleWifi_Wifi_SendStatusInfo(BLEWIFI_IND_IP_STATUS_NOTIFY);

#ifdef ALI_BLE_WIFI_PROVISION
/*
    if(g_Ali_wifi_provision==1)
    {
#if ALI_TOKEN_FROM_DEVICE
        awss_cmp_local_init(AWSS_LC_INIT_SUC);
        awss_suc_notify_stop();
        awss_suc_notify();
#else
        uint8_t rsp[] = { 0x01, 0x01, 0x01 };
        tx_func_indicate(BZ_CMD_STATUS, rsp, sizeof(rsp));

#endif
        g_Ali_wifi_provision =0;
    }
*/
    BleWifi_Ctrl_EventStatusSet(BLEWIFI_CTRL_EVENT_BIT_LINK_CONN, true);
    if (true == BleWifi_Ctrl_EventStatusGet(BLEWIFI_CTRL_EVENT_BIT_ALI_WIFI_PRO))
    {
        BleWifi_Ctrl_EventStatusSet(BLEWIFI_CTRL_EVENT_BIT_ALI_STOP_BLE, true);
        BleWifi_Ctrl_EventStatusSet(BLEWIFI_CTRL_EVENT_BIT_ALI_WIFI_PRO, false);
    }
#endif
    BleWifi_Ctrl_EventStatusSet(BLEWIFI_CTRL_EVENT_BIT_GOT_IP, true);

}

static void BleWifi_Ctrl_TaskEvtHandler_WifiAutoConnectInd(uint32_t evt_type, void *data, int len)
{
    BLEWIFI_INFO("BLEWIFI: MSG BLEWIFI_CTRL_MSG_WIFI_AUTO_CONNECT_IND \r\n");
    BleWifi_Ctrl_DoAutoConnect();
}

#ifdef ALI_BLE_WIFI_PROVISION
static void BleWifi_Ctrl_TaskEvtHandler_AliWifiConnect(uint32_t evt_type, void *data, int len)
{
    uint8_t ssid_len=0;
    uint8_t pw_len=0;
    uint8_t bssid_len=0;
    uint8_t apptoken_len=0;
    uint8_t*  ap_data = (uint8_t *)(data);
				
    memset(&g_apInfo,0,sizeof(g_apInfo));
				
			
    ssid_len = *(ap_data);
    memcpy(g_apInfo.ssid, (ap_data+1), ssid_len);
				
    bssid_len = *(ap_data+ssid_len+1);
    memcpy(g_apInfo.bssid, (ap_data+ssid_len+2), bssid_len);
				
    pw_len = *(ap_data+ssid_len+bssid_len+2);
    memcpy(g_apInfo.pw, (ap_data+ssid_len+bssid_len+3), pw_len);

    apptoken_len = *(ap_data+ssid_len+bssid_len+pw_len+3);
    memcpy(g_apInfo.apptoken, (ap_data+ssid_len+bssid_len+pw_len+4), apptoken_len);

    
#ifdef ALI_OPL_DBG
    BLEWIFI_INFO("AliWifiConnect::Send apptoken data(%d):", apptoken_len);
    for(int i=0;i<apptoken_len;i++){
        BLEWIFI_INFO("0x%02x ", g_apInfo.apptoken[i]);
    }
    BLEWIFI_INFO("\r\n");
#endif

//    awss_set_token(g_apInfo.apptoken);

    uint8_t tmpdata[2];
    tmpdata[0] = 1;	// Enable to scan AP whose SSID is hidden
    tmpdata[1] = 2;	// mixed mode
    //g_Ali_wifi_provision =1;
    BleWifi_Ctrl_EventStatusSet(BLEWIFI_CTRL_EVENT_BIT_ALI_WIFI_PRO_1, true);
    g_u32AppAliWifiScanCount = 0;
    BleWifi_Wifi_DoScan(tmpdata, 2, 1);
}
#endif

static int entry_compare(const void *pEntry1, const void *pEntry2)
{
    int iRet = 0;

    T_BleWifi_Ctrl_SortDevSched *ptSched1 = (T_BleWifi_Ctrl_SortDevSched *)pEntry1;
    T_BleWifi_Ctrl_SortDevSched *ptSched2 = (T_BleWifi_Ctrl_SortDevSched *)pEntry2;

    if(ptSched1->ptSched->u8IsValid == ptSched2->ptSched->u8IsValid)
    {
        #ifdef BLEWIFI_SCHED_EXT
        int32_t s32TotalMin1 = (ptSched1->ptSched->u8Hour * 60) + (ptSched1->ptSched->u8Min) - (ptSched1->ptSchedExt->s32TimeZone / 60);
        int32_t s32TotalMin2 = (ptSched2->ptSched->u8Hour * 60) + (ptSched2->ptSched->u8Min) - (ptSched2->ptSchedExt->s32TimeZone / 60);
        #else
        int32_t s32TotalMin1 = (ptSched1->ptSched->u8Hour * 60) + (ptSched1->ptSched->u8Min) - (ptSched1->ptSched->s32TimeZone / 60);
        int32_t s32TotalMin2 = (ptSched2->ptSched->u8Hour * 60) + (ptSched2->ptSched->u8Min) - (ptSched2->ptSched->s32TimeZone / 60);
        #endif

        if(s32TotalMin1 < s32TotalMin2)
        {
            iRet = -1;
        }
        else if(s32TotalMin1 > s32TotalMin2)
        {
            iRet = 1;
        }
        else
        {
            if((ptSched1->ptSched->u8RepeatMask != 0) && (ptSched2->ptSched->u8RepeatMask == 0))
            {
                iRet = -1;
            }
            else if((ptSched1->ptSched->u8RepeatMask == 0) && (ptSched2->ptSched->u8RepeatMask != 0))
            {
                iRet = 1;
            }
            else
            {
                if(ptSched1->u8Idx < ptSched2->u8Idx)
                {
                    iRet = -1;
                }
                else
                {
                    iRet = 1;
                }
            }
        }
    }
    else
    {
        // move invalid entry to last
        if(!(ptSched1->ptSched->u8IsValid))
        {
            iRet = 1;
        }
        else if(!(ptSched2->ptSched->u8IsValid))
        {
            iRet = -1;
        }
    }

    return iRet;
}

static void dev_sched_sort(void)
{
    uint8_t i = 0;

    for(i = 0; i < MW_FIM_GP13_DEV_SCHED_NUM; i++)
    {
        g_taSortDevSched[i].u8Idx = i;
        g_taSortDevSched[i].ptSched = &(g_taDevSched[i]);

        #ifdef BLEWIFI_SCHED_EXT
        g_taSortDevSched[i].ptSchedExt = &(g_taDevSchedExt[i]);
        #endif
    }

    // sorting
    qsort((void *)g_taSortDevSched, MW_FIM_GP13_DEV_SCHED_NUM, sizeof(T_BleWifi_Ctrl_SortDevSched), entry_compare);
    g_u8SortDevSchedIdx = 0;
    return;
}

static void BleWifi_Ctrl_DevSchedTimerCallBack(void const *argu)
{
    BleWifi_Ctrl_MsgSend(BLEWIFI_CTRL_MSG_DEV_SCHED_TIMEOUT, (uint8_t *)&g_u32DevSchedTimerSeq, sizeof(g_u32DevSchedTimerSeq));
}

SHM_DATA void BleWifi_Ctrl_DevSchedStart(void)
{
    uint32_t u32DiffSec = 0;

    osTimerStop(g_tDevSchedTimer);

    for(; g_u8SortDevSchedIdx < MW_FIM_GP13_DEV_SCHED_NUM; g_u8SortDevSchedIdx++)
    {
        if((g_taSortDevSched[g_u8SortDevSchedIdx].ptSched->u8Enable) && (g_taSortDevSched[g_u8SortDevSchedIdx].ptSched->u8IsValid))
        {
            struct tm tInfo = {0};
            uint32_t u32Time = 0; // current seconds of today (based on time zone of current DevSched)
            uint8_t u8Mask = 0;

            #ifdef BLEWIFI_SCHED_EXT
            BleWifi_SntpGet(&tInfo, g_taSortDevSched[g_u8SortDevSchedIdx].ptSchedExt->s32TimeZone);
            #else
            BleWifi_SntpGet(&tInfo, g_taSortDevSched[g_u8SortDevSchedIdx].ptSched->s32TimeZone);
            #endif

            u32Time = (tInfo.tm_hour * 3600) + (tInfo.tm_min * 60) + tInfo.tm_sec;
        
            #ifdef BLEWIFI_SCHED_EXT
            BLEWIFI_WARN("time[%02u:%02u:%02u] wday[%u]: seconds_of_today[%u] for zone[%d]\n", tInfo.tm_hour, tInfo.tm_min, tInfo.tm_sec, tInfo.tm_wday, u32Time, 
                         g_taSortDevSched[g_u8SortDevSchedIdx].ptSchedExt->s32TimeZone);
            #else
            BLEWIFI_WARN("time[%02u:%02u:%02u] wday[%u]: seconds_of_today[%u] for zone[%d]\n", tInfo.tm_hour, tInfo.tm_min, tInfo.tm_sec, tInfo.tm_wday, u32Time, 
                         g_taSortDevSched[g_u8SortDevSchedIdx].ptSched->s32TimeZone);
            #endif
        
            if(tInfo.tm_wday == 0)
            {
                // sunday
                u8Mask = DEV_SCHED_REPEAT_MASK(DEV_SCHED_REPEAT_SUN);
            }
            else
            {
                // monday ~ saturday
                u8Mask = DEV_SCHED_REPEAT_MASK(tInfo.tm_wday);
            }

            if((g_taSortDevSched[g_u8SortDevSchedIdx].ptSched->u8RepeatMask == 0) || 
               (g_taSortDevSched[g_u8SortDevSchedIdx].ptSched->u8RepeatMask & u8Mask))
            {
                int32_t u32Start = (g_taSortDevSched[g_u8SortDevSchedIdx].ptSched->u8Hour * 3600) + (g_taSortDevSched[g_u8SortDevSchedIdx].ptSched->u8Min * 60);

                if(u32Start > u32Time)
                {
                    u32DiffSec = u32Start - u32Time;
                }
                else if((u32Start + 30) > u32Time)
                {
                    // a little bit late
                    u32DiffSec = 1;
                }

                if(u32DiffSec)
                {
                    #ifdef BLEWIFI_SCHED_EXT
                    BLEWIFI_WARN("[%s %d] process [%02u:%02u] Z[%d][%.2f] E[%u] V[%u] I[%u] sort_idx[%u] after %u seconds\n", __func__, __LINE__, 
                                 g_taSortDevSched[g_u8SortDevSchedIdx].ptSched->u8Hour, g_taSortDevSched[g_u8SortDevSchedIdx].ptSched->u8Min, 
                                 g_taSortDevSched[g_u8SortDevSchedIdx].ptSchedExt->s32TimeZone, ((double)g_taSortDevSched[g_u8SortDevSchedIdx].ptSchedExt->s32TimeZone / 3600), 
                                 g_taSortDevSched[g_u8SortDevSchedIdx].ptSched->u8Enable, 
                                 g_taSortDevSched[g_u8SortDevSchedIdx].ptSched->u8IsValid, g_taSortDevSched[g_u8SortDevSchedIdx].u8Idx, 
                                 g_u8SortDevSchedIdx, u32DiffSec);
                    #else
                    BLEWIFI_WARN("[%s %d] process [%02u:%02u] Z[%d][%.2f] E[%u] V[%u] I[%u] sort_idx[%u] after %u seconds\n", __func__, __LINE__, 
                                 g_taSortDevSched[g_u8SortDevSchedIdx].ptSched->u8Hour, g_taSortDevSched[g_u8SortDevSchedIdx].ptSched->u8Min, 
                                 g_taSortDevSched[g_u8SortDevSchedIdx].ptSched->s32TimeZone, ((double)g_taSortDevSched[g_u8SortDevSchedIdx].ptSched->s32TimeZone / 3600), 
                                 g_taSortDevSched[g_u8SortDevSchedIdx].ptSched->u8Enable, 
                                 g_taSortDevSched[g_u8SortDevSchedIdx].ptSched->u8IsValid, g_taSortDevSched[g_u8SortDevSchedIdx].u8Idx, 
                                 g_u8SortDevSchedIdx, u32DiffSec);
                    #endif

                    break;
                }
            }
        }
    }

    //if(g_u8SortDevSchedIdx == MW_FIM_GP13_DEV_SCHED_NUM)
    if(!u32DiffSec)
    {
        uint32_t u32OneDaySec = 86400;
        uint32_t u32MinSecToTomorrow = u32OneDaySec;
        uint8_t u8ActSchedExist = 0;
        uint32_t u32CheckedTimeZone = 8640000; // impossible timezone
        uint8_t i = 0;

        for(i = 0; i < MW_FIM_GP13_DEV_SCHED_NUM; i++)
        {
            if((g_taSortDevSched[i].ptSched->u8Enable) && (g_taSortDevSched[i].ptSched->u8IsValid))
            {
                #ifdef BLEWIFI_SCHED_EXT
                if(u32CheckedTimeZone != g_taSortDevSched[i].ptSchedExt->s32TimeZone)
                #else
                if(u32CheckedTimeZone != g_taSortDevSched[i].ptSched->s32TimeZone)
                #endif
                {
                    struct tm tInfo = {0};
                    uint32_t u32Time = 0;
                    uint32_t u32SecToTomorrow = 0;
    
                    #ifdef BLEWIFI_SCHED_EXT
                    BleWifi_SntpGet(&tInfo, g_taSortDevSched[i].ptSchedExt->s32TimeZone);
                    u32CheckedTimeZone = g_taSortDevSched[i].ptSchedExt->s32TimeZone;
                    #else
                    BleWifi_SntpGet(&tInfo, g_taSortDevSched[i].ptSched->s32TimeZone);
                    u32CheckedTimeZone = g_taSortDevSched[i].ptSched->s32TimeZone;
                    #endif
    
                    u32Time = (tInfo.tm_hour * 3600) + (tInfo.tm_min * 60) + tInfo.tm_sec;
    
                    if(u32Time < u32OneDaySec)
                    {
                        u32SecToTomorrow = u32OneDaySec - u32Time;
    
                        if(u32SecToTomorrow < u32MinSecToTomorrow)
                        {
                            u32MinSecToTomorrow = u32SecToTomorrow;
                        }
    
                        u8ActSchedExist = 1;
                    }
                    else
                    {
                        BLEWIFI_ERROR("[%s %d] current time[%02u:%02u:%02u] is invalid\n", __func__, __LINE__, 
                                      tInfo.tm_hour, tInfo.tm_min, tInfo.tm_sec);
                    }
                }
            }
        }

        if(u8ActSchedExist)
        {
            u32DiffSec = u32MinSecToTomorrow;

            BLEWIFI_WARN("[%s %d] check sched again after [%u] seconds\n", __func__, __LINE__, u32DiffSec);
        }
    }

    if(u32DiffSec)
    {
        BLEWIFI_WARN("[%s %d] start timer: %u seconds\n", __func__, __LINE__, u32DiffSec);

        ++g_u32DevSchedTimerSeq;
        osTimerStart(g_tDevSchedTimer, (u32DiffSec * osKernelSysTickFrequency)); // unit: ms
    }
    else
    {
        BLEWIFI_WARN("[%s %d] unnecessary to start timer\n", __func__, __LINE__);
    }

    return;
}

void BleWifi_Ctrl_DevSchedInit(void)
{
    uint8_t i = 0;

    if(!g_tDevSchedTimer)
    {
        osTimerDef_t tTimerButtonDef = {0};

        tTimerButtonDef.ptimer = BleWifi_Ctrl_DevSchedTimerCallBack;
        g_tDevSchedTimer = osTimerCreate(&tTimerButtonDef, osTimerOnce, NULL);

        if(!g_tDevSchedTimer)
        {
            BLEWIFI_ERROR("DevSchedInit: osTimerCreate fail\n");
        }
    }

    // get device schedule
    for(i = 0; i < MW_FIM_GP13_DEV_SCHED_NUM; i++)
    {
        if (MW_FIM_OK != MwFim_FileRead(MW_FIM_IDX_GP13_PROJECT_DEV_SCHED, i, MW_FIM_GP13_DEV_SCHED_SIZE, (uint8_t*)&(g_taDevSched[i])))
        {
            // if fail, get the default value
            BLEWIFI_ERROR("DevSchedInit: MwFim_FileRead fail\n");

            memcpy(&(g_taDevSched[i]), &g_tMwFimDefaultGp13DevSched, MW_FIM_GP13_DEV_SCHED_SIZE);
        }

        #ifdef BLEWIFI_SCHED_EXT
        if (MW_FIM_OK != MwFim_FileRead(MW_FIM_IDX_GP16_PROJECT_DEV_SCHED_EXT, i, MW_FIM_GP16_DEV_SCHED_EXT_SIZE, (uint8_t*)&(g_taDevSchedExt[i])))
        {
            // if fail, get the default value
            BLEWIFI_ERROR("DevSchedInit: MwFim_FileRead fail for sched ext\n");

            memcpy(&(g_taDevSchedExt[i]), &g_tMwFimDefaultGp16DevSchedExt, MW_FIM_GP16_DEV_SCHED_EXT_SIZE);
        }
        #endif
    }

    dev_sched_sort();

    //turn on device and led
    light_ctrl_set_switch(SWITCH_ON);
    return;
}

int BleWifi_Ctrl_DevSchedSetAll(T_BleWifi_Ctrl_DevSchedAll *ptSchedAll)
{
    int iRet = -1;

    if(!ptSchedAll)
    {
        BLEWIFI_ERROR("DevSchedSetAll: ptAllSched is NULL\n");
        goto done;
    }

    iRet = BleWifi_Ctrl_MsgSend(BLEWIFI_CTRL_MSG_DEV_SCHED_SET_ALL, (uint8_t *)ptSchedAll, sizeof(T_BleWifi_Ctrl_DevSchedAll));

done:
    return iRet;
}

static void BleWifi_Ctrl_TaskEvtHandler_DevSchedSetAll(uint32_t evt_type, void *data, int len)
{
    T_BleWifi_Ctrl_DevSchedAll *ptSchedAll = NULL;
    uint32_t u32Size = MW_FIM_GP13_DEV_SCHED_SIZE;
    uint8_t i = 0;
    uint8_t u8Update = 0;

    #ifdef BLEWIFI_SCHED_EXT
    uint32_t u32ExtSize = MW_FIM_GP16_DEV_SCHED_EXT_SIZE;
    #endif

    if(!data)
    {
        BLEWIFI_ERROR("DevSchedSetAll: data is NULL\n");
        goto done;
    }

    ptSchedAll = (T_BleWifi_Ctrl_DevSchedAll *)data;

    for(i = 0; (i < ptSchedAll->u8Num) && (i < MW_FIM_GP13_DEV_SCHED_NUM); i++)
    {
        if((ptSchedAll->taSched[i].u8Hour > 23) || (ptSchedAll->taSched[i].u8Min > 59))
        {
            // invalid time
            ptSchedAll->taSched[i].u8Enable = 0;
        }

        if(memcmp(&(ptSchedAll->taSched[i]), &(g_taDevSched[i]), u32Size))
        {
            memcpy(&(g_taDevSched[i]), &(ptSchedAll->taSched[i]), u32Size);

            if(MwFim_FileWrite(MW_FIM_IDX_GP13_PROJECT_DEV_SCHED, i, u32Size, (uint8_t *)&(g_taDevSched[i])) != MW_FIM_OK)
            {
                BLEWIFI_ERROR("DevSchedSetAll: MwFim_FileWrite fail\n");
            }

            u8Update = 1;
        }

        #ifdef BLEWIFI_SCHED_EXT
        if(memcmp(&(ptSchedAll->taSchedExt[i]), &(g_taDevSchedExt[i]), u32ExtSize))
        {
            memcpy(&(g_taDevSchedExt[i]), &(ptSchedAll->taSchedExt[i]), u32ExtSize);

            if(MwFim_FileWrite(MW_FIM_IDX_GP16_PROJECT_DEV_SCHED_EXT, i, u32ExtSize, (uint8_t *)&(g_taDevSchedExt[i])) != MW_FIM_OK)
            {
                BLEWIFI_ERROR("DevSchedSetAll: MwFim_FileWrite fail for sched ext\n");
            }

            u8Update = 1;
        }
        #endif
    }

    // post local timer setting
    iot_update_local_timer(0);

    if(u8Update)
    {
        dev_sched_sort();

        BleWifi_Ctrl_DevSchedStart();
    }

done:
    return;
}

static void BleWifi_Ctrl_TaskEvtHandler_DevSchedTimeout(uint32_t evt_type, void *data, int len)
{
    uint32_t *pu32Seq = (uint32_t *)data;

    BLEWIFI_INFO("BLEWIFI: MSG BLEWIFI_CTRL_MSG_DEV_SCHED_TIMEOUT \r\n");

    if(*pu32Seq != g_u32DevSchedTimerSeq)
    {
        BLEWIFI_WARN("sched timer id not match\n");
        goto done;
    }

    if(g_u8SortDevSchedIdx < MW_FIM_GP13_DEV_SCHED_NUM)
    {
        if((g_taSortDevSched[g_u8SortDevSchedIdx].ptSched->u8Enable) && (g_taSortDevSched[g_u8SortDevSchedIdx].ptSched->u8IsValid))
        {
            #ifdef ADA_REMOTE_CTRL 
            /* stop effect operation triggered by key controller*/
            light_effect_timer_stop();
            #endif

            /* stop effect operation triggered by APP*/
            light_effect_stop(NULL);

            if(g_taSortDevSched[g_u8SortDevSchedIdx].ptSched->u8DevOn != light_ctrl_get_switch())
            {
                light_ctrl_set_switch(g_taSortDevSched[g_u8SortDevSchedIdx].ptSched->u8DevOn);
                iot_update_local_timer(1);
            }

            if (light_ctrl_get_switch() && light_effect_get_status())
                light_effect_start(NULL, 0);

            // disable non-repeat entry automatically
            if(g_taSortDevSched[g_u8SortDevSchedIdx].ptSched->u8RepeatMask == 0)
            {
                uint8_t u8FimIdx = g_taSortDevSched[g_u8SortDevSchedIdx].u8Idx;

                g_taDevSched[u8FimIdx].u8Enable = 0;

                if(MwFim_FileWrite(MW_FIM_IDX_GP13_PROJECT_DEV_SCHED, u8FimIdx, MW_FIM_GP13_DEV_SCHED_SIZE, (uint8_t *)&(g_taDevSched[u8FimIdx])) != MW_FIM_OK)
                {
                    BLEWIFI_ERROR("DevSchedTimeout: MwFim_FileWrite fail\n");
                }

                iot_update_local_timer(0);
            }
        }

        g_u8SortDevSchedIdx += 1;
    }
    else
    {
        g_u8SortDevSchedIdx = 0;
    }
    
    BleWifi_Ctrl_DevSchedStart();

done:
    return;
}

static void BleWifi_Ctrl_TaskEvtHandler_DevSchedStart(uint32_t evt_type, void *data, int len)
{
    BleWifi_Ctrl_DevSchedStart();
}

#ifdef BLEWIFI_LIGHT_CTRL
static void BleWifi_Ctrl_TaskEvtHandler_LightCtrlTimeout(uint32_t evt_type, void *data, int len)
{
    extern void light_effect_update(void);

    light_effect_update();
}
#endif

#ifdef BLEWIFI_LIGHT_DRV
static void BleWifi_Ctrl_TaskEvtHandler_LightDrvTimeout(uint32_t evt_type, void *data, int len)
{
    extern void fade_step_exe_callback(uint32_t u32TmrIdx);

    fade_step_exe_callback(1);
}
#endif

static void BleWifi_Ctrl_TaskEvtHandler_OtherOtaOn(uint32_t evt_type, void *data, int len)
{
    BLEWIFI_INFO("BLEWIFI: MSG BLEWIFI_CTRL_MSG_OTHER_OTA_ON \r\n");
    BleWifi_Ctrl_EventStatusSet(BLEWIFI_CTRL_EVENT_BIT_OTA, true);
}

static void BleWifi_Ctrl_TaskEvtHandler_OtherOtaOff(uint32_t evt_type, void *data, int len)
{
    BLEWIFI_INFO("BLEWIFI: MSG BLEWIFI_CTRL_MSG_OTHER_OTA_OFF \r\n");
    BleWifi_Ctrl_EventStatusSet(BLEWIFI_CTRL_EVENT_BIT_OTA, false);
    msg_print_uart1("OK\r\n");
    
    // restart the system
    osDelay(BLEWIFI_CTRL_RESET_DELAY);
    Hal_Sys_SwResetAll();
}

static void BleWifi_Ctrl_TaskEvtHandler_OtherOtaOffFail(uint32_t evt_type, void *data, int len)
{
    BLEWIFI_INFO("BLEWIFI: MSG BLEWIFI_CTRL_MSG_OTHER_OTA_OFF_FAIL \r\n");
    BleWifi_Ctrl_EventStatusSet(BLEWIFI_CTRL_EVENT_BIT_OTA, false);
    msg_print_uart1("ERROR\r\n");
}

static void BleWifi_Ctrl_TaskEvtHandler_OtherSysTimer(uint32_t evt_type, void *data, int len)
{
    BLEWIFI_INFO("BLEWIFI: MSG BLEWIFI_CTRL_MSG_OTHER_SYS_TIMER \r\n");
    BleWifi_Ctrl_SysStatusChange();
}

void BleWifi_Ctrl_NetworkingStart_LedBlink(void)
{
    g_NetLedTransState = NETWORK_START_LED;
    g_NetLedTimeCounter = 0;
    //turn on the blue light

    #ifdef BLEWIFI_REFINE_INIT_FLOW
    light_ctrl_set_hsv(240, 100, 100, LIGHT_FADE_OFF, DONT_UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
    #else
    light_ctrl_set_hsv(240, 100, 100, LIGHT_FADE_OFF, UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
    #endif
    
    osTimerStop(g_tAppCtrlNetworkingLedTimerId);
    osTimerStart(g_tAppCtrlNetworkingLedTimerId, BLEWIFI_CTRL_LEDTRANS_TIME);
}

void BleWifi_Ctrl_NetworkingStop_LedBlink(void)
{
    g_NetLedTimeCounter = 0;
    osTimerStop(g_tAppCtrlNetworkingLedTimerId);
    osTimerStart(g_tAppCtrlNetworkingLedTimerId, BLEWIFI_CTRL_LEDTRANS_TIME);
}

void BleWifi_Ctrl_DevSchedClear(void)
{
    uint32_t u32Size = MW_FIM_GP13_DEV_SCHED_SIZE;
    uint8_t i = 0;

    #ifdef BLEWIFI_SCHED_EXT
    uint32_t u32ExtSize = MW_FIM_GP16_DEV_SCHED_EXT_SIZE;
    #endif

    ++g_u32DevSchedTimerSeq;
    osTimerStop(g_tDevSchedTimer);

    for(i = 0; i < MW_FIM_GP13_DEV_SCHED_NUM; i++)
    {
        if(memcmp(&(g_taDevSched[i]), &g_tMwFimDefaultGp13DevSched, u32Size))
        {
            memcpy(&(g_taDevSched[i]), &g_tMwFimDefaultGp13DevSched, u32Size);

            if(MwFim_FileWrite(MW_FIM_IDX_GP13_PROJECT_DEV_SCHED, i, u32Size, (uint8_t *)&(g_taDevSched[i])) != MW_FIM_OK)
            {
                BLEWIFI_ERROR("DevSchedClear: MwFim_FileWrite fail\n");
            }
        }

        #ifdef BLEWIFI_SCHED_EXT
        if(memcmp(&(g_taDevSchedExt[i]), &g_tMwFimDefaultGp16DevSchedExt, u32ExtSize))
        {
            memcpy(&(g_taDevSchedExt[i]), &g_tMwFimDefaultGp16DevSchedExt, u32ExtSize);

            if(MwFim_FileWrite(MW_FIM_IDX_GP16_PROJECT_DEV_SCHED_EXT, i, u32ExtSize, (uint8_t *)&(g_taDevSchedExt[i])) != MW_FIM_OK)
            {
                BLEWIFI_ERROR("DevSchedExtClear: MwFim_FileWrite fail\n");
            }
        }
        #endif
    }

    dev_sched_sort();

    return;
}

void BleWifi_Ctrl_NetworkingStart(void)
{
    g_led_mp_mode_flag = 0;

    if (false == BleWifi_Ctrl_EventStatusGet(BLEWIFI_CTRL_EVENT_BIT_NETWORK))
    {
        BLEWIFI_INFO("[%s %d] start\n", __func__, __LINE__);

        BleWifi_Ctrl_EventStatusSet(BLEWIFI_CTRL_EVENT_BIT_NETWORK, true);

        // stop light effect
        light_effect_stop(NULL);

        // start LED blink
        BleWifi_Ctrl_NetworkingStart_LedBlink();

        // restart timer
        osTimerStop(g_tAppButtonBleAdvTimerId);
        osTimerStart(g_tAppButtonBleAdvTimerId, BLEWIFI_CTRL_BLEADVSTOP_TIME);

        BLEWIFI_WARN("[%s %d] restart ble adv timer[%u]\n", __func__, __LINE__, BLEWIFI_CTRL_BLEADVSTOP_TIME);

        #ifdef BLEWIFI_REFINE_INIT_FLOW
        iot_load_cfg(0);
        iot_update_cfg();
        #else
        // clear auto-connect setting
        if(get_auto_connect_ap_num())
        {
            wifi_auto_connect_reset();
            set_auto_connect_save_ap_num(1);
        }

        // start adv
        BleWifi_Ble_StartAdvertising();
        #endif

        BleWifi_Ctrl_DevSchedClear();
    }
    else
    {
        BLEWIFI_WARN("[%s %d] BLEWIFI_CTRL_EVENT_BIT_NETWORK already true\n", __func__, __LINE__);
    }
}

void BleWifi_Ctrl_NetworkingStop(uint8_t u8RestartBleAdvTimer)
{
    if (true == BleWifi_Ctrl_EventStatusGet(BLEWIFI_CTRL_EVENT_BIT_NETWORK))
    {
        BLEWIFI_INFO("[%s %d] start\n", __func__, __LINE__);

        BleWifi_Ctrl_EventStatusSet(BLEWIFI_CTRL_EVENT_BIT_NETWORK, false);

        #ifdef BLEWIFI_REFINE_INIT_FLOW
        if((u8RestartBleAdvTimer) && (true == BleWifi_Ctrl_EventStatusGet(BLEWIFI_CTRL_EVENT_BIT_NETWORK_BLE_ADV)))
        {
            osTimerStop(g_tAppButtonBleAdvTimerId);
            osTimerStart(g_tAppButtonBleAdvTimerId, BLEWIFI_CTRL_BLEDISC_TIME);
        }
        #else
        // stop timer
        osTimerStop(g_tAppButtonBleAdvTimerId);

        // stop adv or disconnect
        if(false == BleWifi_Ctrl_EventStatusGet(BLEWIFI_CTRL_EVENT_BIT_BLE))
        {
            BleWifi_Ble_StopAdvertising();
        }
        else
        {
            BleWifi_Ble_Disconnect();
        }
        #endif

        // stop LED blink
        BleWifi_Ctrl_NetworkingStop_LedBlink();
    }
    else
    {
        BLEWIFI_WARN("[%s %d] BLEWIFI_CTRL_EVENT_BIT_NETWORK already false\n", __func__, __LINE__);
    }

#if 0
    #ifdef BLEWIFI_REFINE_INIT_FLOW
    if (true == BleWifi_Ctrl_EventStatusGet(BLEWIFI_CTRL_EVENT_BIT_NETWORK_BLE_ADV))
    {
        BleWifi_Ctrl_EventStatusSet(BLEWIFI_CTRL_EVENT_BIT_NETWORK_BLE_ADV, false);
    
        // stop timer
        osTimerStop(g_tAppButtonBleAdvTimerId);
    
        // stop adv or disconnect
        if(false == BleWifi_Ctrl_EventStatusGet(BLEWIFI_CTRL_EVENT_BIT_BLE))
        {
            BleWifi_Ble_StopAdvertising();
        }
        else
        {
            BleWifi_Ble_Disconnect();
        }
    }
    else
    {
        BLEWIFI_WARN("[%s %d] BLEWIFI_CTRL_EVENT_BIT_NETWORK_BLE_ADV already false\n", __func__, __LINE__);
    }
    #endif
#endif
}

#ifdef BLEWIFI_REFINE_INIT_FLOW
static void BleWifi_Ctrl_TaskEvtHandler_BleAdvStart(uint32_t evt_type, void *data, int len)
{
    if (false == BleWifi_Ctrl_EventStatusGet(BLEWIFI_CTRL_EVENT_BIT_NETWORK_BLE_ADV))
    {
        BLEWIFI_INFO("[%s %d] start\n", __func__, __LINE__);

        BleWifi_Ctrl_EventStatusSet(BLEWIFI_CTRL_EVENT_BIT_NETWORK_BLE_ADV, true);
    
        // restart timer
        osTimerStop(g_tAppButtonBleAdvTimerId);
        osTimerStart(g_tAppButtonBleAdvTimerId, BLEWIFI_CTRL_BLEADVSTOP_TIME);

        // start adv
        BleWifi_Ble_StartAdvertising();
    }
    else
    {
        BLEWIFI_WARN("[%s %d] BLEWIFI_CTRL_EVENT_BIT_NETWORK_BLE_ADV already true\n", __func__, __LINE__);
    }
}
#endif

static void BleWifi_Ctrl_TaskEvtHandler_NetworkingStart(uint32_t evt_type, void *data, int len)
{
    BleWifi_Ctrl_NetworkingStart();
}

static void BleWifi_Ctrl_TaskEvtHandler_NetworkingStop(uint32_t evt_type, void *data, int len)
{
    g_NetLedTransState = NETWORK_STOP_LED;
    BleWifi_Ctrl_NetworkingStop(1);
}

void BleWifi_Ctrl_TaskEvtHandler(uint32_t evt_type, void *data, int len)
{
    uint32_t i = 0;

    while (g_tCtrlEvtHandlerTbl[i].ulEventId != 0xFFFFFFFF)
    {
        // match
        if (g_tCtrlEvtHandlerTbl[i].ulEventId == evt_type)
        {
            g_tCtrlEvtHandlerTbl[i].fpFunc(evt_type, data, len);
            break;
        }

        i++;
    }

    // not match
    if (g_tCtrlEvtHandlerTbl[i].ulEventId == 0xFFFFFFFF)
    {
    }
}

#ifdef ALI_SINGLE_TASK
extern void ali_netlink_Task(void *args);
extern void Iot_Data_RxTask(void *args);
extern void *CoAPServer_yield(void *param);

osThreadId g_tAliCtrlTaskId = NULL;

void Ali_Ctrl_Task(void *args)
{
    void *pArg = NULL;

    while(1)
    {
        ali_netlink_Task(pArg);
        Iot_Data_RxTask(pArg);

        #ifdef ALI_RHYTHM_SUPPORT
        CoAPServer_yield(pArg);
        #endif
    }
}
#endif //#ifdef ALI_SINGLE_TASK

void BleWifi_Ctrl_Task(void *args)
{
    osEvent rxEvent;
    xBleWifiCtrlMessage_t *rxMsg;

    for(;;)
    {
        /* Wait event */
        rxEvent = osMessageGet(g_tAppCtrlQueueId, osWaitForever);
        if(rxEvent.status != osEventMessage)
            continue;

        rxMsg = (xBleWifiCtrlMessage_t *)rxEvent.value.p;
        BleWifi_Ctrl_TaskEvtHandler(rxMsg->event, rxMsg->ucaMessage, rxMsg->length);

        /* Release buffer */
        if (rxMsg != NULL)
            free(rxMsg);
    }
}

int BleWifi_Ctrl_MsgSend(int msg_type, uint8_t *data, int data_len)
{
    xBleWifiCtrlMessage_t *pMsg = 0;

	if (NULL == g_tAppCtrlQueueId)
	{
        BLEWIFI_ERROR("BLEWIFI: queue is null\n");
        return -1;
	}

    /* Mem allocate */
    pMsg = malloc(sizeof(xBleWifiCtrlMessage_t) + data_len);
    if (pMsg == NULL)
	{
        BLEWIFI_ERROR("BLEWIFI: malloc fail\n");
	    goto error;
    }
    
    pMsg->event = msg_type;
    pMsg->length = data_len;
    if (data_len > 0)
    {
        memcpy(pMsg->ucaMessage, data, data_len);
    }

    if (osMessagePut(g_tAppCtrlQueueId, (uint32_t)pMsg, 0) != osOK)
    {
        BLEWIFI_ERROR("BLEWIFI: osMessagePut fail\n");
        goto error;
    }

    return 0;

error:
	if (pMsg != NULL)
	{
	    free(pMsg);
    }
	
	return -1;
}

static void BleWifi_Ctrl_TaskEvtHandler_ButtonBleAdvTimeOut(uint32_t evt_type, void *data, int len)
{
    g_NetLedTransState = NETWORK_STOP_TOUT_LED;
    BleWifi_Ctrl_NetworkingStop(0);

    #ifdef BLEWIFI_REFINE_INIT_FLOW
    if (true == BleWifi_Ctrl_EventStatusGet(BLEWIFI_CTRL_EVENT_BIT_NETWORK_BLE_ADV))
    {
        BleWifi_Ctrl_EventStatusSet(BLEWIFI_CTRL_EVENT_BIT_NETWORK_BLE_ADV, false);
    
        // stop timer
        osTimerStop(g_tAppButtonBleAdvTimerId);
    
        // stop adv or disconnect
        if(false == BleWifi_Ctrl_EventStatusGet(BLEWIFI_CTRL_EVENT_BIT_BLE))
        {
            BleWifi_Ble_StopAdvertising();
        }
        else
        {
            BleWifi_Ble_Disconnect();
        }
    }
    else
    {
        BLEWIFI_WARN("[%s %d] BLEWIFI_CTRL_EVENT_BIT_NETWORK_BLE_ADV already false\n", __func__, __LINE__);
    }
    #endif
}

static void BleWifi_ButtonPress_BleAdvTimerCallBack(void const *argu)
{    
    BleWifi_Ctrl_MsgSend(BLEWIFI_CTRL_MSG_BUTTON_BLE_ADV_TIMEOUT, NULL, 0);
}

void BleWifi_Ctrl_BleAdv_Timer_Init(void)
{
    osTimerDef_t tTimerButtonBleAdvDef;
    
    // create the timer
    tTimerButtonBleAdvDef.ptimer = BleWifi_ButtonPress_BleAdvTimerCallBack;
    g_tAppButtonBleAdvTimerId = osTimerCreate(&tTimerButtonBleAdvDef, osTimerOnce, NULL);
    if (g_tAppButtonBleAdvTimerId == NULL)
    {
        BLEWIFI_ERROR("BleAdvTimer: osTimerCreate fail\n");
    }
}

#ifdef BLEWIFI_REFINE_INIT_FLOW
static void BleWifi_Networking_LedTransformTimerCallBack(void const *argu)
{
    BleWifi_Ctrl_MsgSend(BLEWIFI_CTRL_MSG_NETWORKING_LED_TIMEOUT, NULL, 0);
}

static void BleWifi_Ctrl_TaskEvtHandler_NetworkingLedTimeOut(uint32_t evt_type, void *data, int len)
{
    uint8_t u8Restart = 1;

    osTimerStop(g_tAppCtrlNetworkingLedTimerId);

    g_NetLedTimeCounter++;

    switch(g_NetLedTransState)
    {
        case NETWORK_START_LED:
            if (g_NetLedTimeCounter < 1)
            {
                break;
            }
            else 
            {
                if (g_NetLedTimeCounter%2!=0)
                {
                    //turn on the red light
                    light_ctrl_set_hsv(0, 100, 100, LIGHT_FADE_OFF, DONT_UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
                }
                else
                {
                    //turn off the red light
                    light_ctrl_set_hsv(0, 0, 0, LIGHT_FADE_OFF, DONT_UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
                }
                break;
            }

        case NETWORK_STOP_LED:
            if (g_NetLedTimeCounter==1)
            {
                //turn on the red light
                light_ctrl_set_hsv(0, 100, 100, LIGHT_FADE_OFF, DONT_UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
            }
            else if (g_NetLedTimeCounter==2)
            {
                //turn on the green light
                light_ctrl_set_hsv(120, 100, 100, LIGHT_FADE_OFF, DONT_UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
            }
            else if (g_NetLedTimeCounter==3)
            {
                //turn on the blue light
                light_ctrl_set_hsv(240, 100, 100, LIGHT_FADE_OFF, DONT_UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
            }
            else if (g_NetLedTimeCounter==4)
            {
                if(light_ctrl_get_light_type() != LT_RGB)
                {
                    //turn on the cold light
                    light_ctrl_set_ctb(7000, 100, LIGHT_FADE_OFF, DONT_UPDATE_LED_STATUS);
                }
            }
            else
            {
                g_NetLedTimeCounter = 0;

                //light_ctrl_set_ctb(light_ctrl_get_brightness(), LIGHT_FADE_ON, DONT_UPDATE_LED_STATUS);

                iot_apply_cfg(0);
                
                u8Restart = 0;
            }
            
            break;

        case NETWORK_STOP_TOUT_LED:
            {
                g_NetLedTimeCounter = 0;

                //light_ctrl_set_hsv(0, 100, 100, LIGHT_FADE_OFF, DONT_UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ); //turn on the red light

                iot_apply_cfg(0);
                
                u8Restart = 0;
            }

            break;

        default:
            BLEWIFI_ERROR("LedTransform: Wrong State\n");
            break;
    }

    if(u8Restart)
    {
        osTimerStart(g_tAppCtrlNetworkingLedTimerId, BLEWIFI_CTRL_LEDTRANS_TIME);
    }

    return;
}
#else //#ifdef BLEWIFI_REFINE_INIT_FLOW
static void BleWifi_Networking_LedTransformTimerCallBack(void const *argu)
{
    g_NetLedTimeCounter++;
    switch(g_NetLedTransState)
    {
        case NETWORK_START_LED:
            if (g_NetLedTimeCounter < 1)
            {
                break;
            }
            else 
            {
                if (g_NetLedTimeCounter%2!=0)
                {
                    //turn on the red light
                    light_ctrl_set_hsv(0, 100, 100, LIGHT_FADE_OFF, UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
                }
                else
                {
                    //turn off the red light
                    light_ctrl_set_hsv(0, 0, 0, LIGHT_FADE_OFF, UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
                }
                break;
            }
        case NETWORK_STOP_LED:
            if (g_NetLedTimeCounter==1)
            {
                //turn on the red light
                light_ctrl_set_hsv(0, 100, 100, LIGHT_FADE_OFF, UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
            }
            else if (g_NetLedTimeCounter==2)
            {
                //turn on the green light
                light_ctrl_set_hsv(120, 100, 100, LIGHT_FADE_OFF, UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
            }
            else if (g_NetLedTimeCounter==3)
            {
                //turn on the blue light
                light_ctrl_set_hsv(240, 100, 100, LIGHT_FADE_OFF, UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
            }
            else if (g_NetLedTimeCounter==4)
            {
                //turn on the cold light
                light_ctrl_set_ctb(100, LIGHT_FADE_OFF, UPDATE_LED_STATUS);
            }
            else
            {
                g_NetLedTimeCounter = 0;
                light_ctrl_set_ctb(light_ctrl_get_brightness(), LIGHT_FADE_ON, UPDATE_LED_STATUS);
                osTimerStop(g_tAppCtrlNetworkingLedTimerId);
            }
            break;
        case NETWORK_STOP_TOUT_LED:
            {
                g_NetLedTimeCounter = 0;
                //turn on the red light
                light_ctrl_set_hsv(0, 100, 100, LIGHT_FADE_OFF, UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
                osTimerStop(g_tAppCtrlNetworkingLedTimerId);
            }
            break;
        default:
            BLEWIFI_ERROR("LedTransform: Wrong State\n");
            break;
    }
}
#endif //#ifdef BLEWIFI_REFINE_INIT_FLOW

void BleWifi_Ctrl_BleAdvLedBlink_Timer_Init(void)
{
    osTimerDef_t tTimerCtrlNetworkLedStartStopDef;

    //create the Network Stop LED Blink timer
    tTimerCtrlNetworkLedStartStopDef.ptimer = BleWifi_Networking_LedTransformTimerCallBack;

    #ifdef BLEWIFI_REFINE_INIT_FLOW
    g_tAppCtrlNetworkingLedTimerId = osTimerCreate(&tTimerCtrlNetworkLedStartStopDef, osTimerOnce, (void*)0);
    #else
    g_tAppCtrlNetworkingLedTimerId = osTimerCreate(&tTimerCtrlNetworkLedStartStopDef, osTimerPeriodic, (void*)0);
    #endif
    
    if (g_tAppCtrlNetworkingLedTimerId == NULL)
    {
        BLEWIFI_ERROR("AdvLedBlink: osTimerCreate fail\n");
    }
}

#if (SNTP_FUNCTION_EN == 1)
volatile uint8_t g_u8SntpReady = 0;

static void sntp_run(void)
{
    int iSntpRet = 0;
    uint32_t u32Timer = 86400000; // ms

    BLEWIFI_INFO("SntpTimeOut\n");

    osTimerStop(g_tAppCtrlSntpTimerId);

    if(true != BleWifi_Ctrl_EventStatusGet(BLEWIFI_CTRL_EVENT_BIT_GOT_IP))
    {
        BLEWIFI_ERROR("SntpTimeOut: got_ip not true\n");
        goto done;
    }

    iSntpRet = BleWifi_SntpInit();

    if(iSntpRet)
    {
        // SNTP success
        g_u8SntpReady = 1;
        BleWifi_Ctrl_DevSchedStart();
    }
    else
    {
        // SNTP fail
        BLEWIFI_ERROR("SntpTimeOut: SNTP fail\n");
        goto done;
    }

done:
    if(!g_u8SntpReady)
    {
        u32Timer = 60000; // ms
    }

    BLEWIFI_INFO("SntpTimeOut: Restart Timer[%u]\n", u32Timer);

    osTimerStart(g_tAppCtrlSntpTimerId, u32Timer); // unit: ms
    return;
}
/*
static void BleWifi_Ctrl_TaskEvtHandler_SntpStart(uint32_t evt_type, void *data, int len)
{
    BLEWIFI_INFO("SntpStart\n");

    osTimerStop(g_tAppCtrlSntpTimerId);

    osTimerStart(g_tAppCtrlSntpTimerId, 60000);
}
*/
static void BleWifi_Ctrl_TaskEvtHandler_SntpTimeOut(uint32_t evt_type, void *data, int len)
{
    sntp_run();
}

static void BleWifi_Ctrl_SntpTimerCallBack(void const *argu)
{    
    BleWifi_Ctrl_MsgSend(BLEWIFI_CTRL_MSG_SNTP_TIMEOUT, NULL, 0);
}

void BleWifi_Ctrl_Sntp_Timer_Init(void)
{
    osTimerDef_t tTimerSntpDef = {0};

    //create SNTP timer
    tTimerSntpDef.ptimer = BleWifi_Ctrl_SntpTimerCallBack;
    g_tAppCtrlSntpTimerId = osTimerCreate(&tTimerSntpDef, osTimerPeriodic, (void*)0);

    if (g_tAppCtrlSntpTimerId == NULL)
    {
        BLEWIFI_ERROR("Sntp_Timer: osTimerCreate fail\n");
    }
}
#endif //#if (SNTP_FUNCTION_EN == 1)

#ifdef BLEWIFI_REFINE_INIT_FLOW
#else
void BleWifi_Ctrl_BootCntUpdate(void)
{
    if (MW_FIM_OK != MwFim_FileRead(MW_FIM_IDX_GP14_PROJECT_BOOT_STATUS, 0, MW_FIM_GP14_BOOT_STATUS_SIZE, (uint8_t *)&g_tBootStatus))
    {
        BLEWIFI_ERROR("BootCntUpdate: MwFim_FileRead fail\n");

        // if fail, get the default value
        memcpy((void *)&g_tBootStatus, &g_tMwFimDefaultGp14BootStatus, MW_FIM_GP14_BOOT_STATUS_SIZE);
    }

    BLEWIFI_INFO("[%s %d] read boot_cnt[%u]\n", __func__, __LINE__, g_tBootStatus.u8Cnt);

    if(g_tBootStatus.u8Cnt < BLEWIFI_CTRL_BOOT_CNT_FOR_ALI_RESET)
    {
        g_tBootStatus.u8Cnt += 1;

        if (MW_FIM_OK != MwFim_FileWrite(MW_FIM_IDX_GP14_PROJECT_BOOT_STATUS, 0, MW_FIM_GP14_BOOT_STATUS_SIZE, (uint8_t *)&g_tBootStatus))
        {
            BLEWIFI_ERROR("BootCntUpdate: MwFim_FileWrite fail\n");
        }
    }

    BLEWIFI_WARN("[%s %d] current boot_cnt[%u]\n", __func__, __LINE__, g_tBootStatus.u8Cnt);

    return;
}
#endif

#define OS_TASK_STACK_SIZE_ALI_BLEWIFI_CTRL		(640)
#define OS_TASK_STACK_SIZE_ALI_CTRL		        (1280)

void BleWifi_Ctrl_Init(void)
{
    osThreadDef_t task_def;
    osMessageQDef_t blewifi_queue_def;
    osTimerDef_t timer_auto_connect_def;
    osTimerDef_t timer_sys_def;

    /* Create ble-wifi task */
#ifdef ALI_SINGLE_TASK
    osThreadDef_t ali_task_def = {0};

    ali_task_def.name = "ali_ctrl";
    ali_task_def.stacksize = OS_TASK_STACK_SIZE_ALI_CTRL;
    ali_task_def.tpriority = OS_TASK_PRIORITY_APP;
    ali_task_def.pthread = Ali_Ctrl_Task;

    g_tAliCtrlTaskId = osThreadCreate(&ali_task_def, NULL);

    if(g_tAliCtrlTaskId == NULL)
    {
        BLEWIFI_INFO("BLEWIFI: ali_ctrl task create fail \r\n");
    }
    else
    {
        BLEWIFI_INFO("BLEWIFI: ali_ctrl task create successful \r\n");
    }
#endif //#ifdef ALI_SINGLE_TASK

    task_def.name = "blewifi ctrl";
    task_def.stacksize = OS_TASK_STACK_SIZE_ALI_BLEWIFI_CTRL;
    task_def.tpriority = OS_TASK_PRIORITY_APP;
    task_def.pthread = BleWifi_Ctrl_Task;

    g_tAppCtrlTaskId = osThreadCreate(&task_def, (void*)NULL);
    if(g_tAppCtrlTaskId == NULL)
    {
        BLEWIFI_INFO("BLEWIFI: ctrl task create fail \r\n");
    }
    else
    {
        BLEWIFI_INFO("BLEWIFI: ctrl task create successful \r\n");
    }

    /* Create message queue*/
    blewifi_queue_def.item_sz = sizeof(xBleWifiCtrlMessage_t);
    blewifi_queue_def.queue_sz = BLEWIFI_CTRL_QUEUE_SIZE;
    g_tAppCtrlQueueId = osMessageCreate(&blewifi_queue_def, NULL);
    if(g_tAppCtrlQueueId == NULL)
    {
        BLEWIFI_ERROR("BLEWIFI: osMessageCreate fail\n");
    }

    /* create timer to trig auto connect */
    timer_auto_connect_def.ptimer = BleWifi_Ctrl_AutoConnectTrigger;
    g_tAppCtrlAutoConnectTriggerTimer = osTimerCreate(&timer_auto_connect_def, osTimerOnce, NULL);
    if(g_tAppCtrlAutoConnectTriggerTimer == NULL)
    {
        BLEWIFI_ERROR("BLEWIFI: osTimerCreate fail\n");
    }

    /* create timer to trig the sys state */
    timer_sys_def.ptimer = BleWifi_Ctrl_SysTimeout;
    g_tAppCtrlSysTimer = osTimerCreate(&timer_sys_def, osTimerOnce, NULL);
    if(g_tAppCtrlSysTimer == NULL)
    {
        BLEWIFI_ERROR("BLEWIFI: osTimerCreate fail\n");
    }

    /* Create the event group */
    g_tAppCtrlEventGroup = xEventGroupCreate();
    if(g_tAppCtrlEventGroup == NULL)
    {
        BLEWIFI_ERROR("BLEWIFI: xEventGroupCreate fail\n");
    }

    #ifdef BLEWIFI_REFINE_INIT_FLOW
    #else
    if(g_tBootStatus.u8Cnt >= BLEWIFI_CTRL_BOOT_CNT_FOR_ALI_RESET)
    {
        BLEWIFI_WARN("[%s %d] enable PREPARE_ALI_BOOT_RESET\n", __func__, __LINE__);

        BleWifi_Ctrl_EventStatusSet(BLEWIFI_CTRL_EVENT_BIT_PREPARE_ALI_BOOT_RESET, true);

        #ifdef ALI_UNBIND_REFINE
        HAL_SetReportReset(1);
        #endif
    }
    #endif

    /* the init state of system mode is init */
    g_ulAppCtrlSysMode = MW_FIM_SYS_MODE_INIT;

    // get the settings of Wifi connect settings
	if (MW_FIM_OK != MwFim_FileRead(MW_FIM_IDX_GP11_PROJECT_WIFI_CONNECT_SETTINGS, 0, MW_FIM_GP11_WIFI_CONNECT_SETTINGS_SIZE, (uint8_t*)&g_tAppCtrlWifiConnectSettings))
    {
        // if fail, get the default value
        memcpy(&g_tAppCtrlWifiConnectSettings, &g_tMwFimDefaultGp11WifiConnectSettings, MW_FIM_GP11_WIFI_CONNECT_SETTINGS_SIZE);
    }

    /* the init state of SYS is init */
    g_ubAppCtrlSysStatus = BLEWIFI_CTRL_SYS_INIT;
    // start the sys timer
    osTimerStop(g_tAppCtrlSysTimer);
    osTimerStart(g_tAppCtrlSysTimer, BLEWIFI_COM_SYS_TIME_INIT);

    /* Init the DTIM time (ms) */
    g_ulAppCtrlWifiDtimTime = g_tAppCtrlWifiConnectSettings.ulDtimInterval;

    // the idle of the connection retry
    g_ubAppCtrlRequestRetryTimes = BLEWIFI_CTRL_AUTO_CONN_STATE_IDLE;
    g_ulAppCtrlAutoConnectInterval = g_tAppCtrlWifiConnectSettings.ulAutoConnectIntervalInit;

    // ble adv timer init
    BleWifi_Ctrl_BleAdv_Timer_Init();

    // ble adv Led Blink timer init
    BleWifi_Ctrl_BleAdvLedBlink_Timer_Init();

    #if (SNTP_FUNCTION_EN == 1)
    BleWifi_Ctrl_Sntp_Timer_Init();
    #endif
}
