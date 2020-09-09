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

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "ali_linkkitsdk_decl.h"
#include "blewifi_configuration.h"
#include "hal_vic.h"
#include "blewifi_ctrl.h"

#include "light_control.h"
#include "iot_rb_data.h"
#include "blewifi_ctrl_http_ota.h"
#include "blewifi_common.h"
#include "infra_cjson.h"
#include "infra_config.h"
#include "infra_report.h"
#include "dev_bind_wrapper.h"
#include "mqtt_wrapper.h"
#include "util_func.h"
#include "mw_fim_default_group15_project.h"
#include "mw_fim_default_group17_project.h"
#include "lwip/etharp.h"
#include "infra_net.h"
#include "iotx_cm_internal.h"
#include "iotx_mqtt_client.h"

#ifdef BLEWIFI_SCHED_EXT
#include "mw_fim_default_group16_project.h"
#endif

#ifdef ADA_REMOTE_CTRL
#include "ada_lightbulb.h"
#endif

char DEMO_PRODUCT_KEY[IOTX_PRODUCT_KEY_LEN + 1] = {0};
char DEMO_DEVICE_NAME[IOTX_DEVICE_NAME_LEN + 1] = {0};
char DEMO_DEVICE_SECRET[IOTX_DEVICE_SECRET_LEN + 1] = {0};
char DEMO_PRODUCT_SECRET[IOTX_PRODUCT_SECRET_LEN + 1] = {0};

extern osTimerId g_tRhythm;
extern uint8_t g_rhythm_S, g_rhythm_V;
extern uint16_t g_rhythm_H;

static user_example_ctx_t g_user_example_ctx;
extern T_MwFim_GP13_Dev_Sched g_taDevSched[MW_FIM_GP13_DEV_SCHED_NUM];

#ifdef BLEWIFI_SCHED_EXT
extern T_MwFim_GP16_Dev_Sched_Ext g_taDevSchedExt[MW_FIM_GP13_DEV_SCHED_NUM];
#endif

//uint8_t g_light_reboot_flag = 0;

T_MwFim_GP15_Reboot_Status g_tReboot = {0};
T_MwFim_GP15_Light_Status g_tLightStatus = {0};
T_MwFim_GP15_Ctb_Status g_tCtbStatus = {0};
hsv_t g_tHsvStatus = {0};
T_MwFim_GP15_Scenes_Status g_tScenesStatus = {0};
hsv_t g_taColorArr[MW_FIM_GP15_COLOR_ARRAY_NUM] = {0};

uint8_t g_u8CurrColorArrayEnable = 0;

#ifdef ALI_NO_POST_MODE
uint8_t g_certification_mode_flag = 0;
uint32_t g_certification_cnt = 0;
char g_certification_str[64]={0};
#endif

#ifdef ALI_POST_CTRL
typedef struct
{
    uint8_t u8Used;
    int iId;
    uint32_t u32Cnt;
} T_PostInfo;

T_PostInfo g_tPrevPostInfo = {0};

extern uint32_t g_u32RecvMsgId;

void post_info_clear(void)
{
    memset(&g_tPrevPostInfo, 0, sizeof(g_tPrevPostInfo));
    return;
}

void post_info_update(int iId)
{
    g_tPrevPostInfo.u8Used = 1;
    g_tPrevPostInfo.iId = iId;
    return;
}
#endif //#ifdef ALI_POST_CTRL

user_example_ctx_t *user_example_get_ctx(void)
{
    return &g_user_example_ctx;
}

SHM_DATA void iot_set_scenes_color(uint8_t u8WorkMode)
{
    if(u8WorkMode == WMODE_MANUAL)
    {
        uint16_t hue = 0;
        uint8_t saturation = 0;
        uint8_t value = 0;
        light_ctrl_set_workmode(WMODE_MANUAL);
        light_ctrl_get_manual_light_status(&hue,&saturation,&value);
        light_ctrl_set_hsv(hue, saturation, value, LIGHT_FADE_ON, DONT_UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
        light_ctrl_set_scenescolor(hue,saturation,value);
    }
    else if(u8WorkMode == WMODE_READING)
    {
        light_ctrl_set_workmode(WMODE_READING);
        //light_ctrl_set_rgb(255,250,247);
        light_ctrl_set_hsv(22, 3, 100, LIGHT_FADE_ON, DONT_UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
        light_ctrl_set_scenescolor(22,3,100);
    }
    else if(u8WorkMode == WMODE_CINEMA)
    {
        light_ctrl_set_workmode(WMODE_CINEMA);
        //light_ctrl_set_rgb(127,97,186);
        light_ctrl_set_hsv(260, 48, 73, LIGHT_FADE_ON, DONT_UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
        light_ctrl_set_scenescolor(260,48,73);
    }
    else if(u8WorkMode == WMODE_MIDNIGHT)
    {
        light_ctrl_set_workmode(WMODE_MIDNIGHT);
        //light_ctrl_set_rgb(68,31,0);
        light_ctrl_set_hsv(27, 100, 27, LIGHT_FADE_ON, DONT_UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
        light_ctrl_set_scenescolor(27,100,27);
    }
    else if(u8WorkMode == WMODE_LIVING)
    {
        light_ctrl_set_workmode(WMODE_LIVING);
        //light_ctrl_set_rgb(255,164,46);
        light_ctrl_set_hsv(34, 82, 100, LIGHT_FADE_ON, DONT_UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
        light_ctrl_set_scenescolor(34,82,100);
    }
    else if(u8WorkMode == WMODE_SOFT)
    {
        light_ctrl_set_workmode(WMODE_SOFT);
        //light_ctrl_set_rgb(222,29,93);
        light_ctrl_set_hsv(340, 87, 87, LIGHT_FADE_ON, DONT_UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
        light_ctrl_set_scenescolor(340,87,87);
    }
    else if (u8WorkMode == WMODE_STREAMER)
    {
        light_effect_t effect_config = {
            .effect_type      = EFFTCT_TYPE_FLASH,
            .color_num        = 8,
            .color_index      = 0,
            .transition_speed = 1000,
            .fade_step_ms     = 2,
            .color_arr[0].hue =   0, .color_arr[0].saturation = 100, .color_arr[0].value = 100, //red
            .color_arr[1].hue =  39, .color_arr[1].saturation = 100, .color_arr[1].value = 100, //orange
            .color_arr[2].hue =  60, .color_arr[2].saturation = 100, .color_arr[2].value = 100, //yellow
            .color_arr[3].hue = 120, .color_arr[3].saturation = 100, .color_arr[3].value = 100, //green
            .color_arr[4].hue = 210, .color_arr[4].saturation = 100, .color_arr[4].value = 100, //blue
            .color_arr[5].hue = 240, .color_arr[5].saturation = 100, .color_arr[5].value = 100, //cyan
            .color_arr[6].hue = 273, .color_arr[6].saturation = 100, .color_arr[6].value = 100, //purple
            .color_arr[7].hue =   0, .color_arr[7].saturation =   0, .color_arr[7].value = 100, //white
        };

        light_ctrl_set_workmode(WMODE_STREAMER);
        light_effect_config(&effect_config);
        light_effect_start(NULL, get_light_effect_config()->color_index);
    }
}

void iot_set_color_array(hsv_t *taColorArr, uint16_t u16ColorNum, uint8_t u8Apply)
{
    light_effect_set_effect_type(EFFTCT_TYPE_FLASH);
    light_effect_set_color_array(taColorArr, u16ColorNum);

    if(u16ColorNum)
    {
        light_ctrl_set_scenescolor(taColorArr[0].hue, taColorArr[0].saturation, taColorArr[0].value);

        if(u8Apply)
        {
            light_effect_start(NULL, 0);
        }
    }
}

void iot_update_cfg_for_no_cw_dev(void)
{
    if(light_ctrl_get_light_type() == LT_RGB)
    {
        // no cold/warm lights
        g_tLightStatus.u8LightMode = 1; // default light mode for no-cold-warm-lights device
        g_tHsvStatus.hue = 60;          // default hue for no-cold-warm-lights device
    }

    return;
}

int iot_load_cfg(uint8_t u8Mode)
{
    uint8_t i = 0;

    if(u8Mode)
    {
        // load saved configuration
        //printf("load saved configuration\n");

        if(MW_FIM_OK != MwFim_FileRead(MW_FIM_IDX_GP15_PROJECT_LIGHT_STATUS, 0, MW_FIM_GP15_LIGHT_STATUS_SIZE, (uint8_t*)&g_tLightStatus))
        {
            memcpy(&g_tLightStatus, &g_tMwFimDefaultGp15LightStatus, MW_FIM_GP15_LIGHT_STATUS_SIZE);
        }
    
        if(MW_FIM_OK != MwFim_FileRead(MW_FIM_IDX_GP15_PROJECT_CTB_STATUS, 0, MW_FIM_GP15_CTB_STATUS_SIZE, (uint8_t*)&g_tCtbStatus))
        {
            memcpy(&g_tCtbStatus, &g_tMwFimDefaultGp15CtbStatus, MW_FIM_GP15_CTB_STATUS_SIZE);
        }
    
        if(MW_FIM_OK != MwFim_FileRead(MW_FIM_IDX_GP15_PROJECT_HSV_STATUS, 0, MW_FIM_GP15_HSV_STATUS_SIZE, (uint8_t*)&g_tHsvStatus))
        {
            memcpy(&g_tHsvStatus, &g_tMwFimDefaultGp15HsvStatus, MW_FIM_GP15_HSV_STATUS_SIZE);
        }
    
        if(MW_FIM_OK != MwFim_FileRead(MW_FIM_IDX_GP15_PROJECT_SCENES_STATUS, 0, MW_FIM_GP15_SCENES_STATUS_SIZE, (uint8_t*)&g_tScenesStatus))
        {
            memcpy(&g_tScenesStatus, &g_tMwFimDefaultGp15ScenesStatus, MW_FIM_GP15_SCENES_STATUS_SIZE);
        }
    
        for(i = 0; i < MW_FIM_GP15_COLOR_ARRAY_NUM; i++)
        {
            if(MW_FIM_OK != MwFim_FileRead(MW_FIM_IDX_GP15_PROJECT_COLOR_ARRAY_STATUS, i, MW_FIM_GP15_COLOR_ARRAY_SIZE, (uint8_t*)&(g_taColorArr[i])))
            {
                memcpy(&(g_taColorArr[i]), &g_tMwFimDefaultGp15ColorArrayStatus, MW_FIM_GP15_COLOR_ARRAY_SIZE);
            }
        }

        iot_update_cfg_for_no_cw_dev();
    }
    else
    {
        // load default configuration
        //printf("load default configuration\n");

        memcpy(&g_tLightStatus, &g_tMwFimDefaultGp15LightStatus, MW_FIM_GP15_LIGHT_STATUS_SIZE);
        memcpy(&g_tCtbStatus, &g_tMwFimDefaultGp15CtbStatus, MW_FIM_GP15_CTB_STATUS_SIZE);
        memcpy(&g_tHsvStatus, &g_tMwFimDefaultGp15HsvStatus, MW_FIM_GP15_HSV_STATUS_SIZE);
        memcpy(&g_tScenesStatus, &g_tMwFimDefaultGp15ScenesStatus, MW_FIM_GP15_SCENES_STATUS_SIZE);

        iot_update_cfg_for_no_cw_dev();
    
        for(i = 0; i < MW_FIM_GP15_COLOR_ARRAY_NUM; i++)
        {
            memcpy(&(g_taColorArr[i]), &g_tMwFimDefaultGp15ColorArrayStatus, MW_FIM_GP15_COLOR_ARRAY_SIZE);
        }
    }

    return 0;
}

int iot_update_cfg(void)
{
    light_ctrl_update_switch(g_tLightStatus.u8LightSwitch);
    light_ctrl_set_mode(g_tLightStatus.u8LightMode);
    light_ctrl_update_brightness(g_tCtbStatus.u8Brightness);
    colortemperature_force_overwrite(g_tCtbStatus.u32ColorTemp);
    light_ctrl_update_hsv(g_tHsvStatus.hue, g_tHsvStatus.saturation, g_tHsvStatus.value);
    light_ctrl_set_manual_light_status(g_tHsvStatus.hue, g_tHsvStatus.saturation, g_tHsvStatus.value);

    return 0;
}

int iot_apply_cfg(uint8_t u8Mode)
{
    int iRet = 1; // 0: after unbind or bind/1: after reboot

    if(u8Mode)
    {
        if(MW_FIM_OK != MwFim_FileRead(MW_FIM_IDX_GP15_PROJECT_REBOOT_STATUS, 0, MW_FIM_GP15_REBOOT_STATUS_SIZE, (uint8_t*)&g_tReboot))
        {
            memcpy(&g_tReboot, &g_tMwFimDefaultGp15RebootStatus, MW_FIM_GP15_REBOOT_STATUS_SIZE);
        }

        if(g_tReboot.u8Reason)
        {
            g_tReboot.u8Reason = 0;
    
            if(MW_FIM_OK != MwFim_FileWrite(MW_FIM_IDX_GP15_PROJECT_REBOOT_STATUS, 0, MW_FIM_GP15_REBOOT_STATUS_SIZE, (uint8_t*)&g_tReboot))
            {
                //printf("[%s %d] MwFim_FileWrite fail\n", __func__, __LINE__);
            }

            iot_load_cfg(1);

            iRet = 0;
    
            //printf("apply saved configuration\n");
        }
        else
        {
            iot_load_cfg(0);

            //printf("apply default configuration\n");
        }

        iot_update_cfg();
    }
    else
    {
        //printf("apply current configuration\n");
    }

    light_ctrl_set_switch(g_tLightStatus.u8LightSwitch);

    if(light_ctrl_get_switch())
    {
        if(light_ctrl_get_mode() == MODE_SCENES)
        {
            light_ctrl_set_workmode(g_tScenesStatus.u8WorkMode);

            if(g_tScenesStatus.u8ColorArrayEnable)
            {
                g_u8CurrColorArrayEnable = 1;
                light_effect_set_speed(g_tScenesStatus.u8ColorSpeed, 1);
                iot_set_color_array(g_taColorArr, g_tScenesStatus.u16ColorNum, 1);
            }
            else
            {
                iot_set_scenes_color(light_ctrl_get_workmode());
            }
        }
    }

    return iRet;
}

SHM_DATA void iot_save_all_cfg(void)
{
    T_MwFim_GP15_Light_Status tLightStatus = {0};
    T_MwFim_GP15_Ctb_Status tCtbStatus = {0};
    hsv_t tHsvStatus = {0};
    T_MwFim_GP15_Scenes_Status tScenesStatus = {0};
    hsv_t *taColorArr = NULL;

    uint32_t u32Size = 0;
    uint8_t i = 0;

    T_MwFim_GP15_Light_Status tFimLightStatus = {0};
    T_MwFim_GP15_Ctb_Status tFimCtbStatus = {0};
    hsv_t tFimHsvStatus = {0};
    T_MwFim_GP15_Scenes_Status tFimScenesStatus = {0};
    hsv_t taFimColorArr[MW_FIM_GP15_COLOR_ARRAY_NUM] = {0};

    if(MW_FIM_OK != MwFim_FileRead(MW_FIM_IDX_GP15_PROJECT_LIGHT_STATUS, 0, MW_FIM_GP15_LIGHT_STATUS_SIZE, (uint8_t*)&tFimLightStatus))
    {
        memcpy(&tFimLightStatus, &g_tMwFimDefaultGp15LightStatus, MW_FIM_GP15_LIGHT_STATUS_SIZE);
    }

    if(MW_FIM_OK != MwFim_FileRead(MW_FIM_IDX_GP15_PROJECT_CTB_STATUS, 0, MW_FIM_GP15_CTB_STATUS_SIZE, (uint8_t*)&tFimCtbStatus))
    {
        memcpy(&tFimCtbStatus, &g_tMwFimDefaultGp15CtbStatus, MW_FIM_GP15_CTB_STATUS_SIZE);
    }

    if(MW_FIM_OK != MwFim_FileRead(MW_FIM_IDX_GP15_PROJECT_HSV_STATUS, 0, MW_FIM_GP15_HSV_STATUS_SIZE, (uint8_t*)&tFimHsvStatus))
    {
        memcpy(&tFimHsvStatus, &g_tMwFimDefaultGp15HsvStatus, MW_FIM_GP15_HSV_STATUS_SIZE);
    }

    if(MW_FIM_OK != MwFim_FileRead(MW_FIM_IDX_GP15_PROJECT_SCENES_STATUS, 0, MW_FIM_GP15_SCENES_STATUS_SIZE, (uint8_t*)&tFimScenesStatus))
    {
        memcpy(&tFimScenesStatus, &g_tMwFimDefaultGp15ScenesStatus, MW_FIM_GP15_SCENES_STATUS_SIZE);
    }

    for(i = 0; i < MW_FIM_GP15_COLOR_ARRAY_NUM; i++)
    {
        if(MW_FIM_OK != MwFim_FileRead(MW_FIM_IDX_GP15_PROJECT_COLOR_ARRAY_STATUS, i, MW_FIM_GP15_COLOR_ARRAY_SIZE, (uint8_t*)&(taFimColorArr[i])))
        {
            memcpy(&(taFimColorArr[i]), &g_tMwFimDefaultGp15ColorArrayStatus, MW_FIM_GP15_COLOR_ARRAY_SIZE);
        }
    }

    tLightStatus.u8LightSwitch = light_ctrl_get_switch();
    tLightStatus.u8LightMode = light_ctrl_get_mode();
    u32Size = sizeof(g_tLightStatus);

    if(memcmp(&tFimLightStatus, &tLightStatus, u32Size))
    {
        //printf("[%s %d] save LightStatus\n", __func__, __LINE__);

        if(MW_FIM_OK != MwFim_FileWrite(MW_FIM_IDX_GP15_PROJECT_LIGHT_STATUS, 0, MW_FIM_GP15_LIGHT_STATUS_SIZE, (uint8_t*)&tLightStatus))
        {
            //printf("[%s %d] MwFim_FileWrite fail\n", __func__, __LINE__);
        }
    }

    tCtbStatus.u8Brightness = light_ctrl_get_brightness();
    tCtbStatus.u32ColorTemp = light_ctrl_get_color_temperature();
    u32Size = sizeof(g_tCtbStatus);

    if(memcmp(&tFimCtbStatus, &tCtbStatus, u32Size))
    {
        //printf("[%s %d] save CtbStatus\n", __func__, __LINE__);

        if(MW_FIM_OK != MwFim_FileWrite(MW_FIM_IDX_GP15_PROJECT_CTB_STATUS, 0, MW_FIM_GP15_CTB_STATUS_SIZE, (uint8_t*)&tCtbStatus))
        {
            //printf("[%s %d] MwFim_FileWrite fail\n", __func__, __LINE__);
        }
    }

    if(!light_ctrl_get_hsv(&(tHsvStatus.hue), &(tHsvStatus.saturation), &(tHsvStatus.value)))
    {
        u32Size = sizeof(g_tHsvStatus);

        if(memcmp(&tFimHsvStatus, &tHsvStatus, u32Size))
        {
            //printf("[%s %d] save HsvStatus\n", __func__, __LINE__);

            if(MW_FIM_OK != MwFim_FileWrite(MW_FIM_IDX_GP15_PROJECT_HSV_STATUS, 0, MW_FIM_GP15_HSV_STATUS_SIZE, (uint8_t*)&tHsvStatus))
            {
                //printf("[%s %d] MwFim_FileWrite fail\n", __func__, __LINE__);
            }
        }
    }

    tScenesStatus.u8WorkMode = light_ctrl_get_workmode();
    tScenesStatus.u8ColorSpeed = light_effect_get_speed();
    tScenesStatus.u16ColorNum = light_effect_get_color_num();
    tScenesStatus.u8ColorArrayEnable = g_u8CurrColorArrayEnable;
    u32Size = sizeof(g_tScenesStatus);

    if(memcmp(&tFimScenesStatus, &tScenesStatus, u32Size))
    {
        //printf("[%s %d] save ScenesStatus\n", __func__, __LINE__);

        if(MW_FIM_OK != MwFim_FileWrite(MW_FIM_IDX_GP15_PROJECT_SCENES_STATUS, 0, MW_FIM_GP15_SCENES_STATUS_SIZE, (uint8_t*)&tScenesStatus))
        {
            //printf("[%s %d] MwFim_FileWrite fail\n", __func__, __LINE__);
        }
    }

    if((g_tScenesStatus.u8ColorArrayEnable) && (g_tScenesStatus.u16ColorNum))
    {
        taColorArr = light_effect_get_color_arr();

        if(taColorArr)
        {
            u32Size = sizeof(hsv_t);
    
            for(i = 0; i < MW_FIM_GP15_COLOR_ARRAY_NUM; i++)
            {
                if(memcmp(&(taFimColorArr[i]), &(taColorArr[i]), u32Size))
                {
                    //printf("[%s %d] save ColorArr[%u]\n", __func__, __LINE__, i);
    
                    if(MW_FIM_OK != MwFim_FileWrite(MW_FIM_IDX_GP15_PROJECT_COLOR_ARRAY_STATUS, i, MW_FIM_GP15_COLOR_ARRAY_SIZE, (uint8_t*)&(taColorArr[i])))
                    {
                        //printf("[%s %d] MwFim_FileWrite fail i[%u]\n", __func__, __LINE__, i);
                    }
                }
            }
        }
    }

    if(g_tReboot.u8Reason == 0)
    {
        g_tReboot.u8Reason = 1;
    
        //printf("[%s %d] save RebootStatus\n", __func__, __LINE__);
    
        if(MW_FIM_OK != MwFim_FileWrite(MW_FIM_IDX_GP15_PROJECT_REBOOT_STATUS, 0, MW_FIM_GP15_REBOOT_STATUS_SIZE, (uint8_t*)&g_tReboot))
        {
            //printf("[%s %d] MwFim_FileWrite fail\n", __func__, __LINE__);
        }
    }

    return;
}

SHM_DATA void iot_update_light_property(uint16_t u16Flag, uint32_t u32MsgId)
{
    IoT_Properity_t tProp = {0};

    tProp.u32MsgId = u32MsgId;
    tProp.u16Flag = u16Flag;

    if(IoT_Ring_Buffer_Push(&tProp) != IOT_RB_DATA_OK)
    {
        BLEWIFI_ERROR("[%s %d] IoT_Ring_Buffer_Push fail\n", __func__, __LINE__);
    }

    return;
}

SHM_DATA uint8_t iot_is_local_timer_valid(void)
{
    uint8_t u8IsValid = 0;
    uint8_t i = 0;

    for (i = 0; i < MW_FIM_GP13_DEV_SCHED_NUM; i++)
    {
        if(g_taDevSched[i].u8IsValid)
        {
            u8IsValid = 1;
            break;
        }
    }

    return u8IsValid;
}

SHM_DATA uint32_t iot_get_local_timer(char *ps8Buf, uint32_t u32BufSize, uint32_t u32Offset)
{
    uint8_t i = 0, j = 0, k = 0;

    u32Offset += snprintf(ps8Buf + u32Offset, u32BufSize - u32Offset, "\"LocalTimer\":[");

    if(iot_is_local_timer_valid())
    {
        for (i = 0; i < MW_FIM_GP13_DEV_SCHED_NUM; i++)
        {
            char s8aRepeatBuf[LOCALTIMER_REPEAT_BUF_LEN] = {0};
            uint32_t u8RepeatBufSize = LOCALTIMER_REPEAT_BUF_LEN;
            uint32_t u32RepeatOffset = 0;
    
            if(i)
            {
                u32Offset += snprintf(ps8Buf + u32Offset, u32BufSize - u32Offset, ",");
            }
    
            k = 0;
    
            for(j = 1; j < 8; j++)
            {
                if (g_taDevSched[i].u8RepeatMask & (1 << j))
                {
                    if(k)
                    {
                       u32RepeatOffset += snprintf(s8aRepeatBuf + u32RepeatOffset, u8RepeatBufSize - u32RepeatOffset, ",");
                    }
    
                    u32RepeatOffset += snprintf(s8aRepeatBuf + u32RepeatOffset, u8RepeatBufSize - u32RepeatOffset, "%u", j);
                    k++;
                 }
            }
    
            if(!k)
            {
                strcpy(s8aRepeatBuf, "*");
            }
    
            u32Offset += snprintf(ps8Buf + u32Offset, u32BufSize - u32Offset, "{\"LightSwitch\":%u,\"Timer\":\"%u %u * * %s\",\"Enable\":%u,\"IsValid\":%u,\"TimezoneOffset\":%d}"
                                , g_taDevSched[i].u8DevOn
                                , g_taDevSched[i].u8Min
                                , g_taDevSched[i].u8Hour
                                , s8aRepeatBuf
                                , g_taDevSched[i].u8Enable
                                , g_taDevSched[i].u8IsValid
    
                                #ifdef BLEWIFI_SCHED_EXT
                                , g_taDevSchedExt[i].s32TimeZone
                                #else
                                , g_taDevSched[i].s32TimeZone
                                #endif
                                );
        }
    }

    u32Offset += snprintf(ps8Buf + u32Offset, u32BufSize - u32Offset, "]");

    return u32Offset;
}

SHM_DATA int iot_update_local_timer(uint8_t u8Mask)
{
    int iRet = -1;
    char s8aPayload[64] = {0}; // for {"LightSwitch":1,"LocalTimer":[]}
    uint32_t u32BufSize = sizeof(s8aPayload);
    char *property_payload = s8aPayload;
    uint32_t u32Offset = 0;
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();
    uint8_t u8AlreadyWrite = 0;

    if(u8Mask & IOT_MASK_LOCAL_TIMER)
    {
        if(iot_is_local_timer_valid())
        {
            u32BufSize += PROPERTY_LOCALTIMER_PAYLOAD_LEN;

            property_payload = (char *)HAL_Malloc(u32BufSize);

            if(!property_payload)
            {
                printf("HAL_Malloc fail\n");
                goto done;
            }
        }
    }

    u32Offset += snprintf(property_payload + u32Offset, u32BufSize - u32Offset, "{");

    if(u8Mask & IOT_MASK_LIGHT_SWITCH)
    {
        if(u8AlreadyWrite)
        {
            u32Offset += snprintf(property_payload + u32Offset, u32BufSize - u32Offset, ",");
        }

        u32Offset += snprintf(property_payload + u32Offset, u32BufSize - u32Offset, "\"LightSwitch\":%d", light_ctrl_get_switch());
        u8AlreadyWrite = 1;
    }

    if(u8Mask & IOT_MASK_LOCAL_TIMER)
    {
        if(u8AlreadyWrite)
        {
            u32Offset += snprintf(property_payload + u32Offset, u32BufSize - u32Offset, ",");
        }

        u32Offset = iot_get_local_timer(property_payload, u32BufSize, u32Offset);
        u8AlreadyWrite = 1;
    }

    u32Offset += snprintf(property_payload + u32Offset, u32BufSize - u32Offset, "}");

    printf("\npost for local_timer mask[%02X]: buf_size[%u] len[%u]\n", u8Mask, u32BufSize, u32Offset);
    printf("%s\n\n", property_payload);

    iRet = IOT_Linkkit_Report(user_example_ctx->master_devid, ITM_MSG_POST_PROPERTY,
                      (unsigned char *)property_payload, u32Offset);

done:
    if(property_payload)
    {
        if(property_payload != s8aPayload)
        {
            HAL_Free(property_payload);
        }
    }

    return iRet;
}

void user_curr_status_post(void)
{
    uint16_t u16OutputFlag = 0;

    #ifdef ALI_POST_CTRL
    IoT_Ring_Buffer_ResetBuffer();
    post_info_clear();
    #endif

    SET_BIT(u16OutputFlag, PROPERTY_LIGHT_SWITCH);
    SET_BIT(u16OutputFlag, PROPERTY_LIGHTTYPE);
    SET_BIT(u16OutputFlag, PROPERTY_LIGHTMODE);
    SET_BIT(u16OutputFlag, PROPERTY_LOCALTIMER);

    if(light_ctrl_get_light_type() != LT_RGB)
    {
        // Even though device has no warm-light, we still post color temperature to update settings in ali-cloud.
        // Because device may have used firmware of warm-light and posted unsupported color temperature before.
        SET_BIT(u16OutputFlag, PROPERTY_COLORTEMPERATURE);
    }

    iot_update_light_property(u16OutputFlag, 0);
    return;
}

static int user_connected_event_handler(void)
{
    extern volatile uint8_t g_u8IotPostSuspend;
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();

    EXAMPLE_TRACE("Cloud Connected");
    user_example_ctx->cloud_connected = 1;
	
    if(g_u8IotPostSuspend)
    {
        g_u8IotPostSuspend = 0;
    }
    else
    {
        user_curr_status_post();
    }

    {
        extern T_MwFim_GP17_AliyunMqttCfg g_tAliyunMqttCfg;
        extern int guider_get_dynamic_mqtt_url(char *p_mqtt_url, int mqtt_url_buff_len);
    
        char s8aMqttUrl[MW_FIM_GP17_MQTT_URL_SIZE] = {0};
    
        if(!guider_get_dynamic_mqtt_url(s8aMqttUrl, MW_FIM_GP17_MQTT_URL_SIZE))
        {
            if(strcmp(s8aMqttUrl, g_tAliyunMqttCfg.s8aUrl))
            {
                snprintf(g_tAliyunMqttCfg.s8aUrl, sizeof(g_tAliyunMqttCfg.s8aUrl), "%s", s8aMqttUrl);

                //printf("update mqtt_url[%s]\n", g_tAliyunMqttCfg.s8aUrl);
    
                if(MwFim_FileWrite(MW_FIM_IDX_GP17_PROJECT_ALIYUN_MQTT_CFG, 0, MW_FIM_GP17_ALIYUN_MQTT_CFG_SIZE, (uint8_t*)&g_tAliyunMqttCfg) != MW_FIM_OK)
                {
                    BLEWIFI_ERROR("MwFim_FileWrite fail for MQTT_URL[%s]\n", g_tAliyunMqttCfg.s8aUrl);
                }
            }
        }
    }

#if defined(OTA_ENABLED) && defined(BUILD_AOS)
    ota_service_init(NULL);
#endif

#if (WIFI_OTA_FUNCTION_EN == 1)
    EXAMPLE_TRACE("BLEWIFI: OTA start \r\n");
    blewifi_ctrl_ota_sched_start(30);
#endif

    return 0;
}

static int user_disconnected_event_handler(void)
{
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();

    EXAMPLE_TRACE("Cloud Disconnected");

    user_example_ctx->cloud_connected = 0;

    #ifdef ALI_POST_CTRL
    IoT_Ring_Buffer_ResetBuffer();
    post_info_clear();
    #endif

    //EXAMPLE_TRACE("lwip_one_shot_arp_enable\n");
    lwip_one_shot_arp_enable();

    return 0;
}

static int user_down_raw_data_arrived_event_handler(const int devid, const unsigned char *payload,
        const int payload_len)
{
    EXAMPLE_TRACE("Down Raw Message, Devid: %d, Payload Length: %d", devid, payload_len);
    return 0;
}

static int user_service_request_event_handler(const int devid, const char *serviceid, const int serviceid_len,
        const char *request, const int request_len,
        char **response, int *response_len)
{
    int iRet = -1;

    EXAMPLE_TRACE("Service Request Received, Devid: %d, Service ID: %.*s, Payload: %s", devid, serviceid_len,
                  serviceid,
                  request);

    lite_cjson_t tRoot = {0};
    
    /* Try To Find LightSwitch Property */		//LightSwitch   Value:<int>,0,1
    lite_cjson_t tSaturation = {0};
    lite_cjson_t tHue = {0};
    lite_cjson_t tValue = {0};
    lite_cjson_t tLightDuration = {0};

    if((!light_ctrl_get_switch()) || (light_ctrl_get_mode() != MODE_HSV))
    {
        goto done;
    }
    
    if (lite_cjson_parse(request, request_len, &tRoot)) {
        EXAMPLE_TRACE("JSON Parse Error");
        goto done;
    }

    if (!lite_cjson_object_item(&tRoot, "Saturation", 10, &tSaturation) && 
        !lite_cjson_object_item(&tRoot, "Hue", 3, &tHue) && 
        !lite_cjson_object_item(&tRoot, "Value", 5, &tValue) && 
        !lite_cjson_object_item(&tRoot, "LightDuration", 13, &tLightDuration)) {

        light_ctrl_set_hsv(tHue.value_int, tSaturation.value_int, tValue.value_int, LIGHT_FADE_OFF, DONT_UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);   

        g_rhythm_H = tHue.value_int;
        g_rhythm_S = tSaturation.value_int;
        g_rhythm_V = tValue.value_int;
    
        if (tLightDuration.value_int!=0) {
            osTimerStop(g_tRhythm);
            osTimerStart(g_tRhythm, tLightDuration.value_int);
        }

        iRet = 0;
    }

done:
    return iRet;
}

static int dev_sched_time_set(char *sTime, T_MwFim_GP13_Dev_Sched *ptSched)
{
    int iRet = -1;
    uint8_t u8Mask = 0;
    unsigned int uiMin = 0;
    unsigned int uiHour = 0;
    unsigned int uiaDay[8] = {0};
    int iNum = 0;
    int i = 0;

    iNum = sscanf(sTime, "%u %u %*[^ ] %*[^ ] %u,%u,%u,%u,%u,%u,%u"
                  , &uiMin, &uiHour
                  , &(uiaDay[0]), &(uiaDay[1]), &(uiaDay[2]), &(uiaDay[3])
                  , &(uiaDay[4]), &(uiaDay[5]), &(uiaDay[6]));

    if(iNum < 2)
    {
        goto done;
    }

    if((uiMin > 59) || (uiHour > 23))
    {
        goto done;
    }

    ptSched->u8Hour = (uint8_t)uiHour;
    ptSched->u8Min = (uint8_t)uiMin;

    iNum -= 2;

    for(i = 0; i < iNum; i++)
    {
        if((uiaDay[i] >= 1) && (uiaDay[i] <= 7))
        {
            u8Mask |= (1 << uiaDay[i]);
        }
    }

    ptSched->u8RepeatMask = u8Mask;

    iRet = 0;

done:
    return iRet;
}

typedef struct
{
    uint32_t u32CurrMsgId;
    uint32_t u32CurrDiscardCnt;
    uint32_t u32DiscardThreshold;
    uint32_t u32UpdateTime;

    // for debugging
    uint32_t u32MaxExpireOffset;
    uint32_t u32MaxDiscardCnt;
} T_MsgStatus;

T_MsgStatus g_taMsgStatus[PROPERTY_MAX] = 
{
    {0, 0, 2,  0,   0, 0},   // PROPERTY_LIGHT_SWITCH
    {0, 0, 30, 0,   0, 0},   // PROPERTY_COLORTEMPERATURE
    {0, 0, 30, 0,   0, 0},   // PROPERTY_BRIGHTNESS
    {0, 0, 2,  0,   0, 0},   // PROPERTY_WORKMODE
    {0, 0, 2,  0,   0, 0},   // PROPERTY_LIGHTMODE
    {0, 0, 2,  0,   0, 0},   // PROPERTY_COLORSPEED
    {0, 0, 2,  0,   0, 0},   // PROPERTY_LIGHTTYPE
    {0, 0, 2,  0,   0, 0},   // PROPERTY_COLORARR
    {0, 0, 30, 0,   0, 0},   // PROPERTY_HSVCOLOR
    {0, 0, 2,  0,   0, 0},   // PROPERTY_RGBCOLOR
    {0, 0, 2,  0,   0, 0},   // PROPERTY_SCENESCOLOR
    {0, 0, 2,  0,   0, 0},   // PROPERTY_LOCALTIMER
};

SHM_DATA int is_expired_msg(uint32_t u32Idx, uint32_t u32MsgId)
{
    int iRet = 1;
    uint32_t u32OlderMsgIdThreshold = 100;
    uint32_t u32TimeThreshold = 5; // second
    uint32_t u32Time = 0; // second

    if(u32Idx >= PROPERTY_MAX)
    {
        goto done;
    }

    util_get_current_time(&u32Time, NULL);

    printf("id[%u] time[%u]: idx[%u] curr_id[%u] upd_time[%u]\n", u32MsgId, u32Time, u32Idx, g_taMsgStatus[u32Idx].u32CurrMsgId, g_taMsgStatus[u32Idx].u32UpdateTime);

    if(g_taMsgStatus[u32Idx].u32CurrMsgId)
    {
        if(u32MsgId <= g_taMsgStatus[u32Idx].u32CurrMsgId)
        {
            uint32_t u32Offset = g_taMsgStatus[u32Idx].u32CurrMsgId - u32MsgId;

            if(u32Offset < u32OlderMsgIdThreshold)
            {
                // receive a little older msg id

                uint32_t u32DiscardThreshold = 0;

                // for debugging
                if(u32Offset > g_taMsgStatus[u32Idx].u32MaxExpireOffset)
                {
                    g_taMsgStatus[u32Idx].u32MaxExpireOffset = u32Offset;
                }

                g_taMsgStatus[u32Idx].u32CurrDiscardCnt += 1;

                if(u32MsgId < (u32OlderMsgIdThreshold + 30))
                {
                    if((g_taMsgStatus[u32Idx].u32UpdateTime) && 
                       (u32Time >= (g_taMsgStatus[u32Idx].u32UpdateTime + u32TimeThreshold)))
                    {
                        // a few seconds after last update => it's new msg
                        printf("time[%u] > (%u + %u): update\n", u32Time, g_taMsgStatus[u32Idx].u32UpdateTime, u32TimeThreshold);
                        goto update;
                    }

                    u32DiscardThreshold = g_taMsgStatus[u32Idx].u32DiscardThreshold / 2;
                }
                else
                {
                    u32DiscardThreshold = g_taMsgStatus[u32Idx].u32DiscardThreshold;
                }

                if(g_taMsgStatus[u32Idx].u32CurrDiscardCnt < u32DiscardThreshold)
                {
                    // receive few older msgs => it might be expired

                    // for debugging
                    if(g_taMsgStatus[u32Idx].u32CurrDiscardCnt > g_taMsgStatus[u32Idx].u32MaxDiscardCnt)
                    {
                        g_taMsgStatus[u32Idx].u32MaxDiscardCnt = g_taMsgStatus[u32Idx].u32CurrDiscardCnt;
                    }

                    printf("expired id[%u] for idx[%u] curr_id[%u]: cnt[%u] max_cnt[%u] max_offset[%u]\n", 
                           u32MsgId, u32Idx, g_taMsgStatus[u32Idx].u32CurrMsgId, 
                           g_taMsgStatus[u32Idx].u32CurrDiscardCnt, g_taMsgStatus[u32Idx].u32MaxDiscardCnt, 
                           g_taMsgStatus[u32Idx].u32MaxExpireOffset);
                    
                    goto done;
                }
                else
                {
                    // receive enough older msgs => they are not expired and we need to update current msg id of all types

                    uint32_t i = 0;

                    for(i = 0; i < PROPERTY_MAX; i++)
                    {
                        g_taMsgStatus[i].u32CurrMsgId = u32MsgId;
                        g_taMsgStatus[i].u32CurrDiscardCnt = 0;
                        g_taMsgStatus[i].u32UpdateTime = u32Time;
                    }

                    printf("update curr_id[%u] for all idx\n", u32MsgId);

                    iRet = 0;
                    goto done;
                }
            }
            else
            {
                // receive much smaller msg id => update current msg id directly
            }
        }
        else
        {
            // receive bigger msg id => update current msg id directly
        }
    }

update:
    g_taMsgStatus[u32Idx].u32CurrMsgId = u32MsgId;
    g_taMsgStatus[u32Idx].u32CurrDiscardCnt = 0;
    g_taMsgStatus[u32Idx].u32UpdateTime = u32Time;

    printf("update idx[%u] curr_id[%u] upd_time[%u]\n", u32Idx, g_taMsgStatus[u32Idx].u32CurrMsgId, g_taMsgStatus[u32Idx].u32UpdateTime);

    iRet = 0;

done:
    return iRet;
}

static int user_property_set_event_handler(const int devid, const char *request, const int request_len)
{
#ifdef ALI_NO_POST_MODE
    if(request_len <= 64)
    {
        if (memcmp(g_certification_str, request, request_len)==0)
        {
            g_certification_cnt++;
            if (g_certification_cnt == 8)
            {
                g_certification_mode_flag = 1;
            }
        }
        else
        {
            g_certification_cnt = 0;
            g_certification_mode_flag = 0;
        }
        memset(g_certification_str,0,64);
        memcpy(g_certification_str,request,request_len);
    }
#endif

    int iRet = -1;

    lite_cjson_t tRoot = {0};

    lite_cjson_t tLightSwitch = {0};
    lite_cjson_t tLocalTimer = {0};
    lite_cjson_t tBrightness = {0};
    lite_cjson_t tWorkMode = {0};
    lite_cjson_t tColorSpeed = {0};
    lite_cjson_t tColorArr = {0};
    lite_cjson_t tHSVColor = {0};
    lite_cjson_t tRGBColor = {0};
    lite_cjson_t tColorTemperature = {0};
    lite_cjson_t tLightMode = {0};
    //lite_cjson_t tLightType = {0};

    uint16_t u16OutputFlag = 0;
    uint32_t u32MsgId = g_u32RecvMsgId;
    
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();
//    printf("Property Set Received, Devid: %d, Request: %s\r\n", devid, request);

    #ifdef ADA_REMOTE_CTRL
    /* stop effect operation triggered by key controller*/
    light_effect_timer_stop();
    #endif

    /* stop effect operation triggered by APP*/
    light_effect_stop(NULL);

    g_u8CurrColorArrayEnable = 0;

    /* Parse Request */
    if (lite_cjson_parse(request, request_len, &tRoot)) {
        EXAMPLE_TRACE("JSON Parse Error");
        goto done;
    }

    /* Try To Find LightSwitch Property */		//LightSwitch   Value:<int>,0,1
    if (!lite_cjson_object_item(&tRoot, "LightSwitch", 11, &tLightSwitch)) {
        EXAMPLE_TRACE("LightSwitch Enable : %d\r\n", tLightSwitch.value_int);

        if(!is_expired_msg(PROPERTY_LIGHT_SWITCH, u32MsgId))
        {
            light_ctrl_set_switch(tLightSwitch.value_int);
    
            if (tLightSwitch.value_int && light_effect_get_status())
                light_effect_start(NULL, 0);
    
            #ifdef ALI_NO_POST_MODE
            if (!g_certification_mode_flag)
            #endif
            {
                SET_BIT(u16OutputFlag, PROPERTY_LIGHT_SWITCH);
                //iot_update_light_property(u16OutputFlag, u32MsgId);
            }
        }
    }
    
    /* Try To Find LocalTimer Property */
    if (!lite_cjson_object_item(&tRoot, "LocalTimer", 10, &tLocalTimer) && lite_cjson_is_array(&tLocalTimer)) {
        if (light_ctrl_get_switch() && light_effect_get_status())
            light_effect_start(NULL, 0);
        int index = 0;
        int iArraySize = tLocalTimer.size;
        T_BleWifi_Ctrl_DevSchedAll tSchedAll = {0};

        EXAMPLE_TRACE("Local Timer Size: %d", iArraySize);

        tSchedAll.u8Num = (uint8_t)iArraySize;

        for (index = 0; index < iArraySize && index < MW_FIM_GP13_DEV_SCHED_NUM; index++) {
            lite_cjson_t tSubItem = {0};

            if(!lite_cjson_array_item(&tLocalTimer, index, &tSubItem))
            {
                if (lite_cjson_is_object(&tSubItem)) {
                    lite_cjson_t tLightSwitch = {0};
                    lite_cjson_t tTimer = {0};
                    lite_cjson_t tEnable = {0};
                    lite_cjson_t tIsValid = {0};
                    lite_cjson_t tTimeZone = {0};

                    if(!lite_cjson_object_item(&tSubItem, "LightSwitch", 11, &tLightSwitch) && 
                       !lite_cjson_object_item(&tSubItem, "Timer", 5, &tTimer) && 
                       !lite_cjson_object_item(&tSubItem, "Enable", 6, &tEnable) && 
                       !lite_cjson_object_item(&tSubItem, "IsValid", 7, &tIsValid) && 
                       !lite_cjson_object_item(&tSubItem, "TimezoneOffset", 14, &tTimeZone))
                    {
                        char sBuf[32] = {0};

                        if(tTimer.value_length < 32)
                        {
                            memcpy(sBuf, tTimer.value, tTimer.value_length);
                            sBuf[tTimer.value_length] = 0;
                        }

                        EXAMPLE_TRACE("Local Timer: Index[%d] LightSwitch[%d] Timer[%s] Enable[%d] IsValid[%d] TimezoneOffset[%d]", 
                                      index, tLightSwitch.value_int, sBuf, tEnable.value_int, tIsValid.value_int, tTimeZone.value_int);
    
                        tSchedAll.taSched[index].u8Enable = tEnable.value_int;
                        tSchedAll.taSched[index].u8IsValid = tIsValid.value_int;
    
                        tSchedAll.taSched[index].u8DevOn = tLightSwitch.value_int;

                        #ifdef BLEWIFI_SCHED_EXT
                        tSchedAll.taSchedExt[index].s32TimeZone = tTimeZone.value_int;
                        #else
                        tSchedAll.taSched[index].s32TimeZone = tTimeZone.value_int;
                        #endif
    
                        if(dev_sched_time_set(sBuf, &(tSchedAll.taSched[index])))
                        {
                            //EXAMPLE_TRACE("dev_sched_time_set fail\r\n");
    
                            tSchedAll.taSched[index].u8Enable = 0;
                        }
                    }
                }
            }
        }

        // post local timer setting in blewifi_ctrl task
        if(BleWifi_Ctrl_DevSchedSetAll(&tSchedAll))
        {
            EXAMPLE_TRACE("BleWifi_Ctrl_DevSchedSetAll fail\r\n");
        }
    }

    /*
    if(light_ctrl_get_switch() == 0)
    {
        goto done;
    }
    */

    /* Try To Find LightMode Property */
    if (!lite_cjson_object_item(&tRoot, "LightMode", 9, &tLightMode)) {
        EXAMPLE_TRACE("LightMode: %d\r\n", tLightMode.value_int);

        if(!is_expired_msg(PROPERTY_LIGHTMODE, u32MsgId))
        {
            if(light_ctrl_get_switch())
            {
                //IoT_Properity.ubLightMode = (uint8_t)tLightMode.value_int;

                if((light_ctrl_get_light_type() == LT_RGB) && 
                   (tLightMode.value_int == MODE_CTB))
                {
                    printf("mode[%u] not supported\n", tLightMode.value_int);
                }
                else
                {
                    light_ctrl_set_mode((uint8_t)tLightMode.value_int);
                }
            
                light_ctrl_set_switch(1);

                if(light_ctrl_get_mode() == MODE_SCENES)
                {
                    iot_set_scenes_color(light_ctrl_get_workmode());
                }
        
                SET_BIT(u16OutputFlag, PROPERTY_LIGHTMODE);
                //iot_update_light_property(u16OutputFlag, u32MsgId);
            }
            else
            {
                light_ctrl_set_mode((uint8_t)tLightMode.value_int);
            }
        }
    }

    if(light_ctrl_get_light_type() != LT_RGB)
    {
        /* Try To Find Brightness Property */		//Brightness				Value:<int> 0~100
        if (!lite_cjson_object_item(&tRoot, "Brightness", 10, &tBrightness)) {
            EXAMPLE_TRACE("Brightness: %d\r\n", tBrightness.value_int);
    
            if(!is_expired_msg(PROPERTY_BRIGHTNESS, u32MsgId))
            {
                uint8_t u8SameValue = 0;
    
                light_effect_set_status(0);
        
                if(tBrightness.value_int == light_ctrl_get_brightness())
                {
                    //printf("same brightness\n");
                    u8SameValue = 1;
                    //goto done;
                }
        
                if(tBrightness.value_int>100)
                {
                    //printf("[Brightness],Invalid Value:%d\n",tBrightness.value_int);
                }
                else
                {
                    if((light_ctrl_get_switch()) && (light_ctrl_get_mode() == MODE_CTB))
                    {
                        if(!u8SameValue)
                        {
                            if(tBrightness.value_int == 1)
                            {
                                uint32_t brightness_colortemp_tmp = light_ctrl_get_color_temperature();
        
                                if(brightness_colortemp_tmp > 4500)
                                {
                                    light_ctrl_set_ctb(7000, 1, LIGHT_FADE_ON, UPDATE_LED_STATUS);
                                }
                                else
                                {
                                    light_ctrl_set_ctb(2000, 1, LIGHT_FADE_ON, UPDATE_LED_STATUS);
                                }
                            }
                            else
                            {
                                light_ctrl_set_brightness(tBrightness.value_int);
                            }
                    
                            #ifdef ALI_NO_POST_MODE
                            if (!g_certification_mode_flag)
                            #endif
                            {
                                SET_BIT(u16OutputFlag, PROPERTY_BRIGHTNESS);
                                SET_BIT(u16OutputFlag, PROPERTY_LIGHTMODE);
                                //iot_update_light_property(u16OutputFlag, u32MsgId);
                            }
                        }
                    }
                    else
                    {
                        //printf("[%s %d] recv Brightness: curr mode[%u] is NOT MODE_CTB\n", __func__, __LINE__, light_ctrl_get_mode());
                        light_ctrl_update_brightness(tBrightness.value_int);
                    }
                }
            }
        }
    }

    if((light_ctrl_get_light_type() == LT_CW) || 
       ((light_ctrl_get_light_type() == LT_RGBCW)))
    {
            /* Try To Find ColorTemperature Property */		//ColorTemperature   Value:<int> 2000~7000
        if (!lite_cjson_object_item(&tRoot, "ColorTemperature", 16, &tColorTemperature)) {
            EXAMPLE_TRACE("ColorTemperature: %d\r\n", tColorTemperature.value_int);
            
            if(!is_expired_msg(PROPERTY_COLORTEMPERATURE, u32MsgId))
            {
                uint8_t u8SameValue = 0;
    
                light_effect_set_status(0);
    
                if(tColorTemperature.value_int == light_ctrl_get_color_temperature())
                {
                    //printf("same temperature\n");
                    u8SameValue = 1;
                    //goto done;
                }
    
                if(tColorTemperature.value_int<2000 || tColorTemperature.value_int>7000 )
                {
                    printf("[ColorTemperature],Invalid Value:%d\n",tColorTemperature.value_int);
                }
                else
                {
                    if((light_ctrl_get_switch()) && (light_ctrl_get_mode() == MODE_CTB))
                    {
                        if(!u8SameValue)
                        {
                            uint32_t brightness_colortemp_tmp = light_ctrl_get_brightness();
                            if(brightness_colortemp_tmp == 1)
                            {
                                if(tColorTemperature.value_int > 4500)
                                {
                                    light_ctrl_set_ctb(7000, 1, LIGHT_FADE_ON, UPDATE_LED_STATUS);
                                }
                                else
                                {
                                    light_ctrl_set_ctb(2000,1, LIGHT_FADE_ON, UPDATE_LED_STATUS);
                                }
                            }
                            else
                            {
                                light_ctrl_set_color_temperature(tColorTemperature.value_int);
                            }
                            //colortemperature_force_overwrite(tColorTemperature.value_int);
    
                            #ifdef ALI_NO_POST_MODE
                            if (!g_certification_mode_flag)
                            #endif
                            {
                                SET_BIT(u16OutputFlag, PROPERTY_COLORTEMPERATURE);
                                SET_BIT(u16OutputFlag, PROPERTY_LIGHTMODE);
                                //iot_update_light_property(u16OutputFlag, u32MsgId);
                            }
                        }
                    }
                    else
                    {
                        colortemperature_force_overwrite(tColorTemperature.value_int);
                    }
                }
            }
        }
    }

    /* Try To Find HSVColor Property */  //HSVColor   Value:[child1] Hue  Value:<double>0~360   [child2] Saturation  Value:<double>0~100   [child3] Value  Value:<double>0~100
    if (!lite_cjson_object_item(&tRoot, "HSVColor", 8, &tHSVColor)) {
        lite_cjson_t tSaturation = {0};
        lite_cjson_t tValue = {0};
        lite_cjson_t tHue = {0};

        if(!lite_cjson_object_item(&tHSVColor, "Saturation", 10, &tSaturation) && 
           !lite_cjson_object_item(&tHSVColor, "Value", 5, &tValue) && 
           !lite_cjson_object_item(&tHSVColor, "Hue", 3, &tHue))
        {
            EXAMPLE_TRACE("H[%d] S[%d] V[%d]", tHue.value_int, tSaturation.value_int, tValue.value_int);

            if(!is_expired_msg(PROPERTY_HSVCOLOR, u32MsgId))
            {
                uint8_t u8SameValue = 0;

                light_effect_set_status(0);
    
                if((tHue.value_int == light_ctrl_get_hue()) && 
                   (tSaturation.value_int == light_ctrl_get_saturation()) && 
                   (tValue.value_int == light_ctrl_get_value()))
                {
                    //printf("same HSV\n");
                    u8SameValue = 1;
                    //goto done;
                }
    
                if((light_ctrl_get_switch()) && (light_ctrl_get_mode() == MODE_HSV))
                {
                    if(!u8SameValue)
                    {
                        light_ctrl_set_hsv(tHue.value_int, tSaturation.value_int, tValue.value_int, LIGHT_FADE_ON, UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);

                        #ifdef ALI_NO_POST_MODE
                        if (!g_certification_mode_flag)
                        #endif
                        {
                            SET_BIT(u16OutputFlag, PROPERTY_HSVCOLOR);
                            SET_BIT(u16OutputFlag, PROPERTY_LIGHTMODE);
                            //iot_update_light_property(u16OutputFlag, u32MsgId);
                        }
                    }
                }
                else
                {
                    //printf("[%s %d] recv HSVColor: curr mode[%u] is NOT MODE_HSV\n", __func__, __LINE__, light_ctrl_get_mode());
                    light_ctrl_update_hsv(tHue.value_int, tSaturation.value_int, tValue.value_int);
                }
    
                light_ctrl_set_manual_light_status(tHue.value_int, tSaturation.value_int, tValue.value_int);
            }
        }
    }

    /* Try To Find WorkMode Property */	//WorkMode   Value:<enum> 0~5
    if (!lite_cjson_object_item(&tRoot, "WorkMode", 8, &tWorkMode)) {
        EXAMPLE_TRACE("WorkMode: %d\r\n",tWorkMode.value_int);

        if(!is_expired_msg(PROPERTY_WORKMODE, u32MsgId))
        {
            if((light_ctrl_get_switch()) && (light_ctrl_get_mode() == MODE_SCENES))
            {
                light_effect_set_status(0);
    
                iot_set_scenes_color(tWorkMode.value_int);
            
                light_ctrl_set_mode(MODE_SCENES);
        
                #ifdef ALI_NO_POST_MODE
                if (!g_certification_mode_flag)
                #endif
                {
                    SET_BIT(u16OutputFlag, PROPERTY_SCENESCOLOR);
                    SET_BIT(u16OutputFlag, PROPERTY_WORKMODE);
                    SET_BIT(u16OutputFlag, PROPERTY_LIGHTMODE);
                    //iot_update_light_property(u16OutputFlag, u32MsgId);
                }
            }
            else
            {
                //printf("[%s %d] recv WorkMode: curr mode[%u] is NOT MODE_SCENES\n", __func__, __LINE__, light_ctrl_get_mode());
                light_ctrl_set_workmode(tWorkMode.value_int);
    
                //SET_BIT(u16OutputFlag, PROPERTY_WORKMODE);
                //iot_update_light_property(u16OutputFlag, u32MsgId);
            }
        }
    }

    /* Try To Find ColorSpeed Property */
    if (!lite_cjson_object_item(&tRoot, "ColorSpeed", 10, &tColorSpeed)) {
        EXAMPLE_TRACE("ColorSpeed: %d\r\n", tColorSpeed.value_int);

        if(!is_expired_msg(PROPERTY_COLORSPEED, u32MsgId))
        {
            if((light_ctrl_get_switch()) && (light_ctrl_get_mode() == MODE_SCENES))
            {
                //IoT_Properity.iColorSpeed = (int32_t)tColorSpeed.value_int;
                //IoT_Ring_Buffer_Push(&IoT_Properity);
                light_effect_set_speed(tColorSpeed.value_int, 1);
            }
            else
            {
                light_effect_set_speed(tColorSpeed.value_int, 0);
            }
        }
    }

    /* Try To Find ColorArr Property */
    if (!lite_cjson_object_item(&tRoot, "ColorArr", 8, &tColorArr) && lite_cjson_is_array(&tColorArr)) {
        EXAMPLE_TRACE("ColorArr Size: %d", tColorArr.size);

        if(!is_expired_msg(PROPERTY_COLORARR, u32MsgId))
        {
            int index = 0;
            int color_num = 0;
            hsv_t color_arr[6] = {0};

            light_effect_set_status(1);
    
            for (index = 0; index < tColorArr.size && index < 6; index++) {
                lite_cjson_t tSubItem = {0};
    
                if(!lite_cjson_array_item(&tColorArr, index, &tSubItem))
                {
                    if (lite_cjson_is_object(&tSubItem)) {
                        lite_cjson_t tHue = {0};
                        lite_cjson_t tSaturation = {0};
                        lite_cjson_t tValue = {0};
                        lite_cjson_t tEnable = {0};
    
                        if(!lite_cjson_object_item(&tSubItem, "Hue", 3, &tHue) && 
                           !lite_cjson_object_item(&tSubItem, "Saturation", 10, &tSaturation) && 
                           !lite_cjson_object_item(&tSubItem, "Value", 5, &tValue) && 
                           !lite_cjson_object_item(&tSubItem, "Enable", 6, &tEnable))
                        {
                            EXAMPLE_TRACE("H[%d] S[%d] V[%d] Enable[%d]", tHue.value_int, tSaturation.value_int, tValue.value_int, tEnable.value_int);
                            
                            if (tEnable.value_int == 1)
                            {
                                color_arr[index].hue = tHue.value_int;
                                color_arr[index].saturation = tSaturation.value_int;
                                color_arr[index].value = tValue.value_int;
                                color_num = index + 1;
                            }
                        }
                    }
                }
            }

            if((light_ctrl_get_switch()) && (light_ctrl_get_mode() == MODE_SCENES))
            {
                iot_set_color_array(color_arr, color_num, 1);
                g_u8CurrColorArrayEnable = 1;
        
                SET_BIT(u16OutputFlag, PROPERTY_COLORARR);
                //iot_update_light_property(u16OutputFlag, u32MsgId);
            }
            else
            {
                iot_set_color_array(color_arr, color_num, 0);
            }
        }
    }

    /* Try To Find RGBColor Property */		//RGBColor   Value:[child1] Red  Value:<int>0~255  [child2] Blue  Value:<int>0~255   [child3] Green  Value:<int>0~255
    if (!lite_cjson_object_item(&tRoot, "RGBColor", 8, &tRGBColor)) {
        lite_cjson_t tRed = {0};
        lite_cjson_t tGreen = {0};
        lite_cjson_t tBlue = {0};

        if(!is_expired_msg(PROPERTY_RGBCOLOR, u32MsgId))
        {
            if(!lite_cjson_object_item(&tHSVColor, "Red", 3, &tRed) && 
               !lite_cjson_object_item(&tHSVColor, "Green", 5, &tGreen) && 
               !lite_cjson_object_item(&tHSVColor, "Blue", 4, &tBlue))
            {
                EXAMPLE_TRACE("R[%d] G[%d] B[%d]", tRed.value_int, tGreen.value_int, tBlue.value_int);

                if((light_ctrl_get_switch()) && (light_ctrl_get_mode() == MODE_HSV))
                {
                    light_effect_set_status(0);
                    light_ctrl_set_rgb(tRed.value_int,tGreen.value_int,tBlue.value_int);
        
                    #ifdef ALI_NO_POST_MODE
                    if (!g_certification_mode_flag)
                    #endif
                    {
                        SET_BIT(u16OutputFlag, PROPERTY_HSVCOLOR);
                        SET_BIT(u16OutputFlag, PROPERTY_LIGHTMODE);
                        //iot_update_light_property(u16OutputFlag, u32MsgId);
                    }
                }
                else
                {
                    light_ctrl_update_rgb(tRed.value_int,tGreen.value_int,tBlue.value_int);
                }
            }
        }
    }

    iRet = 0;

done:
    if(!iRet)
    {
        if(u16OutputFlag)
        {
            iot_update_light_property(u16OutputFlag, u32MsgId);
        }
    }

    return iRet;
}

static int user_property_get_event_handler(const int devid, const char *request, const int request_len, char **response,
        int *response_len)
{
    uint32_t u32BufSize = 256;
    char *ps8Buf = NULL;
    uint32_t u32Offset = 0;
    
    EXAMPLE_TRACE("Property Get Received, Devid: %d, Request: %s", devid, request);

    if(iot_is_local_timer_valid())
    {
        u32BufSize += PROPERTY_LOCALTIMER_PAYLOAD_LEN;
    }

    ps8Buf = (char *)HAL_Malloc(u32BufSize);

    if(ps8Buf)
    {
        uint8_t u8LightMode = light_ctrl_get_mode();

        u32Offset += snprintf(ps8Buf + u32Offset, u32BufSize - u32Offset, "{\"LightSwitch\":%d,\"LightMode\":%d,\"LightType\":%d", 
                             light_ctrl_get_switch(),
                             u8LightMode,
                             light_ctrl_get_light_type());

        switch(u8LightMode)
        {
            case MODE_CTB:
                if((light_ctrl_get_light_type() == LT_CW) || 
                   ((light_ctrl_get_light_type() == LT_RGBCW)))
                {
                    u32Offset += snprintf(ps8Buf + u32Offset, u32BufSize - u32Offset, ",\"ColorTemperature\":%d", light_ctrl_get_color_temperature());
                }
                
                u32Offset += snprintf(ps8Buf + u32Offset, u32BufSize - u32Offset, ",\"Brightness\":%d", light_ctrl_get_brightness());
                break;
    
            case MODE_HSV:
                u32Offset += snprintf(ps8Buf + u32Offset, u32BufSize - u32Offset, ",\"HSVColor\":{\"Saturation\":%d,\"Value\":%d,\"Hue\":%d}", 
                                      light_ctrl_get_saturation(), light_ctrl_get_value(), light_ctrl_get_hue());
                break;
    
            case MODE_SCENES:
            {
                uint16_t u16SceneHue = 0;
                uint8_t u8SceneSaturation = 0;
                uint8_t u8SceneValue = 0;
            
                light_ctrl_get_scenescolor(&u16SceneHue, &u8SceneSaturation, &u8SceneValue);
    
                u32Offset += snprintf(ps8Buf + u32Offset, u32BufSize - u32Offset, ",\"WorkMode\":%d,\"ScenesColor\":{\"Saturation\":%d,\"Value\":%d,\"Hue\":%d}", 
                                      light_ctrl_get_workmode(), u8SceneSaturation, u8SceneValue, u16SceneHue);
                break;
            }
    
            default:
                break;
        }

        u32Offset += snprintf(ps8Buf + u32Offset, u32BufSize - u32Offset, ",");
        u32Offset = iot_get_local_timer(ps8Buf, u32BufSize, u32Offset);
        u32Offset += snprintf(ps8Buf + u32Offset, u32BufSize - u32Offset, "}");

        *response = ps8Buf;
        *response_len = u32Offset;

        printf("property_get malloc[%u] len[%u]\n", u32BufSize, *response_len);
        printf("%s\n\n", *response);
    }

    return SUCCESS_RETURN;
}

static int user_report_reply_event_handler(const int devid, const int msgid, const int code, const char *reply,
        const int reply_len)
{
    const char *reply_value = (reply == NULL) ? ("NULL") : (reply);
    const int reply_value_len = (reply_len == 0) ? (strlen("NULL")) : (reply_len);

    EXAMPLE_TRACE("Message Post Reply Received, Devid: %d, Message ID: %d, Code: %d, Reply: %.*s", devid, msgid, code,
                  reply_value_len,
                  reply_value);

    if(g_tPrevPostInfo.u8Used)
    {
        if(code == 200)
        {
            if(msgid == g_tPrevPostInfo.iId)
            {
                EXAMPLE_TRACE("msgid[%d] == prev_post_id[%d], clear post info.\n", msgid, g_tPrevPostInfo.iId);

                g_tPrevPostInfo.u32Cnt += 1;

                if(g_tPrevPostInfo.u32Cnt == 1)
                {
                    extern void Iot_TsSyncEnable(uint8_t u8Enable);

                    Iot_TsSyncEnable(1);
                }

                //post_info_clear();
                g_tPrevPostInfo.u8Used = 0;
            }
            else
            {
                EXAMPLE_TRACE("msgid[%d] != prev_post_id[%d]\n", msgid, g_tPrevPostInfo.iId);
            }
        }
        else
        {
            extern iotx_cm_connection_t *_mqtt_conncection;
            extern void iotx_mc_set_client_state(iotx_mc_client_t *pClient, iotx_mc_state_t newState);

            EXAMPLE_TRACE("code[%d]: trigger reconnect\n", code);

            if((_mqtt_conncection) && (_mqtt_conncection->context))
            {
                iotx_mc_set_client_state(_mqtt_conncection->context, IOTX_MC_STATE_DISCONNECTED);
            }
            else
            {
                EXAMPLE_TRACE("failed to trigger reconnect\n");
            }

            // clear buffer/post_info and enable one_shot_arp in user_disconnected_event_handler()
        }
    }

    return 0;
}

static int user_trigger_event_reply_event_handler(const int devid, const int msgid, const int code, const char *eventid,
        const int eventid_len, const char *message, const int message_len)
{
    EXAMPLE_TRACE("Trigger Event Reply Received, Devid: %d, Message ID: %d, Code: %d, EventID: %.*s, Message: %.*s", devid,
                  msgid, code,
                  eventid_len,
                  eventid, message_len, message);

    return 0;
}

static int user_timestamp_reply_event_handler(const char *timestamp)
{
    EXAMPLE_TRACE("Current Timestamp: %s", timestamp);

    #ifdef ALI_TIMESTAMP
    if(timestamp)
    {
        uint64_t u64TimestampMs = 0;
        uint32_t u32Timestamp = 0;

        extern void Iot_NextSyncTimeSet(void);
    
        u64TimestampMs = strtoull(timestamp, NULL, 10);
        u32Timestamp = u64TimestampMs / 1000;

        BleWifi_CurrentTimeSet(u32Timestamp);
    
        BleWifi_Ctrl_MsgSend(BLEWIFI_CTRL_MSG_DEV_SCHED_START, NULL, 0);

        Iot_NextSyncTimeSet();
    }
    #endif

    return 0;
}

static int user_initialized(const int devid)
{
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();
    EXAMPLE_TRACE("Device Initialized, Devid: %d", devid);

    if (user_example_ctx->master_devid == devid) {
        user_example_ctx->master_initialized = 1;
    }

    return 0;
}

/** type:
  *
  * 0 - new firmware exist
  *
  */
static int user_fota_event_handler(int type, const char *version)
{
    char buffer[128] = {0};
    int buffer_length = 128;
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();

    if (type == 0) {
        char s8aFrimwareVer[IOTX_FIRMWARE_VER_LEN+1] = {0};

        HAL_GetFirmwareVersion(s8aFrimwareVer);

        EXAMPLE_TRACE("New Firmware Version: %s", version);
        EXAMPLE_TRACE("Current Firmware Version: %s", s8aFrimwareVer);

        IOT_Linkkit_Query(user_example_ctx->master_devid, ITM_MSG_QUERY_FOTA_DATA, (unsigned char *)buffer, buffer_length);
    }

    return 0;
}

#ifdef ALI_POST_CTRL
SHM_DATA void user_post_property(IoT_Properity_t *ptProp)
{
    int iRes = 0;
    //char property_payload[256] = {0};
    uint32_t u32Offset = 0;
    uint8_t already_wrt_flag = 0;
    //time_t property_timestamp = BleWifi_SntpGetRawData();
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();

    //char *ColorArr_update = "{\"ColorArr\":{\"Saturation\":%d,\"Value\":%d,\"Hue\":%d,\"Enable\":%d}}";
    char *HSVColor_update = "\"HSVColor\":{\"Saturation\":%d,\"Value\":%d,\"Hue\":%d}";
    char *ScenesColor_update = "\"ScenesColor\":{\"Saturation\":%d,\"Value\":%d,\"Hue\":%d}";

    char s8aPayload[256] = {0};
    uint32_t u32BufSize = sizeof(s8aPayload);
    char *property_payload = s8aPayload;

    /*
    uint8_t u8WarmLightEnabled = 0;

    if((light_ctrl_get_light_type() == LT_CW) || 
       (light_ctrl_get_light_type() == LT_RGBCW))
    {
        u8WarmLightEnabled = 1;
    }
    */

    /*
    if((CHK_BIT(ptProp->u16Flag, PROPERTY_HSVCOLOR)) || 
       (CHK_BIT(ptProp->u16Flag, PROPERTY_BRIGHTNESS)))
    {
        SET_BIT(ptProp->u16Flag, PROPERTY_LIGHT_SWITCH);
        SET_BIT(ptProp->u16Flag, PROPERTY_LIGHTMODE);
        SET_BIT(ptProp->u16Flag, PROPERTY_LIGHTTYPE);
    }
    else*/ if(CHK_BIT(ptProp->u16Flag, PROPERTY_LIGHTMODE))
    {
        switch(light_ctrl_get_mode())
        {
        case MODE_CTB:
            SET_BIT(ptProp->u16Flag, PROPERTY_BRIGHTNESS);

            //if(u8WarmLightEnabled)
            if(light_ctrl_get_light_type() != LT_RGB)
            {
                SET_BIT(ptProp->u16Flag, PROPERTY_COLORTEMPERATURE);
            }

            break;

        case MODE_HSV:
            SET_BIT(ptProp->u16Flag, PROPERTY_HSVCOLOR);
            break;

        case MODE_SCENES:
            SET_BIT(ptProp->u16Flag, PROPERTY_WORKMODE);
            SET_BIT(ptProp->u16Flag, PROPERTY_SCENESCOLOR);
            break;

        default:
            break;
        }

        SET_BIT(ptProp->u16Flag, PROPERTY_LIGHT_SWITCH);
        SET_BIT(ptProp->u16Flag, PROPERTY_LIGHTTYPE);
    }
    else if((CHK_BIT(ptProp->u16Flag, PROPERTY_HSVCOLOR)) || 
            (CHK_BIT(ptProp->u16Flag, PROPERTY_BRIGHTNESS)) || 
            (CHK_BIT(ptProp->u16Flag, PROPERTY_COLORTEMPERATURE)))
    {
        SET_BIT(ptProp->u16Flag, PROPERTY_LIGHT_SWITCH);
        SET_BIT(ptProp->u16Flag, PROPERTY_LIGHTMODE);
        SET_BIT(ptProp->u16Flag, PROPERTY_LIGHTTYPE);

        if(CHK_BIT(ptProp->u16Flag, PROPERTY_BRIGHTNESS))
        {
            //if(u8WarmLightEnabled)
            if(light_ctrl_get_light_type() != LT_RGB)
            {
                SET_BIT(ptProp->u16Flag, PROPERTY_COLORTEMPERATURE);
            }
        }
        else if(CHK_BIT(ptProp->u16Flag, PROPERTY_COLORTEMPERATURE))
        {
            SET_BIT(ptProp->u16Flag, PROPERTY_BRIGHTNESS);
        }
    }
    else
    {
        SET_BIT(ptProp->u16Flag, PROPERTY_LIGHT_SWITCH);

        //if(u8WarmLightEnabled)
        if(light_ctrl_get_light_type() != LT_RGB)
        {
            SET_BIT(ptProp->u16Flag, PROPERTY_COLORTEMPERATURE);
        }
        
        SET_BIT(ptProp->u16Flag, PROPERTY_BRIGHTNESS);
        SET_BIT(ptProp->u16Flag, PROPERTY_WORKMODE);
        SET_BIT(ptProp->u16Flag, PROPERTY_LIGHTMODE);
        //SET_BIT(ptProp->u16Flag, PROPERTY_COLORSPEED);
        SET_BIT(ptProp->u16Flag, PROPERTY_LIGHTTYPE);
        //SET_BIT(ptProp->u16Flag, PROPERTY_COLORARR);
        SET_BIT(ptProp->u16Flag, PROPERTY_HSVCOLOR);
        SET_BIT(ptProp->u16Flag, PROPERTY_SCENESCOLOR);
    }

    if(CHK_BIT(ptProp->u16Flag, PROPERTY_LOCALTIMER))
    {
        uint8_t u8IsValid = iot_is_local_timer_valid();

        if(u8IsValid)
        {
            u32BufSize += PROPERTY_LOCALTIMER_PAYLOAD_LEN;

            property_payload = (char *)HAL_Malloc(u32BufSize);

            if(!property_payload)
            {
                goto done;
            }
        }
    }
    
    u32Offset = sprintf( property_payload, "{");
    
    if (CHK_BIT(ptProp->u16Flag, PROPERTY_LIGHT_SWITCH))
    {
        if (already_wrt_flag)
            u32Offset += sprintf( property_payload +u32Offset, ",");
        u32Offset += sprintf( property_payload +u32Offset, "\"LightSwitch\":%d", light_ctrl_get_switch());
        already_wrt_flag = 1;
    }
    
    if (CHK_BIT(ptProp->u16Flag, PROPERTY_COLORTEMPERATURE))
    {
        if (already_wrt_flag)
          u32Offset += sprintf( property_payload +u32Offset, ",");
        
        u32Offset += sprintf( property_payload +u32Offset, "\"ColorTemperature\":%d", light_ctrl_get_color_temperature());
        already_wrt_flag = 1;
    }
    if (CHK_BIT(ptProp->u16Flag, PROPERTY_BRIGHTNESS))
    {
        if (already_wrt_flag)
            u32Offset += sprintf( property_payload +u32Offset, ",");
        
        u32Offset += sprintf( property_payload +u32Offset, "\"Brightness\":%d", light_ctrl_get_brightness());
        already_wrt_flag = 1;
    }
    if (CHK_BIT(ptProp->u16Flag, PROPERTY_WORKMODE))
    {
        if (already_wrt_flag)
            u32Offset += sprintf( property_payload +u32Offset, ",");
        
        u32Offset += sprintf( property_payload +u32Offset, "\"WorkMode\":%d", light_ctrl_get_workmode());
        already_wrt_flag = 1;
    }
    if (CHK_BIT(ptProp->u16Flag, PROPERTY_SCENESCOLOR))
    {
        uint16_t scene_hue = 0;
        uint8_t scene_saturation = 0;
        uint8_t scene_value = 0;
        if (already_wrt_flag)
            u32Offset += sprintf( property_payload +u32Offset, ",");
        
        light_ctrl_get_scenescolor(&scene_hue,&scene_saturation,&scene_value);
        u32Offset += sprintf( property_payload + u32Offset, ScenesColor_update, scene_saturation, scene_value, scene_hue);
        already_wrt_flag = 1;
    }
    if (CHK_BIT(ptProp->u16Flag, PROPERTY_LIGHTMODE))
    {
        if (already_wrt_flag)
            u32Offset += sprintf( property_payload +u32Offset, ",");
        
        u32Offset += sprintf( property_payload +u32Offset, "\"LightMode\":%d", light_ctrl_get_mode());
        already_wrt_flag = 1;
    }
    if (CHK_BIT(ptProp->u16Flag, PROPERTY_COLORSPEED))
    {
        if (already_wrt_flag)
            u32Offset += sprintf( property_payload +u32Offset, ",");
        
        u32Offset += sprintf( property_payload +u32Offset, "\"ColorSpeed\":%d", light_effect_get_speed());
        already_wrt_flag = 1;
    }
    if (CHK_BIT(ptProp->u16Flag, PROPERTY_LIGHTTYPE))
    {
        if (already_wrt_flag)
            u32Offset += sprintf( property_payload +u32Offset, ",");
        
        u32Offset += sprintf( property_payload +u32Offset, "\"LightType\":%d", light_ctrl_get_light_type());
        already_wrt_flag = 1;
    }
    if (CHK_BIT(ptProp->u16Flag, PROPERTY_COLORARR))
    {

    }
    if (CHK_BIT(ptProp->u16Flag, PROPERTY_HSVCOLOR))
    {
        if (already_wrt_flag)
            u32Offset += sprintf( property_payload +u32Offset, ",");

        u32Offset += sprintf( property_payload + u32Offset, HSVColor_update, light_ctrl_get_saturation(), light_ctrl_get_value(), light_ctrl_get_hue());
        already_wrt_flag = 1;
    }

    if (CHK_BIT(ptProp->u16Flag, PROPERTY_LOCALTIMER))
    {
        if (already_wrt_flag)
            u32Offset += sprintf( property_payload +u32Offset, ",");

        u32Offset = iot_get_local_timer(property_payload, u32BufSize, u32Offset);
        already_wrt_flag = 1;
    }
    
    u32Offset += sprintf( property_payload +u32Offset, "}");

    printf("\npost for msg_id[%u]: buf_size[%u] len[%u]\n", ptProp->u32MsgId, u32BufSize, u32Offset);
    printf("%s\n\n", property_payload);

    iRes = IOT_Linkkit_Report(user_example_ctx->master_devid, ITM_MSG_POST_PROPERTY,
                             (unsigned char *)property_payload, u32Offset);

    if(iRes >= 0)
    {
        post_info_update(iRes);
    }

done:
    if(property_payload)
    {
        if(property_payload != s8aPayload)
        {
            HAL_Free(property_payload);
        }
    }

    return;
}
#else
void user_post_property(IoT_Properity_t *ptProp)
{

}
#endif //#ifdef ALI_POST_CTRL

void user_post_event(void)
{

    int res = 0;
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();
    char *event_id = "Error";
    char *event_payload = "NULL";


    event_payload = "{\"ErrorCode\":0}";


    res = IOT_Linkkit_TriggerEvent(user_example_ctx->master_devid, event_id, strlen(event_id),
                                   event_payload, strlen(event_payload));
    EXAMPLE_TRACE("Post Event Message ID: %d", res);
}


void user_deviceinfo_update(void)
{
    int res = 0;
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();
    char *device_info_update = "[{\"attrKey\":\"abc\",\"attrValue\":\"hello,world\"}]";

    res = IOT_Linkkit_Report(user_example_ctx->master_devid, ITM_MSG_DEVICEINFO_UPDATE,
                             (unsigned char *)device_info_update, strlen(device_info_update));
    EXAMPLE_TRACE("Device Info Update Message ID: %d", res);
}

void user_deviceinfo_delete(void)
{
    int res = 0;
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();
    char *device_info_delete = "[{\"attrKey\":\"abc\"}]";

    res = IOT_Linkkit_Report(user_example_ctx->master_devid, ITM_MSG_DEVICEINFO_DELETE,
                             (unsigned char *)device_info_delete, strlen(device_info_delete));
    EXAMPLE_TRACE("Device Info Delete Message ID: %d", res);
}

void user_post_raw_data(void)
{
    int res = 0;
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();
    unsigned char raw_data[7] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};

    res = IOT_Linkkit_Report(user_example_ctx->master_devid, ITM_MSG_POST_RAW_DATA,
                             raw_data, 7);
    EXAMPLE_TRACE("Post Raw Data Message ID: %d", res);
}


int ali_linkkit_init(user_example_ctx_t *user_example_ctx)
{

    iotx_linkkit_dev_meta_info_t    master_meta_info;

    HAL_GetProductKey(DEMO_PRODUCT_KEY);
    HAL_GetProductSecret(DEMO_PRODUCT_SECRET);
    HAL_GetDeviceName(DEMO_DEVICE_NAME);
    HAL_GetDeviceSecret(DEMO_DEVICE_SECRET);
    memset(&master_meta_info, 0, sizeof(iotx_linkkit_dev_meta_info_t));
    memcpy(master_meta_info.product_key, DEMO_PRODUCT_KEY, strlen(DEMO_PRODUCT_KEY));
    memcpy(master_meta_info.product_secret, DEMO_PRODUCT_SECRET, strlen(DEMO_PRODUCT_SECRET));
    memcpy(master_meta_info.device_name, DEMO_DEVICE_NAME, strlen(DEMO_DEVICE_NAME));
    memcpy(master_meta_info.device_secret, DEMO_DEVICE_SECRET, strlen(DEMO_DEVICE_SECRET));

    IOT_RegisterCallback(ITE_CONNECT_SUCC, user_connected_event_handler);
    IOT_RegisterCallback(ITE_DISCONNECTED, user_disconnected_event_handler);
    IOT_RegisterCallback(ITE_RAWDATA_ARRIVED, user_down_raw_data_arrived_event_handler);
    IOT_RegisterCallback(ITE_SERVICE_REQUEST, user_service_request_event_handler);
    IOT_RegisterCallback(ITE_PROPERTY_SET, user_property_set_event_handler);
    IOT_RegisterCallback(ITE_PROPERTY_GET, user_property_get_event_handler);
    IOT_RegisterCallback(ITE_REPORT_REPLY, user_report_reply_event_handler);
    IOT_RegisterCallback(ITE_TRIGGER_EVENT_REPLY, user_trigger_event_reply_event_handler);
    IOT_RegisterCallback(ITE_TIMESTAMP_REPLY, user_timestamp_reply_event_handler);
    IOT_RegisterCallback(ITE_INITIALIZE_COMPLETED, user_initialized);
    IOT_RegisterCallback(ITE_FOTA, user_fota_event_handler);


    /* Choose Login Server, domain should be configured before IOT_Linkkit_Open() */
//#if USE_CUSTOME_DOMAIN
//    IOT_Ioctl(IOTX_IOCTL_SET_MQTT_DOMAIN, (void *)CUSTOME_DOMAIN_MQTT);
//    IOT_Ioctl(IOTX_IOCTL_SET_HTTP_DOMAIN, (void *)CUSTOME_DOMAIN_HTTP);
//#else
//#ifdef WORLDWIDE_USE
//    int domain_type = IOTX_CLOUD_REGION_SINGAPORE;
//#else
//    int domain_type = IOTX_CLOUD_REGION_SHANGHAI;
//#endif    
//    
//    IOT_Ioctl(IOTX_IOCTL_SET_DOMAIN, (void *)&domain_type);
//#endif

    /* Choose Login Method */
    int dynamic_register = 0;
    IOT_Ioctl(IOTX_IOCTL_SET_DYNAMIC_REGISTER, (void *)&dynamic_register);

    /* Choose Whether You Need Post Property/Event Reply */
    int post_event_reply = 1;
    IOT_Ioctl(IOTX_IOCTL_RECV_EVENT_REPLY, (void *)&post_event_reply);



    // Create Master Device Resources
    user_example_ctx->master_devid = IOT_Linkkit_Open(IOTX_LINKKIT_DEV_TYPE_MASTER, &master_meta_info);
    if (user_example_ctx->master_devid < 0)
    {
        printf("IOT_Linkkit_Open Failed\n");
        return -1;
    }
    return 0;
}

void IOT_Linkkit_Tx()
{
    IoT_Properity_t IoT_Properity;

    if (IOT_RB_DATA_OK != IoT_Ring_Buffer_CheckEmpty())
    {
        #ifdef ALI_POST_CTRL
        if(g_tPrevPostInfo.u8Used)
        {
            goto done;
        }
        #endif

        IoT_Ring_Buffer_Pop(&IoT_Properity);
        user_post_property(&IoT_Properity);

        #ifdef ALI_POST_CTRL
        #else
        IoT_Ring_Buffer_ReadIdxUpdate();
        #endif
    }

#ifdef ALI_POST_CTRL
done:
    return;
#endif
}

#ifdef ALI_TIMESTAMP
void user_timestamp_query(void)
{
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();

    IOT_Linkkit_Query(user_example_ctx->master_devid, ITM_MSG_QUERY_TIMESTAMP, NULL, 0);
}
#endif

