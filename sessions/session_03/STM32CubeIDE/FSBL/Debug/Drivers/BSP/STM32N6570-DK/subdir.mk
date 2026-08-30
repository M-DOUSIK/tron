################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/BSP/STM32N6570-DK/stm32n6570_discovery.c \
C:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/BSP/STM32N6570-DK/stm32n6570_discovery_bus.c \
C:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/BSP/STM32N6570-DK/stm32n6570_discovery_xspi.c 

OBJS += \
./Drivers/BSP/STM32N6570-DK/stm32n6570_discovery.o \
./Drivers/BSP/STM32N6570-DK/stm32n6570_discovery_bus.o \
./Drivers/BSP/STM32N6570-DK/stm32n6570_discovery_xspi.o 

C_DEPS += \
./Drivers/BSP/STM32N6570-DK/stm32n6570_discovery.d \
./Drivers/BSP/STM32N6570-DK/stm32n6570_discovery_bus.d \
./Drivers/BSP/STM32N6570-DK/stm32n6570_discovery_xspi.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/BSP/STM32N6570-DK/stm32n6570_discovery.o: C:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/BSP/STM32N6570-DK/stm32n6570_discovery.c Drivers/BSP/STM32N6570-DK/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m55 -std=gnu11 -g3 -DSTM32N657xx -DUSE_FULL_ASSERT -DUSE_HAL_DRIVER -c -IC:/Users/Dousik/Workspace/TRON/sessions/session_03/FSBL/Inc -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/BSP/STM32N6570-DK -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/STM32N6xx_HAL_Driver/Inc -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/CMSIS/Device/ST/STM32N6xx/Include -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/CMSIS/Include -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/BSP/Components/imx335 -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/BSP/Components/Common -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/BSP/Components/rk050hr18 -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Middlewares/ST/STM32_ISP_Library/evision/Inc -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Middlewares/ST/STM32_ISP_Library/isp/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -mcmse -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Drivers/BSP/STM32N6570-DK/stm32n6570_discovery_bus.o: C:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/BSP/STM32N6570-DK/stm32n6570_discovery_bus.c Drivers/BSP/STM32N6570-DK/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m55 -std=gnu11 -g3 -DSTM32N657xx -DUSE_FULL_ASSERT -DUSE_HAL_DRIVER -c -IC:/Users/Dousik/Workspace/TRON/sessions/session_03/FSBL/Inc -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/BSP/STM32N6570-DK -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/STM32N6xx_HAL_Driver/Inc -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/CMSIS/Device/ST/STM32N6xx/Include -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/CMSIS/Include -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/BSP/Components/imx335 -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/BSP/Components/Common -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/BSP/Components/rk050hr18 -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Middlewares/ST/STM32_ISP_Library/evision/Inc -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Middlewares/ST/STM32_ISP_Library/isp/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -mcmse -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Drivers/BSP/STM32N6570-DK/stm32n6570_discovery_xspi.o: C:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/BSP/STM32N6570-DK/stm32n6570_discovery_xspi.c Drivers/BSP/STM32N6570-DK/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m55 -std=gnu11 -g3 -DSTM32N657xx -DUSE_FULL_ASSERT -DUSE_HAL_DRIVER -c -IC:/Users/Dousik/Workspace/TRON/sessions/session_03/FSBL/Inc -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/BSP/STM32N6570-DK -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/STM32N6xx_HAL_Driver/Inc -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/CMSIS/Device/ST/STM32N6xx/Include -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/CMSIS/Include -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/BSP/Components/imx335 -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/BSP/Components/Common -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/BSP/Components/rk050hr18 -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Middlewares/ST/STM32_ISP_Library/evision/Inc -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Middlewares/ST/STM32_ISP_Library/isp/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -mcmse -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-BSP-2f-STM32N6570-2d-DK

clean-Drivers-2f-BSP-2f-STM32N6570-2d-DK:
	-$(RM) ./Drivers/BSP/STM32N6570-DK/stm32n6570_discovery.cyclo ./Drivers/BSP/STM32N6570-DK/stm32n6570_discovery.d ./Drivers/BSP/STM32N6570-DK/stm32n6570_discovery.o ./Drivers/BSP/STM32N6570-DK/stm32n6570_discovery.su ./Drivers/BSP/STM32N6570-DK/stm32n6570_discovery_bus.cyclo ./Drivers/BSP/STM32N6570-DK/stm32n6570_discovery_bus.d ./Drivers/BSP/STM32N6570-DK/stm32n6570_discovery_bus.o ./Drivers/BSP/STM32N6570-DK/stm32n6570_discovery_bus.su ./Drivers/BSP/STM32N6570-DK/stm32n6570_discovery_xspi.cyclo ./Drivers/BSP/STM32N6570-DK/stm32n6570_discovery_xspi.d ./Drivers/BSP/STM32N6570-DK/stm32n6570_discovery_xspi.o ./Drivers/BSP/STM32N6570-DK/stm32n6570_discovery_xspi.su

.PHONY: clean-Drivers-2f-BSP-2f-STM32N6570-2d-DK

