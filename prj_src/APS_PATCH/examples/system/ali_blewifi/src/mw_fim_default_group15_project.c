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
#include "mw_fim_default_group15_project.h"


// Sec 2: Constant Definitions, Imported Symbols, miscellaneous


/********************************************
Declaration of data structure
********************************************/
// Sec 3: structure, uniou, enum, linked list


/********************************************
Declaration of Global Variables & Functions
********************************************/
// Sec 4: declaration of global variable
const T_MwFim_GP15_Reboot_Status g_tMwFimDefaultGp15RebootStatus = {0};

// the address buffer of reboot status
uint32_t g_ulaMwFimAddrBufferGP15RebootStatus[MW_FIM_GP15_REBOOT_STATUS_NUM];

const T_MwFim_GP15_Light_Status g_tMwFimDefaultGp15LightStatus = {1, 0};

// the address buffer of light status
uint32_t g_ulaMwFimAddrBufferGP15LightStatus[MW_FIM_GP15_LIGHT_STATUS_NUM];

const T_MwFim_GP15_Ctb_Status g_tMwFimDefaultGp15CtbStatus = {100};

// the address buffer of ctb status
uint32_t g_ulaMwFimAddrBufferGP15CtbStatus[MW_FIM_GP15_CTB_STATUS_NUM];

const hsv_t g_tMwFimDefaultGp15HsvStatus = {0, 100, 100};

// the address buffer of hsv status
uint32_t g_ulaMwFimAddrBufferGP15HsvStatus[MW_FIM_GP15_HSV_STATUS_NUM];

const T_MwFim_GP15_Scenes_Status g_tMwFimDefaultGp15ScenesStatus = {0, 100, 0, 0};

// the address buffer of scenes status
uint32_t g_ulaMwFimAddrBufferGP15ScenesStatus[MW_FIM_GP15_SCENES_STATUS_NUM];

const hsv_t g_tMwFimDefaultGp15ColorArrayStatus = {0};

// the address buffer of color array status
uint32_t g_ulaMwFimAddrBufferGP15ColorArrayStatus[MW_FIM_GP15_COLOR_ARRAY_NUM];

// the information table of group 15
const T_MwFimFileInfo g_taMwFimGroupTable15_project[] =
{
    {MW_FIM_IDX_GP15_PROJECT_REBOOT_STATUS,      MW_FIM_GP15_REBOOT_STATUS_NUM,  MW_FIM_GP15_REBOOT_STATUS_SIZE, (uint8_t*)&g_tMwFimDefaultGp15RebootStatus,     g_ulaMwFimAddrBufferGP15RebootStatus},

    {MW_FIM_IDX_GP15_PROJECT_LIGHT_STATUS,       MW_FIM_GP15_LIGHT_STATUS_NUM,   MW_FIM_GP15_LIGHT_STATUS_SIZE,  (uint8_t*)&g_tMwFimDefaultGp15LightStatus,      g_ulaMwFimAddrBufferGP15LightStatus},
    {MW_FIM_IDX_GP15_PROJECT_CTB_STATUS,         MW_FIM_GP15_CTB_STATUS_NUM,     MW_FIM_GP15_CTB_STATUS_SIZE,    (uint8_t*)&g_tMwFimDefaultGp15CtbStatus,        g_ulaMwFimAddrBufferGP15CtbStatus},
    {MW_FIM_IDX_GP15_PROJECT_HSV_STATUS,         MW_FIM_GP15_HSV_STATUS_NUM,     MW_FIM_GP15_HSV_STATUS_SIZE,    (uint8_t*)&g_tMwFimDefaultGp15HsvStatus,        g_ulaMwFimAddrBufferGP15HsvStatus},
    {MW_FIM_IDX_GP15_PROJECT_SCENES_STATUS,      MW_FIM_GP15_SCENES_STATUS_NUM,  MW_FIM_GP15_SCENES_STATUS_SIZE, (uint8_t*)&g_tMwFimDefaultGp15ScenesStatus,     g_ulaMwFimAddrBufferGP15ScenesStatus},
    {MW_FIM_IDX_GP15_PROJECT_COLOR_ARRAY_STATUS, MW_FIM_GP15_COLOR_ARRAY_NUM,    MW_FIM_GP15_COLOR_ARRAY_SIZE,   (uint8_t*)&g_tMwFimDefaultGp15ColorArrayStatus, g_ulaMwFimAddrBufferGP15ColorArrayStatus},

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
