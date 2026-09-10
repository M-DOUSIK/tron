################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/Users/Dousik/Workspace/TRON/sessions/session_16/FSBL/mtk3_bsp2/sysdepend/ra_fsp/device/hal_i3c_i2c/hal_i3c_i2c.c 

OBJS += \
./mtk3_bsp2/sysdepend/ra_fsp/device/hal_i3c_i2c/hal_i3c_i2c.o 

C_DEPS += \
./mtk3_bsp2/sysdepend/ra_fsp/device/hal_i3c_i2c/hal_i3c_i2c.d 


# Each subdirectory must supply rules for building sources it contributes
mtk3_bsp2/sysdepend/ra_fsp/device/hal_i3c_i2c/hal_i3c_i2c.o: C:/Users/Dousik/Workspace/TRON/sessions/session_16/FSBL/mtk3_bsp2/sysdepend/ra_fsp/device/hal_i3c_i2c/hal_i3c_i2c.c mtk3_bsp2/sysdepend/ra_fsp/device/hal_i3c_i2c/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m55 -std=gnu11 -DSTM32N657xx -DUSE_FULL_ASSERT -DUSE_HAL_DRIVER -D_STM32CUBE_DISCOVERY_N657_ -c -I../../../FSBL/Inc -I../../../Drivers/BSP/STM32N6570-DK -I../../../Drivers/STM32N6xx_HAL_Driver/Inc -I../../../Drivers/CMSIS/Device/ST/STM32N6xx/Include -I../../../Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -I../../../Drivers/CMSIS/Include -I../../../Drivers/BSP/Components/imx335 -I../../../Drivers/BSP/Components/Common -I../../../Drivers/BSP/Components/rk050hr18 -I../../../Middlewares/ST/STM32_ISP_Library/evision/Inc -I../../../Middlewares/ST/STM32_ISP_Library/isp/Inc -I../../../FSBL/mtk3_bsp2 -I../../../FSBL/mtk3_bsp2/config -I../../../FSBL/mtk3_bsp2/include -I../../../FSBL/mtk3_bsp2/mtkernel/kernel/knlinc -Os -ffunction-sections -fdata-sections -Wall -fno-toplevel-reorder -fstack-usage -fcyclomatic-complexity -mcmse -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-mtk3_bsp2-2f-sysdepend-2f-ra_fsp-2f-device-2f-hal_i3c_i2c

clean-mtk3_bsp2-2f-sysdepend-2f-ra_fsp-2f-device-2f-hal_i3c_i2c:
	-$(RM) ./mtk3_bsp2/sysdepend/ra_fsp/device/hal_i3c_i2c/hal_i3c_i2c.cyclo ./mtk3_bsp2/sysdepend/ra_fsp/device/hal_i3c_i2c/hal_i3c_i2c.d ./mtk3_bsp2/sysdepend/ra_fsp/device/hal_i3c_i2c/hal_i3c_i2c.o ./mtk3_bsp2/sysdepend/ra_fsp/device/hal_i3c_i2c/hal_i3c_i2c.su

.PHONY: clean-mtk3_bsp2-2f-sysdepend-2f-ra_fsp-2f-device-2f-hal_i3c_i2c

