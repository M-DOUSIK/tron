################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Projects/STM32N6570-DK/Applications/DCMIPP/DCMIPP_ContinuousMode/FSBL/Src/main.c \
C:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Projects/STM32N6570-DK/Applications/DCMIPP/DCMIPP_ContinuousMode/FSBL/Src/stm32n6xx_hal_msp.c \
C:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Projects/STM32N6570-DK/Applications/DCMIPP/DCMIPP_ContinuousMode/FSBL/Src/stm32n6xx_it.c \
../Application/User/syscalls.c \
../Application/User/sysmem.c 

OBJS += \
./Application/User/main.o \
./Application/User/stm32n6xx_hal_msp.o \
./Application/User/stm32n6xx_it.o \
./Application/User/syscalls.o \
./Application/User/sysmem.o 

C_DEPS += \
./Application/User/main.d \
./Application/User/stm32n6xx_hal_msp.d \
./Application/User/stm32n6xx_it.d \
./Application/User/syscalls.d \
./Application/User/sysmem.d 


# Each subdirectory must supply rules for building sources it contributes
Application/User/main.o: C:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Projects/STM32N6570-DK/Applications/DCMIPP/DCMIPP_ContinuousMode/FSBL/Src/main.c Application/User/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m55 -std=gnu11 -g3 -DSTM32N657xx -DUSE_FULL_ASSERT -DUSE_HAL_DRIVER -c -IC:/Users/Dousik/Workspace/TRON/sessions/session_03/FSBL/Inc -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/BSP/STM32N6570-DK -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/STM32N6xx_HAL_Driver/Inc -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/CMSIS/Device/ST/STM32N6xx/Include -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/CMSIS/Include -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/BSP/Components/imx335 -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/BSP/Components/Common -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/BSP/Components/rk050hr18 -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Middlewares/ST/STM32_ISP_Library/evision/Inc -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Middlewares/ST/STM32_ISP_Library/isp/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -mcmse -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Application/User/stm32n6xx_hal_msp.o: C:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Projects/STM32N6570-DK/Applications/DCMIPP/DCMIPP_ContinuousMode/FSBL/Src/stm32n6xx_hal_msp.c Application/User/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m55 -std=gnu11 -g3 -DSTM32N657xx -DUSE_FULL_ASSERT -DUSE_HAL_DRIVER -c -IC:/Users/Dousik/Workspace/TRON/sessions/session_03/FSBL/Inc -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/BSP/STM32N6570-DK -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/STM32N6xx_HAL_Driver/Inc -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/CMSIS/Device/ST/STM32N6xx/Include -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/CMSIS/Include -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/BSP/Components/imx335 -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/BSP/Components/Common -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/BSP/Components/rk050hr18 -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Middlewares/ST/STM32_ISP_Library/evision/Inc -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Middlewares/ST/STM32_ISP_Library/isp/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -mcmse -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Application/User/stm32n6xx_it.o: C:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Projects/STM32N6570-DK/Applications/DCMIPP/DCMIPP_ContinuousMode/FSBL/Src/stm32n6xx_it.c Application/User/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m55 -std=gnu11 -g3 -DSTM32N657xx -DUSE_FULL_ASSERT -DUSE_HAL_DRIVER -c -IC:/Users/Dousik/Workspace/TRON/sessions/session_03/FSBL/Inc -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/BSP/STM32N6570-DK -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/STM32N6xx_HAL_Driver/Inc -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/CMSIS/Device/ST/STM32N6xx/Include -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/CMSIS/Include -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/BSP/Components/imx335 -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/BSP/Components/Common -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/BSP/Components/rk050hr18 -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Middlewares/ST/STM32_ISP_Library/evision/Inc -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Middlewares/ST/STM32_ISP_Library/isp/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -mcmse -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Application/User/%.o Application/User/%.su Application/User/%.cyclo: ../Application/User/%.c Application/User/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m55 -std=gnu11 -g3 -DSTM32N657xx -DUSE_FULL_ASSERT -DUSE_HAL_DRIVER -c -IC:/Users/Dousik/Workspace/TRON/sessions/session_03/FSBL/Inc -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/BSP/STM32N6570-DK -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/STM32N6xx_HAL_Driver/Inc -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/CMSIS/Device/ST/STM32N6xx/Include -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/CMSIS/Include -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/BSP/Components/imx335 -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/BSP/Components/Common -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/BSP/Components/rk050hr18 -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Middlewares/ST/STM32_ISP_Library/evision/Inc -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Middlewares/ST/STM32_ISP_Library/isp/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -mcmse -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Application-2f-User

clean-Application-2f-User:
	-$(RM) ./Application/User/main.cyclo ./Application/User/main.d ./Application/User/main.o ./Application/User/main.su ./Application/User/stm32n6xx_hal_msp.cyclo ./Application/User/stm32n6xx_hal_msp.d ./Application/User/stm32n6xx_hal_msp.o ./Application/User/stm32n6xx_hal_msp.su ./Application/User/stm32n6xx_it.cyclo ./Application/User/stm32n6xx_it.d ./Application/User/stm32n6xx_it.o ./Application/User/stm32n6xx_it.su ./Application/User/syscalls.cyclo ./Application/User/syscalls.d ./Application/User/syscalls.o ./Application/User/syscalls.su ./Application/User/sysmem.cyclo ./Application/User/sysmem.d ./Application/User/sysmem.o ./Application/User/sysmem.su

.PHONY: clean-Application-2f-User

