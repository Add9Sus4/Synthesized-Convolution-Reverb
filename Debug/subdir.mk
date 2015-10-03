################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../convolution.c \
../convolve.c \
../dawsonaudio.c \
../fft.c \
../impulse.c \
../vector.c 

OBJS += \
./convolution.o \
./convolve.o \
./dawsonaudio.o \
./fft.o \
./impulse.o \
./vector.o 

C_DEPS += \
./convolution.d \
./convolve.d \
./dawsonaudio.d \
./fft.d \
./impulse.d \
./vector.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


