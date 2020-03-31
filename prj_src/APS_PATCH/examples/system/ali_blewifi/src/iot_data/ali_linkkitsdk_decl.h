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

#ifndef __ALI_LINKKITSDK_DECL_H__
#define __ALI_LINKKITSDK_DECL_H__

#include <stdlib.h>
#include <string.h>
#include "cmsis_os.h"
#include "sys_os_config.h"


#include "dev_sign_api.h"
#include "mqtt_api.h"
#include "dev_model_api.h"
#include "dm_wrapper.h"

#include "kvmgr.h"
#include "kv.h"
#include "awss_notify.h"
#include "cJSON.h"

#include "ali_hal_decl.h"
#include "iot_rb_data.h"
#include "light_control.h"

// 15: {"LocalTimer":[
// 99: {"LightSwitch":0,"Timer":"53 14 * * 1,2,3,4,5,6,7","Enable":1,"IsValid":1,"TimezoneOffset":-43200},
//  3: ]}\0
#define PROPERTY_LOCALTIMER_PAYLOAD_LEN    1024 // >= (15 + (99 * MW_FIM_GP13_DEV_SCHED_NUM) + 3)
//#define PROPERTY_THE_OTHER_PAYLOAD_LEN     512
#define LOCALTIMER_REPEAT_BUF_LEN          16


void *HAL_Malloc(uint32_t size);
void HAL_Free(void *ptr);
uint64_t HAL_UptimeMs(void);
int HAL_Snprintf(char *str, const int len, const char *fmt, ...);

enum light_property{
    PROPERTY_LIGHT_SWITCH = 0,
    PROPERTY_COLORTEMPERATURE,
    PROPERTY_BRIGHTNESS,
    PROPERTY_WORKMODE,
    PROPERTY_LIGHTMODE,
    PROPERTY_COLORSPEED,
    PROPERTY_LIGHTTYPE,
    PROPERTY_COLORARR,
    PROPERTY_HSVCOLOR,
    PROPERTY_RGBCOLOR,
    PROPERTY_SCENESCOLOR,
    PROPERTY_LOCALTIMER,

    PROPERTY_INIT,
    PROPERTY_MAX = PROPERTY_INIT
};


typedef struct {
    int master_devid;
    int cloud_connected;
    int master_initialized;
} user_example_ctx_t;

#if USE_CUSTOME_DOMAIN
    #define CUSTOME_DOMAIN_MQTT     "iot-as-mqtt.cn-shanghai.aliyuncs.com"
    #define CUSTOME_DOMAIN_HTTP     "iot-auth.cn-shanghai.aliyuncs.com"
#endif

#define USER_EXAMPLE_YIELD_TIMEOUT_MS (200) //Kevin add it

#define EXAMPLE_TRACE(...)                               \
do {                                                     \
    HAL_Printf("\033[1;32;40m%s.%d: ", __func__, __LINE__);  \
    HAL_Printf(__VA_ARGS__);                                 \
    HAL_Printf("\033[0m\r\n");                                   \
} while (0)



int ali_linkkit_init(user_example_ctx_t *user_example_ctx);

void IOT_Linkkit_Tx(void);

void iot_update_local_timer(uint8_t u8LightSwitch);
void iot_set_scenes_color(uint8_t u8WorkMode);
void iot_set_color_array(hsv_t *taColorArr, uint16_t u16ColorNum, uint8_t u8Apply);
int iot_load_cfg(uint8_t u8Mode);
int iot_update_cfg(void);
int iot_apply_cfg(uint8_t u8Mode);
void iot_save_all_cfg(void);

#endif
