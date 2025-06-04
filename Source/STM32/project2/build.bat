@echo off
REM Script để build và nạp firmware STM32
echo === Dang build firmware ===

REM Đường dẫn đến các công cụ
set ARM_GCC_PATH=C:\ST\STM32CubeCLT_1.18.0\GNU-tools-for-STM32\bin
set STM32_PROGRAMMER=C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe

REM Tạo thư mục build nếu chưa có
if not exist build mkdir build
cd build

REM Build project bằng arm-gcc trực tiếp thay vì qua CMake
echo Compiling...
"%ARM_GCC_PATH%\arm-none-eabi-gcc" -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -O0 -g3 -Wall -fdata-sections -ffunction-sections ^
-I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include ^
-DSTM32F407xx -DUSE_HAL_DRIVER ^
../Core/Src/main.c ../Core/Src/i2c_lcd.c ../Core/Src/BH1750.c ../Core/Src/DHT22.c ^
../Core/Src/gpio.c ../Core/Src/adc.c ../Core/Src/i2c.c ../Core/Src/tim.c ../Core/Src/usart.c ../Core/Src/stm32f4xx_it.c ^
../Core/Src/stm32f4xx_hal_msp.c ../Drivers/STM32F4xx_HAL_Driver/Src/*.c ../Core/Src/system_stm32f4xx.c ^
-T../STM32F407XX_FLASH.ld ^
-Wl,-Map=build.map,--cref -Wl,--gc-sections -lc -lm --specs=nano.specs -o firmware.elf

if %ERRORLEVEL% NEQ 0 (
    echo Lỗi biên dịch!
    cd ..
    exit /b 1
)

REM Tạo binary từ elf
echo Generating binary...
"%ARM_GCC_PATH%\arm-none-eabi-objcopy" -O binary firmware.elf firmware.bin
"%ARM_GCC_PATH%\arm-none-eabi-size" firmware.elf

REM Nạp chương trình
echo === Nạp firmware ===
if exist "%STM32_PROGRAMMER%" (
    "%STM32_PROGRAMMER%" -c port=SWD -w firmware.bin 0x8000000 -v -rst
) else (
    echo Không tìm thấy STM32_Programmer_CLI. Vui lòng cài đặt STM32CubeProgrammer.
)

cd ..
echo Hoàn tất! 