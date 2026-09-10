################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/Users/Dousik/Workspace/TRON/sessions/session_16/FSBL/mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/core/rxv2/int_rxv2.c \
C:/Users/Dousik/Workspace/TRON/sessions/session_16/FSBL/mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/core/rxv2/wusec_rvx2.c 

OBJS += \
./mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/core/rxv2/int_rxv2.o \
./mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/core/rxv2/wusec_rvx2.o 

C_DEPS += \
./mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/core/rxv2/int_rxv2.d \
./mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/core/rxv2/wusec_rvx2.d 


# Each subdirectory must supply rules for building sources it contributes
mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/core/rxv2/int_rxv2.o: C:/Users/Dousik/Workspace/TRON/sessions/session_16/FSBL/mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/core/rxv2/int_rxv2.c mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/core/rxv2/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m55 -std=gnu11 -DSTM32N657xx -DUSE_FULL_ASSERT -DUSE_HAL_DRIVER -D_STM32CUBE_DISCOVERY_N657_ -c -I../../../FSBL/Inc -I../../../Drivers/BSP/STM32N6570-DK -I../../../Drivers/STM32N6xx_HAL_Driver/Inc -I../../../Drivers/CMSIS/Device/ST/STM32N6xx/Include -I../../../Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -I../../../Drivers/CMSIS/Include -I../../../Drivers/BSP/Components/imx335 -I../../../Drivers/BSP/Components/Common -I../../../Drivers/BSP/Components/rk050hr18 -I../../../Middlewares/ST/STM32_ISP_Library/evision/Inc -I../../../Middlewares/ST/STM32_ISP_Library/isp/Inc -I../../../FSBL/mtk3_bsp2 -I../../../FSBL/mtk3_bsp2/config -I../../../FSBL/mtk3_bsp2/include -I../../../FSBL/mtk3_bsp2/mtkernel/kernel/knlinc -Os -ffunction-sections -fdata-sections -Wall -fno-toplevel-reorder -fstack-usage -fcyclomatic-complexity -mcmse -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/core/rxv2/wusec_rvx2.o: C:/Users/Dousik/Workspace/TRON/sessions/session_16/FSBL/mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/core/rxv2/wusec_rvx2.c mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/core/rxv2/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m55 -std=gnu11 -DSTM32N657xx -DUSE_FULL_ASSERT -DUSE_HAL_DRIVER -D_STM32CUBE_DISCOVERY_N657_ -c -I../../../FSBL/Inc -I../../../Drivers/BSP/STM32N6570-DK -I../../../Drivers/STM32N6xx_HAL_Driver/Inc -I../../../Drivers/CMSIS/Device/ST/STM32N6xx/Include -I../../../Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -I../../../Drivers/CMSIS/Include -I../../../Drivers/BSP/Components/imx335 -I../../../Drivers/BSP/Components/Common -I../../../Drivers/BSP/Components/rk050hr18 -I../../../Middlewares/ST/STM32_ISP_Library/evision/Inc -I../../../Middlewares/ST/STM32_ISP_Library/isp/Inc -I../../../FSBL/mtk3_bsp2 -I../../../FSBL/mtk3_bsp2/config -I../../../FSBL/mtk3_bsp2/include -I../../../FSBL/mtk3_bsp2/mtkernel/kernel/knlinc -Os -ffunction-sections -fdata-sections -Wall -fno-toplevel-reorder -fstack-usage -fcyclomatic-complexity -mcmse -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-mtk3_bsp2-2f-mtkernel-2f-lib-2f-libtk-2f-sysdepend-2f-cpu-2f-core-2f-rxv2

clean-mtk3_bsp2-2f-mtkernel-2f-lib-2f-libtk-2f-sysdepend-2f-cpu-2f-core-2f-rxv2:
	-$(RM) ./mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/core/rxv2/int_rxv2.cyclo ./mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/core/rxv2/int_rxv2.d ./mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/core/rxv2/int_rxv2.o ./mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/core/rxv2/int_rxv2.su ./mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/core/rxv2/wusec_rvx2.cyclo ./mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/core/rxv2/wusec_rvx2.d ./mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/core/rxv2/wusec_rvx2.o ./mtk3_bsp2/mtkernel/lib/libtk/sysdepend/cpu/core/rxv2/wusec_rvx2.su

.PHONY: clean-mtk3_bsp2-2f-mtkernel-2f-lib-2f-libtk-2f-sysdepend-2f-cpu-2f-core-2f-rxv2

