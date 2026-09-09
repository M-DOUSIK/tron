################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/Users/Dousik/Workspace/TRON/sessions/session_12/Middlewares/ST/STM32_ISP_Library/isp/Src/isp_algo.c \
C:/Users/Dousik/Workspace/TRON/sessions/session_12/Middlewares/ST/STM32_ISP_Library/isp/Src/isp_core.c \
C:/Users/Dousik/Workspace/TRON/sessions/session_12/Middlewares/ST/STM32_ISP_Library/isp/Src/isp_services.c 

OBJS += \
./Middlewares/STM32_ISP/isp_algo.o \
./Middlewares/STM32_ISP/isp_core.o \
./Middlewares/STM32_ISP/isp_services.o 

C_DEPS += \
./Middlewares/STM32_ISP/isp_algo.d \
./Middlewares/STM32_ISP/isp_core.d \
./Middlewares/STM32_ISP/isp_services.d 


# Each subdirectory must supply rules for building sources it contributes
Middlewares/STM32_ISP/isp_algo.o: C:/Users/Dousik/Workspace/TRON/sessions/session_12/Middlewares/ST/STM32_ISP_Library/isp/Src/isp_algo.c Middlewares/STM32_ISP/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m55 -std=gnu11 -DDEBUG -DSTM32N657xx -DUSE_FULL_ASSERT -DUSE_HAL_DRIVER -D_STM32CUBE_DISCOVERY_N657_ -c -I../../../FSBL/Inc -I../../../Drivers/BSP/STM32N6570-DK -I../../../Drivers/STM32N6xx_HAL_Driver/Inc -I../../../Drivers/CMSIS/Device/ST/STM32N6xx/Include -I../../../Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -I../../../Drivers/CMSIS/Include -I../../../Drivers/BSP/Components/imx335 -I../../../Drivers/BSP/Components/Common -I../../../Drivers/BSP/Components/rk050hr18 -I../../../Middlewares/ST/STM32_ISP_Library/evision/Inc -I../../../Middlewares/ST/STM32_ISP_Library/isp/Inc -I../../../FSBL/mtk3_bsp2 -I../../../FSBL/mtk3_bsp2/config -I../../../FSBL/mtk3_bsp2/include -I../../../FSBL/mtk3_bsp2/mtkernel/kernel/knlinc -Os -ffunction-sections -fdata-sections -Wall -fno-toplevel-reorder -fstack-usage -fcyclomatic-complexity -mcmse -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Middlewares/STM32_ISP/isp_core.o: C:/Users/Dousik/Workspace/TRON/sessions/session_12/Middlewares/ST/STM32_ISP_Library/isp/Src/isp_core.c Middlewares/STM32_ISP/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m55 -std=gnu11 -DDEBUG -DSTM32N657xx -DUSE_FULL_ASSERT -DUSE_HAL_DRIVER -D_STM32CUBE_DISCOVERY_N657_ -c -I../../../FSBL/Inc -I../../../Drivers/BSP/STM32N6570-DK -I../../../Drivers/STM32N6xx_HAL_Driver/Inc -I../../../Drivers/CMSIS/Device/ST/STM32N6xx/Include -I../../../Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -I../../../Drivers/CMSIS/Include -I../../../Drivers/BSP/Components/imx335 -I../../../Drivers/BSP/Components/Common -I../../../Drivers/BSP/Components/rk050hr18 -I../../../Middlewares/ST/STM32_ISP_Library/evision/Inc -I../../../Middlewares/ST/STM32_ISP_Library/isp/Inc -I../../../FSBL/mtk3_bsp2 -I../../../FSBL/mtk3_bsp2/config -I../../../FSBL/mtk3_bsp2/include -I../../../FSBL/mtk3_bsp2/mtkernel/kernel/knlinc -Os -ffunction-sections -fdata-sections -Wall -fno-toplevel-reorder -fstack-usage -fcyclomatic-complexity -mcmse -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Middlewares/STM32_ISP/isp_services.o: C:/Users/Dousik/Workspace/TRON/sessions/session_12/Middlewares/ST/STM32_ISP_Library/isp/Src/isp_services.c Middlewares/STM32_ISP/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m55 -std=gnu11 -DDEBUG -DSTM32N657xx -DUSE_FULL_ASSERT -DUSE_HAL_DRIVER -D_STM32CUBE_DISCOVERY_N657_ -c -I../../../FSBL/Inc -I../../../Drivers/BSP/STM32N6570-DK -I../../../Drivers/STM32N6xx_HAL_Driver/Inc -I../../../Drivers/CMSIS/Device/ST/STM32N6xx/Include -I../../../Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -I../../../Drivers/CMSIS/Include -I../../../Drivers/BSP/Components/imx335 -I../../../Drivers/BSP/Components/Common -I../../../Drivers/BSP/Components/rk050hr18 -I../../../Middlewares/ST/STM32_ISP_Library/evision/Inc -I../../../Middlewares/ST/STM32_ISP_Library/isp/Inc -I../../../FSBL/mtk3_bsp2 -I../../../FSBL/mtk3_bsp2/config -I../../../FSBL/mtk3_bsp2/include -I../../../FSBL/mtk3_bsp2/mtkernel/kernel/knlinc -Os -ffunction-sections -fdata-sections -Wall -fno-toplevel-reorder -fstack-usage -fcyclomatic-complexity -mcmse -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Middlewares-2f-STM32_ISP

clean-Middlewares-2f-STM32_ISP:
	-$(RM) ./Middlewares/STM32_ISP/isp_algo.cyclo ./Middlewares/STM32_ISP/isp_algo.d ./Middlewares/STM32_ISP/isp_algo.o ./Middlewares/STM32_ISP/isp_algo.su ./Middlewares/STM32_ISP/isp_core.cyclo ./Middlewares/STM32_ISP/isp_core.d ./Middlewares/STM32_ISP/isp_core.o ./Middlewares/STM32_ISP/isp_core.su ./Middlewares/STM32_ISP/isp_services.cyclo ./Middlewares/STM32_ISP/isp_services.d ./Middlewares/STM32_ISP/isp_services.o ./Middlewares/STM32_ISP/isp_services.su

.PHONY: clean-Middlewares-2f-STM32_ISP

