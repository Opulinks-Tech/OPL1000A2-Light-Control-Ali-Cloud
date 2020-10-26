#ifndef __LOCAL_SYMPHONY_H__
#define __LOCAL_SYMPHONY_H__

#include <stdio.h>
#include <stdint.h>

#include "at_cmd_common.h"

#define SYMP_ENERGY_MIN                 0
#define SYMP_ENERGY_MAX                 (512 * 512)

/* symphony params */
#define SYMP_VALUE_STRATEGY             1
#define SYMP_DEVIATION_STRATEGY         2
#define SYMP_STRATEGY                   SYMP_DEVIATION_STRATEGY

#define SYMP_BLOCK_SIZE		        	8
#define SYMP_BLOCKS_NUM		        	10
#define SYMP_SAMPLE_RATE		        80
#define SYMP_DEFAULT_ENERGY_THRESHOLD   (SYMP_ENERGY_MAX * 0.0003)
#define SYMP_DEFAULT_DEVIATE_THRESHOLD  1.2
#define SYMP_DEFAULT_MIC_OFFSET         512
#define SYMP_MIC_MOV_AVG_NUM            128

/* hardwared & os params */
#define SYMP_MIC_INPUT_IO               GPIO_IDX_07
#define SYMP_ADC_AVG_NUM                1
#define SYMP_FEED_TIMER                 0
#define SYMP_TASK_PRIOIRTY              osPriorityNormal
#define SYMP_TASK_STACK_SIZE            512

/* debug params */
#define SYMP_ASSERT(cond)               { if (!cond) { SYMP_LOG("[Symphony] asserted from %s:%d\n", __FILE__, __LINE__); while (1); } }
#define SYMP_LOG(x...)                  msg_print_uart1(x)
#define SYMP_DEBUG_EN                   1

typedef void (*Symp_BlinkEvent_t)(uint32_t strength);

/* public functions */
void Symp_Init(Symp_BlinkEvent_t);
void Symp_SetTriggerThreshold(uint32_t energy, uint32_t deviation);
void Symp_Start(void);
void Symp_Stop(void);

#endif
