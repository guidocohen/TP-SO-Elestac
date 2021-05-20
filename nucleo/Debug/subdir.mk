################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../nucleo.c 

OBJS += \
./nucleo.o 

C_DEPS += \
./nucleo.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/git/tp-2016-1c-YoNoFui/libraries/commons-YoNoFui" -include"/home/utnso/git/tp-2016-1c-YoNoFui/libraries/commons-YoNoFui/sockets.c" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


