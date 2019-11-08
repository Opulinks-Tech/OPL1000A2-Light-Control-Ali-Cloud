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


#include "ali_linkkitsdk_decl.h"
#include "blewifi_configuration.h"
#include "hal_vic.h"
#include "blewifi_ctrl.h"

#include "light_control.h"
#include "iot_rb_data.h"
#include "blewifi_ctrl_http_ota.h"

char DEMO_PRODUCT_KEY[IOTX_PRODUCT_KEY_LEN + 1] = {0};
char DEMO_DEVICE_NAME[IOTX_DEVICE_NAME_LEN + 1] = {0};
char DEMO_DEVICE_SECRET[IOTX_DEVICE_SECRET_LEN + 1] = {0};
char DEMO_PRODUCT_SECRET[IOTX_PRODUCT_SECRET_LEN + 1] = {0};


static user_example_ctx_t g_user_example_ctx;
extern T_MwFim_GP13_Dev_Sched g_taDevSched[MW_FIM_GP13_DEV_SCHED_NUM];
uint8_t g_light_reboot_flag = 0;

user_example_ctx_t *user_example_get_ctx(void)
{
    return &g_user_example_ctx;
}

void iot_update_light_property(uint8_t property_type)
{
    char *property_payload = NULL;
    uint32_t u32BufSize = PROPERTY_THE_OTHER_PAYLOAD_LEN;
    uint32_t u32Offset = 0;
    uint8_t i = 0, j = 0, k = 0;
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();

    if(property_type == PROPERTY_LOCALTIMER)
    {
        u32BufSize = PROPERTY_LOCALTIMER_PAYLOAD_LEN;
    }

    property_payload = (char *)HAL_Malloc(u32BufSize);

    if(!property_payload)
    {
        printf("Malloc property payload fail\n");
        return;
    }

    memset(property_payload,0,sizeof(char)*u32BufSize);

    char *HSVColor_update = "{\"HSVColor\":{\"Saturation\":%d,\"Value\":%d,\"Hue\":%d}}";
    switch(property_type)
    {
        case PROPERTY_LIGHT_SWITCH:
             sprintf( property_payload, "{\"LightSwitch\":%d}", light_ctrl_get_switch());
             break;
        case PROPERTY_COLORTEMPERATURE:
             sprintf( property_payload, "{\"ColorTemperature\":%d}", light_ctrl_get_color_temperature());
             break;
        case PROPERTY_BRIGHTNESS:
             sprintf( property_payload, "{\"Brightness\":%d}", light_ctrl_get_brightness());
             break;
        case PROPERTY_WORKMODE:
             sprintf( property_payload, "{\"WorkMode\":%d}", light_ctrl_get_workmode());
             break;
        case PROPERTY_LIGHTMODE:
             sprintf( property_payload, "{\"LightMode\":%d}", light_ctrl_get_mode());
             break;
        case PROPERTY_COLORSPEED:
             sprintf( property_payload, "{\"ColorSpeed\":%d}", light_effect_get_speed());
             break;
        case PROPERTY_LIGHTTYPE:
             sprintf( property_payload, "{\"LightType\":%d}", light_ctrl_get_light_type());
             break;
        case PROPERTY_COLORARR:
             break;
        case PROPERTY_HSVCOLOR:
        case PROPERTY_RGBCOLOR:
             sprintf( property_payload, HSVColor_update, light_ctrl_get_saturation(), light_ctrl_get_value(), light_ctrl_get_hue());
             break;
        case PROPERTY_LOCALTIMER:
             u32Offset = snprintf(property_payload, u32BufSize, "{\"LocalTimer\":[");

             for(i = 0; i < MW_FIM_GP13_DEV_SCHED_NUM; i++)
             {
                 char s8aRepeatBuf[LOCALTIMER_REPEAT_BUF_LEN] = {0};
                 uint32_t u8RepeatBufSize = LOCALTIMER_REPEAT_BUF_LEN;
                 uint32_t u32RepeatOffset = 0;

                 if(i)
                 {
                     u32Offset += snprintf(property_payload + u32Offset, u32BufSize - u32Offset, ",");
                 }

                 k = 0;

                 for(j = 1; j < 8; j++)
                 {
                     if(g_taDevSched[i].u8RepeatMask & (1 << j))
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

                 u32Offset += snprintf(property_payload + u32Offset, u32BufSize - u32Offset, "{\"LightSwitch\":%u,\"Timer\":\"%u %u * * %s\",\"Enable\":%u,\"IsValid\":%u}"
                                       , g_taDevSched[i].u8DevOn
                                       , g_taDevSched[i].u8Min
                                       , g_taDevSched[i].u8Hour
                                       , s8aRepeatBuf
                                       , g_taDevSched[i].u8Enable
                                       , g_taDevSched[i].u8IsValid
                                       );
               }

             u32Offset += snprintf(property_payload + u32Offset, u32BufSize - u32Offset, "]}");
             break;
        default:
             printf("Do Not Support this property\n");
             break;
    }
    IOT_Linkkit_Report(user_example_ctx->master_devid, ITM_MSG_POST_PROPERTY,
                      (unsigned char *)property_payload, strlen(property_payload));
    HAL_Free(property_payload);
}

static int user_connected_event_handler(void)
{
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();

    EXAMPLE_TRACE("Cloud Connected");
    user_example_ctx->cloud_connected = 1;
    if(g_light_reboot_flag)
    {
        iot_update_light_property(PROPERTY_LIGHTTYPE);
        iot_update_light_property(PROPERTY_HSVCOLOR);
        iot_update_light_property(PROPERTY_LIGHTMODE);
        osDelay(300);
        iot_update_light_property(PROPERTY_LIGHT_SWITCH);
        g_light_reboot_flag = 0;
    }
#if defined(OTA_ENABLED) && defined(BUILD_AOS)
    ota_service_init(NULL);
#endif

    return 0;
}

static int user_disconnected_event_handler(void)
{
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();

    EXAMPLE_TRACE("Cloud Disconnected");

    user_example_ctx->cloud_connected = 0;

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

    EXAMPLE_TRACE("Service Request Received, Devid: %d, Service ID: %.*s, Payload: %s", devid, serviceid_len,
                  serviceid,
                  request);

    return 0;
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

static int user_property_set_event_handler(const int devid, const char *request, const int request_len)
{
    cJSON *request_root = NULL, *item_LightSwitch = NULL;

    cJSON *item_localtimer = NULL;
    cJSON *sub_item = NULL, *item_timer = NULL, *item_enable = NULL, *item_isvalid = NULL, *subitem_lightswitch = NULL;

    cJSON *item_colortemperature = NULL;

    cJSON *item_brightness = NULL;

    cJSON *item_HSVColor = NULL;
    cJSON *item_saturation = NULL, *item_value = NULL, *item_hue = NULL;

    cJSON *item_workmode = NULL;

    cJSON *item_lightmode = NULL;

    cJSON *item_colorspeed = NULL;

    cJSON *item_lighttype = NULL;

    cJSON *item_colorarr = NULL;
    cJSON *CASubitem_saturation = NULL, *CASubitem_hue = NULL, *CASubitem_value = NULL, *CASubitem_enable = NULL;

    cJSON *item_RgbColor = NULL;
    cJSON *item_red = NULL, *item_green = NULL, *item_blue = NULL;
    uint32_t brightness_colortemp_tmp=0;

    user_example_ctx_t *user_example_ctx = user_example_get_ctx();
    printf("Property Set Received, Devid: %d, Request: %s\r\n", devid, request);

    /* stop effect operation triggered by APP*/
    light_effect_stop(NULL);

    /* Parse Request */
    request_root = cJSON_Parse(request);
    if (request_root == NULL || !cJSON_IsObject(request_root)) {
        EXAMPLE_TRACE("JSON Parse Error");
        return -1;
    }

    item_localtimer = cJSON_GetObjectItem(request_root, "LocalTimer");
    /* Try To Find LightSwitch Property */		//LightSwitch   Value:<int>,0,1
    item_LightSwitch = cJSON_GetObjectItem(request_root, "LightSwitch");

    if((light_ctrl_get_switch()==0)&& item_localtimer==NULL && item_LightSwitch==NULL)
    {
        goto DONE;
    }

    if (item_LightSwitch != NULL) {
        EXAMPLE_TRACE("LightSwitch Enable : %d\r\n", item_LightSwitch->valueint);

        light_ctrl_set_switch(item_LightSwitch->valueint);
        if (item_LightSwitch->valueint && light_effect_get_status())
            light_effect_start(NULL, 0);
        iot_update_light_property(PROPERTY_LIGHT_SWITCH);
    }

    /* Try To Find ColorTemperature Property */		//ColorTemperature   Value:<int> 2000~7000
    item_colortemperature = cJSON_GetObjectItem(request_root, "ColorTemperature");
    if (item_colortemperature != NULL) {
        EXAMPLE_TRACE("ColorTemperature: %d\r\n", item_colortemperature->valueint);
        light_effect_set_status(0);

        if(item_colortemperature->valueint < 2000 || item_colortemperature->valueint > 7000)
        {
            printf("[ColorTemperature],Invalid Value:%d\n",item_colortemperature->valueint);
        }
        else
        {
            brightness_colortemp_tmp = light_ctrl_get_brightness();
            if(brightness_colortemp_tmp == 1)
            {
                if(item_colortemperature->valueint > 4500)
                {
                    light_ctrl_set_ctb(7000, 1, LIGHT_FADE_ON);
                }
                else
                {
                    light_ctrl_set_ctb(2000,1, LIGHT_FADE_ON);
                }
            }
            else
            {
                light_ctrl_set_color_temperature(item_colortemperature->valueint);
            }
            colortemperature_force_overwrite(item_colortemperature->valueint);
        }
        iot_update_light_property(PROPERTY_COLORTEMPERATURE);
        iot_update_light_property(PROPERTY_LIGHTMODE);
    }


    /* Try To Find Brightness Property */		//Brightness				Value:<int> 0~100
    item_brightness = cJSON_GetObjectItem(request_root, "Brightness");
    if (item_brightness != NULL) {
        EXAMPLE_TRACE("Brightness: %d\r\n", item_brightness->valueint);
        light_effect_set_status(0);

        if(item_brightness->valueint>100)
        {
            printf("[Brightness],Invalid Value:%d\n",item_brightness->valueint);
        }
        else
        {
            brightness_colortemp_tmp = light_ctrl_get_color_temperature();
            if(item_brightness->valueint == 1)
            {
                if(brightness_colortemp_tmp > 4500)
                {
                    light_ctrl_set_ctb(7000, 1, LIGHT_FADE_ON);
                }
                else
                {
                    light_ctrl_set_ctb(2000, 1, LIGHT_FADE_ON);
                }
            }
            else
            {
                light_ctrl_set_brightness(item_brightness->valueint);
            }
            colortemperature_force_overwrite(brightness_colortemp_tmp);
        }
        iot_update_light_property(PROPERTY_BRIGHTNESS);
        iot_update_light_property(PROPERTY_LIGHTMODE);
    }

    /* Try To Find WorkMode Property */	//WorkMode   Value:<enum> 0~5
    item_workmode = cJSON_GetObjectItem(request_root, "WorkMode");
    if (item_workmode != NULL) {
        EXAMPLE_TRACE("WorkMode: %d\r\n", item_workmode->valueint);
        light_effect_set_status(0);

        if(item_workmode->valueint==WMODE_MANUAL)
        {
            uint16_t hue = 0;
            uint8_t saturation = 0;
            uint8_t value = 0;
            light_ctrl_set_workmode(WMODE_MANUAL);
            light_ctrl_get_manual_light_status(&hue,&saturation,&value);
            light_ctrl_set_hsv(hue, saturation, value, LIGHT_FADE_ON);
        }
        else if(item_workmode->valueint==WMODE_READING)
        {
            light_ctrl_set_workmode(WMODE_READING);
            light_ctrl_set_rgb(255,250,247);
        }
        else if(item_workmode->valueint==WMODE_CINEMA)
        {
            light_ctrl_set_workmode(WMODE_CINEMA);
            light_ctrl_set_rgb(206,56,0);
        }
        else if(item_workmode->valueint==WMODE_MIDNIGHT)
        {
            light_ctrl_set_workmode(WMODE_MIDNIGHT);
            light_ctrl_set_rgb(189,117,0);
        }
        else if(item_workmode->valueint==WMODE_LIVING)
        {
            light_ctrl_set_workmode(WMODE_LIVING);
            light_ctrl_set_rgb(53,84,181);
        }
        else if(item_workmode->valueint==WMODE_SOFT)
        {
            light_ctrl_set_workmode(WMODE_SOFT);
            light_ctrl_set_rgb(254,157,154);
        }
        else if (item_workmode->valueint==WMODE_STREAMER)
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
        iot_update_light_property(PROPERTY_HSVCOLOR);
        iot_update_light_property(PROPERTY_WORKMODE);
        iot_update_light_property(PROPERTY_LIGHTMODE);
    }

    /* Try To Find LightMode Property */
    item_lightmode = cJSON_GetObjectItem(request_root, "LightMode");
    if (item_lightmode != NULL) {
    }

    /* Try To Find ColorSpeed Property */
    item_colorspeed = cJSON_GetObjectItem(request_root, "ColorSpeed");
    if (item_colorspeed != NULL) {
        EXAMPLE_TRACE("ColorSpeed: %d\r\n", item_colorspeed->valueint);
        light_effect_set_speed(item_colorspeed->valueint);
    }

    /* Try To Find LightType Property */
    item_lighttype = cJSON_GetObjectItem(request_root, "LightType");
    if (item_lighttype != NULL) {
    }

    /* Try To Find ColorArr Property */
    item_colorarr = cJSON_GetObjectItem(request_root, "ColorArr");
    if (item_colorarr != NULL && cJSON_IsArray(item_colorarr)) {
        EXAMPLE_TRACE("ColorArr Size: %d", cJSON_GetArraySize(item_colorarr));
        light_effect_set_status(1);
        int index = 0;
        int color_num = 0;
        hsv_t color_arr[6] = {0};

        for (index = 0; index < cJSON_GetArraySize(item_colorarr); index++) {
            sub_item = cJSON_GetArrayItem(item_colorarr, index);
            if (sub_item != NULL && cJSON_IsObject(sub_item)) {
                CASubitem_hue        = cJSON_GetObjectItem(sub_item, "Hue");
                CASubitem_saturation = cJSON_GetObjectItem(sub_item, "Saturation");
                CASubitem_value      = cJSON_GetObjectItem(sub_item, "Value");
                CASubitem_enable     = cJSON_GetObjectItem(sub_item, "Enable");
                if (CASubitem_saturation != NULL && CASubitem_hue != NULL && CASubitem_value != NULL && CASubitem_enable != NULL) {
                    EXAMPLE_TRACE("Hue           : %d", CASubitem_hue->valueint);
                    EXAMPLE_TRACE("Saturation    : %d", CASubitem_saturation->valueint);
                    EXAMPLE_TRACE("Value         : %d", CASubitem_value->valueint);
                    EXAMPLE_TRACE("Enable        : %d", CASubitem_enable->valueint);
                    if (CASubitem_enable->valueint == 1)
                    {
                        color_arr[index].hue = CASubitem_hue->valueint;
                        color_arr[index].saturation = CASubitem_saturation->valueint;
                        color_arr[index].value = CASubitem_value->valueint;
                        color_num = index + 1;
                    }
                }
            }
        }
        light_ctrl_set_mode(MODE_HSV);
        light_effect_set_effect_type(EFFTCT_TYPE_FLASH);
        light_effect_set_color_array(color_arr, color_num);
        light_effect_start(NULL, 0);
    }


    /* Try To Find HSVColor Property */  //HSVColor   Value:[child1] Hue  Value:<double>0~360   [child2] Saturation  Value:<double>0~100   [child3] Value  Value:<double>0~100
    item_HSVColor = cJSON_GetObjectItem(request_root, "HSVColor");
    if (item_HSVColor != NULL) {
        light_effect_set_status(0);
        item_saturation = cJSON_GetObjectItem(item_HSVColor, "Saturation");
        item_hue = cJSON_GetObjectItem(item_HSVColor, "Hue");
        item_value = cJSON_GetObjectItem(item_HSVColor, "Value");
        EXAMPLE_TRACE("Saturation: %d", item_saturation->valueint);
        EXAMPLE_TRACE("Value     : %d", item_value->valueint);
        EXAMPLE_TRACE("Hue       : %d", item_hue->valueint);

        light_ctrl_set_hsv(item_hue->valueint,item_saturation->valueint,item_value->valueint, LIGHT_FADE_ON);
        light_ctrl_set_manual_light_status(item_hue->valueint,item_saturation->valueint,item_value->valueint);
        iot_update_light_property(PROPERTY_HSVCOLOR);
        iot_update_light_property(PROPERTY_LIGHTMODE);
        //read rgb value and update
    }

    /* Try To Find RGBColor Property */		//RGBColor   Value:[child1] Red  Value:<int>0~255  [child2] Blue  Value:<int>0~255   [child3] Green  Value:<int>0~255
    item_RgbColor = cJSON_GetObjectItem(request_root, "RGBColor");
    if (item_RgbColor != NULL) {
        light_effect_set_status(0);
        item_red = cJSON_GetObjectItem(item_RgbColor, "Red");
        item_green = cJSON_GetObjectItem(item_RgbColor, "Green");
        item_blue = cJSON_GetObjectItem(item_RgbColor, "Blue");
        EXAMPLE_TRACE("Red  : %d", item_red->valueint);
        EXAMPLE_TRACE("Green: %d", item_green->valueint);
        EXAMPLE_TRACE("Blue : %d", item_blue->valueint);

        light_ctrl_set_rgb(item_red->valueint ,item_green->valueint,item_blue->valueint);
        iot_update_light_property(PROPERTY_RGBCOLOR);
        iot_update_light_property(PROPERTY_LIGHTMODE);
    }


     /* Try To Find LocalTimer Property */
    EXAMPLE_TRACE("Local Timer Size: %d", cJSON_GetArraySize(item_localtimer));
    if (item_localtimer != NULL && cJSON_IsArray(item_localtimer)) {
        if (light_ctrl_get_switch() && light_effect_get_status())
            light_effect_start(NULL, 0);
        int index = 0;
        int iArraySize = cJSON_GetArraySize(item_localtimer);
        T_BleWifi_Ctrl_DevSchedAll tSchedAll = {0};

        tSchedAll.u8Num = (uint8_t)iArraySize;

        for (index = 0; index < iArraySize; index++) {
            sub_item = cJSON_GetArrayItem(item_localtimer, index);
            if (sub_item != NULL && cJSON_IsObject(sub_item)) {
                item_timer          = NULL;
                item_enable         = NULL;
                item_isvalid        = NULL;
                subitem_lightswitch = NULL;
                subitem_lightswitch = cJSON_GetObjectItem(sub_item, "LightSwitch");
                item_timer          = cJSON_GetObjectItem(sub_item, "Timer");
                item_enable         = cJSON_GetObjectItem(sub_item, "Enable");
                item_isvalid        = cJSON_GetObjectItem(sub_item, "IsValid");
                if (item_timer != NULL && item_enable != NULL && item_isvalid != NULL && subitem_lightswitch != NULL) {
                    EXAMPLE_TRACE("Local Timer Index: %d", index);
                    EXAMPLE_TRACE("LightSwitch      : %d", subitem_lightswitch->valueint);
                    EXAMPLE_TRACE("Timer            : %s", item_timer->valuestring);
                    EXAMPLE_TRACE("Enable           : %d", item_enable->valueint);
                    EXAMPLE_TRACE("IsValid          : %d", item_isvalid->valueint);

                    tSchedAll.taSched[index].u8Enable = item_enable->valueint;
                    tSchedAll.taSched[index].u8IsValid = item_isvalid->valueint;

                    tSchedAll.taSched[index].u8DevOn = subitem_lightswitch->valueint;

                    if(dev_sched_time_set(item_timer->valuestring, &(tSchedAll.taSched[index])))
                    {
                        EXAMPLE_TRACE("dev_sched_time_set fail\r\n");

                        tSchedAll.taSched[index].u8Enable = 0;
                    }
                }
            }
        }

        if(BleWifi_Ctrl_DevSchedSetAll(&tSchedAll))
        {
            EXAMPLE_TRACE("BleWifi_Ctrl_DevSchedSetAll fail\r\n");
        }
        int res = IOT_Linkkit_Report(user_example_ctx->master_devid, ITM_MSG_POST_PROPERTY,
                             (unsigned char *)request, request_len);
    }

    DONE:
    cJSON_Delete(request_root);

    return 0;
}

static int user_property_get_event_handler(const int devid, const char *request, const int request_len, char **response,
        int *response_len)
{


    EXAMPLE_TRACE("Property Get Received, Devid: %d, Request: %s", devid, request);
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

void user_post_property(IoT_Properity_t *ptProp)
{

}

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


    /* Choose Login Server, domain should be configured before IOT_Linkkit_Open() */
#if USE_CUSTOME_DOMAIN
    IOT_Ioctl(IOTX_IOCTL_SET_MQTT_DOMAIN, (void *)CUSTOME_DOMAIN_MQTT);
    IOT_Ioctl(IOTX_IOCTL_SET_HTTP_DOMAIN, (void *)CUSTOME_DOMAIN_HTTP);
#else
    int domain_type = IOTX_CLOUD_REGION_SHANGHAI;
    IOT_Ioctl(IOTX_IOCTL_SET_DOMAIN, (void *)&domain_type);
#endif

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
        IoT_Ring_Buffer_Pop(&IoT_Properity);
        user_post_property(&IoT_Properity);
        IoT_Ring_Buffer_ReadIdxUpdate();
    }
}
