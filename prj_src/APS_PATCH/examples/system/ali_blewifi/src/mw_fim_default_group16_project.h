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
#ifndef _MW_FIM_DEFAULT_GROUP16_PROJECT_H_
#define _MW_FIM_DEFAULT_GROUP16_PROJECT_H_

#ifdef __cplusplus
extern "C" {
#endif

// Sec 0: Comment block of the file


// Sec 1: Include File
#include "mw_fim.h"
#include "blewifi_configuration.h"

#ifdef BLEWIFI_SCHED_EXT

#include "mw_fim_default_group13_project.h"


// Sec 2: Constant Definitions, Imported Symbols, miscellaneous
// the file ID
// xxxx_xxxx_xxxx_xxxx_xxxx_xxxx_xxxx_xxxx
// ^^^^ ^^^^ Zone (0~3)
//           ^^^^ ^^^^ Group (0~8), 0 is reserved for swap
//                     ^^^^ ^^^^ ^^^^ ^^^^ File ID, start from 0
typedef enum
{
    MW_FIM_IDX_GP16_PROJECT_START = 0x01060000,             // the start IDX of group 16

    MW_FIM_IDX_GP16_PROJECT_DEV_SCHED_EXT,

    MW_FIM_IDX_GP16_PROJECT_MAX
} E_MwFimIdxGroup16_Project;

/******************************
Declaration of data structure
******************************/
// Sec 3: structure, uniou, enum, linked list
typedef struct
{
    int32_t s32TimeZone; // seconds
} T_MwFim_GP16_Dev_Sched_Ext;

#define MW_FIM_GP16_DEV_SCHED_EXT_SIZE      sizeof(T_MwFim_GP16_Dev_Sched_Ext)
#define MW_FIM_GP16_DEV_SCHED_EXT_NUM       MW_FIM_GP13_DEV_SCHED_NUM

/********************************************
Declaration of Global Variables & Functions
********************************************/
// Sec 4: declaration of global variable
extern const T_MwFimFileInfo g_taMwFimGroupTable16_project[];

extern const T_MwFim_GP16_Dev_Sched_Ext g_tMwFimDefaultGp16DevSchedExt;


// Sec 5: declaration of global function prototype


/***************************************************
Declaration of static Global Variables & Functions
***************************************************/
// Sec 6: declaration of static global variable


// Sec 7: declaration of static function prototype

#endif //#ifdef BLEWIFI_SCHED_EXT

#ifdef __cplusplus
}
#endif

#endif // _MW_FIM_DEFAULT_GROUP16_PROJECT_H_
