#ifndef _INFRA_CONFIG_H_
#define _INFRA_CONFIG_H_

#ifndef GCC
typedef unsigned int uint32_t;
typedef unsigned char uint8_t;
#endif
#define PLATFORM_HAS_STDINT
#define PLATFORM_HAS_DYNMEM
#define PLATFORM_HAS_OS
#define INFRA_STRING
#define INFRA_NET
#define INFRA_LIST
//#define INFRA_LOG_NETWORK_PAYLOAD
#define INFRA_LOG
#define INFRA_LOG_ALL_MUTED
#define INFRA_LOG_MUTE_FLW
#define INFRA_LOG_MUTE_DBG
#define INFRA_LOG_MUTE_INF
#define INFRA_LOG_MUTE_WRN
#define INFRA_LOG_MUTE_ERR
#define INFRA_LOG_MUTE_CRT
#define INFRA_TIMER
#define INFRA_JSON_PARSER
#define INFRA_CJSON
#define INFRA_MD5_ALT
#define INFRA_MD5
#define INFRA_SHA1
#define INFRA_SHA1_ALT
#define INFRA_SHA256
#define INFRA_SHA256_ALT
#define INFRA_REPORT
#define INFRA_HTTPC
#define INFRA_COMPAT
#define INFRA_CLASSIC

#ifdef WORLDWIDE_USE
#define INFRA_PREAUTH
#endif

#define INFRA_AES
#define DEV_SIGN
#define MQTT_COMM_ENABLED
#define MQTT_DEFAULT_IMPL

#ifdef WORLDWIDE_USE
#define MQTT_PRE_AUTH
#else
#define MQTT_DIRECT
#endif

#define DEVICE_MODEL_CLASSIC
#define LOG_REPORT_TO_CLOUD
#define DEVICE_MODEL_ENABLED
#define ALCS_ENABLED
#define HAL_KV

#define OTA_ENABLED
//#if defined(WORLDWIDE_USE) || defined(OTA_ENABLED)
#define SUPPORT_TLS
//#endif

#define HAL_CRYPTO
#define HAL_UDP

#define COAP_PACKET
#define COAP_SERVER
#define DEV_RESET
#define DEV_BIND_ENABLED
#define WIFI_PROVISION_ENABLED
#define AWSS_DISABLE_REGISTRAR


// modified for ali_lib by Jeff, 20190525
// redefine the HAL_Printf function
#if 1   // the modified
#define HAL_Printf  printf
#else   // the original
#endif

#define ALI_SINGLE_TASK
#define ALI_TASK_POLLING_PERIOD     (10) // ms

#define COAP_ENABLE
#define ALI_RHYTHM_SUPPORT

#ifdef ALI_RHYTHM_SUPPORT
#ifndef COAP_ENABLE
#error "No Enable CoAP"
#endif
#endif

#define ALI_APP_TOKEN_BACKUP
#define ALI_KEEPALIVE_INTERVAL      (10000) // ms

#define ALI_REPORT_TOKEN_AFTER_UNBIND

#ifdef WORLDWIDE_USE
//#define ALI_UNBIND_REFINE
#endif

#ifdef SUPPORT_TLS
#define ALI_HTTP_COMPATIBLE
#endif

#define timer_dbg           printf
//#define timer_dbg(...)

#define SHM_DATA    __attribute__((section("SHM_REGION")))

#endif
