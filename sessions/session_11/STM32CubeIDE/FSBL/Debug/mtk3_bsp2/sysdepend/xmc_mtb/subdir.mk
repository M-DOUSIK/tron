################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/Users/Dousik/Workspace/TRON/sessions/session_11/FSBL/mtk3_bsp2/sysdepend/xmc_mtb/devinit.c \
C:/Users/Dousik/Workspace/TRON/sessions/session_11/FSBL/mtk3_bsp2/sysdepend/xmc_mtb/hw_setting.c \
C:/Users/Dousik/Workspace/TRON/sessions/session_11/FSBL/mtk3_bsp2/sysdepend/xmc_mtb/power_save.c 

OBJS += \
./mtk3_bsp2/sysdepend/xmc_mtb/devinit.o \
./mtk3_bsp2/sysdepend/xmc_mtb/hw_setting.o \
./mtk3_bsp2/sysdepend/xmc_mtb/power_save.o 

C_DEPS += \
./mtk3_bsp2/sysdepend/xmc_mtb/devinit.d \
./mtk3_bsp2/sysdepend/xmc_mtb/hw_setting.d \
./mtk3_bsp2/sysdepend/xmc_mtb/power_save.d 


# Each subdirectory must supply rules for building sources it contributes
mtk3_bsp2/sysdepend/xmc_mtb/devinit.o: C:/Users/Dousik/Workspace/TRON/sessions/session_11/FSBL/mtk3_bsp2/sysdepend/xmc_mtb/devinit.c mtk3_bsp2/sysdepend/xmc_mtb/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m55 -std=gnu11 -g3 -DSTM32N657xx -DUSE_FULL_ASSERT -DUSE_HAL_DRIVER -D_STM32CUBE_DISCOVERY_N657_ -c -I../../../FSBL/Inc -I../../../Drivers/BSP/STM32N6570-DK -I../../../Drivers/STM32N6xx_HAL_Driver/Inc -I../../../Drivers/CMSIS/Device/ST/STM32N6xx/Include -I../../../Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -I../../../Drivers/CMSIS/Include -I../../../Drivers/BSP/Components/imx335 -I../../../Drivers/BSP/Components/Common -I../../../Drivers/BSP/Components/rk050hr18 -I../../../Middlewares/ST/STM32_ISP_Library/evision/Inc -I../../../Middlewares/ST/STM32_ISP_Library/isp/Inc -I../../../FSBL/mtk3_bsp2 -I../../../FSBL/mtk3_bsp2/config -I../../../FSBL/mtk3_bsp2/include -I../../../FSBL/mtk3_bsp2/mtkernel/kernel/knlinc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -mcmse -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
mtk3_bsp2/sysdepend/xmc_mtb/hw_setting.o: C:/Users/Dousik/Workspace/TRON/sessions/session_11/FSBL/mtk3_bsp2/sysdepend/xmc_mtb/hw_setting.c mtk3_bsp2/sysdepend/xmc_mtb/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m55 -std=gnu11 -g3 -DSTM32N657xx -DUSE_FULL_ASSERT -DUSE_HAL_DRIVER -D_STM32CUBE_DISCOVERY_N657_ -c -I../../../FSBL/Inc -I../../../Drivers/BSP/STM32N6570-DK -I../../../Drivers/STM32N6xx_HAL_Driver/Inc -I../../../Drivers/CMSIS/Device/ST/STM32N6xx/Include -I../../../Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -I../../../Drivers/CMSIS/Include -I../../../Drivers/BSP/Components/imx335 -I../../../Drivers/BSP/Components/Common -I../../../Drivers/BSP/Components/rk050hr18 -I../../../Middlewares/ST/STM32_ISP_Library/evision/Inc -I../../../Middlewares/ST/STM32_ISP_Library/isp/Inc -I../../../FSBL/mtk3_bsp2 -I../../../FSBL/mtk3_bsp2/config -I../../../FSBL/mtk3_bsp2/include -I../../../FSBL/mtk3_bsp2/mtkernel/kernel/knlinc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -mcmse -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
mtk3_bsp2/sysdepend/xmc_mtb/power_save.o: C:/Users/Dousik/Workspace/TRON/sessions/session_11/FSBL/mtk3_bsp2/sysdepend/xmc_mtb/power_save.c mtk3_bsp2/sysdepend/xmc_mtb/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m55 -std=gnu11 -g3 -DSTM32N657xx -DUSE_FULL_ASSERT -DUSE_HAL_DRIVER -D_STM32CUBE_DISCOVERY_N657_ -c -I../../../FSBL/Inc -I../../../Drivers/BSP/STM32N6570-DK -I../../../Drivers/STM32N6xx_HAL_Driver/Inc -I../../../Drivers/CMSIS/Device/ST/STM32N6xx/Include -I../../../Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -I../../../Drivers/CMSIS/Include -I../../../Drivers/BSP/Components/imx335 -I../../../Drivers/BSP/Components/Common -I../../../Drivers/BSP/Components/rk050hr18 -I../../../Middlewares/ST/STM32_ISP_Library/evision/Inc -I../../../Middlewares/ST/STM32_ISP_Library/isp/Inc -I../../../FSBL/mtk3_bsp2 -I../../../FSBL/mtk3_bsp2/config -I../../../FSBL/mtk3_bsp2/include -I../../../FSBL/mtk3_bsp2/mtkernel/kernel/knlinc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -mcmse -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-mtk3_bsp2-2f-sysdepend-2f-xmc_mtb

clean-mtk3_bsp2-2f-sysdepend-2f-xmc_mtb:
	-$(RM) ./mtk3_bsp2/sysdepend/xmc_mtb/devinit.cyclo ./mtk3_bsp2/sysdepend/xmc_mtb/devinit.d ./mtk3_bsp2/sysdepend/xmc_mtb/devinit.o ./mtk3_bsp2/sysdepend/xmc_mtb/devinit.su ./mtk3_bsp2/sysdepend/xmc_mtb/hw_setting.cyclo ./mtk3_bsp2/sysdepend/xmc_mtb/hw_setting.d ./mtk3_bsp2/sysdepend/xmc_mtb/hw_setting.o ./mtk3_bsp2/sysdepend/xmc_mtb/hw_setting.su ./mtk3_bsp2/sysdepend/xmc_mtb/power_save.cyclo ./mtk3_bsp2/sysdepend/xmc_mtb/power_save.d ./mtk3_bsp2/sysdepend/xmc_mtb/power_save.o ./mtk3_bsp2/sysdepend/xmc_mtb/power_save.su

.PHONY: clean-mtk3_bsp2-2f-sysdepend-2f-xmc_mtb

