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
 * @file blewifi_app.c
 * @author Vincent Chen
 * @date 12 Feb 2018
 * @brief File creates the wifible app task architecture.
 *
 */
#include "blewifi_configuration.h"
#include "blewifi_common.h"
#include "blewifi_app.h"
#include "blewifi_wifi_api.h"
#include "blewifi_ble_api.h"
#include "blewifi_ctrl.h"
#include "blewifi_ctrl_http_ota.h"
#include "iot_data.h"
#include "sys_common_api.h"
#include "ps_public.h"
#include "mw_fim_default_group03.h"
#include "mw_fim_default_group03_patch.h"
#include "mw_fim_default_group11_project.h"
#include "app_at_cmd.h"
#include "wifi_api.h"

//light control
#include "hal_pwm.h"
#include "light_control.h"
#include "hal_pin.h"
#include "hal_pin_def.h"

#ifdef ALI_BLE_WIFI_PROVISION
#include "cmsis_os.h"
#include "lwip/netdb.h"
#include "errno.h"
#include "infra_config.h"
#include "breeze_export.h"
#include "string.h"
#include "infra_defs.h"
#include "ali_hal_decl.h"
#include "infra_compat.h"

#define hal_emerg(...)      HAL_Printf("[prt] "), HAL_Printf(__VA_ARGS__), HAL_Printf("\r\n")
#define hal_crit(...)       HAL_Printf("[prt] "), HAL_Printf(__VA_ARGS__), HAL_Printf("\r\n")
#define hal_err(...)        HAL_Printf("[prt] "), HAL_Printf(__VA_ARGS__), HAL_Printf("\r\n")
#define hal_warning(...)    HAL_Printf("[prt] "), HAL_Printf(__VA_ARGS__), HAL_Printf("\r\n")
#define hal_info(...)       HAL_Printf("[prt] "), HAL_Printf(__VA_ARGS__), HAL_Printf("\r\n")
#define hal_debug(...)      HAL_Printf("[prt] "), HAL_Printf(__VA_ARGS__), HAL_Printf("\r\n")

breeze_dev_info_t dinfo;
extern void linkkit_event_monitor(int event);
#endif

blewifi_ota_t *gTheOta = 0;
extern uint8_t g_light_reboot_flag;

osTimerId g_tAdaLedMPBlinkId;

//light type and light ctrl data structure init
void light_type_discern(void)
{
    Hal_Pin_ConfigSet(9, PIN_TYPE_GPIO_INPUT, PIN_DRIVING_HIGH);
    Hal_Pin_ConfigSet(10, PIN_TYPE_GPIO_INPUT, PIN_DRIVING_HIGH);
    Hal_Pin_ConfigSet(11, PIN_TYPE_GPIO_INPUT, PIN_DRIVING_HIGH);
    uint8_t lighttype = 0;
    light_ctrl_init();

    Hal_Pwm_Init();
    Hal_Pwm_ClockSourceSet(HAL_PWM_CLK_32K);
    cancel_default_breath();

    if(Hal_Vic_GpioInput(GPIO_IDX_09))
        lighttype = lighttype | 4;
    if(Hal_Vic_GpioInput(GPIO_IDX_10))
        lighttype = lighttype | 2;
    if(Hal_Vic_GpioInput(GPIO_IDX_11))
        lighttype = lighttype | 1;

    lighttype = TMP_LT_RGBCW;

    switch(lighttype)
    {
        case TMP_LT_RGB:
            light_ctrl_set_light_type(LT_RGB,HF_PWR);
            Hal_Pwm_SyncEnable(HAL_PWM_IDX_5|HAL_PWM_IDX_4|HAL_PWM_IDX_3);
            break;
        case TMP_LT_C:
            light_ctrl_set_light_type(LT_C,HF_PWR);
            Hal_Pwm_SyncEnable(HAL_PWM_IDX_2);
             break;
        case TMP_LT_CW:
            light_ctrl_set_light_type(LT_CW,HF_PWR);
            Hal_Pwm_SyncEnable(HAL_PWM_IDX_2|HAL_PWM_IDX_1);
            break;
        case TMP_LT_RGBC:
            light_ctrl_set_light_type(LT_RGBC,HF_PWR);
            Hal_Pwm_SyncEnable(HAL_PWM_IDX_5|HAL_PWM_IDX_4|HAL_PWM_IDX_3|HAL_PWM_IDX_2);
            break;
        case TMP_LT_RGBCW:
            light_ctrl_set_light_type(LT_RGBCW,HF_PWR);
            Hal_Pwm_SyncEnable(HAL_PWM_IDX_5|HAL_PWM_IDX_4|HAL_PWM_IDX_3|HAL_PWM_IDX_2|HAL_PWM_IDX_1);
            break;
        case TMP_LT_RGBCW_FPWR:
            light_ctrl_set_light_type(LT_RGBCW,FULL_PWR);
            Hal_Pwm_SyncEnable(HAL_PWM_IDX_5|HAL_PWM_IDX_4|HAL_PWM_IDX_3|HAL_PWM_IDX_2|HAL_PWM_IDX_1);
            break;
        case TMP_LT_CW_FPWR:
            light_ctrl_set_light_type(LT_CW,FULL_PWR);
            Hal_Pwm_SyncEnable(HAL_PWM_IDX_2|HAL_PWM_IDX_1);
            break;
	      default:
            light_ctrl_set_light_type(LT_RGBCW,HF_PWR);
            Hal_Pwm_SyncEnable(HAL_PWM_IDX_5|HAL_PWM_IDX_4|HAL_PWM_IDX_3|HAL_PWM_IDX_2|HAL_PWM_IDX_1);
            break;
    }
    Hal_Pin_ConfigSet(9, PIN_TYPE_NONE, PIN_DRIVING_LOW);
    Hal_Pin_ConfigSet(10, PIN_TYPE_NONE, PIN_DRIVING_LOW);
    Hal_Pin_ConfigSet(11, PIN_TYPE_NONE, PIN_DRIVING_LOW);
    light_ctrl_set_ctb(2000 , 1, LIGHT_FADE_ON);
    light_ctrl_set_hsv(0 ,0 ,0 , LIGHT_FADE_ON);
    light_ctrl_set_hsv(0 ,100 ,100 , LIGHT_FADE_ON);
    g_light_reboot_flag = 1;
}

void BleWifiAppInit(void)
{
    T_MwFim_SysMode tSysMode;
    T_MwFim_GP11_PowerSaving tPowerSaving;

    gTheOta = 0;
	
#if (SNTP_FUNCTION_EN == 1)
    g_ulSntpSecondInit = SNTP_SEC_2019;     // Initialize the Sntp Value
    g_ulSystemSecondInit = 0;               // Initialize System Clock Time
#endif

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

    // only for the user mode
    if ((tSysMode.ubSysMode == MW_FIM_SYS_MODE_INIT) || (tSysMode.ubSysMode == MW_FIM_SYS_MODE_USER))
    {
        /* Update Boot Counter */
        BleWifi_Ctrl_BootCntUpdate();

        /* Wi-Fi Initialization */
        BleWifi_Wifi_Init();

        /* BLE Stack Initialization */
        BleWifi_Ble_Init();

        /* blewifi "control" task Initialization */
        BleWifi_Ctrl_Init();

        /* blewifi HTTP OTA */
        #if (WIFI_OTA_FUNCTION_EN == 1)
        blewifi_ctrl_http_ota_task_create();

        // init ota schedule
        #if (WIFI_OTA_AUTOCHECK_EN == 1)
        blewifi_ctrl_ota_sched_init();
        #endif
        #endif

        Ali_Hal_Devive_init();
        HAL_GetProductKey(dinfo.product_key);
        HAL_GetProductSecret(dinfo.product_secret);
        HAL_GetDeviceName(dinfo.device_name);
        HAL_GetDeviceSecret(dinfo.device_secret);
        dinfo.product_id = HAL_GetProductId();
        iotx_event_regist_cb(linkkit_event_monitor);

        /* IoT device Initialization */
        #if (IOT_DEVICE_DATA_TX_EN == 1) || (IOT_DEVICE_DATA_RX_EN == 1)
        Iot_Data_Init();
        #endif

        // move the settings to blewifi_ctrl, when the sys status is changed from Init to Noraml
        /* Power saving settings */
        //if (tSysMode.ubSysMode == MW_FIM_SYS_MODE_USER)
        //    ps_smart_sleep(tPowerSaving.ubPowerSaving);

        /* RF Power settings */
        BleWifi_RFPowerSetting(tPowerSaving.ubRFPower);

        // init device schedule
        BleWifi_Ctrl_DevSchedInit();
    }

    // update the system mode
    BleWifi_Ctrl_SysModeSet(tSysMode.ubSysMode);

    // add app cmd
    app_at_cmd_add();

		//light pwm init
    light_type_discern();

#if W_DEBUG
    light_state_debug_print();
#endif
}
