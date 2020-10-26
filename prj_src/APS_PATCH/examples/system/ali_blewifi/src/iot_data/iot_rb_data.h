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

#ifndef __IOT_RB_DATA_H__
#define __IOT_RB_DATA_H__

#include <stdint.h>
#include <stdbool.h>
#include "mw_fim_default_group13_project.h"
#include "blewifi_configuration.h"

#define IOT_RB_DATA_OK      0
#define IOT_RB_DATA_FAIL    1

#define IOT_RB_COUNT        (2) //(32)

#pragma anon_unions

#if 0
typedef struct{
    int32_t iSaturation; 
    int32_t iValue;
    int32_t iHue;   
}HSVColor_t;

typedef struct{
    int32_t iSaturation; 
    int32_t iValue;
    int32_t iHue; 
    int8_t  bEnable;
    int8_t  baReserved[3];
}ColorArr_t;

typedef struct{
    int32_t iRed; 
    int32_t iGreen;
    int32_t iBlue; 
}RGBColor_t;

#define LOCAL_TIMER_SIZE (10)
typedef struct{
    int8_t *bpTimer[LOCAL_TIMER_SIZE]; 
    uint8_t ubEnable[LOCAL_TIMER_SIZE]; 
    uint8_t ubIsValid[LOCAL_TIMER_SIZE]; 
    uint8_t ubLightSwitch[LOCAL_TIMER_SIZE];    
}LocalTimer_t;
#endif

typedef struct{
    #ifdef ALI_POST_CTRL
    uint32_t u32MsgId;
    uint16_t u16Flag;
    #else
    uint8_t u8Type;
    uint8_t ubaReserved[3];

    union
    {
        T_MwFim_GP13_Dev_Sched taSched[MW_FIM_GP13_DEV_SCHED_NUM];
    };
    #endif
} IoT_Properity_t;

/*
typedef struct{
    uint8_t ubLightSwitch;
    uint8_t ubaReserved[3];
    
    
    RGBColor_t stRgbColor;
    
    int8_t iWorkMode;
    int8_t ibaWmReserved[3];
    
    int32_t iColorTemperature;
    
    int32_t iBrightness;
    
    HSVColor_t stHuvColor;
    
    ColorArr_t stColorArr;
   
    
    int32_t iColorSpeed;
    
    uint8_t ubLightType;
    uint8_t ubaLtReserved[3];
    
    uint8_t ubLightMode;
    uint8_t ubaLmReserved[3];
    
    LocalTimer_t stLocalTimer;
    
    
  
} IoT_Properity_t;
*/
typedef struct
{
    uint32_t ulReadIdx;
    uint32_t ulWriteIdx;
    IoT_Properity_t taProperity[IOT_RB_COUNT];
} IoT_Ring_buffer_t;

void IoT_Ring_Buffer_Init(void);
uint8_t IoT_Ring_Buffer_Push(IoT_Properity_t *ptProperity);
uint8_t IoT_Ring_Buffer_Pop(IoT_Properity_t *ptProperity);
uint8_t IoT_Ring_Buffer_CheckEmpty(void);
void IoT_Ring_Buffer_ResetBuffer(void);
uint8_t IoT_Ring_Buffer_ReadIdxUpdate(void);

#endif // __IOT_RB_DATA_H__

