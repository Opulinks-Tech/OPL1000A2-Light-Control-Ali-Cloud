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

/***********************
Head Block of The File
***********************/
#ifndef _MW_FIM_DEFAULT_GROUP17_PROJECT_H_
#define _MW_FIM_DEFAULT_GROUP17_PROJECT_H_

#ifdef __cplusplus
extern "C" {
#endif

// Sec 0: Comment block of the file


// Sec 1: Include File
#include "mw_fim.h"
#include "infra_defs.h"


// Sec 2: Constant Definitions, Imported Symbols, miscellaneous
// the file ID
// xxxx_xxxx_xxxx_xxxx_xxxx_xxxx_xxxx_xxxx
// ^^^^ ^^^^ Zone (0~3)
//           ^^^^ ^^^^ Group (0~8), 0 is reserved for swap
//                     ^^^^ ^^^^ ^^^^ ^^^^ File ID, start from 0
typedef enum
{
    MW_FIM_IDX_GP17_PROJECT_START = 0x01070000,             // the start IDX of group 17
    
    MW_FIM_IDX_GP17_PROJECT_ALIYUN_INFO,
    MW_FIM_IDX_GP17_PROJECT_ALIYUN_MQTT_CFG,
    
    MW_FIM_IDX_GP17_PROJECT_MAX
} E_MwFimIdxGroup17_Project;


/******************************
Declaration of data structure
******************************/
// Sec 3: structure, uniou, enum, linked list
// Aliyun Info
typedef struct
{
    uint32_t ulRegionID;

} T_MwFim_GP17_AliyunInfo;

#define MW_FIM_GP17_MQTT_URL_SIZE   128

typedef struct
{
    char s8aUrl[MW_FIM_GP17_MQTT_URL_SIZE];

} T_MwFim_GP17_AliyunMqttCfg;

#define MW_FIM_GP17_ALIYUN_INFO_SIZE  sizeof(T_MwFim_GP17_AliyunInfo)
#define MW_FIM_GP17_ALIYUN_INFO_NUM   1

#define MW_FIM_GP17_ALIYUN_MQTT_CFG_SIZE    sizeof(T_MwFim_GP17_AliyunMqttCfg)
#define MW_FIM_GP17_ALIYUN_MQTT_CFG_NUM     1

/********************************************
Declaration of Global Variables & Functions
********************************************/
// Sec 4: declaration of global variable
extern const T_MwFimFileInfo g_taMwFimGroupTable17_project[];

extern const T_MwFim_GP17_AliyunInfo g_tMwFimDefaultGp17AliyunInfo;
extern const T_MwFim_GP17_AliyunMqttCfg g_tMwFimDefaultGp17AliyunMqttCfg;


// Sec 5: declaration of global function prototype


/***************************************************
Declaration of static Global Variables & Functions
***************************************************/
// Sec 6: declaration of static global variable


// Sec 7: declaration of static function prototype


#ifdef __cplusplus
}
#endif

#endif // _MW_FIM_DEFAULT_GROUP17_PROJECT_H_
