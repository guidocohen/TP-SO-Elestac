################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../swap.c 

OBJS += \
./swap.o 

C_DEPS += \
./swap.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	gcc -I"/home/utnso/git/tp-2016-1c-YoNoFui/libraries/commons-YoNoFui" -include"/home/utnso/git/tp-2016-1c-YoNoFui/libraries/commons-YoNoFui/sockets.c" -O0 -g3 -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


