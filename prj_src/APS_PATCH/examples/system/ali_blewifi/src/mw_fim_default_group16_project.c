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
// Sec 0: Comment block of the file


// Sec 1: Include File
#include "blewifi_configuration.h"

#ifdef BLEWIFI_SCHED_EXT

#include "mw_fim_default_group16_project.h"


// Sec 2: Constant Definitions, Imported Symbols, miscellaneous


/********************************************
Declaration of data structure
********************************************/
// Sec 3: structure, uniou, enum, linked list


/********************************************
Declaration of Global Variables & Functions
********************************************/
// Sec 4: declaration of global variable
const T_MwFim_GP16_Dev_Sched_Ext g_tMwFimDefaultGp16DevSchedExt = 
{
    28800 //s32TimeZone
};

// the address buffer of device schedule extention
uint32_t g_ulaMwFimAddrBufferGP16DevSchedExt[MW_FIM_GP16_DEV_SCHED_EXT_NUM];

// the information table of group 16
const T_MwFimFileInfo g_taMwFimGroupTable16_project[] =
{
    {MW_FIM_IDX_GP16_PROJECT_DEV_SCHED_EXT, MW_FIM_GP16_DEV_SCHED_EXT_NUM, MW_FIM_GP16_DEV_SCHED_EXT_SIZE, (uint8_t*)&g_tMwFimDefaultGp16DevSchedExt, g_ulaMwFimAddrBufferGP16DevSchedExt},

    // the end, don't modify and remove it
    {0xFFFFFFFF,            0x00,              0x00,               NULL,                            NULL}
};


// Sec 5: declaration of global function prototype


/***************************************************
Declaration of static Global Variables & Functions
***************************************************/
// Sec 6: declaration of static global variable


// Sec 7: declaration of static function prototype


/***********
C Functions
***********/
// Sec 8: C Functions

#endif //#ifdef BLEWIFI_SCHED_EXT

