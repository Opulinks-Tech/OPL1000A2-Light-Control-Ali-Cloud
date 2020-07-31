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

#include <stdlib.h>
#include <string.h>
#include "iot_rb_data.h"
#include "light_control.h"
#include "infra_config.h"

#ifdef ALI_POST_CTRL
#include "ali_linkkitsdk_decl.h"
#endif

SHM_DATA IoT_Ring_buffer_t g_tRBData;

#ifdef ADA_REMOTE_CTRL
osSemaphoreId g_tIotRbSem = NULL;

int iot_rb_lock(void)
{
    int iRet = -1;
    int iStatus = 0;

    if(!g_tIotRbSem)
    {
        //printf("[%s %d] sem is null\n", __func__, __LINE__);
        goto done;
    }

    iStatus = osSemaphoreWait(g_tIotRbSem, osWaitForever);

    if(iStatus != osOK)
    {
        //printf("[%s %d] osSemaphoreWait fail, status[%d]\n", __func__, __LINE__, iStatus);
        goto done;
    }

    iRet = 0;

done:
    return iRet;
}

int iot_rb_unlock(void)
{
    int iRet = -1;

    if(!g_tIotRbSem)
    {
        //printf("[%s %d] sem is null\n", __func__, __LINE__);
        goto done;
    }
    
    if(osSemaphoreRelease(g_tIotRbSem) != osOK)
    {
        //printf("[%s %d] osSemaphoreWait fail\n", __func__, __LINE__);
        goto done;
    }

    iRet = 0;

done:
    return iRet;
}
#else
#define iot_rb_lock(...)
#define iot_rb_unlock(...)
#endif

void IoT_Ring_Buffer_Init(void)
{
    #ifdef ADA_REMOTE_CTRL
    if(g_tIotRbSem == NULL)
    {
        osSemaphoreDef_t tSemDef = {0};

        g_tIotRbSem = osSemaphoreCreate(&tSemDef, 1);
    }
    #endif
    
    #ifdef ALI_POST_CTRL
    memset(&g_tRBData, 0, sizeof(IoT_Ring_buffer_t));
    #else
    IoT_Ring_Buffer_ResetBuffer();
    #endif
}

uint8_t IoT_Ring_Buffer_Push(IoT_Properity_t *ptProperity)
{
    uint32_t ulWriteNext;
    
    if (ptProperity == NULL)
        return IOT_RB_DATA_FAIL;

    iot_rb_lock();

    // full, ulWriteIdx + 1 == ulReadIdx
    ulWriteNext = (g_tRBData.ulWriteIdx + 1) % IOT_RB_COUNT;
    
    // Read index alway prior to write index
    if (ulWriteNext == g_tRBData.ulReadIdx)
    {
        // discard the oldest data, and read index move forware one step.
        g_tRBData.ulReadIdx = (g_tRBData.ulReadIdx + 1) % IOT_RB_COUNT;
    }

    // update the temperature data to write index
	memcpy(&(g_tRBData.taProperity[g_tRBData.ulWriteIdx]), ptProperity, sizeof(IoT_Properity_t));
    g_tRBData.ulWriteIdx = ulWriteNext;

    iot_rb_unlock();

    return IOT_RB_DATA_OK;
}

uint8_t IoT_Ring_Buffer_Pop(IoT_Properity_t *ptProperity)
{
    uint8_t bRet = IOT_RB_DATA_FAIL;
    
    if (ptProperity == NULL)
        return IOT_RB_DATA_FAIL;

    iot_rb_lock();

    // empty, ulWriteIdx == ulReadIdx
    if (g_tRBData.ulWriteIdx == g_tRBData.ulReadIdx)
        goto done;

	memcpy(ptProperity, &(g_tRBData.taProperity[g_tRBData.ulReadIdx]), sizeof(IoT_Properity_t));
	
    #ifdef ALI_POST_CTRL
    g_tRBData.ulReadIdx = (g_tRBData.ulReadIdx + 1) % IOT_RB_COUNT;
    #endif
	
    bRet = IOT_RB_DATA_OK;

done:
    iot_rb_unlock();
    return bRet;
}

uint8_t IoT_Ring_Buffer_CheckEmpty(void)
{
    uint8_t u8Ret = IOT_RB_DATA_FAIL;

    iot_rb_lock();

    // empty, ulWriteIdx == ulReadIdx
    if (g_tRBData.ulWriteIdx == g_tRBData.ulReadIdx)
    {
        u8Ret = IOT_RB_DATA_OK;
    }

    iot_rb_unlock();
    return u8Ret;
}

void IoT_Ring_Buffer_ResetBuffer(void)
{
    uint32_t i = 0;

    iot_rb_lock();

    for(i = 0; i < IOT_RB_COUNT; i++)
    {
        memset(&(g_tRBData.taProperity[i]), 0, sizeof(IoT_Properity_t));
    }

    g_tRBData.ulReadIdx = 0;
    g_tRBData.ulWriteIdx = 0;

    iot_rb_unlock();
    return;
}

uint8_t IoT_Ring_Buffer_ReadIdxUpdate(void)
{
    uint32_t u32Cnt = 0;

    iot_rb_lock();

    u32Cnt = (g_tRBData.ulWriteIdx + IOT_RB_COUNT - g_tRBData.ulReadIdx) % IOT_RB_COUNT;

    if(u32Cnt)
    {
        g_tRBData.ulReadIdx = (g_tRBData.ulReadIdx + 1) % IOT_RB_COUNT;
    }

    iot_rb_unlock();

    return IOT_RB_DATA_OK;
}

