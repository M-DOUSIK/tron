################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/Users/Dousik/Workspace/TRON/scratch/sample_model/.ai/run/run-1/Appli/Core/Src/system_stm32n6xx_s.c 

OBJS += \
./Drivers/CMSIS/system_stm32n6xx_s.o 

C_DEPS += \
./Drivers/CMSIS/system_stm32n6xx_s.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/CMSIS/system_stm32n6xx_s.o: C:/Users/Dousik/Workspace/TRON/scratch/sample_model/.ai/run/run-1/Appli/Core/Src/system_stm32n6xx_s.c Drivers/CMSIS/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m55 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32N657xx -DHAVE_NETWORK_INFO -DLL_ATON_DUMP_DEBUG_API -DLL_ATON_PLATFORM=LL_ATON_PLAT_STM32N6 -DLL_ATON_OSAL=LL_ATON_OSAL_BARE_METAL -DLL_ATON_RT_MODE=LL_ATON_RT_ASYNC -DLL_ATON_SW_FALLBACK -DLL_ATON_EB_DBG_INFO -DLL_ATON_DBG_BUFFER_INFO_EXCLUDED=1 -DATON_STAI_API -c -I../../../Appli/Core/Inc -I../../../Secure_nsclib -I../../../Drivers/STM32N6xx_HAL_Driver/Inc -I../../../Drivers/CMSIS/Device/ST/STM32N6xx/Include -I../../../Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -I../../../Drivers/CMSIS/Include -I../../../Middlewares/ST/AI/Npu -I../../../Middlewares/ST/AI/Inc -I../../../Middlewares/ST/AI/Misc/Inc -I../../../Middlewares/ST/AI/Validation/Inc -I../../../Middlewares/ST/AI/Misc/Src -I../../../Middlewares/ST/AI/Validation/Src -I../../../Appli/AI/App -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -mcmse -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-CMSIS

clean-Drivers-2f-CMSIS:
	-$(RM) ./Drivers/CMSIS/system_stm32n6xx_s.cyclo ./Drivers/CMSIS/system_stm32n6xx_s.d ./Drivers/CMSIS/system_stm32n6xx_s.o ./Drivers/CMSIS/system_stm32n6xx_s.su

.PHONY: clean-Drivers-2f-CMSIS

