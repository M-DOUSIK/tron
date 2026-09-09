################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/Users/Dousik/Workspace/TRON/sessions/session_13/FSBL/mtk3_bsp2/mtkernel/lib/libtk/fastlock.c \
C:/Users/Dousik/Workspace/TRON/sessions/session_13/FSBL/mtk3_bsp2/mtkernel/lib/libtk/fastmlock.c \
C:/Users/Dousik/Workspace/TRON/sessions/session_13/FSBL/mtk3_bsp2/mtkernel/lib/libtk/kmalloc.c 

OBJS += \
./mtk3_bsp2/mtkernel/lib/libtk/fastlock.o \
./mtk3_bsp2/mtkernel/lib/libtk/fastmlock.o \
./mtk3_bsp2/mtkernel/lib/libtk/kmalloc.o 

C_DEPS += \
./mtk3_bsp2/mtkernel/lib/libtk/fastlock.d \
./mtk3_bsp2/mtkernel/lib/libtk/fastmlock.d \
./mtk3_bsp2/mtkernel/lib/libtk/kmalloc.d 


# Each subdirectory must supply rules for building sources it contributes
mtk3_bsp2/mtkernel/lib/libtk/fastlock.o: C:/Users/Dousik/Workspace/TRON/sessions/session_13/FSBL/mtk3_bsp2/mtkernel/lib/libtk/fastlock.c mtk3_bsp2/mtkernel/lib/libtk/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m55 -std=gnu11 -g3 -DDEBUG -DMEDSIGHT_DEBUG=1 -DSTM32N657xx -DUSE_FULL_ASSERT -DUSE_HAL_DRIVER -D_STM32CUBE_DISCOVERY_N657_ -c -I../../../FSBL/Inc -I../../../Drivers/BSP/STM32N6570-DK -I../../../Drivers/STM32N6xx_HAL_Driver/Inc -I../../../Drivers/CMSIS/Device/ST/STM32N6xx/Include -I../../../Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -I../../../Drivers/CMSIS/Include -I../../../Drivers/BSP/Components/imx335 -I../../../Drivers/BSP/Components/Common -I../../../Drivers/BSP/Components/rk050hr18 -I../../../Middlewares/ST/STM32_ISP_Library/evision/Inc -I../../../Middlewares/ST/STM32_ISP_Library/isp/Inc -I../../../FSBL/mtk3_bsp2 -I../../../FSBL/mtk3_bsp2/config -I../../../FSBL/mtk3_bsp2/include -I../../../FSBL/mtk3_bsp2/mtkernel/kernel/knlinc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -mcmse -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
mtk3_bsp2/mtkernel/lib/libtk/fastmlock.o: C:/Users/Dousik/Workspace/TRON/sessions/session_13/FSBL/mtk3_bsp2/mtkernel/lib/libtk/fastmlock.c mtk3_bsp2/mtkernel/lib/libtk/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m55 -std=gnu11 -g3 -DDEBUG -DMEDSIGHT_DEBUG=1 -DSTM32N657xx -DUSE_FULL_ASSERT -DUSE_HAL_DRIVER -D_STM32CUBE_DISCOVERY_N657_ -c -I../../../FSBL/Inc -I../../../Drivers/BSP/STM32N6570-DK -I../../../Drivers/STM32N6xx_HAL_Driver/Inc -I../../../Drivers/CMSIS/Device/ST/STM32N6xx/Include -I../../../Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -I../../../Drivers/CMSIS/Include -I../../../Drivers/BSP/Components/imx335 -I../../../Drivers/BSP/Components/Common -I../../../Drivers/BSP/Components/rk050hr18 -I../../../Middlewares/ST/STM32_ISP_Library/evision/Inc -I../../../Middlewares/ST/STM32_ISP_Library/isp/Inc -I../../../FSBL/mtk3_bsp2 -I../../../FSBL/mtk3_bsp2/config -I../../../FSBL/mtk3_bsp2/include -I../../../FSBL/mtk3_bsp2/mtkernel/kernel/knlinc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -mcmse -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
mtk3_bsp2/mtkernel/lib/libtk/kmalloc.o: C:/Users/Dousik/Workspace/TRON/sessions/session_13/FSBL/mtk3_bsp2/mtkernel/lib/libtk/kmalloc.c mtk3_bsp2/mtkernel/lib/libtk/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m55 -std=gnu11 -g3 -DDEBUG -DMEDSIGHT_DEBUG=1 -DSTM32N657xx -DUSE_FULL_ASSERT -DUSE_HAL_DRIVER -D_STM32CUBE_DISCOVERY_N657_ -c -I../../../FSBL/Inc -I../../../Drivers/BSP/STM32N6570-DK -I../../../Drivers/STM32N6xx_HAL_Driver/Inc -I../../../Drivers/CMSIS/Device/ST/STM32N6xx/Include -I../../../Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -I../../../Drivers/CMSIS/Include -I../../../Drivers/BSP/Components/imx335 -I../../../Drivers/BSP/Components/Common -I../../../Drivers/BSP/Components/rk050hr18 -I../../../Middlewares/ST/STM32_ISP_Library/evision/Inc -I../../../Middlewares/ST/STM32_ISP_Library/isp/Inc -I../../../FSBL/mtk3_bsp2 -I../../../FSBL/mtk3_bsp2/config -I../../../FSBL/mtk3_bsp2/include -I../../../FSBL/mtk3_bsp2/mtkernel/kernel/knlinc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -mcmse -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-mtk3_bsp2-2f-mtkernel-2f-lib-2f-libtk

clean-mtk3_bsp2-2f-mtkernel-2f-lib-2f-libtk:
	-$(RM) ./mtk3_bsp2/mtkernel/lib/libtk/fastlock.cyclo ./mtk3_bsp2/mtkernel/lib/libtk/fastlock.d ./mtk3_bsp2/mtkernel/lib/libtk/fastlock.o ./mtk3_bsp2/mtkernel/lib/libtk/fastlock.su ./mtk3_bsp2/mtkernel/lib/libtk/fastmlock.cyclo ./mtk3_bsp2/mtkernel/lib/libtk/fastmlock.d ./mtk3_bsp2/mtkernel/lib/libtk/fastmlock.o ./mtk3_bsp2/mtkernel/lib/libtk/fastmlock.su ./mtk3_bsp2/mtkernel/lib/libtk/kmalloc.cyclo ./mtk3_bsp2/mtkernel/lib/libtk/kmalloc.d ./mtk3_bsp2/mtkernel/lib/libtk/kmalloc.o ./mtk3_bsp2/mtkernel/lib/libtk/kmalloc.su

.PHONY: clean-mtk3_bsp2-2f-mtkernel-2f-lib-2f-libtk

