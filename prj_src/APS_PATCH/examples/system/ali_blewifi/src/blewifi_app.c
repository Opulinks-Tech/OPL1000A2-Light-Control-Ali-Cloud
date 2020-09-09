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
#include "mw_fim_default_group14_project.h"
#include "mw_fim_default_group17_project.h"
#include "app_at_cmd.h"
#include "wifi_api.h"

//light control
#include "hal_pwm.h"
#include "light_control.h"
#include "hal_pin.h"
#include "hal_pin_def.h"
#include "mw_ota.h"
#include "ali_linkkitsdk_decl.h"

#ifdef ADA_REMOTE_CTRL
#include "ada_ucmd_parser.h"
#include "ada_uart_transport.h"
#include "uart_cmd_task.h"
#endif

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

#ifdef LED_CALLBACK_INFO_DEBUG
#define LEDC_DEBUG(FORMAT, ARGS) printf(FORMAT, ARGS)
#else
#define LEDC_DEBUG(FORMAT, ARGS)
#endif

SHM_DATA breeze_dev_info_t dinfo;
extern void linkkit_event_monitor(int event);
#endif

blewifi_ota_t *gTheOta = 0;
//extern uint8_t g_light_reboot_flag;

extern T_MwFim_GP17_AliyunInfo g_tAliyunInfo;
extern T_MwFim_GP17_AliyunMqttCfg g_tAliyunMqttCfg;

//ada mp light control
uint8_t g_led_mp_mode_flag = 0;  //1:mp mode, 0:normal
uint32_t g_led_mp_cnt = 0;
osTimerId g_tAdaLedMPBlinkId;
osTimerId g_tRhythm;
uint8_t g_rhythm_S = 0, g_rhythm_V = 0;
uint16_t g_rhythm_H = 0;

static void ada_led_mp_timer_callback(void const *argu)
{
    if (g_led_mp_mode_flag == 0)
    {
        osTimerStop(g_tAdaLedMPBlinkId);
    }
    if (g_led_mp_cnt < 600)    //light up R->G->B(->C)(->W)
    {
        uint8_t rgbcw_indicator = 0;

        if(light_ctrl_get_light_type() == LT_RGB)
        {
            rgbcw_indicator = g_led_mp_cnt % 3;
        }
        else if(light_ctrl_get_light_type() == LT_RGBCW)
        {
            rgbcw_indicator = g_led_mp_cnt % 5;
        }
        else
        {
            rgbcw_indicator = g_led_mp_cnt % 4;
        }
        
        switch (rgbcw_indicator)
        {
            case 0:    //RED
                light_ctrl_set_hsv(0 ,100 ,100 , LIGHT_FADE_OFF, UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
                break;
            case 1:    //GREEN
                light_ctrl_set_hsv(120 ,100 ,100 , LIGHT_FADE_OFF, UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
                break;
            case 2:    //BLUE
                light_ctrl_set_hsv(240 ,100 ,100 , LIGHT_FADE_OFF, UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
                break;

            case 3:    //COLD (e.g. W : WRITE)
                if(light_ctrl_get_light_type() != LT_RGB)
                {
                    light_ctrl_set_ctb(7000, 100, LIGHT_FADE_OFF, UPDATE_LED_STATUS);
                }
                
                break;

            case 4:    //WARM
                //if(light_ctrl_get_light_type() != LT_RGB)
                {
                    if((light_ctrl_get_light_type() == LT_CW) || 
                       ((light_ctrl_get_light_type() == LT_RGBCW)))
                    {
                        light_ctrl_set_ctb(2000, 100, LIGHT_FADE_OFF, UPDATE_LED_STATUS);
                    }
                }
                
                break;
            
            default:
                LEDC_DEBUG("[MP]rgbcw_indicator wrong!\n",());
                break;
        }
    }
    else if (g_led_mp_cnt == 600)
    {
        //turn on RGB 100%
        light_ctrl_set_hsv(0 ,0 ,100 , LIGHT_FADE_OFF, UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
    }
    else if (g_led_mp_cnt == 3000)
    {
        if(light_ctrl_get_light_type() != LT_RGB)
        {
            //turn on Cold
            light_ctrl_set_ctb(7000, 100, LIGHT_FADE_OFF, UPDATE_LED_STATUS);
        }
    }
    else if (g_led_mp_cnt == 5400)  // 45 minutes
    {
        if((light_ctrl_get_light_type() == LT_CW) || 
           ((light_ctrl_get_light_type() == LT_RGBCW)))
        {
            //turn on Warm
            light_ctrl_set_ctb(2000, 100, LIGHT_FADE_OFF, UPDATE_LED_STATUS);
        }
        else
        {
            //turn on Red 10%
            light_ctrl_set_hsv(0 ,100 ,10 , LIGHT_FADE_OFF, UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
            g_led_mp_mode_flag = 0;
            g_led_mp_cnt = 0;
            //Stop timer
            osTimerStop(g_tAdaLedMPBlinkId);
        }
    }
    else if (g_led_mp_cnt == 7800)  // 65 minutes
    {
        if((light_ctrl_get_light_type() == LT_CW) || 
           ((light_ctrl_get_light_type() == LT_RGBCW)))
        {
            //turn on Red 10%
            light_ctrl_set_hsv(0 ,100 ,10 , LIGHT_FADE_OFF, UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
            g_led_mp_mode_flag = 0;
            g_led_mp_cnt = 0;
            //Stop timer
            osTimerStop(g_tAdaLedMPBlinkId);
        }
    }

    g_led_mp_cnt++;
}

static void ada_led_mp_timer_init(void)
{
    g_led_mp_cnt = 0;
    osTimerDef_t tTimerCtrMPLedBlinkDef;
    
    tTimerCtrMPLedBlinkDef.ptimer = ada_led_mp_timer_callback;
    g_tAdaLedMPBlinkId = osTimerCreate(&tTimerCtrMPLedBlinkDef, osTimerPeriodic, (void*)0);
    if (g_tAdaLedMPBlinkId == NULL)
    {
        //printf("To create the timer for AdaLedMPTimer is fail.\n");
    }
}

static void Rhythm_Random_Color_TimerCallBack(void const *argu)
{
    if((!light_ctrl_get_switch()) || (light_ctrl_get_mode() != MODE_HSV))
    {
        goto done;
    }
    
    light_ctrl_set_hsv(g_rhythm_H, g_rhythm_S, g_rhythm_V, LIGHT_FADE_OFF, DONT_UPDATE_LED_STATUS, 20);  

done:
    return;
}

void Rhythm_Random_Color_Init(void)
{
    osTimerDef_t tTimerRhythmDef;
    tTimerRhythmDef.ptimer = Rhythm_Random_Color_TimerCallBack;

    g_tRhythm = osTimerCreate(&tTimerRhythmDef, osTimerOnce,(void*)0);
		
    if (g_tRhythm == NULL)
    {
        LEDC_DEBUG("To create the timer for Rhythm Color Timer is fail.\n",());
    }
}

//light type and light ctrl data structure init
void light_type_discern(void)
{
    Hal_Pin_ConfigSet(9, PIN_TYPE_GPIO_INPUT, PIN_DRIVING_HIGH);
    Hal_Pin_ConfigSet(10, PIN_TYPE_GPIO_INPUT, PIN_DRIVING_HIGH);
    Hal_Pin_ConfigSet(11, PIN_TYPE_GPIO_INPUT, PIN_DRIVING_HIGH);
    uint8_t lighttype = 0;
    light_ctrl_init();

    Hal_Pwm_Init();
    Hal_Pwm_ClockSourceSet(HAL_PWM_CLK_22M);
    cancel_default_breath();

    if(Hal_Vic_GpioInput(GPIO_IDX_09))
        lighttype = lighttype | 4;
    if(Hal_Vic_GpioInput(GPIO_IDX_10))
        lighttype = lighttype | 2;
    if(Hal_Vic_GpioInput(GPIO_IDX_11))
        lighttype = lighttype | 1;

#ifdef BLEWIFI_RGB_LED
    lighttype = TMP_LT_RGB;
#elif defined BLEWIFI_RGBCW_LED
    lighttype = TMP_LT_RGBCW;
#else
    lighttype = TMP_LT_RGBC;
#endif

    light_ctrl_set_ctb(0, 0, LIGHT_FADE_OFF, UPDATE_LED_STATUS);
		
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

    if(iot_apply_cfg(1))
    {
        // apply default configuration
        g_led_mp_mode_flag = 1;
    }

    //g_light_reboot_flag = 1;
}

void fw_ver_get(char *sBuf, uint32_t u32BufLen)
{
    char *saMonth[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    char s8aMonth[8] = {0};
    int iYear = 0;
    int iMonth = 0;
    int iDay = 0;
    int iNum = 0;
    uint16_t u16ProjectId = 0;
    uint16_t u16ChipId = 0;
    uint16_t u16FirmwareId = 0;
    
    iNum = sscanf(__DATE__, "%s %d %d", s8aMonth, &iDay, &iYear); // build date: e.g. "Jan 30 2020"

    if(iNum > 0)
    {
        uint8_t i = 0;

        for(i = 0; i < 12; i++)
        {
            if(memcmp(s8aMonth, saMonth[i], 3) == 0)
            {
                iMonth = i + 1;
                break;
            }
        }
    }

    MwOta_VersionGet(&u16ProjectId, &u16ChipId, &u16FirmwareId);

    snprintf(sBuf, u32BufLen, "%s-%04u%02u%02u.%u", 
             SYSINFO_APP_VERSION, 
             iYear, iMonth, iDay, 
             u16FirmwareId);

    return;
}

extern void HAL_SetFirmwareVersion(_IN_ char *FwVersion);
SHM_DATA void BleWifiAppInit(void)
{
    T_MwFim_SysMode tSysMode;
    T_MwFim_GP11_PowerSaving tPowerSaving;

    gTheOta = 0;

//#if (SNTP_FUNCTION_EN == 1)
    g_ulSntpSecondInit = 0;                 // Initialize the Sntp Value
    g_ulSystemSecondInit = 0;               // Initialize System Clock Time
//#endif

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

        char s8aVerStr[32] = {0};
        
        fw_ver_get(s8aVerStr, sizeof(s8aVerStr));
        HAL_SetFirmwareVersion(s8aVerStr);

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

        if(MwFim_FileRead(MW_FIM_IDX_GP17_PROJECT_ALIYUN_INFO, 0, MW_FIM_GP17_ALIYUN_INFO_SIZE, (uint8_t*)&g_tAliyunInfo) != MW_FIM_OK)
        {
            // if fail, get the default value
            memcpy(&g_tAliyunInfo, &g_tMwFimDefaultGp17AliyunInfo, MW_FIM_GP17_ALIYUN_INFO_SIZE);
        }
        printf("\nFIM_RegionID:%d\n", g_tAliyunInfo.ulRegionID);

        #if 1
        if(MwFim_FileRead(MW_FIM_IDX_GP17_PROJECT_ALIYUN_MQTT_CFG, 0, MW_FIM_GP17_ALIYUN_MQTT_CFG_SIZE, (uint8_t*)&g_tAliyunMqttCfg) != MW_FIM_OK)
        {
            // if fail, get the default value
            memcpy(&g_tAliyunMqttCfg, &g_tMwFimDefaultGp17AliyunMqttCfg, MW_FIM_GP17_ALIYUN_MQTT_CFG_SIZE);
        }

        printf("\nFIM_MQTT_URL:[%s]\n", g_tAliyunMqttCfg.s8aUrl);

        if(g_tAliyunMqttCfg.s8aUrl[0])
        {
            iotx_guider_set_dynamic_region_only(g_tAliyunInfo.ulRegionID);
            iotx_guider_get_kv_env(); //update guider_env
            iotx_guider_set_dynamic_mqtt_url(g_tAliyunMqttCfg.s8aUrl);
        }
        else
        {
            iotx_guider_set_dynamic_region(g_tAliyunInfo.ulRegionID);
        }
        #else
        iotx_guider_set_dynamic_region(g_tAliyunInfo.ulRegionID);
        #endif
    }

    // update the system mode
    BleWifi_Ctrl_SysModeSet(tSysMode.ubSysMode);

    // add app cmd
    app_at_cmd_add();

    #ifdef ADA_REMOTE_CTRL
    uart_cmd_init(UART_NUM_0, 9600, ada_ctrl_uart_input_impl);
    uart_cmd_handler_register(uart_cmd_process, NULL);
    #endif

    //light pwm init
    light_type_discern();

    ada_led_mp_timer_init();
    Rhythm_Random_Color_Init();

#if W_DEBUG
    light_state_debug_print();
#endif
}
