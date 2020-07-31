/******************************************************************************
*  Copyright 2017 - 2018, Opulinks Technology Ltd.
*  ---------------------------------------------------------------------------
*  Statement:
*  ----------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Opulinks Technology Ltd. (C) 2018
******************************************************************************/

#ifndef __ADA_UART_TRANSPORT_H__
#define __ADA_UART_TRANSPORT_H__

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void ada_ctrl_uart_input_impl(uint32_t data);

#ifdef __cplusplus
}
#endif

#endif /* __ADA_UART_TRANSPORT_H__ */
