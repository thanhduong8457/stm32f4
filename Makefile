.PHONY: all build cmake clean format flash

BUILD_DIR := build
BUILD_TYPE ?= Debug
STM32_TARGET ?= stm32f1

all: build

cmake:
	cmake \
		-S. \
		-B${BUILD_DIR} \
		-DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
		-DCMAKE_TOOLCHAIN_FILE=gcc-arm-none-eabi.cmake \
		-DSTM32_TARGET=${STM32_TARGET} \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON

build: cmake
	$(MAKE) -C ${BUILD_DIR} --no-print-directory

flash:
	st-flash --reset write ${BUILD_DIR}/adas_mcu_dev.bin 0x08000000

SRCS := $(shell find . -name '*.[ch]' -or -name '*.[ch]pp')
format: $(addsuffix .format,${SRCS})

%.format: %
	clang-format -i $<

clean:
	rm -rf $(BUILD_DIR)
