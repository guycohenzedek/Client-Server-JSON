################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: ../../../bin/gsi_parse_json_client_1

# Tool invocations
../../../bin/gsi_parse_json_client_1: $(C_OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C Linker'
	gcc $(LIBDIRS) -o $@ $(C_OBJS) $(USER_OBJS) $(LIBS) -DLOG_LEVEL=$(LOG_LEVEL)
	objdump -x --source $@ > $@.objdump
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(ARCHIVES) $(C_OBJS) $(C_DEPS)
	-@echo ' '

deploy:
	@echo "Nothing to deploy"

.PHONY: all clean dependents

-include ../makefile.targets
