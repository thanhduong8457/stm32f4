#include "platform/stm32f4/usb/usb_composite_device.hpp"

#include "FreeRTOS.h"
#include "portmacro.h"
#include "stm32f4xx.h"
#include "tusb.h"

namespace platform::stm32f4
{
namespace
{

constexpr uint8_t kDataCdc = 0;
constexpr uint8_t kServiceCdc = ADAS_USB_EXTRA_SERVICE_CDC ? 1 : 0;
constexpr uintptr_t kUsbOtgFsBase = 0x50000000UL;
constexpr uintptr_t kUsbOtgGccfgOffset = 0x38UL;
constexpr uint32_t kUsbOtgGccfgVbusASen = 1UL << 18;
constexpr uint32_t kUsbOtgGccfgVbusBSen = 1UL << 19;
constexpr uint32_t kUsbOtgGccfgNoVbusSens = 1UL << 21;

void sendToCdc(uint8_t cdcIndex, char ch)
{
    if (!tud_cdc_n_connected(cdcIndex))
    {
        return;
    }

    while (tud_cdc_n_write_available(cdcIndex) == 0U)
    {
        tud_task();
    }

    (void)tud_cdc_n_write_char(cdcIndex, ch);
    (void)tud_cdc_n_write_flush(cdcIndex);
}

} // namespace

void UsbCompositeDevice::setRxSink(hal::IUartRxSink *sink)
{
    rxSink_ = sink;
}

void UsbCompositeDevice::initialize()
{
    initializeHardware();
}

void UsbCompositeDevice::initializeHardware()
{
    GPIO_InitTypeDef gpioInit{};

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_OTG_FS, ENABLE);

    GPIO_PinAFConfig(GPIOA, GPIO_PinSource11, GPIO_AF_OTG_FS);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource12, GPIO_AF_OTG_FS);

    gpioInit.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_12;
    gpioInit.GPIO_Mode = GPIO_Mode_AF;
    gpioInit.GPIO_Speed = GPIO_Speed_100MHz;
    gpioInit.GPIO_OType = GPIO_OType_PP;
    gpioInit.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &gpioInit);

    volatile uint32_t &gccfg =
        *reinterpret_cast<volatile uint32_t *>(kUsbOtgFsBase + kUsbOtgGccfgOffset);
    gccfg |= kUsbOtgGccfgNoVbusSens;
    gccfg &= ~kUsbOtgGccfgVbusBSen;
    gccfg &= ~kUsbOtgGccfgVbusASen;
}

void UsbCompositeDevice::poll()
{
    if (!initialized_)
    {
        NVIC_SetPriority(OTG_FS_IRQn, 5);
        initialized_ = tusb_init();
        if (!initialized_)
        {
            return;
        }
    }

    tud_task();
    drainCdc(kDataCdc, hal::ByteStreamChannel::Data);

#if ADAS_USB_EXTRA_SERVICE_CDC
    drainCdc(kServiceCdc, hal::ByteStreamChannel::Service);
#endif
}

void UsbCompositeDevice::drainCdc(uint8_t cdcIndex, hal::ByteStreamChannel channel)
{
    if (rxSink_ == nullptr)
    {
        return;
    }

    uint8_t byte = 0;
    while (tud_cdc_n_available(cdcIndex) != 0U)
    {
        if (tud_cdc_n_read(cdcIndex, &byte, 1) == 0U)
        {
            break;
        }
        (void)rxSink_->onRxByteFromIsr(byte, channel, nullptr);
    }
}

void UsbCompositeDevice::send(char ch)
{
    sendTo(hal::ByteStreamChannel::Data, ch);
}

void UsbCompositeDevice::send(const char *text)
{
    sendTo(hal::ByteStreamChannel::Data, text);
}

void UsbCompositeDevice::sendTo(hal::ByteStreamChannel channel, char ch)
{
    if (!initialized_)
    {
        return;
    }

    sendToCdc(cdcIndexFor(channel), ch);
}

void UsbCompositeDevice::sendTo(hal::ByteStreamChannel channel, const char *text)
{
    while (*text != '\0')
    {
        sendTo(channel, *text++);
    }
}

bool UsbCompositeDevice::sendKeyboardReport(uint8_t modifier, const uint8_t keycode[6])
{
    if (!initialized_ || !tud_hid_ready())
    {
        return false;
    }

    return tud_hid_keyboard_report(0, modifier, keycode);
}

bool UsbCompositeDevice::releaseKeyboard()
{
    uint8_t keycode[6]{};
    return sendKeyboardReport(0, keycode);
}

uint8_t UsbCompositeDevice::cdcIndexFor(hal::ByteStreamChannel channel) const
{
#if ADAS_USB_EXTRA_SERVICE_CDC
    if (channel == hal::ByteStreamChannel::Service)
    {
        return kServiceCdc;
    }
#else
    (void)channel;
#endif
    return kDataCdc;
}

} // namespace platform::stm32f4

extern "C"
{

void OTG_FS_IRQHandler(void)
{
    tud_int_handler(0);
}

void tud_mount_cb(void)
{
}

void tud_umount_cb(void)
{
}

void tud_suspend_cb(bool remote_wakeup_en)
{
    (void)remote_wakeup_en;
}

void tud_resume_cb(void)
{
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type,
                                uint8_t *buffer, uint16_t reqlen)
{
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)reqlen;
    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type,
                            uint8_t const *buffer, uint16_t bufsize)
{
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)bufsize;
}

} // extern "C"
