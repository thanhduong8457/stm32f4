#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "stm32f4xx.h"
#include "tusb.h"

#define USB_VID 0xCafe
#define USB_PID (ADAS_USB_EXTRA_SERVICE_CDC ? 0x4103 : 0x4102)
#define USB_BCD 0x0200

#ifndef UID_BASE
#define UID_BASE 0x1FFF7A10UL
#endif

enum
{
    ITF_NUM_HID = 0,
    ITF_NUM_CDC_DATA_CONTROL,
    ITF_NUM_CDC_DATA_DATA,
#if ADAS_USB_EXTRA_SERVICE_CDC
    ITF_NUM_CDC_SERVICE_CONTROL,
    ITF_NUM_CDC_SERVICE_DATA,
#endif
    ITF_NUM_TOTAL,
};

enum
{
    STRID_LANGID = 0,
    STRID_MANUFACTURER,
    STRID_PRODUCT,
    STRID_SERIAL,
    STRID_HID,
    STRID_DATA_CDC,
    STRID_SERVICE_CDC,
};

#define EPNUM_HID_IN 0x81
#define EPNUM_DATA_CDC_NOTIF_IN 0x82
#define EPNUM_DATA_CDC_OUT 0x03
#define EPNUM_DATA_CDC_IN 0x83
#define EPNUM_SERVICE_CDC_NOTIF_IN 0x84
#define EPNUM_SERVICE_CDC_OUT 0x05
#define EPNUM_SERVICE_CDC_IN 0x85

#define CONFIG_TOTAL_LEN                                                                          \
    (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN + TUD_CDC_DESC_LEN +                                  \
     (ADAS_USB_EXTRA_SERVICE_CDC ? TUD_CDC_DESC_LEN : 0))

static tusb_desc_device_t const desc_device = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = USB_BCD,
    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = USB_VID,
    .idProduct = USB_PID,
    .bcdDevice = 0x0100,
    .iManufacturer = STRID_MANUFACTURER,
    .iProduct = STRID_PRODUCT,
    .iSerialNumber = STRID_SERIAL,
    .bNumConfigurations = 0x01,
};

uint8_t const *tud_descriptor_device_cb(void)
{
    return (uint8_t const *)&desc_device;
}

uint8_t const desc_hid_report[] = {
    TUD_HID_REPORT_DESC_KEYBOARD(),
};

uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    (void)instance;
    return desc_hid_report;
}

static uint8_t const desc_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),
    TUD_HID_DESCRIPTOR(ITF_NUM_HID, STRID_HID, HID_ITF_PROTOCOL_KEYBOARD,
                       sizeof(desc_hid_report), EPNUM_HID_IN, CFG_TUD_HID_EP_BUFSIZE, 10),
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC_DATA_CONTROL, STRID_DATA_CDC, EPNUM_DATA_CDC_NOTIF_IN, 16,
                       EPNUM_DATA_CDC_OUT, EPNUM_DATA_CDC_IN, 64),
#if ADAS_USB_EXTRA_SERVICE_CDC
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC_SERVICE_CONTROL, STRID_SERVICE_CDC,
                       EPNUM_SERVICE_CDC_NOTIF_IN, 16, EPNUM_SERVICE_CDC_OUT,
                       EPNUM_SERVICE_CDC_IN, 64),
#endif
};

uint8_t const *tud_descriptor_configuration_cb(uint8_t index)
{
    (void)index;
    return desc_configuration;
}

static char const *const string_desc_arr[] = {
    (const char[]){0x09, 0x04},
    "ADAS MCU",
    ADAS_USB_EXTRA_SERVICE_CDC ? "ADAS Keyboard + Data CDC + Service CDC"
                               : "ADAS Keyboard + Data CDC",
    NULL,
    "Keyboard HID",
    "Data COM",
#if ADAS_USB_EXTRA_SERVICE_CDC
    "Service COM",
#endif
};

static uint16_t desc_str[32 + 1];

static size_t append_hex16(uint16_t *out, size_t out_count, uint32_t value)
{
    static char const hex[] = "0123456789ABCDEF";
    size_t written = 0;

    for (int shift = 28; shift >= 0 && written < out_count; shift -= 4)
    {
        out[written++] = hex[(value >> shift) & 0x0FU];
    }

    return written;
}

static size_t make_serial(uint16_t *out, size_t out_count)
{
    uint32_t const *uid = (uint32_t const *)UID_BASE;
    size_t written = 0;

    written += append_hex16(out + written, out_count - written, uid[0]);
    written += append_hex16(out + written, out_count - written, uid[1]);
    written += append_hex16(out + written, out_count - written, uid[2]);

    return written;
}

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    (void)langid;
    size_t chr_count = 0;

    if (index == STRID_LANGID)
    {
        memcpy(&desc_str[1], string_desc_arr[0], 2);
        chr_count = 1;
    }
    else if (index == STRID_SERIAL)
    {
        chr_count = make_serial(desc_str + 1, sizeof(desc_str) / sizeof(desc_str[0]) - 1);
    }
    else
    {
        if (index >= (sizeof(string_desc_arr) / sizeof(string_desc_arr[0])) ||
            string_desc_arr[index] == NULL)
        {
            return NULL;
        }

        char const *str = string_desc_arr[index];
        chr_count = strlen(str);
        size_t const max_count = sizeof(desc_str) / sizeof(desc_str[0]) - 1;
        if (chr_count > max_count)
        {
            chr_count = max_count;
        }

        for (size_t i = 0; i < chr_count; ++i)
        {
            desc_str[1 + i] = (uint8_t)str[i];
        }
    }

    desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2U * chr_count + 2U));
    return desc_str;
}
