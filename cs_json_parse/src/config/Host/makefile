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
all: ../../../lib/libgsi-parse-json-config.a

# Tool invocations
../../../lib/libgsi-parse-json-config.a: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GCC Archiver'
	ar -r  $@ $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(ARCHIVES) $(OBJS) $(C_DEPS)
	-@echo ' '

deploy:
	@echo "Nothing to deploy"

.PHONY: all clean dependents

-include ../makefile.targets
