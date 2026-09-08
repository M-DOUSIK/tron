################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/Users/Dousik/Workspace/TRON/sessions/session_12/Drivers/BSP/Components/gt911/gt911.c \
C:/Users/Dousik/Workspace/TRON/sessions/session_12/Drivers/BSP/Components/gt911/gt911_reg.c 

OBJS += \
./Drivers/BSP/Components/gt911/gt911.o \
./Drivers/BSP/Components/gt911/gt911_reg.o 

C_DEPS += \
./Drivers/BSP/Components/gt911/gt911.d \
./Drivers/BSP/Components/gt911/gt911_reg.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/BSP/Components/gt911/gt911.o: C:/Users/Dousik/Workspace/TRON/sessions/session_12/Drivers/BSP/Components/gt911/gt911.c Drivers/BSP/Components/gt911/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m55 -std=gnu11 -DDEBUG -DSTM32N657xx -DUSE_FULL_ASSERT -DUSE_HAL_DRIVER -D_STM32CUBE_DISCOVERY_N657_ -c -I../../../FSBL/Inc -I../../../Drivers/BSP/STM32N6570-DK -I../../../Drivers/STM32N6xx_HAL_Driver/Inc -I../../../Drivers/CMSIS/Device/ST/STM32N6xx/Include -I../../../Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -I../../../Drivers/CMSIS/Include -I../../../Drivers/BSP/Components/imx335 -I../../../Drivers/BSP/Components/Common -I../../../Drivers/BSP/Components/rk050hr18 -I../../../Middlewares/ST/STM32_ISP_Library/evision/Inc -I../../../Middlewares/ST/STM32_ISP_Library/isp/Inc -I../../../FSBL/mtk3_bsp2 -I../../../FSBL/mtk3_bsp2/config -I../../../FSBL/mtk3_bsp2/include -I../../../FSBL/mtk3_bsp2/mtkernel/kernel/knlinc -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -mcmse -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Drivers/BSP/Components/gt911/gt911_reg.o: C:/Users/Dousik/Workspace/TRON/sessions/session_12/Drivers/BSP/Components/gt911/gt911_reg.c Drivers/BSP/Components/gt911/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m55 -std=gnu11 -DDEBUG -DSTM32N657xx -DUSE_FULL_ASSERT -DUSE_HAL_DRIVER -D_STM32CUBE_DISCOVERY_N657_ -c -I../../../FSBL/Inc -I../../../Drivers/BSP/STM32N6570-DK -I../../../Drivers/STM32N6xx_HAL_Driver/Inc -I../../../Drivers/CMSIS/Device/ST/STM32N6xx/Include -I../../../Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -I../../../Drivers/CMSIS/Include -I../../../Drivers/BSP/Components/imx335 -I../../../Drivers/BSP/Components/Common -I../../../Drivers/BSP/Components/rk050hr18 -I../../../Middlewares/ST/STM32_ISP_Library/evision/Inc -I../../../Middlewares/ST/STM32_ISP_Library/isp/Inc -I../../../FSBL/mtk3_bsp2 -I../../../FSBL/mtk3_bsp2/config -I../../../FSBL/mtk3_bsp2/include -I../../../FSBL/mtk3_bsp2/mtkernel/kernel/knlinc -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -mcmse -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-BSP-2f-Components-2f-gt911

clean-Drivers-2f-BSP-2f-Components-2f-gt911:
	-$(RM) ./Drivers/BSP/Components/gt911/gt911.cyclo ./Drivers/BSP/Components/gt911/gt911.d ./Drivers/BSP/Components/gt911/gt911.o ./Drivers/BSP/Components/gt911/gt911.su ./Drivers/BSP/Components/gt911/gt911_reg.cyclo ./Drivers/BSP/Components/gt911/gt911_reg.d ./Drivers/BSP/Components/gt911/gt911_reg.o ./Drivers/BSP/Components/gt911/gt911_reg.su

.PHONY: clean-Drivers-2f-BSP-2f-Components-2f-gt911

