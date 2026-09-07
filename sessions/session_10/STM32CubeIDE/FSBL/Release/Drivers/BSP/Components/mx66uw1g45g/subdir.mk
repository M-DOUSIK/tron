################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/Users/Dousik/Workspace/TRON/sessions/session_10/Drivers/BSP/Components/mx66uw1g45g/mx66uw1g45g.c 

OBJS += \
./Drivers/BSP/Components/mx66uw1g45g/mx66uw1g45g.o 

C_DEPS += \
./Drivers/BSP/Components/mx66uw1g45g/mx66uw1g45g.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/BSP/Components/mx66uw1g45g/mx66uw1g45g.o: C:/Users/Dousik/Workspace/TRON/sessions/session_10/Drivers/BSP/Components/mx66uw1g45g/mx66uw1g45g.c Drivers/BSP/Components/mx66uw1g45g/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m55 -std=gnu11 -DDEBUG -DSTM32N657xx -DUSE_FULL_ASSERT -DUSE_HAL_DRIVER -c -I../../../FSBL/Inc -I../../../Drivers/BSP/STM32N6570-DK -I../../../Drivers/STM32N6xx_HAL_Driver/Inc -I../../../Drivers/CMSIS/Device/ST/STM32N6xx/Include -I../../../Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -I../../../Drivers/CMSIS/Include -I../../../Drivers/BSP/Components/imx335 -I../../../Drivers/BSP/Components/Common -I../../../Drivers/BSP/Components/rk050hr18 -I../../../Middlewares/ST/STM32_ISP_Library/evision/Inc -I../../../Middlewares/ST/STM32_ISP_Library/isp/Inc -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -mcmse -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-BSP-2f-Components-2f-mx66uw1g45g

clean-Drivers-2f-BSP-2f-Components-2f-mx66uw1g45g:
	-$(RM) ./Drivers/BSP/Components/mx66uw1g45g/mx66uw1g45g.cyclo ./Drivers/BSP/Components/mx66uw1g45g/mx66uw1g45g.d ./Drivers/BSP/Components/mx66uw1g45g/mx66uw1g45g.o ./Drivers/BSP/Components/mx66uw1g45g/mx66uw1g45g.su

.PHONY: clean-Drivers-2f-BSP-2f-Components-2f-mx66uw1g45g

