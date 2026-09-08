################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/Users/Dousik/Workspace/TRON/sessions/session_12/FSBL/mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/stm32h7/int_stm32h7.c \
C:/Users/Dousik/Workspace/TRON/sessions/session_12/FSBL/mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/stm32h7/ptimer_stm32h7.c 

OBJS += \
./mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/stm32h7/int_stm32h7.o \
./mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/stm32h7/ptimer_stm32h7.o 

C_DEPS += \
./mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/stm32h7/int_stm32h7.d \
./mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/stm32h7/ptimer_stm32h7.d 


# Each subdirectory must supply rules for building sources it contributes
mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/stm32h7/int_stm32h7.o: C:/Users/Dousik/Workspace/TRON/sessions/session_12/FSBL/mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/stm32h7/int_stm32h7.c mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/stm32h7/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m55 -std=gnu11 -DDEBUG -DSTM32N657xx -DUSE_FULL_ASSERT -DUSE_HAL_DRIVER -D_STM32CUBE_DISCOVERY_N657_ -c -I../../../FSBL/Inc -I../../../Drivers/BSP/STM32N6570-DK -I../../../Drivers/STM32N6xx_HAL_Driver/Inc -I../../../Drivers/CMSIS/Device/ST/STM32N6xx/Include -I../../../Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -I../../../Drivers/CMSIS/Include -I../../../Drivers/BSP/Components/imx335 -I../../../Drivers/BSP/Components/Common -I../../../Drivers/BSP/Components/rk050hr18 -I../../../Middlewares/ST/STM32_ISP_Library/evision/Inc -I../../../Middlewares/ST/STM32_ISP_Library/isp/Inc -I../../../FSBL/mtk3_bsp2 -I../../../FSBL/mtk3_bsp2/config -I../../../FSBL/mtk3_bsp2/include -I../../../FSBL/mtk3_bsp2/mtkernel/kernel/knlinc -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -mcmse -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/stm32h7/ptimer_stm32h7.o: C:/Users/Dousik/Workspace/TRON/sessions/session_12/FSBL/mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/stm32h7/ptimer_stm32h7.c mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/stm32h7/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m55 -std=gnu11 -DDEBUG -DSTM32N657xx -DUSE_FULL_ASSERT -DUSE_HAL_DRIVER -D_STM32CUBE_DISCOVERY_N657_ -c -I../../../FSBL/Inc -I../../../Drivers/BSP/STM32N6570-DK -I../../../Drivers/STM32N6xx_HAL_Driver/Inc -I../../../Drivers/CMSIS/Device/ST/STM32N6xx/Include -I../../../Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -I../../../Drivers/CMSIS/Include -I../../../Drivers/BSP/Components/imx335 -I../../../Drivers/BSP/Components/Common -I../../../Drivers/BSP/Components/rk050hr18 -I../../../Middlewares/ST/STM32_ISP_Library/evision/Inc -I../../../Middlewares/ST/STM32_ISP_Library/isp/Inc -I../../../FSBL/mtk3_bsp2 -I../../../FSBL/mtk3_bsp2/config -I../../../FSBL/mtk3_bsp2/include -I../../../FSBL/mtk3_bsp2/mtkernel/kernel/knlinc -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -mcmse -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-mtk3_bsp2-2f-mtkernel-2f-lib-2f-libtk-2f-sysdepend-2f-cpu-2f-stm32h7

clean-mtk3_bsp2-2f-mtkernel-2f-lib-2f-libtk-2f-sysdepend-2f-cpu-2f-stm32h7:
	-$(RM) ./mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/stm32h7/int_stm32h7.cyclo ./mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/stm32h7/int_stm32h7.d ./mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/stm32h7/int_stm32h7.o ./mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/stm32h7/int_stm32h7.su ./mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/stm32h7/ptimer_stm32h7.cyclo ./mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/stm32h7/ptimer_stm32h7.d ./mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/stm32h7/ptimer_stm32h7.o ./mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/stm32h7/ptimer_stm32h7.su

.PHONY: clean-mtk3_bsp2-2f-mtkernel-2f-lib-2f-libtk-2f-sysdepend-2f-cpu-2f-stm32h7

