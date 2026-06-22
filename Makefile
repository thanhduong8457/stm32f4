.PHONY: all build cmake debug release stm32f1-debug stm32f1-release stm32f4-debug stm32f4-release stm32f4-usb-debug stm32f4-usb-service-debug stm32f4-usb-release clean format flash

BUILD_DIR := build
BUILD_TYPE ?= Debug
STM32_TARGET ?= stm32f1
ADAS_USB_COMPOSITE ?= OFF
ADAS_USB_EXTRA_SERVICE_CDC ?= OFF

all: build

cmake:
	cmake \
		-S. \
		-B${BUILD_DIR} \
		-DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
		-DCMAKE_TOOLCHAIN_FILE=gcc-arm-none-eabi.cmake \
		-DSTM32_TARGET=${STM32_TARGET} \
		-DADAS_USB_COMPOSITE=${ADAS_USB_COMPOSITE} \
		-DADAS_USB_EXTRA_SERVICE_CDC=${ADAS_USB_EXTRA_SERVICE_CDC} \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON

build: cmake
	$(MAKE) -C ${BUILD_DIR} --no-print-directory

debug: BUILD_TYPE := Debug
debug: BUILD_DIR = build/${STM32_TARGET}-debug
debug: build

release: BUILD_TYPE := Release
release: BUILD_DIR = build/${STM32_TARGET}-release
release: build

stm32f1-debug: STM32_TARGET := stm32f1
stm32f1-debug: debug

stm32f1-release: STM32_TARGET := stm32f1
stm32f1-release: release

stm32f4-debug: STM32_TARGET := stm32f4
stm32f4-debug: debug

stm32f4-release: STM32_TARGET := stm32f4
stm32f4-release: release

stm32f4-usb-debug: STM32_TARGET := stm32f4
stm32f4-usb-debug: ADAS_USB_COMPOSITE := ON
stm32f4-usb-debug: BUILD_DIR = build/stm32f4-usb-debug
stm32f4-usb-debug: BUILD_TYPE := Debug
stm32f4-usb-debug: build

stm32f4-usb-service-debug: STM32_TARGET := stm32f4
stm32f4-usb-service-debug: ADAS_USB_COMPOSITE := ON
stm32f4-usb-service-debug: ADAS_USB_EXTRA_SERVICE_CDC := ON
stm32f4-usb-service-debug: BUILD_DIR = build/stm32f4-usb-service-debug
stm32f4-usb-service-debug: BUILD_TYPE := Debug
stm32f4-usb-service-debug: build

stm32f4-usb-release: STM32_TARGET := stm32f4
stm32f4-usb-release: ADAS_USB_COMPOSITE := ON
stm32f4-usb-release: BUILD_DIR = build/stm32f4-usb-release
stm32f4-usb-release: BUILD_TYPE := Release
stm32f4-usb-release: build

flash:
	st-flash --reset write ${BUILD_DIR}/adas_mcu_dev.bin 0x08000000

SRCS := $(shell find . -name '*.[ch]' -or -name '*.[ch]pp')
format: $(addsuffix .format,${SRCS})

%.format: %
	clang-format -i $<

clean:
	rm -rf $(BUILD_DIR)
