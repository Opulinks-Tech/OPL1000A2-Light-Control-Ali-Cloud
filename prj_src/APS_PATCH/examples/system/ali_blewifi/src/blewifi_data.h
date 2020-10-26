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
 * @file blewifi_data.h
 * @author Michael Liao
 * @date 20 Feb 2018
 * @brief File includes the function declaration of blewifi task.
 *
 */

#ifndef __BLEWIFI_DATA_H__
#define __BLEWIFI_DATA_H__

#include <stdint.h>
#include <stdbool.h>

#define DELIMITER_NUM (4)  // The total count of ',' in colud info data.
#define U32_CONVERT_TO_CHAR_MAX_SIZE (10)  //"4,294,967,295"
#define RSP_CLOUD_INFO_READ_PAYLOAD_SIZE (U32_CONVERT_TO_CHAR_MAX_SIZE + IOTX_PRODUCT_KEY_LEN + IOTX_PRODUCT_SECRET_LEN + IOTX_DEVICE_NAME_LEN + IOTX_DEVICE_SECRET_LEN + DELIMITER_NUM + 1)

enum
{
	BLEWIFI_OTA_SUCCESS,
	BLEWIFI_OTA_ERR_NOT_ACTIVE,
	BLEWIFI_OTA_ERR_HW_FAILURE,
	BLEWIFI_OTA_ERR_IN_PROGRESS,
	BLEWIFI_OTA_ERR_INVALID_LEN,
	BLEWIFI_OTA_ERR_CHECKSUM,
	BLEWIFI_OTA_ERR_MEM_CAPACITY_EXCEED,


};

typedef enum {
    BLEWIFI_REQ_SCAN                            = 0x0,          // Wifi scan
    BLEWIFI_REQ_CONNECT                         = 0x1,          // Wifi connect
    BLEWIFI_REQ_DISCONNECT                      = 0x2,          // Wifi disconnect
    BLEWIFI_REQ_RECONNECT                       = 0x3,          // Wifi reconnect
    BLEWIFI_REQ_READ_DEVICE_INFO                = 0x4,          // Wifi read device information
    BLEWIFI_REQ_WRITE_DEVICE_INFO               = 0x5,          // Wifi write device information
    BLEWIFI_REQ_WIFI_STATUS                     = 0x6,          // Wifi read AP status
    BLEWIFI_REQ_RESET                           = 0x7,          // Wifi reset AP
	BLEWIFI_REQ_MANUAL_CONNECT_AP               = 0x8,          // Wifi connect AP by manual

    BLEWIFI_REQ_OTA_VERSION                     = 0x100,        // Ble OTA
    BLEWIFI_REQ_OTA_UPGRADE                     = 0x101,        // Ble OTA
    BLEWIFI_REQ_OTA_RAW                         = 0x102,        // Ble OTA
    BLEWIFI_REQ_OTA_END                         = 0x103,        // Ble OTA

    BLEWIFI_REQ_HTTP_OTA_TRIG                   = 0x200,        // Wifi OTA
    BLEWIFI_REQ_HTTP_OTA_DEVICE_VERSION         = 0x201,        // Wifi OTA
    BLEWIFI_REQ_HTTP_OTA_SERVER_VERSION         = 0x202,        // Wifi OTA

    BLEWIFI_REQ_MP_START                        = 0x400,
    BLEWIFI_REQ_MP_CAL_VBAT                     = 0x401,
    BLEWIFI_REQ_MP_CAL_IO_VOLTAGE               = 0x402,
    BLEWIFI_REQ_MP_CAL_TMPR                     = 0x403,
    BLEWIFI_REQ_MP_SYS_MODE_WRITE               = 0x404,
    BLEWIFI_REQ_MP_SYS_MODE_READ                = 0x405,

    BLEWIFI_REQ_ENG_START                       = 0x600,
    BLEWIFI_REQ_ENG_SYS_RESET                   = 0x601,
    BLEWIFI_REQ_ENG_WIFI_MAC_WRITE              = 0x602,
    BLEWIFI_REQ_ENG_WIFI_MAC_READ               = 0x603,
    BLEWIFI_REQ_ENG_BLE_MAC_WRITE               = 0x604,
    BLEWIFI_REQ_ENG_BLE_MAC_READ                = 0x605,
    BLEWIFI_REQ_ENG_BLE_CMD                     = 0x606,
    BLEWIFI_REQ_ENG_MAC_SRC_WRITE               = 0x607,
    BLEWIFI_REQ_ENG_MAC_SRC_READ                = 0x608,
    BLEWIFI_REQ_ENG_TMPR_CAL_DATA_WRITE         = 0x609,
    BLEWIFI_REQ_ENG_TMPR_CAL_DATA_READ          = 0x60A,
    BLEWIFI_REQ_ENG_VDD_VOUT_VOLTAGE_READ       = 0x60B,
    //4 cmd ID unused
    BLEWIFI_REQ_ENG_BLE_CLOUD_INFO_WRITE        = 0x610,
    BLEWIFI_REQ_ENG_BLE_CLOUD_INFO_READ         = 0x611,

    BLEWIFI_REQ_APP_START                       = 0x800,
    BLEWIFI_REQ_APP_USER_DEF_TMPR_OFFSET_WRITE  = 0x801,
    BLEWIFI_REQ_APP_USER_DEF_TMPR_OFFSET_READ   = 0x802,

    BLEWIFI_RSP_SCAN_REPORT                     = 0x1000,
    BLEWIFI_RSP_SCAN_END                        = 0x1001,
    BLEWIFI_RSP_CONNECT                         = 0x1002,
    BLEWIFI_RSP_DISCONNECT                      = 0x1003,
    BLEWIFI_RSP_RECONNECT                       = 0x1004,
    BLEWIFI_RSP_READ_DEVICE_INFO                = 0x1005,
    BLEWIFI_RSP_WRITE_DEVICE_INFO               = 0x1006,
    BLEWIFI_RSP_WIFI_STATUS                     = 0x1007,
    BLEWIFI_RSP_RESET                           = 0x1008,

    BLEWIFI_RSP_OTA_VERSION                     = 0x1100,
    BLEWIFI_RSP_OTA_UPGRADE                     = 0x1101,
    BLEWIFI_RSP_OTA_RAW                         = 0x1102,
    BLEWIFI_RSP_OTA_END                         = 0x1103,

    BLEWIFI_RSP_HTTP_OTA_TRIG                   = 0x1200,
    BLEWIFI_RSP_HTTP_OTA_DEVICE_VERSION         = 0x1201,
    BLEWIFI_RSP_HTTP_OTA_SERVER_VERSION         = 0x1202,

    BLEWIFI_RSP_MP_START                        = 0x1400,
    BLEWIFI_RSP_MP_CAL_VBAT                     = 0x1401,
    BLEWIFI_RSP_MP_CAL_IO_VOLTAGE               = 0x1402,
    BLEWIFI_RSP_MP_CAL_TMPR                     = 0x1403,
    BLEWIFI_RSP_MP_SYS_MODE_WRITE               = 0x1404,
    BLEWIFI_RSP_MP_SYS_MODE_READ                = 0x1405,

    BLEWIFI_RSP_ENG_START                       = 0x1600,
    BLEWIFI_RSP_ENG_SYS_RESET                   = 0x1601,
    BLEWIFI_RSP_ENG_WIFI_MAC_WRITE              = 0x1602,
    BLEWIFI_RSP_ENG_WIFI_MAC_READ               = 0x1603,
    BLEWIFI_RSP_ENG_BLE_MAC_WRITE               = 0x1604,
    BLEWIFI_RSP_ENG_BLE_MAC_READ                = 0x1605,
    BLEWIFI_RSP_ENG_BLE_CMD                     = 0x1606,
    BLEWIFI_RSP_ENG_MAC_SRC_WRITE               = 0x1607,
    BLEWIFI_RSP_ENG_MAC_SRC_READ                = 0x1608,
    BLEWIFI_RSP_ENG_TMPR_CAL_DATA_WRITE         = 0x1609,
    BLEWIFI_RSP_ENG_TMPR_CAL_DATA_READ          = 0x160A,
    BLEWIFI_RSP_ENG_VDD_VOUT_VOLTAGE_READ       = 0x160B,
    //4 cmd ID unused
    BLEWIFI_RSP_ENG_BLE_CLOUD_INFO_WRITE        = 0x1610,
    BLEWIFI_RSP_ENG_BLE_CLOUD_INFO_READ         = 0x1611,

    BLEWIFI_RSP_APP_START                       = 0x1800,
    BLEWIFI_RSP_APP_USER_DEF_TMPR_OFFSET_WRITE  = 0x1801,
    BLEWIFI_RSP_APP_USER_DEF_TMPR_OFFSET_READ   = 0x1802,

    BLEWIFI_IND_IP_STATUS_NOTIFY                = 0x2000,  // Wifi notify AP status

    BLEWIFI_TYPE_END                            = 0xFFFF
}blewifi_type_id_e;

/* BLEWIF protocol */
typedef struct blewifi_hdr_tag
{
    uint16_t type;
    uint16_t data_len;
    uint8_t  data[]; //variable size
}blewifi_hdr_t;

typedef void (*T_BleWifi_Ble_ProtocolHandler_Fp)(uint16_t type, uint8_t *data, int len);
typedef struct
{
    uint32_t ulEventId;
    T_BleWifi_Ble_ProtocolHandler_Fp fpFunc;
} T_BleWifi_Ble_ProtocolHandlerTbl;

void BleWifi_Ble_DataRecvHandler(uint8_t *data, int len);
void BleWifi_Ble_DataSendEncap(uint16_t type_id, uint8_t *data, int total_data_len);
void BleWifi_Ble_SendResponse(uint16_t type_id, uint8_t status);

void BleWifi_Wifi_OtaTrigReq(uint8_t *data);
void BleWifi_Wifi_OtaTrigRsp(uint8_t status);
void BleWifi_Wifi_OtaDeviceVersionReq(void);
void BleWifi_Wifi_OtaDeviceVersionRsp(uint16_t fid);
void BleWifi_Wifi_OtaServerVersionReq(void);
void BleWifi_Wifi_OtaServerVersionRsp(uint16_t fid);

#endif /* __BLEWIFI_DATA_H__ */

