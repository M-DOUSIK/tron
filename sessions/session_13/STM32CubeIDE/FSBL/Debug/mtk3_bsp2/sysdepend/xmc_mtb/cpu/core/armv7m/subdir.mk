################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/Users/Dousik/Workspace/TRON/sessions/session_13/FSBL/mtk3_bsp2/sysdepend/xmc_mtb/cpu/core/armv7m/cpu_cntl.c \
C:/Users/Dousik/Workspace/TRON/sessions/session_13/FSBL/mtk3_bsp2/sysdepend/xmc_mtb/cpu/core/armv7m/exc_hdr.c \
C:/Users/Dousik/Workspace/TRON/sessions/session_13/FSBL/mtk3_bsp2/sysdepend/xmc_mtb/cpu/core/armv7m/sys_start.c 

S_UPPER_SRCS += \
C:/Users/Dousik/Workspace/TRON/sessions/session_13/FSBL/mtk3_bsp2/sysdepend/xmc_mtb/cpu/core/armv7m/dispatch.S 

OBJS += \
./mtk3_bsp2/sysdepend/xmc_mtb/cpu/core/armv7m/cpu_cntl.o \
./mtk3_bsp2/sysdepend/xmc_mtb/cpu/core/armv7m/dispatch.o \
./mtk3_bsp2/sysdepend/xmc_mtb/cpu/core/armv7m/exc_hdr.o \
./mtk3_bsp2/sysdepend/xmc_mtb/cpu/core/armv7m/sys_start.o 

S_UPPER_DEPS += \
./mtk3_bsp2/sysdepend/xmc_mtb/cpu/core/armv7m/dispatch.d 

C_DEPS += \
./mtk3_bsp2/sysdepend/xmc_mtb/cpu/core/armv7m/cpu_cntl.d \
./mtk3_bsp2/sysdepend/xmc_mtb/cpu/core/armv7m/exc_hdr.d \
./mtk3_bsp2/sysdepend/xmc_mtb/cpu/core/armv7m/sys_start.d 


# Each subdirectory must supply rules for building sources it contributes
mtk3_bsp2/sysdepend/xmc_mtb/cpu/core/armv7m/cpu_cntl.o: C:/Users/Dousik/Workspace/TRON/sessions/session_13/FSBL/mtk3_bsp2/sysdepend/xmc_mtb/cpu/core/armv7m/cpu_cntl.c mtk3_bsp2/sysdepend/xmc_mtb/cpu/core/armv7m/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m55 -std=gnu11 -g3 -DDEBUG -DMEDSIGHT_DEBUG=1 -DSTM32N657xx -DUSE_FULL_ASSERT -DUSE_HAL_DRIVER -D_STM32CUBE_DISCOVERY_N657_ -c -I../../../FSBL/Inc -I../../../Drivers/BSP/STM32N6570-DK -I../../../Drivers/STM32N6xx_HAL_Driver/Inc -I../../../Drivers/CMSIS/Device/ST/STM32N6xx/Include -I../../../Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -I../../../Drivers/CMSIS/Include -I../../../Drivers/BSP/Components/imx335 -I../../../Drivers/BSP/Components/Common -I../../../Drivers/BSP/Components/rk050hr18 -I../../../Middlewares/ST/STM32_ISP_Library/evision/Inc -I../../../Middlewares/ST/STM32_ISP_Library/isp/Inc -I../../../FSBL/mtk3_bsp2 -I../../../FSBL/mtk3_bsp2/config -I../../../FSBL/mtk3_bsp2/include -I../../../FSBL/mtk3_bsp2/mtkernel/kernel/knlinc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -mcmse -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
mtk3_bsp2/sysdepend/xmc_mtb/cpu/core/armv7m/dispatch.o: C:/Users/Dousik/Workspace/TRON/sessions/session_13/FSBL/mtk3_bsp2/sysdepend/xmc_mtb/cpu/core/armv7m/dispatch.S mtk3_bsp2/sysdepend/xmc_mtb/cpu/core/armv7m/subdir.mk
	arm-none-eabi-gcc -mcpu=cortex-m55 -g3 -DDEBUG -D_STM32CUBE_DISCOVERY_N657_ -c -I../../../FSBL/mtk3_bsp2 -I../../../FSBL/mtk3_bsp2/config -I../../../FSBL/mtk3_bsp2/include -I../../../FSBL/mtk3_bsp2/mtkernel/kernel/knlinc -x assembler-with-cpp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@" "$<"
mtk3_bsp2/sysdepend/xmc_mtb/cpu/core/armv7m/exc_hdr.o: C:/Users/Dousik/Workspace/TRON/sessions/session_13/FSBL/mtk3_bsp2/sysdepend/xmc_mtb/cpu/core/armv7m/exc_hdr.c mtk3_bsp2/sysdepend/xmc_mtb/cpu/core/armv7m/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m55 -std=gnu11 -g3 -DDEBUG -DMEDSIGHT_DEBUG=1 -DSTM32N657xx -DUSE_FULL_ASSERT -DUSE_HAL_DRIVER -D_STM32CUBE_DISCOVERY_N657_ -c -I../../../FSBL/Inc -I../../../Drivers/BSP/STM32N6570-DK -I../../../Drivers/STM32N6xx_HAL_Driver/Inc -I../../../Drivers/CMSIS/Device/ST/STM32N6xx/Include -I../../../Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -I../../../Drivers/CMSIS/Include -I../../../Drivers/BSP/Components/imx335 -I../../../Drivers/BSP/Components/Common -I../../../Drivers/BSP/Components/rk050hr18 -I../../../Middlewares/ST/STM32_ISP_Library/evision/Inc -I../../../Middlewares/ST/STM32_ISP_Library/isp/Inc -I../../../FSBL/mtk3_bsp2 -I../../../FSBL/mtk3_bsp2/config -I../../../FSBL/mtk3_bsp2/include -I../../../FSBL/mtk3_bsp2/mtkernel/kernel/knlinc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -mcmse -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
mtk3_bsp2/sysdepend/xmc_mtb/cpu/core/armv7m/sys_start.o: C:/Users/Dousik/Workspace/TRON/sessions/session_13/FSBL/mtk3_bsp2/sysdepend/xmc_mtb/cpu/core/armv7m/sys_start.c mtk3_bsp2/sysdepend/xmc_mtb/cpu/core/armv7m/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m55 -std=gnu11 -g3 -DDEBUG -DMEDSIGHT_DEBUG=1 -DSTM32N657xx -DUSE_FULL_ASSERT -DUSE_HAL_DRIVER -D_STM32CUBE_DISCOVERY_N657_ -c -I../../../FSBL/Inc -I../../../Drivers/BSP/STM32N6570-DK -I../../../Drivers/STM32N6xx_HAL_Driver/Inc -I../../../Drivers/CMSIS/Device/ST/STM32N6xx/Include -I../../../Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -I../../../Drivers/CMSIS/Include -I../../../Drivers/BSP/Components/imx335 -I../../../Drivers/BSP/Components/Common -I../../../Drivers/BSP/Components/rk050hr18 -I../../../Middlewares/ST/STM32_ISP_Library/evision/Inc -I../../../Middlewares/ST/STM32_ISP_Library/isp/Inc -I../../../FSBL/mtk3_bsp2 -I../../../FSBL/mtk3_bsp2/config -I../../../FSBL/mtk3_bsp2/include -I../../../FSBL/mtk3_bsp2/mtkernel/kernel/knlinc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -mcmse -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-mtk3_bsp2-2f-sysdepend-2f-xmc_mtb-2f-cpu-2f-core-2f-armv7m

clean-mtk3_bsp2-2f-sysdepend-2f-xmc_mtb-2f-cpu-2f-core-2f-armv7m:
	-$(RM) ./mtk3_bsp2/sysdepend/xmc_mtb/cpu/core/armv7m/cpu_cntl.cyclo ./mtk3_bsp2/sysdepend/xmc_mtb/cpu/core/armv7m/cpu_cntl.d ./mtk3_bsp2/sysdepend/xmc_mtb/cpu/core/armv7m/cpu_cntl.o ./mtk3_bsp2/sysdepend/xmc_mtb/cpu/core/armv7m/cpu_cntl.su ./mtk3_bsp2/sysdepend/xmc_mtb/cpu/core/armv7m/dispatch.d ./mtk3_bsp2/sysdepend/xmc_mtb/cpu/core/armv7m/dispatch.o ./mtk3_bsp2/sysdepend/xmc_mtb/cpu/core/armv7m/exc_hdr.cyclo ./mtk3_bsp2/sysdepend/xmc_mtb/cpu/core/armv7m/exc_hdr.d ./mtk3_bsp2/sysdepend/xmc_mtb/cpu/core/armv7m/exc_hdr.o ./mtk3_bsp2/sysdepend/xmc_mtb/cpu/core/armv7m/exc_hdr.su ./mtk3_bsp2/sysdepend/xmc_mtb/cpu/core/armv7m/sys_start.cyclo ./mtk3_bsp2/sysdepend/xmc_mtb/cpu/core/armv7m/sys_start.d ./mtk3_bsp2/sysdepend/xmc_mtb/cpu/core/armv7m/sys_start.o ./mtk3_bsp2/sysdepend/xmc_mtb/cpu/core/armv7m/sys_start.su

.PHONY: clean-mtk3_bsp2-2f-sysdepend-2f-xmc_mtb-2f-cpu-2f-core-2f-armv7m

