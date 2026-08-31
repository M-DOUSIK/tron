################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/Users/Dousik/Workspace/TRON/scratch/sample_model/.ai/run/run-1/Middlewares/ST/AI/Misc/Src/aiTestHelper_ST_AI.c \
C:/Users/Dousik/Workspace/TRON/scratch/sample_model/.ai/run/run-1/Middlewares/ST/AI/Misc/Src/aiTestUtility.c \
C:/Users/Dousik/Workspace/TRON/scratch/sample_model/.ai/run/run-1/Middlewares/ST/AI/Misc/Src/ai_device_adaptor.c \
C:/Users/Dousik/Workspace/TRON/scratch/sample_model/.ai/run/run-1/Middlewares/ST/AI/Misc/Src/lc_print.c \
C:/Users/Dousik/Workspace/TRON/scratch/sample_model/.ai/run/run-1/Middlewares/ST/AI/Misc/Src/syscalls.c 

OBJS += \
./Middlewares/ST/AI/Misc/Src/aiTestHelper_ST_AI.o \
./Middlewares/ST/AI/Misc/Src/aiTestUtility.o \
./Middlewares/ST/AI/Misc/Src/ai_device_adaptor.o \
./Middlewares/ST/AI/Misc/Src/lc_print.o \
./Middlewares/ST/AI/Misc/Src/syscalls.o 

C_DEPS += \
./Middlewares/ST/AI/Misc/Src/aiTestHelper_ST_AI.d \
./Middlewares/ST/AI/Misc/Src/aiTestUtility.d \
./Middlewares/ST/AI/Misc/Src/ai_device_adaptor.d \
./Middlewares/ST/AI/Misc/Src/lc_print.d \
./Middlewares/ST/AI/Misc/Src/syscalls.d 


# Each subdirectory must supply rules for building sources it contributes
Middlewares/ST/AI/Misc/Src/aiTestHelper_ST_AI.o: C:/Users/Dousik/Workspace/TRON/scratch/sample_model/.ai/run/run-1/Middlewares/ST/AI/Misc/Src/aiTestHelper_ST_AI.c Middlewares/ST/AI/Misc/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m55 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32N657xx -DHAVE_NETWORK_INFO -DLL_ATON_DUMP_DEBUG_API -DLL_ATON_PLATFORM=LL_ATON_PLAT_STM32N6 -DLL_ATON_OSAL=LL_ATON_OSAL_BARE_METAL -DLL_ATON_RT_MODE=LL_ATON_RT_ASYNC -DLL_ATON_SW_FALLBACK -DLL_ATON_EB_DBG_INFO -DLL_ATON_DBG_BUFFER_INFO_EXCLUDED=1 -DATON_STAI_API -c -I../../../Appli/Core/Inc -I../../../Secure_nsclib -I../../../Drivers/STM32N6xx_HAL_Driver/Inc -I../../../Drivers/CMSIS/Device/ST/STM32N6xx/Include -I../../../Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -I../../../Drivers/CMSIS/Include -I../../../Middlewares/ST/AI/Npu -I../../../Middlewares/ST/AI/Inc -I../../../Middlewares/ST/AI/Misc/Inc -I../../../Middlewares/ST/AI/Validation/Inc -I../../../Middlewares/ST/AI/Misc/Src -I../../../Middlewares/ST/AI/Validation/Src -I../../../Appli/AI/App -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -mcmse -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Middlewares/ST/AI/Misc/Src/aiTestUtility.o: C:/Users/Dousik/Workspace/TRON/scratch/sample_model/.ai/run/run-1/Middlewares/ST/AI/Misc/Src/aiTestUtility.c Middlewares/ST/AI/Misc/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m55 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32N657xx -DHAVE_NETWORK_INFO -DLL_ATON_DUMP_DEBUG_API -DLL_ATON_PLATFORM=LL_ATON_PLAT_STM32N6 -DLL_ATON_OSAL=LL_ATON_OSAL_BARE_METAL -DLL_ATON_RT_MODE=LL_ATON_RT_ASYNC -DLL_ATON_SW_FALLBACK -DLL_ATON_EB_DBG_INFO -DLL_ATON_DBG_BUFFER_INFO_EXCLUDED=1 -DATON_STAI_API -c -I../../../Appli/Core/Inc -I../../../Secure_nsclib -I../../../Drivers/STM32N6xx_HAL_Driver/Inc -I../../../Drivers/CMSIS/Device/ST/STM32N6xx/Include -I../../../Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -I../../../Drivers/CMSIS/Include -I../../../Middlewares/ST/AI/Npu -I../../../Middlewares/ST/AI/Inc -I../../../Middlewares/ST/AI/Misc/Inc -I../../../Middlewares/ST/AI/Validation/Inc -I../../../Middlewares/ST/AI/Misc/Src -I../../../Middlewares/ST/AI/Validation/Src -I../../../Appli/AI/App -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -mcmse -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Middlewares/ST/AI/Misc/Src/ai_device_adaptor.o: C:/Users/Dousik/Workspace/TRON/scratch/sample_model/.ai/run/run-1/Middlewares/ST/AI/Misc/Src/ai_device_adaptor.c Middlewares/ST/AI/Misc/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m55 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32N657xx -DHAVE_NETWORK_INFO -DLL_ATON_DUMP_DEBUG_API -DLL_ATON_PLATFORM=LL_ATON_PLAT_STM32N6 -DLL_ATON_OSAL=LL_ATON_OSAL_BARE_METAL -DLL_ATON_RT_MODE=LL_ATON_RT_ASYNC -DLL_ATON_SW_FALLBACK -DLL_ATON_EB_DBG_INFO -DLL_ATON_DBG_BUFFER_INFO_EXCLUDED=1 -DATON_STAI_API -c -I../../../Appli/Core/Inc -I../../../Secure_nsclib -I../../../Drivers/STM32N6xx_HAL_Driver/Inc -I../../../Drivers/CMSIS/Device/ST/STM32N6xx/Include -I../../../Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -I../../../Drivers/CMSIS/Include -I../../../Middlewares/ST/AI/Npu -I../../../Middlewares/ST/AI/Inc -I../../../Middlewares/ST/AI/Misc/Inc -I../../../Middlewares/ST/AI/Validation/Inc -I../../../Middlewares/ST/AI/Misc/Src -I../../../Middlewares/ST/AI/Validation/Src -I../../../Appli/AI/App -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -mcmse -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Middlewares/ST/AI/Misc/Src/lc_print.o: C:/Users/Dousik/Workspace/TRON/scratch/sample_model/.ai/run/run-1/Middlewares/ST/AI/Misc/Src/lc_print.c Middlewares/ST/AI/Misc/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m55 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32N657xx -DHAVE_NETWORK_INFO -DLL_ATON_DUMP_DEBUG_API -DLL_ATON_PLATFORM=LL_ATON_PLAT_STM32N6 -DLL_ATON_OSAL=LL_ATON_OSAL_BARE_METAL -DLL_ATON_RT_MODE=LL_ATON_RT_ASYNC -DLL_ATON_SW_FALLBACK -DLL_ATON_EB_DBG_INFO -DLL_ATON_DBG_BUFFER_INFO_EXCLUDED=1 -DATON_STAI_API -c -I../../../Appli/Core/Inc -I../../../Secure_nsclib -I../../../Drivers/STM32N6xx_HAL_Driver/Inc -I../../../Drivers/CMSIS/Device/ST/STM32N6xx/Include -I../../../Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -I../../../Drivers/CMSIS/Include -I../../../Middlewares/ST/AI/Npu -I../../../Middlewares/ST/AI/Inc -I../../../Middlewares/ST/AI/Misc/Inc -I../../../Middlewares/ST/AI/Validation/Inc -I../../../Middlewares/ST/AI/Misc/Src -I../../../Middlewares/ST/AI/Validation/Src -I../../../Appli/AI/App -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -mcmse -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Middlewares/ST/AI/Misc/Src/syscalls.o: C:/Users/Dousik/Workspace/TRON/scratch/sample_model/.ai/run/run-1/Middlewares/ST/AI/Misc/Src/syscalls.c Middlewares/ST/AI/Misc/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m55 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32N657xx -DHAVE_NETWORK_INFO -DLL_ATON_DUMP_DEBUG_API -DLL_ATON_PLATFORM=LL_ATON_PLAT_STM32N6 -DLL_ATON_OSAL=LL_ATON_OSAL_BARE_METAL -DLL_ATON_RT_MODE=LL_ATON_RT_ASYNC -DLL_ATON_SW_FALLBACK -DLL_ATON_EB_DBG_INFO -DLL_ATON_DBG_BUFFER_INFO_EXCLUDED=1 -DATON_STAI_API -c -I../../../Appli/Core/Inc -I../../../Secure_nsclib -I../../../Drivers/STM32N6xx_HAL_Driver/Inc -I../../../Drivers/CMSIS/Device/ST/STM32N6xx/Include -I../../../Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -I../../../Drivers/CMSIS/Include -I../../../Middlewares/ST/AI/Npu -I../../../Middlewares/ST/AI/Inc -I../../../Middlewares/ST/AI/Misc/Inc -I../../../Middlewares/ST/AI/Validation/Inc -I../../../Middlewares/ST/AI/Misc/Src -I../../../Middlewares/ST/AI/Validation/Src -I../../../Appli/AI/App -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -mcmse -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Middlewares-2f-ST-2f-AI-2f-Misc-2f-Src

clean-Middlewares-2f-ST-2f-AI-2f-Misc-2f-Src:
	-$(RM) ./Middlewares/ST/AI/Misc/Src/aiTestHelper_ST_AI.cyclo ./Middlewares/ST/AI/Misc/Src/aiTestHelper_ST_AI.d ./Middlewares/ST/AI/Misc/Src/aiTestHelper_ST_AI.o ./Middlewares/ST/AI/Misc/Src/aiTestHelper_ST_AI.su ./Middlewares/ST/AI/Misc/Src/aiTestUtility.cyclo ./Middlewares/ST/AI/Misc/Src/aiTestUtility.d ./Middlewares/ST/AI/Misc/Src/aiTestUtility.o ./Middlewares/ST/AI/Misc/Src/aiTestUtility.su ./Middlewares/ST/AI/Misc/Src/ai_device_adaptor.cyclo ./Middlewares/ST/AI/Misc/Src/ai_device_adaptor.d ./Middlewares/ST/AI/Misc/Src/ai_device_adaptor.o ./Middlewares/ST/AI/Misc/Src/ai_device_adaptor.su ./Middlewares/ST/AI/Misc/Src/lc_print.cyclo ./Middlewares/ST/AI/Misc/Src/lc_print.d ./Middlewares/ST/AI/Misc/Src/lc_print.o ./Middlewares/ST/AI/Misc/Src/lc_print.su ./Middlewares/ST/AI/Misc/Src/syscalls.cyclo ./Middlewares/ST/AI/Misc/Src/syscalls.d ./Middlewares/ST/AI/Misc/Src/syscalls.o ./Middlewares/ST/AI/Misc/Src/syscalls.su

.PHONY: clean-Middlewares-2f-ST-2f-AI-2f-Misc-2f-Src

