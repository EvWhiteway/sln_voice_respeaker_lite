/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 * Copyright (c) 2021 XMOS LIMITED
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "usb_descriptors.h"
#include "tusb.h"

#if RESPEAKER_LITE
#define USB_VID                  0x2886
#define USB_PID                  0x0019
#define USB_BCD                  0x0201
#if appconfUSB_ENABLED
#define USB_DEVICE_VERSION_BCD   0x0212
#elif appconfUSB_DFU_ENABLED
#define USB_DEVICE_VERSION_BCD   0x0216
#else
#define USB_DEVICE_VERSION_BCD   0x0212
#endif
#define USB_VENDOR_STR           "Seeed Studio"
#define USB_PRODUCT_STR          "ReSpeaker Lite"
#define USB_SERIAL_STR           "0000000001"
#else
#define USB_VID                  0x20B1
#define USB_PID                  0x4001
#define USB_BCD                  0x0201
#define USB_DEVICE_VERSION_BCD   0x0001
#define USB_VENDOR_STR           "XMOS"
#define USB_PRODUCT_STR          "XCORE-VOICE"
#define USB_SERIAL_STR           "123456"
#endif

#if appconfUSB_DFU_ENABLED && !appconfUSB_ENABLED
#define USB_DEVICE_CLASS         TUSB_CLASS_MISC
#define USB_DEVICE_SUBCLASS      MISC_SUBCLASS_COMMON
#define USB_DEVICE_PROTOCOL      MISC_PROTOCOL_IAD
#define DFU_IAD_LEN              8
#define DFU_PLACEHOLDER_LEN      9
#else
#define USB_DEVICE_CLASS         TUSB_CLASS_UNSPECIFIED
#define USB_DEVICE_SUBCLASS      TUSB_CLASS_UNSPECIFIED
#define USB_DEVICE_PROTOCOL      TUSB_CLASS_UNSPECIFIED
#define DFU_IAD_LEN              0
#define DFU_PLACEHOLDER_LEN      0
#endif

//--------------------------------------------------------------------+
// Device Descriptors
//--------------------------------------------------------------------+
tusb_desc_device_t const desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = USB_BCD,   // For BOS descriptor! https://microchip.my.site.com/s/article/Does-a-USB2-1-Specification-Exist

    .bDeviceClass       = USB_DEVICE_CLASS,
    .bDeviceSubClass    = USB_DEVICE_SUBCLASS,
    .bDeviceProtocol    = USB_DEVICE_PROTOCOL,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor           = USB_VID,
    .idProduct          = USB_PID,
    .bcdDevice          = USB_DEVICE_VERSION_BCD,

    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,

    .bNumConfigurations = 0x01
};

const uint8_t ms_os_string_descriptor[] = {
  0x12, 0x03,                         // bLength, bDescriptorType
  'M',0,'S',0,'F',0,'T',0,'1',0,'0',0,'0',0, // "MSFT100"
  0x20,                               // VendorCode
  0x00
};

// Invoked when received GET DEVICE DESCRIPTOR
// Application return pointer to descriptor
uint8_t const* tud_descriptor_device_cb(void)
{
    return (uint8_t const*) &desc_device;
}

// MSOS 2.0 descriptor copied from the examples in https://github.com/xmos/xcore_iot/blob/develop/test/usb/tinyusb_demos/webusb_serial/src/usb_descriptors.c#L152
// and https://github.com/pololu/libusbp/blob/master/test/firmware/wixel/main.c#L460
#define REQUEST_GET_MS_DESCRIPTOR    0x20

// Microsoft OS 2.0 Descriptors, Table 8
#define MS_OS_20_DESCRIPTOR_INDEX 7

#define MS_OS_10_COMPATIBLE_ID_DESCRIPTOR_INDEX 0x0004
#define MS_OS_10_EXTENDED_PROPERTIES_DESCRIPTOR_INDEX 0x0005
#define MS_OS_10_EXTENDED_PROPERTIES_DESC_LEN 0x008E

#define MS_OS_20_REG_PROPERTY_DESC_LEN        0x0084
#define MS_OS_20_DFU_FUNCTION_DESC_LEN        (0x0008 + 0x0014 + MS_OS_20_REG_PROPERTY_DESC_LEN)
#if DFU_PLACEHOLDER_LEN
#define MS_OS_20_PLACEHOLDER_FUNCTION_DESC_LEN (0x0008 + 0x0014)
#define MS_OS_10_COMPATIBLE_ID_FUNCTION_COUNT 2
#else
#define MS_OS_20_PLACEHOLDER_FUNCTION_DESC_LEN 0
#define MS_OS_10_COMPATIBLE_ID_FUNCTION_COUNT 1
#endif
#define MS_OS_20_DESC_LEN                     (0x000A + 0x0008 + MS_OS_20_DFU_FUNCTION_DESC_LEN + MS_OS_20_PLACEHOLDER_FUNCTION_DESC_LEN)
#define MS_OS_10_COMPATIBLE_ID_DESC_LEN       (0x0010 + (0x0018 * MS_OS_10_COMPATIBLE_ID_FUNCTION_COUNT))

#define BOS_TOTAL_LEN      (TUD_BOS_DESC_LEN + TUD_BOS_MICROSOFT_OS_DESC_LEN)

uint8_t const desc_bos[] =
{
  // total length, number of device caps
  TUD_BOS_DESCRIPTOR(BOS_TOTAL_LEN, 1),

  // Microsoft OS 2.0 descriptor
  TUD_BOS_MS_OS_20_DESCRIPTOR(MS_OS_20_DESC_LEN, REQUEST_GET_MS_DESCRIPTOR)
};

uint8_t const desc_ms_os_20[] =
{
  // Set header: length, type, windows version, total length
  U16_TO_U8S_LE(0x000A), U16_TO_U8S_LE(MS_OS_20_SET_HEADER_DESCRIPTOR), U32_TO_U8S_LE(0x06030000), U16_TO_U8S_LE(MS_OS_20_DESC_LEN),

  // Configuration subset header: length, type, configuration index, reserved, configuration total length
  U16_TO_U8S_LE(0x0008), U16_TO_U8S_LE(MS_OS_20_SUBSET_HEADER_CONFIGURATION), 0, 0, U16_TO_U8S_LE(MS_OS_20_DESC_LEN-0x0A),

  // Function Subset header: length, type, first interface, reserved, subset length
  U16_TO_U8S_LE(0x0008), U16_TO_U8S_LE(MS_OS_20_SUBSET_HEADER_FUNCTION), ITF_NUM_DFU_MODE, 0, U16_TO_U8S_LE(MS_OS_20_DFU_FUNCTION_DESC_LEN),

  // MS OS 2.0 Compatible ID descriptor: length, type, compatible ID, sub compatible ID
  U16_TO_U8S_LE(0x0014), U16_TO_U8S_LE(MS_OS_20_FEATURE_COMPATBLE_ID), 'W', 'I', 'N', 'U', 'S', 'B', 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // sub-compatible

  // MS OS 2.0 Registry property descriptor: length, type
  U16_TO_U8S_LE(MS_OS_20_REG_PROPERTY_DESC_LEN), U16_TO_U8S_LE(MS_OS_20_FEATURE_REG_PROPERTY),
  U16_TO_U8S_LE(0x0007), U16_TO_U8S_LE(0x002A), // wPropertyDataType, wPropertyNameLength and PropertyName "DeviceInterfaceGUIDs\0" in UTF-16
  'D', 0x00, 'e', 0x00, 'v', 0x00, 'i', 0x00, 'c', 0x00, 'e', 0x00, 'I', 0x00, 'n', 0x00, 't', 0x00, 'e', 0x00,
  'r', 0x00, 'f', 0x00, 'a', 0x00, 'c', 0x00, 'e', 0x00, 'G', 0x00, 'U', 0x00, 'I', 0x00, 'D', 0x00, 's', 0x00, 0x00, 0x00,
  U16_TO_U8S_LE(0x0050), // wPropertyDataLength
	//bPropertyData: ?{975F44D9-0D08-43FD-8B3E-127CA8AFFF9D}?.
  '{', 0x00, '9', 0x00, '7', 0x00, '5', 0x00, 'F', 0x00, '4', 0x00, '4', 0x00, 'D', 0x00, '9', 0x00, '-', 0x00,
  '0', 0x00, 'D', 0x00, '0', 0x00, '8', 0x00, '-', 0x00, '4', 0x00, '3', 0x00, 'F', 0x00, 'D', 0x00, '-', 0x00,
  '8', 0x00, 'B', 0x00, '3', 0x00, 'E', 0x00, '-', 0x00, '1', 0x00, '2', 0x00, '7', 0x00, 'C', 0x00, 'A', 0x00,
  '8', 0x00, 'A', 0x00, 'F', 0x00, 'F', 0x00, 'F', 0x00, '9', 0x00, 'D', 0x00, '}', 0x00, 0x00, 0x00, 0x00, 0x00

#if DFU_PLACEHOLDER_LEN
  ,
  // Placeholder interface compatible ID descriptor
  U16_TO_U8S_LE(0x0008), U16_TO_U8S_LE(MS_OS_20_SUBSET_HEADER_FUNCTION), ITF_NUM_DFU_COMPOSITE_PLACEHOLDER, 0, U16_TO_U8S_LE(MS_OS_20_PLACEHOLDER_FUNCTION_DESC_LEN),
  U16_TO_U8S_LE(0x0014), U16_TO_U8S_LE(MS_OS_20_FEATURE_COMPATBLE_ID), 'W', 'I', 'N', 'U', 'S', 'B', 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
#endif
};

uint8_t const * tud_descriptor_bos_cb(void)
{
  return desc_bos;
}


//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+

#if CFG_TUD_AUDIO
const size_t uac2_interface_descriptors_length =
        TUD_AUDIO_DESC_CLK_SRC_LEN
#if AUDIO_OUTPUT_ENABLED
        + TUD_AUDIO_DESC_INPUT_TERM_LEN
#if 0
        + TUD_AUDIO_DESC_FEATURE_UNIT_ONE_CHANNEL_LEN
#endif
        + TUD_AUDIO_DESC_OUTPUT_TERM_LEN
#endif
#if AUDIO_INPUT_ENABLED
        + TUD_AUDIO_DESC_INPUT_TERM_LEN
#if 0
        + TUD_AUDIO_DESC_FEATURE_UNIT_ONE_CHANNEL_LEN
#endif
        + TUD_AUDIO_DESC_OUTPUT_TERM_LEN
#endif
        ;


const size_t uac2_total_descriptors_length =
        TUD_AUDIO_DESC_IAD_LEN +
        TUD_AUDIO_DESC_STD_AC_LEN +
        TUD_AUDIO_DESC_CS_AC_LEN +
        uac2_interface_descriptors_length
#if AUDIO_OUTPUT_ENABLED
        + TUD_AUDIO_DESC_STD_AS_INT_LEN
        + TUD_AUDIO_DESC_STD_AS_INT_LEN
        + TUD_AUDIO_DESC_CS_AS_INT_LEN
        + TUD_AUDIO_DESC_TYPE_I_FORMAT_LEN
        + TUD_AUDIO_DESC_STD_AS_ISO_EP_LEN
        + TUD_AUDIO_DESC_CS_AS_ISO_EP_LEN
#endif
#if AUDIO_INPUT_ENABLED
        + TUD_AUDIO_DESC_STD_AS_INT_LEN
        + TUD_AUDIO_DESC_STD_AS_INT_LEN
        + TUD_AUDIO_DESC_CS_AS_INT_LEN
        + TUD_AUDIO_DESC_TYPE_I_FORMAT_LEN
        + TUD_AUDIO_DESC_STD_AS_ISO_EP_LEN
        + TUD_AUDIO_DESC_CS_AS_ISO_EP_LEN
#endif
        ;

// List of audio descriptor lengths which is required by audio driver - you need as many entries as CFG_TUD_AUDIO - unfortunately this is not possible to determine otherwise
const uint16_t tud_audio_desc_lengths[CFG_TUD_AUDIO] = {
        uac2_total_descriptors_length
};

#define AUDIO_TOTAL_LEN         (CFG_TUD_AUDIO * uac2_total_descriptors_length)
#else
#define AUDIO_TOTAL_LEN         0
#endif

#define CONFIG_TOTAL_LEN        (TUD_CONFIG_DESC_LEN + AUDIO_TOTAL_LEN + DFU_IAD_LEN + TUD_DFU_DESC_LEN(DFU_ALT_COUNT) + DFU_PLACEHOLDER_LEN)
#define EPNUM_AUDIO   0x01

#define AUDIO_INTERFACE_STRING_INDEX 4
#define DFU_INTERFACE_STRING_INDEX   5

uint8_t const desc_configuration[] = {
    // Config number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 400),

#if CFG_TUD_AUDIO
    /* Standard Interface Association Descriptor (IAD) */
    TUD_AUDIO_DESC_IAD(/*_firstitfs*/ ITF_NUM_AUDIO_CONTROL, /*_nitfs*/ 1+AUDIO_OUTPUT_ENABLED+AUDIO_INPUT_ENABLED, /*_stridx*/ 0x00),
    /* Standard AC Interface Descriptor(4.7.1) */
    TUD_AUDIO_DESC_STD_AC(/*_itfnum*/ ITF_NUM_AUDIO_CONTROL, /*_nEPs*/ 0x00, /*_stridx*/ AUDIO_INTERFACE_STRING_INDEX),
    /* Class-Specific AC Interface Header Descriptor(4.7.2) */
    TUD_AUDIO_DESC_CS_AC(/*_bcdADC*/ 0x0200, /*_category*/ AUDIO_FUNC_OTHER, /*_totallen*/ uac2_interface_descriptors_length, /*_ctrl*/ AUDIO_CS_AS_INTERFACE_CTRL_LATENCY_POS),
    /* Clock Source Descriptor(4.7.2.1) */
    TUD_AUDIO_DESC_CLK_SRC(/*_clkid*/ UAC2_ENTITY_CLOCK, /*_attr*/ AUDIO_CLOCK_SOURCE_ATT_INT_PRO_CLK, /*_ctrl*/ (AUDIO_CTRL_R << AUDIO_CLOCK_SOURCE_CTRL_CLK_VAL_POS) | (AUDIO_CTRL_R << AUDIO_CLOCK_SOURCE_CTRL_CLK_FRQ_POS), /*_assocTerm*/ 0x00,  /*_stridx*/ 0x00),


#if AUDIO_OUTPUT_ENABLED
    /* Input Terminal Descriptor(4.7.2.4) */
    TUD_AUDIO_DESC_INPUT_TERM(/*_termid*/ UAC2_ENTITY_SPK_INPUT_TERMINAL, /*_termtype*/ AUDIO_TERM_TYPE_USB_STREAMING, /*_assocTerm*/ 0x00, /*_clkid*/ UAC2_ENTITY_CLOCK, /*_nchannelslogical*/ CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX, /*_channelcfg*/ AUDIO_CHANNEL_CONFIG_NON_PREDEFINED, /*_idxchannelnames*/ 0x00, /*_ctrl*/ AUDIO_CTRL_NONE, /*_stridx*/ 0x00),

    #if 0
    /* Feature Unit Descriptor(4.7.2.8) */
    TUD_AUDIO_DESC_FEATURE_UNIT_ONE_CHANNEL(/*_unitid*/ UAC2_ENTITY_SPK_FEATURE_UNIT, /*_srcid*/ UAC2_ENTITY_SPK_INPUT_TERMINAL, /*_ctrlch0master*/ AUDIO_CTRL_RW << AUDIO_FEATURE_UNIT_CTRL_MUTE_POS | AUDIO_CTRL_RW << AUDIO_FEATURE_UNIT_CTRL_VOLUME_POS, /*_ctrlch1*/ AUDIO_CTRL_RW << AUDIO_FEATURE_UNIT_CTRL_MUTE_POS | AUDIO_CTRL_RW << AUDIO_FEATURE_UNIT_CTRL_VOLUME_POS, /*_stridx*/ 0x00),

    #endif
    /* Output Terminal Descriptor(4.7.2.5) */
    TUD_AUDIO_DESC_OUTPUT_TERM(/*_termid*/ UAC2_ENTITY_SPK_OUTPUT_TERMINAL, /*_termtype*/ AUDIO_TERM_TYPE_OUT_GENERIC_SPEAKER, /*_assocTerm*/ 0x00, /*_srcid*/ UAC2_ENTITY_SPK_INPUT_TERMINAL, /*_clkid*/ UAC2_ENTITY_CLOCK, /*_ctrl*/ AUDIO_CTRL_NONE, /*_stridx*/ 0x00),
#endif


#if AUDIO_INPUT_ENABLED
    /* Input Terminal Descriptor(4.7.2.4) */
    TUD_AUDIO_DESC_INPUT_TERM(/*_termid*/ UAC2_ENTITY_MIC_INPUT_TERMINAL, /*_termtype*/ AUDIO_TERM_TYPE_IN_GENERIC_MIC, /*_assocTerm*/ 0x00, /*_clkid*/ UAC2_ENTITY_CLOCK, /*_nchannelslogical*/ CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX, /*_channelcfg*/ AUDIO_CHANNEL_CONFIG_NON_PREDEFINED, /*_idxchannelnames*/ 0x00, /*_ctrl*/ AUDIO_CTRL_NONE, /*_stridx*/ 0x00),

    #if 0
    /* Feature Unit Descriptor(4.7.2.8) */
    TUD_AUDIO_DESC_FEATURE_UNIT_ONE_CHANNEL(/*_unitid*/ UAC2_ENTITY_MIC_FEATURE_UNIT, /*_srcid*/ UAC2_ENTITY_MIC_INPUT_TERMINAL, /*_ctrlch0master*/ AUDIO_CTRL_RW << AUDIO_FEATURE_UNIT_CTRL_MUTE_POS | AUDIO_CTRL_RW << AUDIO_FEATURE_UNIT_CTRL_VOLUME_POS, /*_ctrlch1*/ AUDIO_CTRL_RW << AUDIO_FEATURE_UNIT_CTRL_MUTE_POS | AUDIO_CTRL_RW << AUDIO_FEATURE_UNIT_CTRL_VOLUME_POS, /*_stridx*/ 0x00),
    #endif

    /* Output Terminal Descriptor(4.7.2.5) */
    TUD_AUDIO_DESC_OUTPUT_TERM(/*_termid*/ UAC2_ENTITY_MIC_OUTPUT_TERMINAL, /*_termtype*/ AUDIO_TERM_TYPE_USB_STREAMING, /*_assocTerm*/ 0x00, /*_srcid*/ UAC2_ENTITY_MIC_INPUT_TERMINAL, /*_clkid*/ UAC2_ENTITY_CLOCK, /*_ctrl*/ AUDIO_CTRL_NONE, /*_stridx*/ 0x00),
#endif

#if AUDIO_OUTPUT_ENABLED
    /* Standard AS Interface Descriptor(4.9.1) */
    /* Interface 1, Alternate 0 - default alternate setting with 0 bandwidth */
    TUD_AUDIO_DESC_STD_AS_INT(/*_itfnum*/ ITF_NUM_AUDIO_STREAMING_SPK, /*_altset*/ 0x00, /*_nEPs*/ 0x00, /*_stridx*/ 0x00),
    /* Standard AS Interface Descriptor(4.9.1) */
    /* Interface 1, Alternate 1 - alternate interface for data streaming */
    TUD_AUDIO_DESC_STD_AS_INT(/*_itfnum*/ ITF_NUM_AUDIO_STREAMING_SPK, /*_altset*/ 0x01, /*_nEPs*/ 0x01, /*_stridx*/ 0x00),
    /* Class-Specific AS Interface Descriptor(4.9.2) */
    TUD_AUDIO_DESC_CS_AS_INT(/*_termid*/ UAC2_ENTITY_SPK_INPUT_TERMINAL, /*_ctrl*/ AUDIO_CTRL_NONE, /*_formattype*/ AUDIO_FORMAT_TYPE_I, /*_formats*/ AUDIO_DATA_FORMAT_TYPE_I_PCM, /*_nchannelsphysical*/ CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX, /*_channelcfg*/ AUDIO_CHANNEL_CONFIG_NON_PREDEFINED, /*_stridx*/ 0x00),
    /* Type I Format Type Descriptor(2.3.1.6 - Audio Formats) */
    TUD_AUDIO_DESC_TYPE_I_FORMAT(CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_RX, CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_RX*8),
    /* Standard AS Isochronous Audio Data Endpoint Descriptor(4.10.1.1) */
    TUD_AUDIO_DESC_STD_AS_ISO_EP(/*_ep*/ EPNUM_AUDIO, /*_attr*/ (TUSB_XFER_ISOCHRONOUS | TUSB_ISO_EP_ATT_ADAPTIVE | TUSB_ISO_EP_ATT_IMPLICIT_FB /*| TUSB_ISO_EP_ATT_DATA*/), /*_maxEPsize*/ CFG_TUD_AUDIO_FUNC_1_EP_OUT_SZ, /*_interval*/ (CFG_TUSB_RHPORT0_MODE & OPT_MODE_HIGH_SPEED) ? 0x04 : 0x01),
    /* Class-Specific AS Isochronous Audio Data Endpoint Descriptor(4.10.1.2) */
    TUD_AUDIO_DESC_CS_AS_ISO_EP(/*_attr*/ AUDIO_CS_AS_ISO_DATA_EP_ATT_NON_MAX_PACKETS_OK, /*_ctrl*/ AUDIO_CTRL_NONE, /*_lockdelayunit*/ AUDIO_CS_AS_ISO_DATA_EP_LOCK_DELAY_UNIT_MILLISEC, /*_lockdelay*/ 0x0003),
#endif

#if AUDIO_INPUT_ENABLED
    /* Standard AS Interface Descriptor(4.9.1) */
    /* Interface 1, Alternate 0 - default alternate setting with 0 bandwidth */
    TUD_AUDIO_DESC_STD_AS_INT(/*_itfnum*/ ITF_NUM_AUDIO_STREAMING_MIC, /*_altset*/ 0x00, /*_nEPs*/ 0x00, /*_stridx*/ 0x00),
    /* Standard AS Interface Descriptor(4.9.1) */
    /* Interface 1, Alternate 1 - alternate interface for data streaming */
    TUD_AUDIO_DESC_STD_AS_INT(/*_itfnum*/ ITF_NUM_AUDIO_STREAMING_MIC, /*_altset*/ 0x01, /*_nEPs*/ 0x01, /*_stridx*/ 0x00),
    /* Class-Specific AS Interface Descriptor(4.9.2) */
    TUD_AUDIO_DESC_CS_AS_INT(/*_termid*/ UAC2_ENTITY_MIC_OUTPUT_TERMINAL, /*_ctrl*/ AUDIO_CTRL_NONE, /*_formattype*/ AUDIO_FORMAT_TYPE_I, /*_formats*/ AUDIO_DATA_FORMAT_TYPE_I_PCM, /*_nchannelsphysical*/ CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX, /*_channelcfg*/ AUDIO_CHANNEL_CONFIG_NON_PREDEFINED, /*_stridx*/ 0x00),
    /* Type I Format Type Descriptor(2.3.1.6 - Audio Formats) */
    TUD_AUDIO_DESC_TYPE_I_FORMAT(CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_TX, CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_TX*8),
    /* Standard AS Isochronous Audio Data Endpoint Descriptor(4.10.1.1) */
    TUD_AUDIO_DESC_STD_AS_ISO_EP(/*_ep*/ 0x80 | EPNUM_AUDIO, /*_attr*/ (TUSB_XFER_ISOCHRONOUS | TUSB_ISO_EP_ATT_ADAPTIVE /*| TUSB_ISO_EP_ATT_IMPLICIT_FB */ | TUSB_ISO_EP_ATT_DATA), /*_maxEPsize*/ CFG_TUD_AUDIO_FUNC_1_EP_IN_SZ, /*_interval*/ (CFG_TUSB_RHPORT0_MODE & OPT_MODE_HIGH_SPEED) ? 0x04 : 0x01),
    /* Class-Specific AS Isochronous Audio Data Endpoint Descriptor(4.10.1.2) */
    TUD_AUDIO_DESC_CS_AS_ISO_EP(/*_attr*/ AUDIO_CS_AS_ISO_DATA_EP_ATT_NON_MAX_PACKETS_OK, /*_ctrl*/ AUDIO_CTRL_NONE, /*_lockdelayunit*/ AUDIO_CS_AS_ISO_DATA_EP_LOCK_DELAY_UNIT_MILLISEC, /*_lockdelay*/ 0x0003),
#endif
#endif

#if DFU_IAD_LEN
    // Standard Interface Association Descriptor (IAD) for DFU-only composite enumeration
    DFU_IAD_LEN, TUSB_DESC_INTERFACE_ASSOCIATION, ITF_NUM_DFU_MODE, 1, TUSB_CLASS_APPLICATION_SPECIFIC, APP_SUBCLASS_DFU_RUNTIME, 0x02, DFU_INTERFACE_STRING_INDEX,
#endif

    // Interface number, Alternate count, starting string index, attributes, detach timeout, transfer size
    TUD_DFU_DESCRIPTOR(ITF_NUM_DFU_MODE, DFU_ALT_COUNT, DFU_INTERFACE_STRING_INDEX, DFU_FUNC_ATTRS, 1000, CFG_TUD_DFU_XFER_BUFSIZE),

#if DFU_PLACEHOLDER_LEN
    // No-endpoint vendor interface to force Windows composite child PDOs in DFU-only builds
    DFU_PLACEHOLDER_LEN, TUSB_DESC_INTERFACE, ITF_NUM_DFU_COMPOSITE_PLACEHOLDER, 0, 0, TUSB_CLASS_VENDOR_SPECIFIC, 0x00, 0x00, 0,
#endif

    }; // desc_configuration

// Invoked when received GET CONFIGURATION DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const* tud_descriptor_configuration_cb(uint8_t index)
{
    (void) index; // for multiple configurations
    return desc_configuration;
}

//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+

// array of pointer to string descriptors
char const *string_desc_arr[] = {(const char[]) {0x09, 0x04}, // 0: is supported language is English (0x0409)
        USB_VENDOR_STR,               // 1: Manufacturer
        USB_PRODUCT_STR,              // 2: Product
        USB_SERIAL_STR,               // 3: Serials, should use chip ID
        USB_PRODUCT_STR,              // 4: Audio Interface
        "DFU FACTORY",               // 5: DFU device
        "DFU UPGRADE",               // 6: DFU device
        "DFU DATAPARTITION",         // 7: DFU device
        };

static uint16_t _desc_str[32];

// Invoked when received GET STRING DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
uint16_t const* tud_descriptor_string_cb(uint8_t index,
                                         uint16_t langid)
{
    (void) langid;

    uint8_t chr_count;

    if (index == 0) {
        memcpy(&_desc_str[1], string_desc_arr[0], 2);
        chr_count = 1;
    }   
    else if ( index == 0xEE )
    {
        // Microsoft OS 1.0 Descriptors
        memcpy(&_desc_str[0], ms_os_string_descriptor, sizeof(ms_os_string_descriptor));
        return _desc_str;
    }
    else {
        // Convert ASCII string into UTF-16

        if (!(index < sizeof(string_desc_arr) / sizeof(string_desc_arr[0])))
            return NULL;

        const char *str = string_desc_arr[index];

        // Cap at max char
        chr_count = strlen(str);
        if (chr_count > 31)
            chr_count = 31;

        for (uint8_t i = 0; i < chr_count; i++) {
            _desc_str[1 + i] = str[i];
        }
    }

    // first byte is length (including header), second byte is string type
    _desc_str[0] = (TUSB_DESC_STRING << 8) | (2 * chr_count + 2);

    return _desc_str;
}

const uint8_t ms_compatible_id_descriptor[] = {
    // Header (16 bytes)
    U32_TO_U8S_LE(MS_OS_10_COMPATIBLE_ID_DESC_LEN),
    0x00,0x01,             // bcdVersion = 1.0
    0x04,0x00,             // wIndex = 0x0004
    MS_OS_10_COMPATIBLE_ID_FUNCTION_COUNT,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00, // reserved

    // Function Section (24 bytes)
    ITF_NUM_DFU_MODE,      // bFirstInterfaceNumber
    0x01,      // reserved
    'W','I','N','U','S','B',0x00,0x00, // CompatibleID = "WINUSB"
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // SubCompatibleID
    0x00,0x00,0x00,0x00,0x00,0x00 // reserved

#if DFU_PLACEHOLDER_LEN
    ,
    // Function Section (24 bytes)
    ITF_NUM_DFU_COMPOSITE_PLACEHOLDER,
    0x01,
    'W','I','N','U','S','B',0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00
#endif
};

const uint8_t ms_extended_properties_descriptor[] = {
    // Extended Properties Header
    U32_TO_U8S_LE(MS_OS_10_EXTENDED_PROPERTIES_DESC_LEN), // dwLength
    0x00, 0x01,                                           // bcdVersion = 1.0
    U16_TO_U8S_LE(MS_OS_10_EXTENDED_PROPERTIES_DESCRIPTOR_INDEX),
    0x01, 0x00,                                           // wCount = 1

    // Custom Property Section: DeviceInterfaceGUID = "{975F44D9-0D08-43FD-8B3E-127CA8AFFF9D}"
    U32_TO_U8S_LE(MS_OS_10_EXTENDED_PROPERTIES_DESC_LEN - 0x0A), // dwSize
    U32_TO_U8S_LE(0x00000001),                                  // dwPropertyDataType = REG_SZ
    U16_TO_U8S_LE(0x0028),                                      // wPropertyNameLength
    'D', 0x00, 'e', 0x00, 'v', 0x00, 'i', 0x00, 'c', 0x00, 'e', 0x00,
    'I', 0x00, 'n', 0x00, 't', 0x00, 'e', 0x00, 'r', 0x00, 'f', 0x00,
    'a', 0x00, 'c', 0x00, 'e', 0x00, 'G', 0x00, 'U', 0x00, 'I', 0x00,
    'D', 0x00, 0x00, 0x00,
    U32_TO_U8S_LE(0x004E),                                      // dwPropertyDataLength
    '{', 0x00, '9', 0x00, '7', 0x00, '5', 0x00, 'F', 0x00, '4', 0x00,
    '4', 0x00, 'D', 0x00, '9', 0x00, '-', 0x00, '0', 0x00, 'D', 0x00,
    '0', 0x00, '8', 0x00, '-', 0x00, '4', 0x00, '3', 0x00, 'F', 0x00,
    'D', 0x00, '-', 0x00, '8', 0x00, 'B', 0x00, '3', 0x00, 'E', 0x00,
    '-', 0x00, '1', 0x00, '2', 0x00, '7', 0x00, 'C', 0x00, 'A', 0x00,
    '8', 0x00, 'A', 0x00, 'F', 0x00, 'F', 0x00, 'F', 0x00, '9', 0x00,
    'D', 0x00, '}', 0x00, 0x00, 0x00
};

bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request)
{
    switch (request->bRequest)
    {
        case REQUEST_GET_MS_DESCRIPTOR:
            if (stage != CONTROL_STAGE_SETUP) return true; // nothing to with DATA & ACK stage

            if ( request->wIndex == MS_OS_10_COMPATIBLE_ID_DESCRIPTOR_INDEX )
            {
                // Send Microsoft OS 1.0 compatible descriptor
                return tud_control_xfer(rhport, request, (void*)(uintptr_t) ms_compatible_id_descriptor, sizeof(ms_compatible_id_descriptor));
            }
            else if ( request->wIndex == MS_OS_10_EXTENDED_PROPERTIES_DESCRIPTOR_INDEX )
            {
                // Send Microsoft OS 1.0 extended properties descriptor
                return tud_control_xfer(rhport, request, (void*)(uintptr_t) ms_extended_properties_descriptor, sizeof(ms_extended_properties_descriptor));
            }
            else if ( request->wIndex == MS_OS_20_DESCRIPTOR_INDEX )
            {
                // Send Microsoft OS 2.0 descriptor
                return tud_control_xfer(rhport, request, (void*)(uintptr_t) desc_ms_os_20, MS_OS_20_DESC_LEN);
            }
            else
            {
                return false;
            }

        default:
            return false;
    }
}

// device qualifier is mostly similar to device descriptor since we don't change configuration based on speed
tusb_desc_device_qualifier_t const desc_device_qualifier =
{
    .bLength            = sizeof(tusb_desc_device_qualifier_t),
    .bDescriptorType    = TUSB_DESC_DEVICE_QUALIFIER,
    .bcdUSB             = 0x0200,

    .bDeviceClass       = TUSB_CLASS_UNSPECIFIED,
    .bDeviceSubClass    = TUSB_CLASS_UNSPECIFIED,
    .bDeviceProtocol    = TUSB_CLASS_UNSPECIFIED,

    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .bNumConfigurations = 0x01,
    .bReserved          = 0x00
};


uint8_t const* tud_descriptor_device_qualifier_cb(void)
{
    return (uint8_t const*) &desc_device_qualifier;
}

uint8_t const* tud_descriptor_other_speed_configuration_cb(uint8_t index)
{
    (void) index; // for multiple configurations

    // Always return our normal descriptor
    return desc_configuration;
}
