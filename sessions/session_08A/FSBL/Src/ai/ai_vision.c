#include "ai_vision.h"
#include "ms_osal.h"
#include <stdio.h>
#include "stai_network.h"
#include "network.h"
#include "stm32n6xx_hal.h"
#include "sd_logger.h"

static stai_network my_network[STAI_NETWORK_CONTEXT_SIZE] __attribute__((aligned(STAI_NETWORK_CONTEXT_ALIGNMENT)));

// ST Edge AI runtime manages the actual input/output memory buffers internally.
// We just need pointers to them to read/write.
static stai_ptr stai_input[STAI_NETWORK_IN_NUM];
static stai_ptr stai_output[STAI_NETWORK_OUT_NUM];

void ai_vision_init(void) {
    extern void aiPreInitialize(void);
    aiPreInitialize();
    
    stai_return_code err = stai_runtime_init();
    if (err != STAI_SUCCESS) {
        printf("ST Edge AI runtime init failed: %d\n", err);
        return;
    }

    err = stai_network_init(my_network);
    if (err != STAI_SUCCESS) {
        printf("ST Edge AI network init failed: %d\n", err);
        return;
    }
    printf("ST Edge AI network initialized successfully!\n");
    
    stai_size in_length = STAI_NETWORK_IN_NUM;
    err = stai_network_get_inputs(my_network, stai_input, &in_length);
    if (err != STAI_SUCCESS) printf("get_inputs failed: %d\n", err);

    stai_size out_length = STAI_NETWORK_OUT_NUM;
    err = stai_network_get_outputs(my_network, stai_output, &out_length);
    if (err != STAI_SUCCESS) printf("get_outputs failed: %d\n", err);
}

void ai_vision_task(void *argument) {
    (void)argument;
    ai_vision_init();

    uint32_t tick = 0;
    while(1) {
        // We can write to stai_input[0] here if we had camera data

        uint32_t start_time = HAL_GetTick();
        printf("Running network...\n");
        stai_return_code err = stai_network_run(my_network, STAI_MODE_SYNC);
        printf("Network run returned %d\n", err);
        uint32_t end_time = HAL_GetTick();
        uint32_t latency = end_time - start_time;

        if (err != STAI_SUCCESS) {
            printf("ST Edge AI network run failed: %d\n", err);
        } else {
            // Find max class
            int8_t* out_data = (int8_t*)stai_output[0];
            int max_idx = 0;
            int8_t max_val = out_data[0];
            for(int i = 1; i < 2; i++) {
                if(out_data[i] > max_val) {
                    max_val = out_data[i];
                    max_idx = i;
                }
            }
            if(tick % 5 == 0) {
                printf("AI Inference tick %lu: Class %d, score %d | Latency: %lu ms. NPU path executed!\\n", tick, max_idx, max_val, latency);
                char log_buf[128];
                snprintf(log_buf, sizeof(log_buf), "AI Prediction: Class=%d Score=%d Latency=%lu", max_idx, max_val, latency);
                SD_Log_Event_Async(log_buf);
            }
        }
        tick++;
        osal_delay_ms(1000); // 1 Hz for this throwaway test
    }
}
