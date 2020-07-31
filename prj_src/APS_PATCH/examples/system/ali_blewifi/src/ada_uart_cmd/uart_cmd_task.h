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

#ifndef __UART_CMD_TASK_H__
#define __UART_CMD_TASK_H__

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "cmsis_os.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UART_CMD_TASK_PRIORITY      osPriorityNormal

#ifndef UART_COMMAND_EVENT
#define UART_COMMAND_EVENT          (0x0001)
#endif

#ifndef UART_QUEUE_SIZE
#define UART_QUEUE_SIZE             (10)
#endif

#define UART_RBUF_SIZE              128

typedef struct
{
    int cursor;
    int length;
    char data[UART_RBUF_SIZE];
} uart_buffer_t;

/**
 * @brief Message Structure for UART CMD Task
 *
 */
typedef struct {
    uint32_t event;
	uint32_t length;
	uint8_t *param;
} uart_event_msg_t;

/**
 * @brief UART peripheral number
 */
typedef enum {
    UART_NUM_0 = 0x0,
    UART_NUM_1 = 0x1,
    UART_NUM_MAX,
} uart_port_t;


/**
  * @brief  Application specified command process callback function
  *
  */
typedef int (*uart_cmd_process_cb_t)(void *data, uint16_t length);
typedef void (*uart_rx_isr_cb_t)(uint32_t);


void uart_cmd_init(uart_port_t uart_num, uint32_t baud_rate, uart_rx_isr_cb_t isr_callback);
void uart_cmd_handler_register(uart_cmd_process_cb_t handler, void *param);
int  uart_cmd_task_send(uart_event_msg_t *msg);

#ifdef __cplusplus
}
#endif

#endif /* __UART_CMD_TASK_H__ */
