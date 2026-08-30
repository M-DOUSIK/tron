# Session 01 Notes - Board Bring-Up: LED Blink

## Exact LED GPIO Pin Used
The user LED (LED1) pin configured for this session is **PO1** (Port O, Pin 1). This was confirmed for the STM32N6570-DK board schematic via web reference. Another user LED (LED2) is available at PG10, but PO1 was chosen for this basic toggle example.
The pin is configured as `GPIO_Output` with the `Appli` pin attribute context to be accessed by our main application.

## Clock Configuration Chosen
The default peripherals clock configured in the STM32CubeMX generated template project was retained, utilizing the `HSI` (High-Speed Internal) oscillator which is mapped to `CKPER` (peripherals clock source). The system operates on its default clock tree without introducing complex clock multipliers in this bare-metal session.

## Assumptions Made About the Board Pinout
It is assumed that the LED is driven active-high (or active-low such that toggling works symmetrically), meaning `HAL_GPIO_TogglePin` with a symmetric 500ms delay (`HAL_Delay(500)`) accurately provides the 1Hz blink rate. We also assume no RTOS or extra peripherals are configured, maintaining strict adherence to the session constraints. PO1 was safely selected as it was initially a free, unassigned pin in the template `.ioc` file.
