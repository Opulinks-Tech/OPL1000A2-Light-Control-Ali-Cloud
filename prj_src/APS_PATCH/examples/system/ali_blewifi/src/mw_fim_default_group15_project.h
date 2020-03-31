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
#ifndef _MW_FIM_DEFAULT_GROUP15_PROJECT_H_
#define _MW_FIM_DEFAULT_GROUP15_PROJECT_H_

#ifdef __cplusplus
extern "C" {
#endif

// Sec 0: Comment block of the file


// Sec 1: Include File
#include "mw_fim.h"
#include "light_control.h"


// Sec 2: Constant Definitions, Imported Symbols, miscellaneous
// the file ID
// xxxx_xxxx_xxxx_xxxx_xxxx_xxxx_xxxx_xxxx
// ^^^^ ^^^^ Zone (0~3)
//           ^^^^ ^^^^ Group (0~8), 0 is reserved for swap
//                     ^^^^ ^^^^ ^^^^ ^^^^ File ID, start from 0
typedef enum
{
    MW_FIM_IDX_GP15_PROJECT_START = 0x01050000,             // the start IDX of group 15

    MW_FIM_IDX_GP15_PROJECT_REBOOT_STATUS,

    MW_FIM_IDX_GP15_PROJECT_LIGHT_STATUS,
    MW_FIM_IDX_GP15_PROJECT_CTB_STATUS,
    MW_FIM_IDX_GP15_PROJECT_HSV_STATUS,
    MW_FIM_IDX_GP15_PROJECT_SCENES_STATUS,
    MW_FIM_IDX_GP15_PROJECT_COLOR_ARRAY_STATUS,

    MW_FIM_IDX_GP15_PROJECT_MAX
} E_MwFimIdxGroup15_Project;

/******************************
Declaration of data structure
******************************/
// Sec 3: structure, uniou, enum, linked list
typedef struct
{
    uint8_t u8Reason;
} T_MwFim_GP15_Reboot_Status;

#define MW_FIM_GP15_REBOOT_STATUS_SIZE      sizeof(T_MwFim_GP15_Reboot_Status)
#define MW_FIM_GP15_REBOOT_STATUS_NUM       1

typedef struct
{
    uint8_t u8LightSwitch;
    uint8_t u8LightMode;
} T_MwFim_GP15_Light_Status;

#define MW_FIM_GP15_LIGHT_STATUS_SIZE       sizeof(T_MwFim_GP15_Light_Status)
#define MW_FIM_GP15_LIGHT_STATUS_NUM        1

typedef struct
{
    uint8_t u8Brightness;
} T_MwFim_GP15_Ctb_Status;

#define MW_FIM_GP15_CTB_STATUS_SIZE         sizeof(T_MwFim_GP15_Ctb_Status)
#define MW_FIM_GP15_CTB_STATUS_NUM          1

//hsv_t

#define MW_FIM_GP15_HSV_STATUS_SIZE         sizeof(hsv_t)
#define MW_FIM_GP15_HSV_STATUS_NUM          1

typedef struct
{
    uint8_t u8WorkMode;
    uint8_t u8ColorSpeed;
    uint16_t u16ColorNum;
    uint8_t u8ColorArrayEnable;
} T_MwFim_GP15_Scenes_Status;

#define MW_FIM_GP15_SCENES_STATUS_SIZE      sizeof(T_MwFim_GP15_Scenes_Status)
#define MW_FIM_GP15_SCENES_STATUS_NUM       1

//hsv_t

#define MW_FIM_GP15_COLOR_ARRAY_SIZE        sizeof(hsv_t)
#define MW_FIM_GP15_COLOR_ARRAY_NUM         LIGHT_COLOR_NUM_MAX

/********************************************
Declaration of Global Variables & Functions
********************************************/
// Sec 4: declaration of global variable
extern const T_MwFimFileInfo g_taMwFimGroupTable15_project[];

extern const T_MwFim_GP15_Reboot_Status g_tMwFimDefaultGp15RebootStatus;
extern const T_MwFim_GP15_Light_Status g_tMwFimDefaultGp15LightStatus;
extern const T_MwFim_GP15_Ctb_Status g_tMwFimDefaultGp15CtbStatus;
extern const hsv_t g_tMwFimDefaultGp15HsvStatus;
extern const T_MwFim_GP15_Scenes_Status g_tMwFimDefaultGp15ScenesStatus;
extern const hsv_t g_tMwFimDefaultGp15ColorArrayStatus;


// Sec 5: declaration of global function prototype


/***************************************************
Declaration of static Global Variables & Functions
***************************************************/
// Sec 6: declaration of static global variable


// Sec 7: declaration of static function prototype


#ifdef __cplusplus
}
#endif

#endif // _MW_FIM_DEFAULT_GROUP15_PROJECT_H_
