
#ifdef BLEWIFI_LOCAL_SYMPHONY

#include <stdlib.h>

#include "local-symphony.h"

#include "hal_tmr.h"
#include "hal_auxadc_internal.h"
#include "hal_auxadc_patch.h"
#include "cmsis_os.h"

#include "hal_vic.h"

#define SYMP_SAMPLE_INTERVAL	(1000000 / SYMP_SAMPLE_RATE)

/* symphony algorithm used */
struct
{
    uint32_t blocks[SYMP_BLOCKS_NUM];
    uint16_t block[SYMP_BLOCK_SIZE];
    uint32_t blocks_energy;
    uint32_t energy_threshold;
    float deviate_threshold;
    float mic_offset;

    uint8_t blocks_count;
    uint8_t blocks_index;
    uint8_t block_index;
    uint8_t enabled;
    uint8_t debug_en;

    Symp_BlinkEvent_t blink_event;

} symp_data;

/* os related resources */
osThreadId sympTaskId;
osSemaphoreId sympLockId;

/* symphony algorithm functions */
static inline void Symp_BookNextSample()
{
    Hal_Tmr_Start(SYMP_FEED_TIMER, SYMP_SAMPLE_INTERVAL);
}

static void Symp_Feed(uint32_t u32TmrIdx)
{
    uint32_t adc_val;

    // NOTE: ADC sample value
    reg_write(0x400090cc, 0);
    reg_write(0x400090cc, 1);
    adc_val = reg_read(0x400090D0) & 0x3FF;
    adc_val = (uint32_t)( ( Hal_Aux_AdcMiniVolt_Convert( adc_val ) * 1023) + 3000/2 ) / 3000;

    symp_data.block[symp_data.block_index++] = abs((int) (symp_data.mic_offset - adc_val));

    symp_data.mic_offset = (symp_data.mic_offset * (SYMP_MIC_MOV_AVG_NUM - 1) + adc_val) / SYMP_MIC_MOV_AVG_NUM;

    if (symp_data.block_index == SYMP_BLOCK_SIZE)
    {
        symp_data.block_index = 0;
        osSemaphoreRelease(sympLockId);
    }
    else if (symp_data.enabled)
    {
        Symp_BookNextSample();
    }
}

#if SYMP_DEBUG_EN
void Symp_DumpBlocks()
{
    int i;

    SYMP_LOG("[Symphony] Dump Last Block\n");
    for (i = 0; i < SYMP_BLOCK_SIZE; i++)
        SYMP_LOG("%d, ", symp_data.block[i]);

    SYMP_LOG("\n[Symphony] Dump All Blocks\n");
    for (i = 0; i < SYMP_BLOCKS_NUM; i++)
        SYMP_LOG("%d, ", symp_data.blocks[i]);
    SYMP_LOG("\nblocks_index: %d, blocks_engery: %d, mic_oft: %.2f\n", symp_data.blocks_index, symp_data.blocks_energy, symp_data.mic_offset);
}
#endif

static inline uint32_t Symp_Pulse()
{
    int i;
    uint32_t ret = 0;
    uint32_t block_energy = 0;
    
    for (i = 0; i < SYMP_BLOCK_SIZE; i++)
    {
        block_energy += symp_data.block[i] * symp_data.block[i];
    }
    block_energy /= SYMP_BLOCK_SIZE;

    if (symp_data.blocks_count < SYMP_BLOCKS_NUM)
    {
        symp_data.blocks_count++;
    }
    else if (block_energy * symp_data.deviate_threshold > (symp_data.blocks_energy / SYMP_BLOCKS_NUM) && block_energy > symp_data.energy_threshold)
    {
        ret = block_energy;
    }

    symp_data.blocks_energy = symp_data.blocks_energy - symp_data.blocks[symp_data.blocks_index] + block_energy;
    symp_data.blocks[symp_data.blocks_index++] = block_energy;
    
    if (symp_data.blocks_index == SYMP_BLOCKS_NUM)
    {
        symp_data.blocks_index = 0;
    }

#if SYMP_DEBUG_EN
    if (symp_data.debug_en)
    {
        Symp_DumpBlocks();
    }
#endif

    return ret;
}

/* hardware initialization */
static inline void Symp_InitAdc()
{
    Hal_Aux_Init();
    Hal_Aux_AdcCal_Init();
    Hal_Aux_AdcUpdateCtrlReg(1);
    g_ulHalAux_AverageCount = SYMP_ADC_AVG_NUM;
    reg_write(0x40001088, reg_read(0x40001088)|1 );

    Hal_Aux_SourceSelect(HAL_AUX_SRC_GPIO, SYMP_MIC_INPUT_IO);
}

static inline void Symp_InitTimer()
{
    Hal_Tmr_Init(SYMP_FEED_TIMER);
    Hal_Tmr_CallBackFuncSet(SYMP_FEED_TIMER, Symp_Feed);
}

/* os related functions */
static inline void Symp_CreateLock()
{
    osSemaphoreDef_t osSemaphoreDef;

    sympLockId = osSemaphoreCreate(&osSemaphoreDef, 1);
    SYMP_ASSERT(sympLockId);

    osSemaphoreWait(sympLockId, 0);
}

static void Symp_Task(void *argument)
{
    while (1)
    {
        if (osSemaphoreWait(sympLockId, osWaitForever) >= 0 && symp_data.enabled)        
        {
            uint32_t energy = Symp_Pulse();

            if (energy > 0 && symp_data.blink_event != NULL)
            {
                symp_data.blink_event(energy);
            }

            Symp_BookNextSample();
        }
    }
}

static inline void Symp_CreateTask()
{
    osThreadDef_t osThreadDef;

    osThreadDef.name = "Symphony";
    osThreadDef.pthread = Symp_Task;
    osThreadDef.tpriority = SYMP_TASK_PRIOIRTY;
    osThreadDef.instances = 1;
    osThreadDef.stacksize = SYMP_TASK_STACK_SIZE;

    sympTaskId = osThreadCreate(&osThreadDef, NULL);
    SYMP_ASSERT(sympTaskId);
}

/* public functions */
void Symp_Init(Symp_BlinkEvent_t event)
{
    symp_data.energy_threshold = SYMP_DEFAULT_ENERGY_THRESHOLD;
    symp_data.deviate_threshold = SYMP_DEFAULT_DEVIATE_THRESHOLD;
    symp_data.mic_offset = SYMP_DEFAULT_MIC_OFFSET;
    symp_data.blink_event = event;
    symp_data.debug_en = 0;

    Symp_InitAdc();
    Symp_InitTimer();
    Symp_CreateLock();
    Symp_CreateTask();
}

void Symp_SetTriggerThreshold(uint32_t energy, uint32_t deviation)
{
    symp_data.energy_threshold = energy;
    symp_data.deviate_threshold = deviation;
}

void Symp_Start()
{
    symp_data.blocks_energy = 0;
    symp_data.blocks_count = 0;
    symp_data.blocks_index = 0;
    symp_data.block_index = 0;
    symp_data.enabled = 1;

    Symp_BookNextSample();
}

void Symp_Stop()
{
    symp_data.enabled = 0;

    Hal_Tmr_Stop(SYMP_FEED_TIMER);
}

#endif //#ifdef BLEWIFI_LOCAL_SYMPHONY

