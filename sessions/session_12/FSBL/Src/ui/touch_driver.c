#include "ui/touch_driver.h"
#include "stm32n6570_discovery.h"
#include "stm32n6570_discovery_ts.h"
#include <stdio.h>

void touch_driver_init(void)
{
    TS_Init_t ts_init;
    ts_init.Width = 800;
    ts_init.Height = 480;
    ts_init.Orientation = TS_SWAP_NONE;
    ts_init.Accuracy = 5;

    int32_t status = BSP_TS_Init(0, &ts_init);
    if (status != BSP_ERROR_NONE)
    {
        printf("Error: BSP_TS_Init failed with status %ld\n", status);
    }
    else
    {
        printf("BSP_TS_Init successful.\n");
    }
}

bool touch_driver_get_touch(uint32_t *x, uint32_t *y)
{
    TS_State_t ts_state = {0};
    if (BSP_TS_GetState(0, &ts_state) == BSP_ERROR_NONE)
    {
        if (ts_state.TouchDetected)
        {
            if (x) *x = ts_state.TouchX;
            if (y) *y = ts_state.TouchY;
            return true;
        }
    }
    return false;
}

void user_button_init(void)
{
    BSP_PB_Init(BUTTON_USER1, BUTTON_MODE_GPIO);
}

bool user_button_is_pressed(void)
{
    return (BSP_PB_GetState(BUTTON_USER1) == BUTTON_PRESSED);
}
