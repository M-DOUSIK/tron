################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include Middlewares/STM32_ISP/subdir.mk
-include Drivers/STM32N6xx_HAL_Driver/subdir.mk
-include Drivers/CMSIS/subdir.mk
-include Drivers/BSP/STM32N6570-DK/subdir.mk
-include Drivers/BSP/Components/aps256xx/subdir.mk
-include Drivers/BSP/Components/IMX335/subdir.mk
-include Application/User/subdir.mk
-include Application/Startup/subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

OPTIONAL_TOOL_DEPS := \
$(wildcard ../makefile.defs) \
$(wildcard ../makefile.init) \
$(wildcard ../makefile.targets) \


BUILD_ARTIFACT_NAME := session_03_FSBL
BUILD_ARTIFACT_EXTENSION := elf
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 
EXECUTABLES += \
session_03_FSBL.elf \

MAP_FILES += \
session_03_FSBL.map \

SIZE_OUTPUT += \
default.size.stdout \

OBJDUMP_LIST += \
session_03_FSBL.list \

OBJCOPY_BIN += \
session_03_FSBL.bin \


# All Target
all: main-build

# Main-build Target
main-build: session_03_FSBL.elf secondary-outputs

# Tool invocations
session_03_FSBL.elf session_03_FSBL.map: $(OBJS) $(USER_OBJS) C:\Users\Dousik\Workspace\TRON\sessions\session_03\STM32CubeIDE\FSBL\STM32N657X0HXQ_AXISRAM2_fsbl.ld makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-gcc -o "session_03_FSBL.elf" @"objects.list" $(USER_OBJS) $(LIBS) -mcpu=cortex-m55 -T"C:\Users\Dousik\Workspace\TRON\sessions\session_03\STM32CubeIDE\FSBL\STM32N657X0HXQ_AXISRAM2_fsbl.ld" --specs=nosys.specs -Wl,-Map="session_03_FSBL.map" -Wl,--gc-sections -static -Wl,--cmse-implib -Wl,--out-implib=./session_03_FSBL_import_lib.o -LC:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0/Middlewares/ST/STM32_ISP_Library/evision/Lib --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -Wl,--start-group -lc -lm -Wl,--end-group
	@echo 'Finished building target: $@'
	@echo ' '

default.size.stdout: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-size  $(EXECUTABLES)
	@echo 'Finished building: $@'
	@echo ' '

session_03_FSBL.list: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-objdump -h -S $(EXECUTABLES) > "session_03_FSBL.list"
	@echo 'Finished building: $@'
	@echo ' '

session_03_FSBL.bin: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-objcopy  -O binary $(EXECUTABLES) "session_03_FSBL.bin"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) default.size.stdout session_03_FSBL.bin session_03_FSBL.elf session_03_FSBL.list session_03_FSBL.map
	-@echo ' '

secondary-outputs: $(SIZE_OUTPUT) $(OBJDUMP_LIST) $(OBJCOPY_BIN)

fail-specified-linker-script-missing:
	@echo 'Error: Cannot find the specified linker script. Check the linker settings in the build configuration.'
	@exit 2

warn-no-linker-script-specified:
	@echo 'Warning: No linker script specified. Check the linker settings in the build configuration.'

.PHONY: all clean dependents main-build fail-specified-linker-script-missing warn-no-linker-script-specified

-include ../makefile.targets
