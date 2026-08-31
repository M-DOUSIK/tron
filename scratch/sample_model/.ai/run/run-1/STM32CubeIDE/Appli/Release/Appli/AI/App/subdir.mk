################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/Users/Dousik/Workspace/TRON/scratch/sample_model/.ai/run/run-1/Appli/AI/App/app_x-cube-ai.c \
C:/Users/Dousik/Workspace/TRON/scratch/sample_model/.ai/run/run-1/Appli/AI/App/network.c \
C:/Users/Dousik/Workspace/TRON/scratch/sample_model/.ai/run/run-1/Appli/AI/App/network_atonbuf.xSPI2.c \
C:/Users/Dousik/Workspace/TRON/scratch/sample_model/.ai/run/run-1/Appli/AI/App/network_weights.c \
C:/Users/Dousik/Workspace/TRON/scratch/sample_model/.ai/run/run-1/Appli/AI/App/npu_cache.c \
C:/Users/Dousik/Workspace/TRON/scratch/sample_model/.ai/run/run-1/Appli/AI/App/npu_init.c \
C:/Users/Dousik/Workspace/TRON/scratch/sample_model/.ai/run/run-1/Appli/AI/App/stai_network.c \
C:/Users/Dousik/Workspace/TRON/scratch/sample_model/.ai/run/run-1/Appli/AI/App/user_init.c 

OBJS += \
./Appli/AI/App/app_x-cube-ai.o \
./Appli/AI/App/network.o \
./Appli/AI/App/network_atonbuf.xSPI2.o \
./Appli/AI/App/network_weights.o \
./Appli/AI/App/npu_cache.o \
./Appli/AI/App/npu_init.o \
./Appli/AI/App/stai_network.o \
./Appli/AI/App/user_init.o 

C_DEPS += \
./Appli/AI/App/app_x-cube-ai.d \
./Appli/AI/App/network.d \
./Appli/AI/App/network_atonbuf.xSPI2.d \
./Appli/AI/App/network_weights.d \
./Appli/AI/App/npu_cache.d \
./Appli/AI/App/npu_init.d \
./Appli/AI/App/stai_network.d \
./Appli/AI/App/user_init.d 


# Each subdirectory must supply rules for building sources it contributes
Appli/AI/App/app_x-cube-ai.o: C:/Users/Dousik/Workspace/TRON/scratch/sample_model/.ai/run/run-1/Appli/AI/App/app_x-cube-ai.c Appli/AI/App/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m55 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32N657xx -DHAVE_NETWORK_INFO -DLL_ATON_DUMP_DEBUG_API -DLL_ATON_PLATFORM=LL_ATON_PLAT_STM32N6 -DLL_ATON_OSAL=LL_ATON_OSAL_BARE_METAL -DLL_ATON_RT_MODE=LL_ATON_RT_ASYNC -DLL_ATON_SW_FALLBACK -DLL_ATON_EB_DBG_INFO -DLL_ATON_DBG_BUFFER_INFO_EXCLUDED=1 -DATON_STAI_API -c -I../../../Appli/Core/Inc -I../../../Secure_nsclib -I../../../Drivers/STM32N6xx_HAL_Driver/Inc -I../../../Drivers/CMSIS/Device/ST/STM32N6xx/Include -I../../../Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -I../../../Drivers/CMSIS/Include -I../../../Middlewares/ST/AI/Npu -I../../../Middlewares/ST/AI/Inc -I../../../Middlewares/ST/AI/Misc/Inc -I../../../Middlewares/ST/AI/Validation/Inc -I../../../Middlewares/ST/AI/Misc/Src -I../../../Middlewares/ST/AI/Validation/Src -I../../../Appli/AI/App -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -mcmse -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Appli/AI/App/network.o: C:/Users/Dousik/Workspace/TRON/scratch/sample_model/.ai/run/run-1/Appli/AI/App/network.c Appli/AI/App/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m55 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32N657xx -DHAVE_NETWORK_INFO -DLL_ATON_DUMP_DEBUG_API -DLL_ATON_PLATFORM=LL_ATON_PLAT_STM32N6 -DLL_ATON_OSAL=LL_ATON_OSAL_BARE_METAL -DLL_ATON_RT_MODE=LL_ATON_RT_ASYNC -DLL_ATON_SW_FALLBACK -DLL_ATON_EB_DBG_INFO -DLL_ATON_DBG_BUFFER_INFO_EXCLUDED=1 -DATON_STAI_API -c -I../../../Appli/Core/Inc -I../../../Secure_nsclib -I../../../Drivers/STM32N6xx_HAL_Driver/Inc -I../../../Drivers/CMSIS/Device/ST/STM32N6xx/Include -I../../../Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -I../../../Drivers/CMSIS/Include -I../../../Middlewares/ST/AI/Npu -I../../../Middlewares/ST/AI/Inc -I../../../Middlewares/ST/AI/Misc/Inc -I../../../Middlewares/ST/AI/Validation/Inc -I../../../Middlewares/ST/AI/Misc/Src -I../../../Middlewares/ST/AI/Validation/Src -I../../../Appli/AI/App -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -mcmse -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Appli/AI/App/network_atonbuf.xSPI2.o: C:/Users/Dousik/Workspace/TRON/scratch/sample_model/.ai/run/run-1/Appli/AI/App/network_atonbuf.xSPI2.c Appli/AI/App/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m55 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32N657xx -DHAVE_NETWORK_INFO -DLL_ATON_DUMP_DEBUG_API -DLL_ATON_PLATFORM=LL_ATON_PLAT_STM32N6 -DLL_ATON_OSAL=LL_ATON_OSAL_BARE_METAL -DLL_ATON_RT_MODE=LL_ATON_RT_ASYNC -DLL_ATON_SW_FALLBACK -DLL_ATON_EB_DBG_INFO -DLL_ATON_DBG_BUFFER_INFO_EXCLUDED=1 -DATON_STAI_API -c -I../../../Appli/Core/Inc -I../../../Secure_nsclib -I../../../Drivers/STM32N6xx_HAL_Driver/Inc -I../../../Drivers/CMSIS/Device/ST/STM32N6xx/Include -I../../../Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -I../../../Drivers/CMSIS/Include -I../../../Middlewares/ST/AI/Npu -I../../../Middlewares/ST/AI/Inc -I../../../Middlewares/ST/AI/Misc/Inc -I../../../Middlewares/ST/AI/Validation/Inc -I../../../Middlewares/ST/AI/Misc/Src -I../../../Middlewares/ST/AI/Validation/Src -I../../../Appli/AI/App -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -mcmse -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Appli/AI/App/network_weights.o: C:/Users/Dousik/Workspace/TRON/scratch/sample_model/.ai/run/run-1/Appli/AI/App/network_weights.c Appli/AI/App/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m55 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32N657xx -DHAVE_NETWORK_INFO -DLL_ATON_DUMP_DEBUG_API -DLL_ATON_PLATFORM=LL_ATON_PLAT_STM32N6 -DLL_ATON_OSAL=LL_ATON_OSAL_BARE_METAL -DLL_ATON_RT_MODE=LL_ATON_RT_ASYNC -DLL_ATON_SW_FALLBACK -DLL_ATON_EB_DBG_INFO -DLL_ATON_DBG_BUFFER_INFO_EXCLUDED=1 -DATON_STAI_API -c -I../../../Appli/Core/Inc -I../../../Secure_nsclib -I../../../Drivers/STM32N6xx_HAL_Driver/Inc -I../../../Drivers/CMSIS/Device/ST/STM32N6xx/Include -I../../../Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -I../../../Drivers/CMSIS/Include -I../../../Middlewares/ST/AI/Npu -I../../../Middlewares/ST/AI/Inc -I../../../Middlewares/ST/AI/Misc/Inc -I../../../Middlewares/ST/AI/Validation/Inc -I../../../Middlewares/ST/AI/Misc/Src -I../../../Middlewares/ST/AI/Validation/Src -I../../../Appli/AI/App -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -mcmse -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Appli/AI/App/npu_cache.o: C:/Users/Dousik/Workspace/TRON/scratch/sample_model/.ai/run/run-1/Appli/AI/App/npu_cache.c Appli/AI/App/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m55 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32N657xx -DHAVE_NETWORK_INFO -DLL_ATON_DUMP_DEBUG_API -DLL_ATON_PLATFORM=LL_ATON_PLAT_STM32N6 -DLL_ATON_OSAL=LL_ATON_OSAL_BARE_METAL -DLL_ATON_RT_MODE=LL_ATON_RT_ASYNC -DLL_ATON_SW_FALLBACK -DLL_ATON_EB_DBG_INFO -DLL_ATON_DBG_BUFFER_INFO_EXCLUDED=1 -DATON_STAI_API -c -I../../../Appli/Core/Inc -I../../../Secure_nsclib -I../../../Drivers/STM32N6xx_HAL_Driver/Inc -I../../../Drivers/CMSIS/Device/ST/STM32N6xx/Include -I../../../Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -I../../../Drivers/CMSIS/Include -I../../../Middlewares/ST/AI/Npu -I../../../Middlewares/ST/AI/Inc -I../../../Middlewares/ST/AI/Misc/Inc -I../../../Middlewares/ST/AI/Validation/Inc -I../../../Middlewares/ST/AI/Misc/Src -I../../../Middlewares/ST/AI/Validation/Src -I../../../Appli/AI/App -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -mcmse -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Appli/AI/App/npu_init.o: C:/Users/Dousik/Workspace/TRON/scratch/sample_model/.ai/run/run-1/Appli/AI/App/npu_init.c Appli/AI/App/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m55 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32N657xx -DHAVE_NETWORK_INFO -DLL_ATON_DUMP_DEBUG_API -DLL_ATON_PLATFORM=LL_ATON_PLAT_STM32N6 -DLL_ATON_OSAL=LL_ATON_OSAL_BARE_METAL -DLL_ATON_RT_MODE=LL_ATON_RT_ASYNC -DLL_ATON_SW_FALLBACK -DLL_ATON_EB_DBG_INFO -DLL_ATON_DBG_BUFFER_INFO_EXCLUDED=1 -DATON_STAI_API -c -I../../../Appli/Core/Inc -I../../../Secure_nsclib -I../../../Drivers/STM32N6xx_HAL_Driver/Inc -I../../../Drivers/CMSIS/Device/ST/STM32N6xx/Include -I../../../Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -I../../../Drivers/CMSIS/Include -I../../../Middlewares/ST/AI/Npu -I../../../Middlewares/ST/AI/Inc -I../../../Middlewares/ST/AI/Misc/Inc -I../../../Middlewares/ST/AI/Validation/Inc -I../../../Middlewares/ST/AI/Misc/Src -I../../../Middlewares/ST/AI/Validation/Src -I../../../Appli/AI/App -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -mcmse -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Appli/AI/App/stai_network.o: C:/Users/Dousik/Workspace/TRON/scratch/sample_model/.ai/run/run-1/Appli/AI/App/stai_network.c Appli/AI/App/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m55 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32N657xx -DHAVE_NETWORK_INFO -DLL_ATON_DUMP_DEBUG_API -DLL_ATON_PLATFORM=LL_ATON_PLAT_STM32N6 -DLL_ATON_OSAL=LL_ATON_OSAL_BARE_METAL -DLL_ATON_RT_MODE=LL_ATON_RT_ASYNC -DLL_ATON_SW_FALLBACK -DLL_ATON_EB_DBG_INFO -DLL_ATON_DBG_BUFFER_INFO_EXCLUDED=1 -DATON_STAI_API -c -I../../../Appli/Core/Inc -I../../../Secure_nsclib -I../../../Drivers/STM32N6xx_HAL_Driver/Inc -I../../../Drivers/CMSIS/Device/ST/STM32N6xx/Include -I../../../Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -I../../../Drivers/CMSIS/Include -I../../../Middlewares/ST/AI/Npu -I../../../Middlewares/ST/AI/Inc -I../../../Middlewares/ST/AI/Misc/Inc -I../../../Middlewares/ST/AI/Validation/Inc -I../../../Middlewares/ST/AI/Misc/Src -I../../../Middlewares/ST/AI/Validation/Src -I../../../Appli/AI/App -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -mcmse -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Appli/AI/App/user_init.o: C:/Users/Dousik/Workspace/TRON/scratch/sample_model/.ai/run/run-1/Appli/AI/App/user_init.c Appli/AI/App/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m55 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32N657xx -DHAVE_NETWORK_INFO -DLL_ATON_DUMP_DEBUG_API -DLL_ATON_PLATFORM=LL_ATON_PLAT_STM32N6 -DLL_ATON_OSAL=LL_ATON_OSAL_BARE_METAL -DLL_ATON_RT_MODE=LL_ATON_RT_ASYNC -DLL_ATON_SW_FALLBACK -DLL_ATON_EB_DBG_INFO -DLL_ATON_DBG_BUFFER_INFO_EXCLUDED=1 -DATON_STAI_API -c -I../../../Appli/Core/Inc -I../../../Secure_nsclib -I../../../Drivers/STM32N6xx_HAL_Driver/Inc -I../../../Drivers/CMSIS/Device/ST/STM32N6xx/Include -I../../../Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -I../../../Drivers/CMSIS/Include -I../../../Middlewares/ST/AI/Npu -I../../../Middlewares/ST/AI/Inc -I../../../Middlewares/ST/AI/Misc/Inc -I../../../Middlewares/ST/AI/Validation/Inc -I../../../Middlewares/ST/AI/Misc/Src -I../../../Middlewares/ST/AI/Validation/Src -I../../../Appli/AI/App -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -mcmse -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Appli-2f-AI-2f-App

clean-Appli-2f-AI-2f-App:
	-$(RM) ./Appli/AI/App/app_x-cube-ai.cyclo ./Appli/AI/App/app_x-cube-ai.d ./Appli/AI/App/app_x-cube-ai.o ./Appli/AI/App/app_x-cube-ai.su ./Appli/AI/App/network.cyclo ./Appli/AI/App/network.d ./Appli/AI/App/network.o ./Appli/AI/App/network.su ./Appli/AI/App/network_atonbuf.xSPI2.cyclo ./Appli/AI/App/network_atonbuf.xSPI2.d ./Appli/AI/App/network_atonbuf.xSPI2.o ./Appli/AI/App/network_atonbuf.xSPI2.su ./Appli/AI/App/network_weights.cyclo ./Appli/AI/App/network_weights.d ./Appli/AI/App/network_weights.o ./Appli/AI/App/network_weights.su ./Appli/AI/App/npu_cache.cyclo ./Appli/AI/App/npu_cache.d ./Appli/AI/App/npu_cache.o ./Appli/AI/App/npu_cache.su ./Appli/AI/App/npu_init.cyclo ./Appli/AI/App/npu_init.d ./Appli/AI/App/npu_init.o ./Appli/AI/App/npu_init.su ./Appli/AI/App/stai_network.cyclo ./Appli/AI/App/stai_network.d ./Appli/AI/App/stai_network.o ./Appli/AI/App/stai_network.su ./Appli/AI/App/user_init.cyclo ./Appli/AI/App/user_init.d ./Appli/AI/App/user_init.o ./Appli/AI/App/user_init.su

.PHONY: clean-Appli-2f-AI-2f-App

