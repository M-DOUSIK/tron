# Session 02 Notes - UART Debug Logging

## UART Peripheral and Pins
For this session, **USART1** was chosen as it is internally routed to the onboard ST-LINK V3EC Virtual COM Port (VCP) on the STM32N6570-DK. The following exact pins were mapped and configured:
- **PE5**: USART1_TX (Alternate Function 7)
- **PE6**: USART1_RX (Alternate Function 7)

This setup allows `printf` to output directly to the serial terminal running at 115200 baud (8N1) via the ST-LINK USB connection.

## Implementation Details
1. **`DEBUG_LOG` Macro**: Created in `debug_log.h`. This macro wraps `printf` and can be disabled globally by setting `ENABLE_DEBUG_LOG` to `0`. This satisfies the constraint that future sessions won't leak sensitive data via UART if deactivated.
2. **Printf Retargeting**: The `__io_putchar` function was implemented in `main.c` to transmit characters via `HAL_UART_Transmit(&huart1, ...)`. We also disabled standard output buffering (`setvbuf(stdout, NULL, _IONBF, 0);`) to prevent batching and ensure prompt logging.

## Verification
- We print `"MedSight Session 02 Active"` at startup.
- The tick counter increments reliably once per second and is sent over USART1.
- **LED Blink Confirmation**: The `PO1` LED from Session 01 is re-configured as an output and continues to toggle inside the main `while(1)` loop, synchronously with the UART tick logs, confirming both subsystems function seamlessly together.
