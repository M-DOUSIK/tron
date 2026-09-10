################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/Users/Dousik/Workspace/TRON/sessions/session_16/FSBL/mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx231/hllint_tbl.c \
C:/Users/Dousik/Workspace/TRON/sessions/session_16/FSBL/mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx231/intvect_tbl.c 

S_UPPER_SRCS += \
C:/Users/Dousik/Workspace/TRON/sessions/session_16/FSBL/mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx231/hllint_ent.S 

OBJS += \
./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx231/hllint_ent.o \
./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx231/hllint_tbl.o \
./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx231/intvect_tbl.o 

S_UPPER_DEPS += \
./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx231/hllint_ent.d 

C_DEPS += \
./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx231/hllint_tbl.d \
./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx231/intvect_tbl.d 


# Each subdirectory must supply rules for building sources it contributes
mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx231/hllint_ent.o: C:/Users/Dousik/Workspace/TRON/sessions/session_16/FSBL/mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx231/hllint_ent.S mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx231/subdir.mk
	arm-none-eabi-gcc -mcpu=cortex-m55 -D_STM32CUBE_DISCOVERY_N657_ -c -I../../../FSBL/mtk3_bsp2 -I../../../FSBL/mtk3_bsp2/config -I../../../FSBL/mtk3_bsp2/include -I../../../FSBL/mtk3_bsp2/mtkernel/kernel/knlinc -x assembler-with-cpp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@" "$<"
mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx231/hllint_tbl.o: C:/Users/Dousik/Workspace/TRON/sessions/session_16/FSBL/mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx231/hllint_tbl.c mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx231/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m55 -std=gnu11 -DSTM32N657xx -DUSE_FULL_ASSERT -DUSE_HAL_DRIVER -D_STM32CUBE_DISCOVERY_N657_ -c -I../../../FSBL/Inc -I../../../Drivers/BSP/STM32N6570-DK -I../../../Drivers/STM32N6xx_HAL_Driver/Inc -I../../../Drivers/CMSIS/Device/ST/STM32N6xx/Include -I../../../Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -I../../../Drivers/CMSIS/Include -I../../../Drivers/BSP/Components/imx335 -I../../../Drivers/BSP/Components/Common -I../../../Drivers/BSP/Components/rk050hr18 -I../../../Middlewares/ST/STM32_ISP_Library/evision/Inc -I../../../Middlewares/ST/STM32_ISP_Library/isp/Inc -I../../../FSBL/mtk3_bsp2 -I../../../FSBL/mtk3_bsp2/config -I../../../FSBL/mtk3_bsp2/include -I../../../FSBL/mtk3_bsp2/mtkernel/kernel/knlinc -Os -ffunction-sections -fdata-sections -Wall -fno-toplevel-reorder -fstack-usage -fcyclomatic-complexity -mcmse -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx231/intvect_tbl.o: C:/Users/Dousik/Workspace/TRON/sessions/session_16/FSBL/mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx231/intvect_tbl.c mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx231/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m55 -std=gnu11 -DSTM32N657xx -DUSE_FULL_ASSERT -DUSE_HAL_DRIVER -D_STM32CUBE_DISCOVERY_N657_ -c -I../../../FSBL/Inc -I../../../Drivers/BSP/STM32N6570-DK -I../../../Drivers/STM32N6xx_HAL_Driver/Inc -I../../../Drivers/CMSIS/Device/ST/STM32N6xx/Include -I../../../Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -I../../../Drivers/CMSIS/Include -I../../../Drivers/BSP/Components/imx335 -I../../../Drivers/BSP/Components/Common -I../../../Drivers/BSP/Components/rk050hr18 -I../../../Middlewares/ST/STM32_ISP_Library/evision/Inc -I../../../Middlewares/ST/STM32_ISP_Library/isp/Inc -I../../../FSBL/mtk3_bsp2 -I../../../FSBL/mtk3_bsp2/config -I../../../FSBL/mtk3_bsp2/include -I../../../FSBL/mtk3_bsp2/mtkernel/kernel/knlinc -Os -ffunction-sections -fdata-sections -Wall -fno-toplevel-reorder -fstack-usage -fcyclomatic-complexity -mcmse -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-mtk3_bsp2-2f-mtkernel-2f-kernel-2f-sysdepend-2f-cpu-2f-rx231

clean-mtk3_bsp2-2f-mtkernel-2f-kernel-2f-sysdepend-2f-cpu-2f-rx231:
	-$(RM) ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx231/hllint_ent.d ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx231/hllint_ent.o ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx231/hllint_tbl.cyclo ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx231/hllint_tbl.d ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx231/hllint_tbl.o ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx231/hllint_tbl.su ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx231/intvect_tbl.cyclo ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx231/intvect_tbl.d ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx231/intvect_tbl.o ./mtk3_bsp2/mtkernel/kernel/sysdepend/cpu/rx231/intvect_tbl.su

.PHONY: clean-mtk3_bsp2-2f-mtkernel-2f-kernel-2f-sysdepend-2f-cpu-2f-rx231

