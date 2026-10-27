# target_f407.mk -- STM32F407ZGT6 (FK407M2-ZGT6 V1.1) target definitions
#
# COMPILE SPIKE. Builds the FreeJoyX application for the F407 with
# `make TARGET=f407 app`. The board layer in board/f407_zgt6 is a copy of
# board/f411_blackpill with only BOARD_ID and HSE_VALUE changed, so the
# image links but is NOT yet bootable on F407 hardware:
#   - board_init.c still carries the F411 25 MHz-HSE PLL recipe (the
#     FK407M2 has an 8 MHz crystal and a 42 MHz APB1 limit);
#   - board_tick.c / board_pwm.c / board_i2c.c / board_uart.c timing
#     constants still assume the F411 96 MHz bus clocks.
# The F407 bootloader is not ported: there are no TARGET_BOOT_* entries,
# so only the `app` goal is supported for this target.
#
# The app image keeps the F411 flash layout (config in sector 4 at
# 0x08010000, application from sector 5 at 0x08020000), which RM0090
# gives identically for the F407, so linker_app_f411.ld is reused.

#######################################
# CPU / FPU
#######################################
TARGET_CPU       = -mcpu=cortex-m4
TARGET_FPU       = -mfpu=fpv4-sp-d16
TARGET_FLOAT_ABI = -mfloat-abi=hard

#######################################
# Preprocessor defines
#######################################
TARGET_C_DEFS = \
-DSTM32F407xx \
-DUSE_FULL_LL_DRIVER \
-DUSE_HAL_DRIVER \
-DBOARD_F407_ZGT6 \
-DMCU_FAMILY_F4

# MCU_FAMILY_F4 selects the shared STM32F4 code paths (see target_f411.mk).
# BOARD_F407_ZGT6 is not tested anywhere in shared code yet, so the
# BlackPill-only blocks (TLE5011 generator, F411 ADC) compile out.

# USE_HAL_DRIVER: as on F411, HAL supplies the flash and USB PCD drivers.
# board/f407_zgt6/Inc/stm32f4xx_hal_conf.h is the F411 copy (FLASH, PCD,
# RCC, GPIO, CORTEX modules) with HSE_VALUE set for the 8 MHz crystal.

#######################################
# Vendor / driver / BSP include paths
#######################################
TARGET_C_INCLUDES = \
-I../Drivers/CMSIS/Core/Include \
-I../Drivers/CMSIS/Device/ST/STM32F4xx/Include \
-I../Drivers/STM32F4xx_HAL_Driver/Inc \
-I../Drivers/STM32_USB_Device_Library/Core/Inc \
-I../Drivers/STM32_USB_Device_Library/Class/CustomHID/Inc \
-I../board/common/Inc \
-I../board/f407_zgt6/Inc

#######################################
# C sources for the APPLICATION build: the shared application/Src files,
# the F407 BSP, the LL + HAL(flash/PCD) drivers, and the Cube USB Device
# library. Entry point is application/Src/main.c (shared with F103).
#######################################
TARGET_APP_C_SOURCES = \
../application/Src/main.c \
../board/f407_zgt6/Src/board_init.c \
../board/f407_zgt6/Src/board_flash.c \
../board/f407_zgt6/Src/board_tick.c \
../board/f407_zgt6/Src/board_encoder.c \
../board/f407_zgt6/Src/board_pins.c \
../board/f407_zgt6/Src/board_spi.c \
../board/f407_zgt6/Src/board_i2c.c \
../board/f407_zgt6/Src/board_uart.c \
../board/f407_zgt6/Src/board_dfu.c \
../board/f407_zgt6/Src/board_misc.c \
../board/f407_zgt6/Src/board_pwm.c \
../board/f407_zgt6/Src/board_ws2812b.c \
../board/f407_zgt6/Src/board_phase_stubs.c \
../board/f407_zgt6/Src/usbd_freejoy_desc.c \
../board/f407_zgt6/Src/usbd_freejoy_if.c \
../board/f407_zgt6/Src/usbd_freejoy_class.c \
../board/f407_zgt6/Src/usbd_conf.c \
../board/f407_zgt6/Src/board_usb.c \
../board/f407_zgt6/Src/stm32f4xx_it.c \
../board/f407_zgt6/Src/board_sensor_irqs.c \
../application/Src/buttons.c \
../application/Src/encoders.c \
../application/Src/image_id.c \
../application/Src/analog.c \
../application/Src/sensor_dispatch.c \
../application/Src/usb_app.c \
../application/Src/joy_report_desc.c \
../application/Src/periphery.c \
../application/Src/axis_to_buttons.c \
../application/Src/bitmap.c \
../application/Src/simhub.c \
../application/Src/config.c \
../application/Src/shift_registers.c \
../application/Src/gpio_expander.c \
../application/Src/leds.c \
../application/Src/led_effects.c \
../application/Src/tle5011.c \
../application/Src/tle5012.c \
../application/Src/as5048a.c \
../application/Src/mcp320x.c \
../application/Src/mlx90363.c \
../application/Src/mlx90393.c \
../application/Src/as5600.c \
../application/Src/ads1115.c \
../utils/crc16.c \
../utils/syscalls.c \
../Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/system_stm32f4xx.c \
../Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_dma.c \
../Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_gpio.c \
../Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_pwr.c \
../Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_rcc.c \
../Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_spi.c \
../Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_tim.c \
../Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_utils.c \
../Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_usb.c \
../Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c \
../Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c \
../Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c \
../Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c \
../Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc_ex.c \
../Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash.c \
../Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ex.c \
../Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pcd.c \
../Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pcd_ex.c \
../Drivers/STM32_USB_Device_Library/Core/Src/usbd_core.c \
../Drivers/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.c \
../Drivers/STM32_USB_Device_Library/Core/Src/usbd_ioreq.c \
../Drivers/STM32_USB_Device_Library/Class/CustomHID/Src/usbd_customhid.c
# stm32f4xx_hal_flash_ramfunc.c deliberately NOT compiled in: it provides
# RAM-resident flash ops for low-power-mode programming, which FreeJoyX
# doesn't use, and it pulls in __HAL_RCC_PWR_CLK_ENABLE which would force
# us to also vendor / enable HAL_RCC. Pure-flash boot-time programming
# uses only stm32f4xx_hal_flash{,_ex}.c.

#######################################
# Linker scripts
#######################################
# F407 sector map matches the F411 for this layout; see header.
TARGET_LDSCRIPT_APP  = linker_app_f411.ld

#######################################
# Startup assembly
#######################################
TARGET_ASM_SOURCES = startup_stm32f407xx.s
