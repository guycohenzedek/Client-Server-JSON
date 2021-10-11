################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/gsi_is_network_tcp.c 

OBJS += \
./src/gsi_is_network_tcp.o 

C_DEPS += \
./src/gsi_is_network_tcp.d

# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc $(INCLUDEDIRS) -O0 -g3 -Wall -Werror -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<" -DLOG_LEVEL=$(LOG_LEVEL)
	@echo 'Finished building: $<'
	@echo ' '


