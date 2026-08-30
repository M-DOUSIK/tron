################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Src/camera_lcd.c 

OBJS += \
./Src/camera_lcd.o 

C_DEPS += \
./Src/camera_lcd.d 


# Each subdirectory must supply rules for building sources it contributes
Src/%.o Src/%.su Src/%.cyclo: ../Src/%.c Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m55 -std=gnu11 -g3 -DSTM32N657xx -DUSE_FULL_ASSERT -DUSE_HAL_DRIVER -c -IC:/Users/Dousik/Workspace/TRON/sessions/session_03/FSBL/Inc -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/BSP/STM32N6570-DK -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/STM32N6xx_HAL_Driver/Inc -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/CMSIS/Device/ST/STM32N6xx/Include -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/CMSIS/Include -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/BSP/Components/imx335 -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/BSP/Components/Common -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Drivers/BSP/Components/rk050hr18 -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Middlewares/ST/STM32_ISP_Library/evision/Inc -IC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Middlewares/ST/STM32_ISP_Library/isp/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -mcmse -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Src

clean-Src:
	-$(RM) ./Src/camera_lcd.cyclo ./Src/camera_lcd.d ./Src/camera_lcd.o ./Src/camera_lcd.su

.PHONY: clean-Src

