################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/Users/Dousik/Workspace/TRON/sessions/session_15/FSBL/mtk3_bsp2/mtkernel/kernel/tstdlib/bitop.c \
C:/Users/Dousik/Workspace/TRON/sessions/session_15/FSBL/mtk3_bsp2/mtkernel/kernel/tstdlib/string.c 

OBJS += \
./mtk3_bsp2/mtkernel/kernel/tstdlib/bitop.o \
./mtk3_bsp2/mtkernel/kernel/tstdlib/string.o 

C_DEPS += \
./mtk3_bsp2/mtkernel/kernel/tstdlib/bitop.d \
./mtk3_bsp2/mtkernel/kernel/tstdlib/string.d 


# Each subdirectory must supply rules for building sources it contributes
mtk3_bsp2/mtkernel/kernel/tstdlib/bitop.o: C:/Users/Dousik/Workspace/TRON/sessions/session_15/FSBL/mtk3_bsp2/mtkernel/kernel/tstdlib/bitop.c mtk3_bsp2/mtkernel/kernel/tstdlib/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m55 -std=gnu11 -DSTM32N657xx -DUSE_FULL_ASSERT -DUSE_HAL_DRIVER -D_STM32CUBE_DISCOVERY_N657_ -c -I../../../FSBL/Inc -I../../../Drivers/BSP/STM32N6570-DK -I../../../Drivers/STM32N6xx_HAL_Driver/Inc -I../../../Drivers/CMSIS/Device/ST/STM32N6xx/Include -I../../../Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -I../../../Drivers/CMSIS/Include -I../../../Drivers/BSP/Components/imx335 -I../../../Drivers/BSP/Components/Common -I../../../Drivers/BSP/Components/rk050hr18 -I../../../Middlewares/ST/STM32_ISP_Library/evision/Inc -I../../../Middlewares/ST/STM32_ISP_Library/isp/Inc -I../../../FSBL/mtk3_bsp2 -I../../../FSBL/mtk3_bsp2/config -I../../../FSBL/mtk3_bsp2/include -I../../../FSBL/mtk3_bsp2/mtkernel/kernel/knlinc -Os -ffunction-sections -fdata-sections -Wall -fno-toplevel-reorder -fstack-usage -fcyclomatic-complexity -mcmse -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
mtk3_bsp2/mtkernel/kernel/tstdlib/string.o: C:/Users/Dousik/Workspace/TRON/sessions/session_15/FSBL/mtk3_bsp2/mtkernel/kernel/tstdlib/string.c mtk3_bsp2/mtkernel/kernel/tstdlib/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m55 -std=gnu11 -DSTM32N657xx -DUSE_FULL_ASSERT -DUSE_HAL_DRIVER -D_STM32CUBE_DISCOVERY_N657_ -c -I../../../FSBL/Inc -I../../../Drivers/BSP/STM32N6570-DK -I../../../Drivers/STM32N6xx_HAL_Driver/Inc -I../../../Drivers/CMSIS/Device/ST/STM32N6xx/Include -I../../../Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -I../../../Drivers/CMSIS/Include -I../../../Drivers/BSP/Components/imx335 -I../../../Drivers/BSP/Components/Common -I../../../Drivers/BSP/Components/rk050hr18 -I../../../Middlewares/ST/STM32_ISP_Library/evision/Inc -I../../../Middlewares/ST/STM32_ISP_Library/isp/Inc -I../../../FSBL/mtk3_bsp2 -I../../../FSBL/mtk3_bsp2/config -I../../../FSBL/mtk3_bsp2/include -I../../../FSBL/mtk3_bsp2/mtkernel/kernel/knlinc -Os -ffunction-sections -fdata-sections -Wall -fno-toplevel-reorder -fstack-usage -fcyclomatic-complexity -mcmse -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-mtk3_bsp2-2f-mtkernel-2f-kernel-2f-tstdlib

clean-mtk3_bsp2-2f-mtkernel-2f-kernel-2f-tstdlib:
	-$(RM) ./mtk3_bsp2/mtkernel/kernel/tstdlib/bitop.cyclo ./mtk3_bsp2/mtkernel/kernel/tstdlib/bitop.d ./mtk3_bsp2/mtkernel/kernel/tstdlib/bitop.o ./mtk3_bsp2/mtkernel/kernel/tstdlib/bitop.su ./mtk3_bsp2/mtkernel/kernel/tstdlib/string.cyclo ./mtk3_bsp2/mtkernel/kernel/tstdlib/string.d ./mtk3_bsp2/mtkernel/kernel/tstdlib/string.o ./mtk3_bsp2/mtkernel/kernel/tstdlib/string.su

.PHONY: clean-mtk3_bsp2-2f-mtkernel-2f-kernel-2f-tstdlib

