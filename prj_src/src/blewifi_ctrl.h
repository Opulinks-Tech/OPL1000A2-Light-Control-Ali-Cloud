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
 * @file blewifi_ctrl.h
 * @author Vincent Chen, Michael Liao
 * @date 20 Feb 2018
 * @brief File includes the function declaration of blewifi ctrl task.
 *
 */

#ifndef __BLEWIFI_CTRL_H__
#define __BLEWIFI_CTRL_H__

#include <stdint.h>
#include <stdbool.h>
#include "blewifi_configuration.h"
#include "mw_fim_default_group11_project.h"
#include "mw_fim_default_group13_project.h"

#define BLEWIFI_CTRL_QUEUE_SIZE         (20)
#define BLEWIFI_CTRL_BLEADVSTOP_TIME        (180000)  // ms
#define BLEWIFI_CTRL_LEDTRANS_TIME      (500)     //ms

typedef enum blewifi_ctrl_msg_type
{
    /* BLE Trigger */
    BLEWIFI_CTRL_MSG_BLE_INIT_COMPLETE = 0,             //BLE report status
    BLEWIFI_CTRL_MSG_BLE_ADVERTISING_CFM,               //BLE report status
    BLEWIFI_CTRL_MSG_BLE_ADVERTISING_EXIT_CFM,          //BLE report status
    BLEWIFI_CTRL_MSG_BLE_ADVERTISING_TIME_CHANGE_CFM,   //BLE report status
    BLEWIFI_CTRL_MSG_BLE_CONNECTION_COMPLETE,           //BLE report status
    BLEWIFI_CTRL_MSG_BLE_CONNECTION_FAIL,               //BLE report status
    BLEWIFI_CTRL_MSG_BLE_DISCONNECT,                    //BLE report status
    BLEWIFI_CTRL_MSG_BLE_DATA_IND,                      //BLE receive the data from peer to device

    BLEWIFI_CTRL_MSG_BLE_NUM,

    /* Wi-Fi Trigger */
    BLEWIFI_CTRL_MSG_WIFI_INIT_COMPLETE = 0x80, //Wi-Fi report status
    BLEWIFI_CTRL_MSG_WIFI_SCAN_DONE_IND,        //Wi-Fi report status
    BLEWIFI_CTRL_MSG_WIFI_CONNECTION_IND,       //Wi-Fi report status
    BLEWIFI_CTRL_MSG_WIFI_DISCONNECTION_IND,    //Wi-Fi report status
    BLEWIFI_CTRL_MSG_WIFI_GOT_IP_IND,           //Wi-Fi report status
    BLEWIFI_CTRL_MSG_WIFI_AUTO_CONNECT_IND,     //Wi-Fi the auto connection is triggered by timer
#ifdef ALI_BLE_WIFI_PROVISION
    BLEWIFI_CTRL_MSG_ALI_WIFI_CONNECT,          //Ali WiFi connect
#endif

    BLEWIFI_CTRL_MSG_WIFI_NUM,

    /* Others Event */
    BLEWIFI_CTRL_MSG_OTHER_OTA_ON = 0x100,      //OTA
    BLEWIFI_CTRL_MSG_OTHER_OTA_OFF,             //OTA success
    BLEWIFI_CTRL_MSG_OTHER_OTA_OFF_FAIL,        //OTA fail
    BLEWIFI_CTRL_MSG_OTHER_SYS_TIMER,           //SYS timer
    BLEWIFI_CTRL_MSG_NETWORKING_START,          //Networking Start
    BLEWIFI_CTRL_MSG_NETWORKING_STOP,           //Networking Stop
    BLEWIFI_CTRL_MSG_BUTTON_BLE_ADV_TIMEOUT,
    
    BLEWIFI_CTRL_MSG_DEV_SCHED_SET,                 //Set Device Schedule
    BLEWIFI_CTRL_MSG_DEV_SCHED_SET_ALL,             //Set All Device Schedules
    BLEWIFI_CTRL_MSG_DEV_SCHED_TIMEOUT,             //Device Schedule Time Out

    BLEWIFI_CTRL_MSG_OTHER__NUM
} blewifi_ctrl_msg_type_e;

typedef struct
{
    uint32_t event;
    uint32_t length;
    uint8_t ucaMessage[];
} xBleWifiCtrlMessage_t;

typedef enum blewifi_ctrl_sys_state
{
    BLEWIFI_CTRL_SYS_INIT = 0x00,       // PS(0), Wifi(1), Ble(1)
    BLEWIFI_CTRL_SYS_NORMAL,            // PS(1), Wifi(1), Ble(1)
    BLEWIFI_CTRL_SYS_BLE_OFF,           // PS(1), Wifi(1), Ble(0)

    BLEWIFI_CTRL_SYS_NUM
} blewifi_ctrl_sys_state_e;

// event group bit (0 ~ 23 bits)
#define BLEWIFI_CTRL_EVENT_BIT_BLE      0x00000001U
#define BLEWIFI_CTRL_EVENT_BIT_WIFI     0x00000002U
#define BLEWIFI_CTRL_EVENT_BIT_OTA      0x00000004U
#define BLEWIFI_CTRL_EVENT_BIT_GOT_IP   0x00000008U
#define BLEWIFI_CTRL_EVENT_BIT_IOT_INIT 0x00000010U
#define BLEWIFI_CTRL_EVENT_BIT_NETWORK  0x00000020U

#ifdef ALI_BLE_WIFI_PROVISION
#define BLEWIFI_CTRL_EVENT_BIT_UNBIND       0x00001000U
#define BLEWIFI_CTRL_EVENT_BIT_LINK_CONN    0x00002000U
#define BLEWIFI_CTRL_EVENT_BIT_ALI_STOP_BLE 0x00004000U
#define BLEWIFI_CTRL_EVENT_BIT_ALI_WIFI_PRO 0x00008000U     // from connection to got ip
#define BLEWIFI_CTRL_EVENT_BIT_ALI_WIFI_PRO_1   0x00010000U // from scan to got ip
#define BLEWIFI_CTRL_EVENT_BIT_PREPARE_ALI_RESET    0x00020000U
#define BLEWIFI_CTRL_EVENT_BIT_WAIT_ALI_RESET       0x00040000U
#define BLEWIFI_CTRL_EVENT_BIT_PREPARE_ALI_BOOT_RESET   0x00080000U
#endif

typedef void (*T_BleWifi_Ctrl_EvtHandler_Fp)(uint32_t evt_type, void *data, int len);
typedef struct
{
    uint32_t ulEventId;
    T_BleWifi_Ctrl_EvtHandler_Fp fpFunc;
} T_BleWifi_Ctrl_EvtHandlerTbl;

typedef struct
{
    uint8_t u8Idx;
    T_MwFim_GP13_Dev_Sched tSched;
} T_BleWifi_Ctrl_DevSchedMsg;

typedef struct
{
    uint8_t u8Num;
    T_MwFim_GP13_Dev_Sched taSched[MW_FIM_GP13_DEV_SCHED_NUM];
} T_BleWifi_Ctrl_DevSchedAll;

typedef struct
{
    uint8_t u8Idx;
    T_MwFim_GP13_Dev_Sched *ptSched;
} T_BleWifi_Ctrl_SortDevSched;

typedef enum
{
    DEV_SCHED_REPEAT_UNUSED = 0,

    DEV_SCHED_REPEAT_MON,
    DEV_SCHED_REPEAT_TUE,
    DEV_SCHED_REPEAT_WED,
    DEV_SCHED_REPEAT_THU,
    DEV_SCHED_REPEAT_FRI,
    DEV_SCHED_REPEAT_SAT,
    DEV_SCHED_REPEAT_SUN,

    DEV_SCHED_REPEAT_MAX
} T_DevSchedRepeat;

typedef enum
{
    DEV_IND_TYPE_STATUS = 0,
    DEV_IND_TYPE_SCHED,

    DEV_IND_TYPE_MAX
} T_DevIndType;

#define DEV_SCHED_REPEAT_MASK(repeat)       (1 << repeat)

#define BLEWIFI_CTRL_AUTO_CONN_STATE_IDLE   (g_tAppCtrlWifiConnectSettings.ubConnectRetry + 1)
#define BLEWIFI_CTRL_AUTO_CONN_STATE_SCAN   (BLEWIFI_CTRL_AUTO_CONN_STATE_IDLE + 1)

extern T_MwFim_GP11_WifiConnectSettings g_tAppCtrlWifiConnectSettings;

void BleWifi_Ctrl_SysModeSet(uint8_t mode);
uint8_t BleWifi_Ctrl_SysModeGet(void);
void BleWifi_Ctrl_EventStatusSet(uint32_t dwEventBit, uint8_t status);
uint8_t BleWifi_Ctrl_EventStatusGet(uint32_t dwEventBit);
uint8_t BleWifi_Ctrl_EventStatusWait(uint32_t dwEventBit, uint32_t millisec);
void BleWifi_Ctrl_DtimTimeSet(uint32_t value);
uint32_t BleWifi_Ctrl_DtimTimeGet(void);
int BleWifi_Ctrl_MsgSend(int msg_type, uint8_t *data, int data_len);
void BleWifi_Ctrl_Init(void);

int BleWifi_Ctrl_DevSchedSet(uint8_t u8Idx, T_MwFim_GP13_Dev_Sched *ptDevSched);
int BleWifi_Ctrl_DevSchedSetAll(T_BleWifi_Ctrl_DevSchedAll *ptSchedAll);
void BleWifi_Ctrl_DevSchedInit(void);

void BleWifi_Ctrl_NetworkingStart(void);
void BleWifi_Ctrl_NetworkingStop(void);

void BleWifi_Ctrl_BootCntUpdate(void);

#endif /* __BLEWIFI_CTRL_H__ */

