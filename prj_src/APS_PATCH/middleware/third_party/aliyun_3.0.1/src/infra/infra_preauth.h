/*
 * Copyright (C) 2015-2018 Alibaba Group Holding Limited
 */

#ifndef __INFRA_PREAUTH__
#define __INFRA_PREAUTH__

#include "infra_defs.h"
#include "infra_types.h"

#ifdef INFRA_LOG
    #include "infra_log.h"
    #define preauth_err(...)   log_err("preauth", __VA_ARGS__)
    #define preauth_info(...)  log_info("preauth", __VA_ARGS__)
    #define preauth_debug(...) log_debug("preauth", __VA_ARGS__)
#else
    #define preauth_err(...)
    #define preauth_info(...)
    #define preauth_debug(...)
#endif


int preauth_get_connection_info(iotx_mqtt_region_types_t region, iotx_dev_meta_info_t *dev_meta,
                                const char *sign, const char *device_id, iotx_sign_mqtt_t *preauth_output);

#define SYS_GUIDER_MALLOC(size) LITE_malloc(size, MEM_MAGIC, "sys.guider")
#define HOST_ADDRESS_LEN    (128)    
#define GUIDER_IOT_ID_LEN           (128)
#define GUIDER_IOT_TOKEN_LEN        (64) 
int iotx_redirect_region_subscribe(void);
int iotx_redirect_set_flag(int flag);
int iotx_redirect_get_flag(void);
int iotx_redirect_get_iotId_iotToken(char *iot_id,char *iot_token,char *host,uint16_t *pport);
#endif /* #ifndef __INFRA_PREAUTH__ */