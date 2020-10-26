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

#include "ble_gatt_if.h"
#include "ble_uuid.h"
#include "ble_gap_if.h"

#include "blewifi_common.h"
#include "blewifi_configuration.h"
#include "blewifi_server_app.h"
#include "blewifi_server_app_gatt.h"
#include "blewifi_ctrl.h"
#include "infra_config.h"
#include "breeze_hal_ble.h"

#include "mw_fim_default_group11_project.h"
#ifdef ALI_BLE_WIFI_PROVISION
#include "transport.h"
extern UINT8 g_icnt;
bool g_noti_flag=false;
bool g_Indi_flag=false;
#endif 
// This is used for GATT service
SHM_DATA static UINT16 gGattSvcUuid = ATT_SVC_GENERIC_ATTRIBUTE;

SHM_DATA static UINT16 gGattServiceChangeUuid      = ATT_CHAR_SERVICE_CHANGED;
SHM_DATA static UINT8  gGattServiceChangeCharVal[] = CHAR_DECL_UUID16_ATTR_VAL(LE_GATT_CHAR_PROP_IND, ATT_CHAR_SERVICE_CHANGED);
SHM_DATA static UINT8  gGattServiceChangeVal[4]    = {0, 0, 0, 0};
SHM_DATA static UINT16 gGattServiceChangeClientCfg = 0;

// This is used for GAP service
SHM_DATA static UINT16 gGapSvcUuid = ATT_SVC_GENERIC_ACCESS;

SHM_DATA static UINT16 gGapDeviceNameUuid          = ATT_CHAR_DEVICE_NAME;
SHM_DATA static UINT8  gGapDeviceNameCharVal[]     = CHAR_DECL_UUID16_ATTR_VAL(LE_GATT_CHAR_PROP_RD | LE_GATT_CHAR_PROP_WR, ATT_CHAR_DEVICE_NAME);
SHM_DATA static UINT8  gGapDeviceNameVal[31]       = BLEWIFI_BLE_GAP_DEVICE_NAME;
SHM_DATA static UINT16 gGapAppearanceUuid          = ATT_CHAR_APPEARANCE;
SHM_DATA static UINT8  gGapAppearanceCharVal[]     = CHAR_DECL_UUID16_ATTR_VAL(LE_GATT_CHAR_PROP_RD, ATT_CHAR_APPEARANCE);
SHM_DATA static UINT8  gGapAppearanceVal[2]        = {0, 0};

SHM_DATA static UINT16 gGapConnParamUuid           = ATT_CHAR_PERIPH_PREF_CON_PARAM;
SHM_DATA static UINT8  gGapConnParamCharVal[]      = CHAR_DECL_UUID16_ATTR_VAL(LE_GATT_CHAR_PROP_RD, ATT_CHAR_PERIPH_PREF_CON_PARAM);
SHM_DATA static UINT16 gGapConnParamVal[4]         = {DEFAULT_DESIRED_MIN_CONN_INTERVAL, DEFAULT_DESIRED_MAX_CONN_INTERVAL, DEFAULT_DESIRED_SLAVE_LATENCY, DEFAULT_DESIRED_SUPERVERSION_TIMEOUT};

// This is used for BLEWIFI service
SHM_DATA static UINT16 gBleWifiSvcUuid             = BLEWIFI_BLE_UUID_SERVICE;

SHM_DATA static UINT16 gBleWifiDataInUuid          = BLEWIFI_BLE_UUID_DATA_IN;
SHM_DATA static UINT8  gBleWifiDataInCharVal[]     = CHAR_DECL_UUID16_ATTR_VAL(LE_GATT_CHAR_PROP_WR_NO_RESP | LE_GATT_CHAR_PROP_WR, BLEWIFI_BLE_UUID_DATA_IN);
SHM_DATA static UINT8  gBleWifiDataInVal[LE_ATT_MAX_MTU];

SHM_DATA static UINT16 gBleWifiDataOutUuid		  = BLEWIFI_BLE_UUID_DATA_OUT;
SHM_DATA static UINT8  gBleWifiDataOutCharVal[]    = CHAR_DECL_UUID16_ATTR_VAL(LE_GATT_CHAR_PROP_NTF, BLEWIFI_BLE_UUID_DATA_OUT);
SHM_DATA static UINT8  gBleWifiDataOutVal[LE_ATT_MAX_MTU];
SHM_DATA static UINT16 gBleWifiDataOutClientCfg 	  = 1;

#ifdef ALI_BLE_WIFI_PROVISION
SHM_DATA static UINT16 gALISvcUuid               = BLE_UUID_AIS_SERVICE;

SHM_DATA static UINT16 gALIReadCharUuid          = BLE_UUID_AIS_RC;
SHM_DATA static UINT8  gALIReadCharVal[]         = CHAR_DECL_UUID16_ATTR_VAL(LE_GATT_CHAR_PROP_RD, BLE_UUID_AIS_RC);
SHM_DATA static UINT8  gALIReadVal[2]            = {0};

SHM_DATA static UINT16 gALIWriteCharUuid         = BLE_UUID_AIS_WC;
SHM_DATA static UINT8  gALIWriteCharVal[]        = CHAR_DECL_UUID16_ATTR_VAL(LE_GATT_CHAR_PROP_RD | LE_GATT_CHAR_PROP_WR, BLE_UUID_AIS_WC);
SHM_DATA static UINT8  gALIWriteVal[2]            = {0};

SHM_DATA static UINT16 gALIIndicateCharUuid      = BLE_UUID_AIS_IC;
SHM_DATA static UINT8  gALIIndicateCharVal[]     = CHAR_DECL_UUID16_ATTR_VAL(LE_GATT_CHAR_PROP_RD | LE_GATT_CHAR_PROP_IND, BLE_UUID_AIS_IC);
SHM_DATA static UINT8  gALIIndicateVal[2]       = {0};
SHM_DATA static UINT16 gALIIndicateValClientCfg  = 2;

SHM_DATA static UINT16 gALIWwnrcCharUuid         = BLE_UUID_AIS_WWNRC;
SHM_DATA static UINT8  gALIWwnrcCharVal[]        = CHAR_DECL_UUID16_ATTR_VAL(LE_GATT_CHAR_PROP_RD | LE_GATT_CHAR_PROP_WR_NO_RESP, BLE_UUID_AIS_WWNRC);
SHM_DATA static UINT8  gALIWwnrcVal[2]            = {0};

SHM_DATA static UINT16 gALINcCharUuid            = BLE_UUID_AIS_NC;
SHM_DATA static UINT8  gALINcCharVal[]           = CHAR_DECL_UUID16_ATTR_VAL(LE_GATT_CHAR_PROP_RD | LE_GATT_CHAR_PROP_NTF, BLE_UUID_AIS_NC);
SHM_DATA static UINT16 gALINcValClientCfg        = 1;
#endif


SHM_DATA static LE_GATT_ATTR_T gGattSvcDb[GATT_IDX_TOP] =
{
    // GATT Service Declaration
    [GATT_IDX_SVC]                 = PRIMARY_SERVICE_DECL_UUID16(&gGattSvcUuid),
    // GATT Service Change Characteristic
    [GATT_IDX_SERVICE_CHANGE_CHAR] = CHARACTERISTIC_DECL_UUID16(gGattServiceChangeCharVal),
    [GATT_IDX_SERVICE_CHANGE_VAL]  = CHARACTERISTIC_UUID16(&gGattServiceChangeUuid, 0, 0, sizeof(gGattServiceChangeVal), gGattServiceChangeVal),
    [GATT_IDX_SERVICE_CHANGE_CFG]  = CHAR_CLIENT_CONFIG_DESCRIPTOR(LE_GATT_PERMIT_AUTHOR_READ | LE_GATT_PERMIT_AUTHOR_WRITE, &gGattServiceChangeClientCfg)
};

SHM_DATA static LE_GATT_ATTR_T gGapSvcDb[GAP_IDX_TOP] =
{
    // GAP Service Declaration
    [GAP_IDX_SVC]                  = PRIMARY_SERVICE_DECL_UUID16(&gGapSvcUuid),
    // GAP Device Name Characteristic
    [GAP_IDX_DEVICE_NAME_CHAR]     = CHARACTERISTIC_DECL_UUID16(gGapDeviceNameCharVal),
    [GAP_IDX_DEVICE_NAME_VAL]      = CHARACTERISTIC_UUID16(&gGapDeviceNameUuid, LE_GATT_PERMIT_READ | LE_GATT_PERMIT_AUTHOR_WRITE, sizeof(gGapDeviceNameVal), sizeof(BLEWIFI_BLE_GAP_DEVICE_NAME) - 1, gGapDeviceNameVal),
    // GAP Appearance Characteristic
    [GAP_IDX_APPEARANCE_CHAR]      = CHARACTERISTIC_DECL_UUID16(gGapAppearanceCharVal),
    [GAP_IDX_APPEARANCE_VAL]       = CHARACTERISTIC_UUID16(&gGapAppearanceUuid, LE_GATT_PERMIT_READ, 0, sizeof(gGapAppearanceVal), gGapAppearanceVal),
    // GAP Connection Parameter Characteristic Declaration
    [GAP_IDX_CONN_PARAM_CHAR]      = CHARACTERISTIC_DECL_UUID16(gGapConnParamCharVal),
    [GAP_IDX_CONN_PARAM_VAL]       = CHARACTERISTIC_UUID16(&gGapConnParamUuid, LE_GATT_PERMIT_READ, 0, sizeof(gGapConnParamVal), gGapConnParamVal)
};

SHM_DATA static LE_GATT_ATTR_T gBwpSvcDb[BWP_IDX_TOP] =
{
    // BLEWIFI Service Declaration
    [BWP_IDX_SVC]                  = PRIMARY_SERVICE_DECL_UUID16(&gBleWifiSvcUuid),
    // BLEWIFI Data In Characteristic
    [BWP_IDX_DATA_IN_CHAR]         = CHARACTERISTIC_DECL_UUID16(gBleWifiDataInCharVal),
    [BWP_IDX_DATA_IN_VAL]          = CHARACTERISTIC_UUID16(&gBleWifiDataInUuid, LE_GATT_PERMIT_AUTHOR_WRITE, sizeof(gBleWifiDataInVal), 0, gBleWifiDataInVal),
    // BLEWIFI Data Out Characteristic
    [BWP_IDX_DATA_OUT_CHAR]        = CHARACTERISTIC_DECL_UUID16(gBleWifiDataOutCharVal),
    [BWP_IDX_DATA_OUT_VAL]         = CHARACTERISTIC_UUID16(&gBleWifiDataOutUuid, 0, sizeof(gBleWifiDataOutVal), 0, gBleWifiDataOutVal),
    [BWP_IDX_DATA_OUT_CFG]         = CHAR_CLIENT_CONFIG_DESCRIPTOR(LE_GATT_PERMIT_AUTHOR_READ | LE_GATT_PERMIT_AUTHOR_WRITE, &gBleWifiDataOutClientCfg)
};

#ifdef ALI_BLE_WIFI_PROVISION
SHM_DATA static LE_GATT_ATTR_T gALISvcDb[ALI_IDX_TOP] =
{
	
	// ALI Service Declaration
    [ALI_IDX_SVC]                   = PRIMARY_SERVICE_DECL_UUID16(&gALISvcUuid),
    // ALI Data rc Characteristic
    [ALI_IDX_DATA_READ_CHAR]        = CHARACTERISTIC_DECL_UUID16(gALIReadCharVal),
    [ALI_IDX_DATA_READ_VAL]         = CHARACTERISTIC_UUID16(&gALIReadCharUuid, LE_GATT_PERMIT_AUTHOR_READ, 0, sizeof(gALIReadVal), gALIReadVal),		
    // ALI Data wc Characteristic
    [ALI_IDX_DATA_WRITE_CHAR]       = CHARACTERISTIC_DECL_UUID16(gALIWriteCharVal),
    [ALI_IDX_DATA_WRITE_VAL]        = CHARACTERISTIC_UUID16(&gALIWriteCharUuid, LE_GATT_PERMIT_READ|LE_GATT_PERMIT_AUTHOR_WRITE, LE_ATT_MAX_MTU, 0, gALIWriteVal),
    // ALI Data ic Characteristic
    [ALI_IDX_DATA_INDICATE_CHAR]    = CHARACTERISTIC_DECL_UUID16(gALIIndicateCharVal),
    [ALI_IDX_DATA_INDICATE_VAL]     = CHARACTERISTIC_UUID16(&gALIIndicateCharUuid, LE_GATT_PERMIT_AUTHOR_READ, sizeof(gALIIndicateVal), 0, gALIIndicateVal),
    [ALI_IDX_DATA_INDICATE_CFG]     = CHAR_CLIENT_CONFIG_DESCRIPTOR(LE_GATT_PERMIT_AUTHOR_READ | LE_GATT_PERMIT_AUTHOR_WRITE, &gALIIndicateValClientCfg),
    // ALI Data wwnrc Characteristic
    [ALI_IDX_DATA_WWNRC_CHAR]       = CHARACTERISTIC_DECL_UUID16(gALIWwnrcCharVal),
    [ALI_IDX_DATA_WWNRC_VAL]        = CHARACTERISTIC_UUID16(&gALIWwnrcCharUuid, LE_GATT_PERMIT_READ|LE_GATT_PERMIT_AUTHOR_WRITE, sizeof(gALIWwnrcVal), 0, gALIWwnrcVal),
    // ALI Data nc Characteristic
    [ALI_IDX_DATA_NOTIFY_CHAR]      = CHARACTERISTIC_DECL_UUID16(gALINcCharVal),
    [ALI_IDX_DATA_NOTIFY_VAL]       = CHARACTERISTIC_UUID16(&gALINcCharUuid, LE_GATT_PERMIT_READ, sizeof(gALIReadVal), 0, gALIReadVal),
    [ALI_IDX_DATA_NOTIFY_CFG]       = CHAR_CLIENT_CONFIG_DESCRIPTOR(LE_GATT_PERMIT_AUTHOR_READ | LE_GATT_PERMIT_AUTHOR_WRITE, &gALINcValClientCfg),
    
};
SHM_DATA static LE_GATT_SERVICE_T *gAliSvc = 0;
#endif


SHM_DATA static LE_GATT_SERVICE_T *gGattSvc = 0;
SHM_DATA static LE_GATT_SERVICE_T *gGapSvc = 0;
SHM_DATA static LE_GATT_SERVICE_T *gBwpSvc = 0;

static void BleWifi_Ble_ServerAppGattMsgHandler_NotifyCfm(TASK task, MESSAGEID id, MESSAGE message);
static void BleWifi_Ble_ServerAppGattMsgHandler_InitCfm(TASK task, MESSAGEID id, MESSAGE message);
static void BleWifi_Ble_ServerAppGattMsgHandler_AccessReadInd(TASK task, MESSAGEID id, MESSAGE message);
static void BleWifi_Ble_ServerAppGattMsgHandler_AccessWriteInd(TASK task, MESSAGEID id, MESSAGE message);
static void BleWifi_Ble_ServerAppGattMsgHandler_ExchangeMtuInd(TASK task, MESSAGEID id, MESSAGE message);
static void BleWifi_Ble_ServerAppGattMsgHandler_ExchangeMtuCfm(TASK task, MESSAGEID id, MESSAGE message);
static void BleWifi_Ble_ServerAppGattMsgHandler_ConfirmationCfm(TASK task, MESSAGEID id, MESSAGE message);
static void BleWifi_Ble_ServerAppGattMsgHandler_OperationTimeout(TASK task, MESSAGEID id, MESSAGE message);
T_BleWifi_Ble_MsgHandlerTbl gBleGattMsgHandlerTbl[] =
{
    {LE_GATT_MSG_NOTIFY_CFM,        BleWifi_Ble_ServerAppGattMsgHandler_NotifyCfm},
    {LE_GATT_MSG_INIT_CFM,          BleWifi_Ble_ServerAppGattMsgHandler_InitCfm},
    {LE_GATT_MSG_ACCESS_READ_IND,   BleWifi_Ble_ServerAppGattMsgHandler_AccessReadInd},
    {LE_GATT_MSG_ACCESS_WRITE_IND,  BleWifi_Ble_ServerAppGattMsgHandler_AccessWriteInd},
    {LE_GATT_MSG_EXCHANGE_MTU_IND,  BleWifi_Ble_ServerAppGattMsgHandler_ExchangeMtuInd},
    {LE_GATT_MSG_EXCHANGE_MTU_CFM,  BleWifi_Ble_ServerAppGattMsgHandler_ExchangeMtuCfm},
    {LE_GATT_MSG_CONFIRMATION_CFM,  BleWifi_Ble_ServerAppGattMsgHandler_ConfirmationCfm},
    {LE_GATT_MSG_OPERATION_TIMEOUT, BleWifi_Ble_ServerAppGattMsgHandler_OperationTimeout},
    
    {0xFFFFFFFF,                    NULL}
};

SHM_DATA static void BleWifi_Ble_AppHandleGattServiceRead(LE_GATT_MSG_ACCESS_READ_IND_T *ind)
{
	UINT8 attErr = 0;
	UINT16 attrid = ind->handle - gGattSvc->startHdl;

    BLEWIFI_INFO("APP-BleWifi_Ble_AppHandleGattServiceRead attId = %d offset = %d\r\n", attrid, ind->offset);
    
	switch (attrid)
    {
		case GATT_IDX_SERVICE_CHANGE_CFG:
		break;

		default:
			attErr = LE_ATT_ERR_READ_NOT_PERMITTED;
        break;
    }

    LeGattAccessReadRsp(ind->conn_hdl, ind->handle, attErr);
}

SHM_DATA static void BleWifi_Ble_AppHandleGattServiceWrite(LE_GATT_MSG_ACCESS_WRITE_IND_T *ind)
{
	UINT8 attErr = 0;
	UINT16 attrid = ind->handle - gGattSvc->startHdl;
	
    BLEWIFI_INFO("APP-BleWifi_Ble_AppHandleGattServiceWrite attId = %d op = %x offset = %d\r\n", attrid, ind->flag, ind->offset);
    
	switch (attrid)
    {
		case GATT_IDX_SERVICE_CHANGE_CFG:
		{
			UINT16 val = *((UINT16 *)ind->pVal);

			BLEWIFI_INFO("APP-GATT_IDX_SERVICE_CHANGE_CFG Len = %d  attr-val = %d\r\n", val);

            LeGattChangeAttrVal(gGattSvc, attrid, 2, &val);
        }
		break;

		default:
            attErr = LE_ATT_ERR_WRITE_NOT_PERMITTED;
        break;
    }

	LeGattAccessWriteRsp(ind->conn_hdl, ind->flag, ind->handle, attErr);
}

SHM_DATA static void BleWifi_Ble_AppHandleGapServiceRead(LE_GATT_MSG_ACCESS_READ_IND_T *ind)
{
	UINT8 attErr = 0;
	UINT16 attrid = ind->handle - gGapSvc->startHdl;

	switch (attrid)
    {
		case GAP_IDX_DEVICE_NAME_VAL:
		case GAP_IDX_APPEARANCE_VAL:
		case GAP_IDX_CONN_PARAM_VAL:
        break;
        
		default:
            attErr = LE_ATT_ERR_READ_NOT_PERMITTED;
        break;
    }

	LeGattAccessReadRsp(ind->conn_hdl, ind->handle, attErr);
}

SHM_DATA static void BleWifi_Ble_AppHandleGapServiceWrite(LE_GATT_MSG_ACCESS_WRITE_IND_T *ind)
{
	UINT8 attErr = 0;
	UINT16 attrid = ind->handle - gGapSvc->startHdl;
	
    BLEWIFI_INFO("APP-BleWifi_Ble_AppHandleGapServiceWrite attId = %d op = %x offset = %d\r\n", attrid, ind->flag, ind->offset);
    
	switch (attrid)
    {
		case GAP_IDX_DEVICE_NAME_VAL:
		{
			BLE_APP_DATA_T *app = BleWifi_Ble_GetEntity();
			UINT8 *p = app->scn_data.buf;
			UINT16 wrLen = ind->len;

			if (!wrLen || (wrLen > 31))
            {
            	attErr = LE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
                
				break;
            }

			LeGattChangeAttrVal(gGapSvc, GAP_IDX_DEVICE_NAME_VAL, wrLen, ind->pVal);

			if (wrLen > 29) 
			{
				wrLen = 29;
				*p++ = wrLen + 1;
				*p++ = GAP_ADTYPE_LOCAL_NAME_SHORT;
            }
            else
			{
				*p++ = wrLen + 1;
				*p++ = GAP_ADTYPE_LOCAL_NAME_COMPLETE;
            }
            
            MemCopy(p, ind->pVal, wrLen);

			LeSetScanRspData(wrLen + 2, app->scn_data.buf);
        }
		break;

		default:
            attErr = LE_ATT_ERR_WRITE_NOT_PERMITTED;
        break;
    }

	LeGattAccessWriteRsp(ind->conn_hdl, ind->flag, ind->handle, attErr);
}

SHM_DATA static void BleWifi_Ble_AppHandleBwpServiceRead(LE_GATT_MSG_ACCESS_READ_IND_T *ind)
{
	UINT8 attErr = 0;
	UINT16 attrid = ind->handle - gBwpSvc->startHdl;

	switch (attrid)
    {
		case BWP_IDX_DATA_OUT_CFG:
        {
			UINT16 enable;
			UINT16 len = 0;

			LeGattGetAttrVal(gBwpSvc, BWP_IDX_DATA_OUT_CFG, &len, &enable);

			BLEWIFI_INFO("APP-BWP_IDX_DATA_OUT_CFG val = %d\r\n", enable);
        }
		break;

		default:
            attErr = LE_ATT_ERR_READ_NOT_PERMITTED;
        break;
    }

	LeGattAccessReadRsp(ind->conn_hdl, ind->handle, attErr);
}

SHM_DATA static void BleWifi_Ble_AppHandleBwpServiceWrite(LE_GATT_MSG_ACCESS_WRITE_IND_T *ind)
{
	UINT8 attErr = 0;
	UINT16 attrid = ind->handle - gBwpSvc->startHdl;
	
    BLEWIFI_INFO("APP - BleWifi_Ble_AppHandleBwpServiceWrite attId = %d op = %x offset = %d len = %d\r\n", attrid, ind->flag, ind->offset, ind->len);
    
	switch (attrid)
    {
		case BWP_IDX_DATA_IN_VAL:
		{
		    // !!! BLE Data in 
		    // receive the data from peer to device
            BleWifi_Ctrl_MsgSend(BLEWIFI_CTRL_MSG_BLE_DATA_IND, ind->pVal, ind->len);
        }
		break;

		case BWP_IDX_DATA_OUT_CFG:
		{
			UINT16 enable = *((UINT16 *)ind->pVal);

			if ((ind->len == 2) && (enable <= 1))
			{
				LeGattChangeAttrVal(gBwpSvc, BWP_IDX_DATA_OUT_CFG, 2, &enable);
            }
            else
                attErr = LE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
        }
        break;

		default:
            attErr = LE_ATT_ERR_WRITE_NOT_PERMITTED;
        break;
    }

	LeGattAccessWriteRsp(ind->conn_hdl, ind->flag, ind->handle, attErr);
}

#ifdef ALI_BLE_WIFI_PROVISION
static void BleWifi_Ble_AppHandleAliServiceWrite(LE_GATT_MSG_ACCESS_WRITE_IND_T *ind)
{
    UINT8 attErr = 0;
    UINT16 attrid = ind->handle - gAliSvc->startHdl;
	
    BLEWIFI_INFO("\nAPP - BleWifi_Ble_AppHandleAliServiceWrite attId = %d op = %x offset = %d len = %d\r\n", attrid, ind->flag, ind->offset, ind->len);
    
    switch (attrid)
    {
        case ALI_IDX_DATA_WRITE_VAL:
        {
#ifdef ALI_OPL_DBG					
            printf("\r\nALI_IDX_DATA_WRITE_VAL:");
//            for(int i=0;i<10;i++)            
            for(int i=0;i<ind->len;i++)
            {
                printf("0x%02x ", ind->pVal[i]);
            }
            printf("\r\n");
#endif
//            printf("length:%d\n", ind->len);
            BleWifi_Ble_SendAppMsgToBle(BLEWIFI_APP_MSG_ALI_SEND_HICLIENT, ind->len, ind->pVal);
    		
    	}
        break;
        case ALI_IDX_DATA_INDICATE_CFG:
        {
            g_Indi_flag = (*((UINT16 *)ind->pVal) == AIS_CCC_VALUE_INDICATE ? true : false);
#ifdef ALI_OPL_DBG					
            printf("APP-ALI_IDX_DATA_INDICATE_CFG val = %d\r\n", *((UINT16 *)ind->pVal));
#endif					
//            LeGattChangeAttrVal(gAliSvc, ALI_IDX_DATA_INDICATE_CFG, 2, &enable);
            reset_tx();
            BleWifi_Ble_SendAppMsgToBle(BLEWIFI_APP_MSG_ALI_SEND_RANDOM, NULL, 0);
        }
		break;
        case ALI_IDX_DATA_NOTIFY_CFG:
        {
            g_noti_flag = (*((UINT16 *)ind->pVal) == AIS_CCC_VALUE_NOTIFY ? true : false);
#ifdef ALI_OPL_DBG					
            printf("APP-ALI_IDX_DATA_NOTIFY_CFG val = %d\r\n", *((UINT16 *)ind->pVal));
#endif            
//            LeGattChangeAttrVal(gAliSvc, ALI_IDX_DATA_NOTIFY_CFG, 2, &enable);
#if 1
            reset_tx();
            BleWifi_Ble_SendAppMsgToBle(BLEWIFI_APP_MSG_ALI_SEND_RANDOM, NULL, 0);
#else            
            BleWifi_Ble_SendAppMsgToBle(BLEWIFI_APP_MSG_ALI_SEND_DEVICE_NAME, 0, 0);
#endif            
        }
        break;
        case ALI_IDX_DATA_WWNRC_VAL:
        {
//            printf("\r\nALI_IDX_DATA_WWNRC_VAL:");
//            LeGattChangeAttrVal(gAliSvc, ALI_IDX_DATA_WWNRC_VAL, ind->len, ind->pVal);	
        }
        break;
        default:
            attErr = LE_ATT_ERR_WRITE_NOT_PERMITTED;
        break;
    }
    LeGattAccessWriteRsp(ind->conn_hdl, ind->flag, ind->handle, attErr);

}


static void BleWifi_Ble_AppHandleAliServiceRead(LE_GATT_MSG_ACCESS_READ_IND_T *ind)
{
#if 0    
    UINT8 attErr = 0;
    UINT16 attrid = ind->handle - gAliSvc->startHdl;

    switch (attrid)
    {
        case ALI_IDX_DATA_INDICATE_CFG:
        {
            UINT16 enable;
            UINT16 len = 0;
            LeGattGetAttrVal(gAliSvc, ALI_IDX_DATA_INDICATE_CFG, &len, &enable);
#ifdef ALI_OPL_DBG
            printf("APP-ALI_IDX_DATA_INDICATE_CFG val = %d\r\n", enable);
#endif					
        }
        break;
        default:
            attErr = LE_ATT_ERR_READ_NOT_PERMITTED;
        break;
    }

	LeGattAccessReadRsp(ind->conn_hdl, ind->handle, attErr);
#endif    
}
#endif

SHM_DATA static void BleWifi_Ble_GattHandleAccessRead(LE_GATT_MSG_ACCESS_READ_IND_T *ind)
{
	if ((ind->handle >= gGattSvc->startHdl) && (ind->handle <= gGattSvc->endHdl))
	{
		BleWifi_Ble_AppHandleGattServiceRead(ind);
	}
	else if ((ind->handle >= gGapSvc->startHdl) && (ind->handle <= gGapSvc->endHdl))
	{
		BleWifi_Ble_AppHandleGapServiceRead(ind);
    }
	else if ((ind->handle >= gBwpSvc->startHdl) && (ind->handle <= gBwpSvc->endHdl))
	{
		BleWifi_Ble_AppHandleBwpServiceRead(ind);
    }
#ifdef ALI_BLE_WIFI_PROVISION
	else if ((ind->handle >= gAliSvc->startHdl) && (ind->handle <= gAliSvc->endHdl))
	{
		BleWifi_Ble_AppHandleAliServiceRead(ind);
    }
#endif
	else
	{
		LeGattAccessReadRsp(ind->conn_hdl, ind->handle, LE_ATT_ERR_READ_NOT_PERMITTED);
    }
}

SHM_DATA static void BleWifi_Ble_GattHandleAccessWrite(LE_GATT_MSG_ACCESS_WRITE_IND_T *ind)
{
	if ((ind->handle >= gGattSvc->startHdl) && (ind->handle <= gGattSvc->endHdl))
	{
		BleWifi_Ble_AppHandleGattServiceWrite(ind);
    }
	else if ((ind->handle >= gGapSvc->startHdl) && (ind->handle <= gGapSvc->endHdl))
	{
		BleWifi_Ble_AppHandleGapServiceWrite(ind);
    }
	else if ((ind->handle >= gBwpSvc->startHdl) && (ind->handle <= gBwpSvc->endHdl))
	{
		BleWifi_Ble_AppHandleBwpServiceWrite(ind);
    }
#ifdef ALI_BLE_WIFI_PROVISION
	else if ((ind->handle >= gAliSvc->startHdl) && (ind->handle <= gAliSvc->endHdl))
	{

		BleWifi_Ble_AppHandleAliServiceWrite(ind);
    }
#endif
	else
	{
		LeGattAccessReadRsp(ind->conn_hdl, ind->handle, LE_ATT_ERR_WRITE_NOT_PERMITTED);
    }
}

SHM_DATA static void BleWifi_Ble_ServerAppGattMsgHandler_NotifyCfm(TASK task, MESSAGEID id, MESSAGE message)
{
    // !!! after BLE data out (after LeGattCharValNotify)
    // trigger the next "send data from buffer to peer"
    BLEWIFI_INFO("APP-LE_GATT_MSG_NOTIFY_CFM\r\n");
    BleWifi_Ble_SendAppMsgToBle(BLEWIFI_APP_MSG_SEND_TO_PEER_CFM, 0, NULL);
}

static void BleWifi_Ble_ServerAppGattMsgHandler_InitCfm(TASK task, MESSAGEID id, MESSAGE message)
{
#if 1
    gBleWifiSvcUuid = BLEWIFI_BLE_UUID_SERVICE;
    gBleWifiDataInUuid = BLEWIFI_BLE_UUID_DATA_IN;
    gBleWifiDataInCharVal[3] = UINT16_LO(gBleWifiDataInUuid);
    gBleWifiDataInCharVal[4] = UINT16_HI(gBleWifiDataInUuid);
    gBleWifiDataOutUuid = BLEWIFI_BLE_UUID_DATA_OUT;
    gBleWifiDataOutCharVal[3] = UINT16_LO(gBleWifiDataOutUuid);
    gBleWifiDataOutCharVal[4] = UINT16_HI(gBleWifiDataOutUuid);
#else
    T_MwFim_GP11_BleServiceUUID tBleServiceUUID;
    
    // !!! after LeGattInit
    BLEWIFI_INFO("APP-LE_GATT_MSG_INIT_CFM\r\n");

    // get the settings of BLE service UUID
    if (MW_FIM_OK != MwFim_FileRead(MW_FIM_IDX_GP11_PROJECT_BLE_SERVICE_UUID, 0, MW_FIM_GP11_BLE_SERVICE_UUID_SIZE, (uint8_t*)&tBleServiceUUID))
    {
        // if fail, get the default value
        memcpy(&tBleServiceUUID, &g_tMwFimDefaultGp11BleServiceUUID, MW_FIM_GP11_BLE_SERVICE_UUID_SIZE);
    }
    gBleWifiSvcUuid = tBleServiceUUID.uwServiceUUID;
    gBleWifiDataInUuid = tBleServiceUUID.uwDataInUUID;
    gBleWifiDataInCharVal[3] = UINT16_LO(gBleWifiDataInUuid);
    gBleWifiDataInCharVal[4] = UINT16_HI(gBleWifiDataInUuid);
    gBleWifiDataOutUuid = tBleServiceUUID.uwDataOutUUID;
    gBleWifiDataOutCharVal[3] = UINT16_LO(gBleWifiDataOutUuid);
    gBleWifiDataOutCharVal[4] = UINT16_HI(gBleWifiDataOutUuid);
#endif
    gGattSvc = LeGattRegisterService(gGattSvcDb, sizeof(gGattSvcDb) / sizeof(LE_GATT_ATTR_T));
    
    if (gGattSvc)
    {
        BLEWIFI_INFO("APP-LeGattRegisterService gGattSvc success\r\n");
    }
    
    gGapSvc = LeGattRegisterService(gGapSvcDb, sizeof(gGapSvcDb) / sizeof(LE_GATT_ATTR_T));
    
    if (gGapSvc)
    {
        BLEWIFI_INFO("APP-LeGattRegisterService gGapSvc success\r\n");
    }
    
    gBwpSvc = LeGattRegisterService(gBwpSvcDb, sizeof(gBwpSvcDb) / sizeof(LE_GATT_ATTR_T));
    
    if (gBwpSvc)
    {
        BLEWIFI_INFO("APP-LeGattRegisterService gBwpSvc success\r\n");
    }
#ifdef ALI_BLE_WIFI_PROVISION
    gAliSvc = LeGattRegisterService(gALISvcDb, sizeof(gALISvcDb) / sizeof(LE_GATT_ATTR_T));
    if (gAliSvc)
    {
        BLEWIFI_INFO("APP-LeGattRegisterService gAliSvc success\r\n");
    }
    BleWifi_Ble_GetEntity()->ali_indhdl = gALISvcDb[ALI_IDX_DATA_INDICATE_VAL].handle;
    BleWifi_Ble_GetEntity()->ali_notihdl = gALISvcDb[ALI_IDX_DATA_NOTIFY_VAL].handle;
    BleWifi_Ble_GetEntity()->ali_writehdl = gALISvcDb[ALI_IDX_DATA_WRITE_VAL].handle;
#endif
    BleWifi_Ble_GetEntity()->store.send_hdl = gBwpSvcDb[BWP_IDX_DATA_OUT_VAL].handle;
}

SHM_DATA static void BleWifi_Ble_ServerAppGattMsgHandler_AccessReadInd(TASK task, MESSAGEID id, MESSAGE message)
{
    BLEWIFI_INFO("APP-LE_GATT_MSG_ACCESS_READ_IND\r\n");
    BleWifi_Ble_GattHandleAccessRead((LE_GATT_MSG_ACCESS_READ_IND_T *)message);
}

SHM_DATA static void BleWifi_Ble_ServerAppGattMsgHandler_AccessWriteInd(TASK task, MESSAGEID id, MESSAGE message)
{
    BLEWIFI_INFO("APP-LE_GATT_MSG_ACCESS_WRITE_IND\r\n");
    BleWifi_Ble_GattHandleAccessWrite((LE_GATT_MSG_ACCESS_WRITE_IND_T *)message);
}

SHM_DATA static void BleWifi_Ble_ServerAppGattMsgHandler_ExchangeMtuInd(TASK task, MESSAGEID id, MESSAGE message)
{
    LE_GATT_MSG_EXCHANGE_MTU_IND_T *ind = (LE_GATT_MSG_EXCHANGE_MTU_IND_T *)message;
    BLEWIFI_INFO("APP-LE_GATT_MSG_EXCHANGE_MTU_IND client mtu = %d\r\n", ind->client_rx_mtu);
    LeGattExchangeMtuRsp(ind->conn_hdl, LE_ATT_MAX_MTU);
}

SHM_DATA static void BleWifi_Ble_ServerAppGattMsgHandler_ExchangeMtuCfm(TASK task, MESSAGEID id, MESSAGE message)
{
    LE_GATT_MSG_EXCHANGE_MTU_CFM_T *cfm = (LE_GATT_MSG_EXCHANGE_MTU_CFM_T *)message;
    BLEWIFI_INFO("APP-LE_GATT_MSG_EXCHANGE_MTU_CFM curr mtu = %d\r\n", cfm->current_rx_mtu);
    BleWifi_Ble_GetEntity()->curr_mtu = cfm->current_rx_mtu;
}

SHM_DATA static void BleWifi_Ble_ServerAppGattMsgHandler_ConfirmationCfm(TASK task, MESSAGEID id, MESSAGE message)
{
#ifdef BLEWIFI_SHOW_INFO
    LE_GATT_MSG_CONFIRMATION_CFM_T *cfm = (LE_GATT_MSG_CONFIRMATION_CFM_T *)message;
    BLEWIFI_INFO("APP-LE_GATT_MSG_CONFIRMATION_CFM curr handle = %d\r\n", cfm->handle);
#endif
#ifdef ALI_BLE_WIFI_PROVISION
    if(g_icnt==0){
        g_icnt+=1;
    }else{
        transport_txdone(TX_INDICATION, 1);
    }
#endif
}

SHM_DATA static void BleWifi_Ble_ServerAppGattMsgHandler_OperationTimeout(TASK task, MESSAGEID id, MESSAGE message)
{
#ifdef BLEWIFI_SHOW_INFO
    LE_GATT_MSG_OPERATION_TIMEOUT_T *ind = (LE_GATT_MSG_OPERATION_TIMEOUT_T *)message;
    BLEWIFI_INFO("APP-LE_GATT_MSG_OPERATION_TIMEOUT op = %x\r\n", ind->att_op);
#endif
}

void BleWifi_Ble_GattIndicateServiceChange(UINT16 conn_hdl)
{
	UINT16 len;
	UINT16 val;
	LE_ERR_STATE rc = LeGattGetAttrVal(gGattSvc, GATT_IDX_SERVICE_CHANGE_CFG, &len, &val);

	if (rc) return;

	if (val == LE_GATT_CLIENT_CFG_INDICATION)
	{
		UINT16 handle[2];

		LeGattGetAttrVal(gGattSvc, GATT_IDX_SERVICE_CHANGE_VAL, &len, handle);

		if (!handle[0] || !handle[1]) return;

		LeGattCharValIndicate(conn_hdl, LeGattGetAttrHandle(gGattSvc, GATT_IDX_SERVICE_CHANGE_VAL), 4, (UINT8 *)handle);
    }
}

void BleWifi_Ble_AppUpdateDevName(UINT8 *name, UINT16 len)
{
    LeGattChangeAttrVal(gGapSvc, GAP_IDX_DEVICE_NAME_VAL, len, name);
}
