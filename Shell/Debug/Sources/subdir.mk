################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Sources/cmd.c \
../Sources/delay.c \
../Sources/fio.c \
../Sources/led.c \
../Sources/mem.c \
../Sources/pcb.c \
../Sources/pushbutton.c \
../Sources/shell.c \
../Sources/uart.c \
../Sources/utl.c 

OBJS += \
./Sources/cmd.o \
./Sources/delay.o \
./Sources/fio.o \
./Sources/led.o \
./Sources/mem.o \
./Sources/pcb.o \
./Sources/pushbutton.o \
./Sources/shell.o \
./Sources/uart.o \
./Sources/utl.o 

C_DEPS += \
./Sources/cmd.d \
./Sources/delay.d \
./Sources/fio.d \
./Sources/led.d \
./Sources/mem.d \
./Sources/pcb.d \
./Sources/pushbutton.d \
./Sources/shell.d \
./Sources/uart.d \
./Sources/utl.d 


# Each subdirectory must supply rules for building sources it contributes
Sources/%.o: ../Sources/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections  -g3 -I"../Sources" -I"../Includes" -std=c99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


