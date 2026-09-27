// Copyright 2021-2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#ifndef APP_CONF_CHECK_H_
#define APP_CONF_CHECK_H_

#if (appconfUSB_ENABLED || appconfUSB_DFU_ENABLED) && appconfSPI_OUTPUT_ENABLED
#error Cannot use both USB and SPI interfaces
#endif

#if appconfUSB_ENABLED && appconfEXTERNAL_MCLK
#error Cannot use USB with an external mclk source
#endif

#if appconfUSB_ENABLED && appconfINTENT_ENABLED
#error Cannot use wakeword engine in USB configurations
#endif

#if appconfI2S_TDM_ENABLED && appconfI2S_AUDIO_SAMPLE_RATE != 3*appconfAUDIO_PIPELINE_SAMPLE_RATE
#error appconfI2S_AUDIO_SAMPLE_RATE must be 48000 to use I2S TDM
#endif

#if appconfMIC_ARRAY_SAMPLE_RATE != appconfAUDIO_PIPELINE_SAMPLE_RATE && \
    appconfMIC_ARRAY_SAMPLE_RATE != 3*appconfAUDIO_PIPELINE_SAMPLE_RATE
#error appconfMIC_ARRAY_SAMPLE_RATE must be 16000 or 48000
#endif

#if appconfMIC_ARRAY_RATE_MULTIPLIER > 1 && \
    (appconfMIC_ARRAY_FRAME_ADVANCE % MIC_ARRAY_CONFIG_SAMPLES_PER_FRAME) != 0
#error MIC_ARRAY_CONFIG_SAMPLES_PER_FRAME must divide appconfMIC_ARRAY_FRAME_ADVANCE
#endif

/* usb_audio.c only implements 1:1 or a fixed 3:1 conversion between the USB
 * rate and the 16 kHz pipeline rate. */
#if appconfUSB_ENABLED && \
    appconfUSB_AUDIO_SAMPLE_RATE != appconfAUDIO_PIPELINE_SAMPLE_RATE && \
    appconfUSB_AUDIO_SAMPLE_RATE != 3*appconfAUDIO_PIPELINE_SAMPLE_RATE
#error appconfUSB_AUDIO_SAMPLE_RATE must be 16000 or 48000
#endif

/* Wideband mic capture (ReSpeaker Lite): the raw mics are sent to the host at
 * the mic array rate, bypassing the 16 kHz pipeline. Only the RAW_PAIR layout
 * has a wideband source for both USB channels; the processed pipeline outputs
 * are 16 kHz and are not upsampled. */
#if RESPEAKER_LITE && appconfUSB_ENABLED && appconfMIC_ARRAY_RATE_MULTIPLIER > 1
#if appconfUSB_AUDIO_SAMPLE_RATE != appconfMIC_ARRAY_SAMPLE_RATE
#error appconfUSB_AUDIO_SAMPLE_RATE must equal appconfMIC_ARRAY_SAMPLE_RATE for wideband mic capture
#endif
#if appconfRESPEAKER_LITE_USB_LAYOUT != appconfRESPEAKER_LITE_USB_LAYOUT_RAW_PAIR || \
    appconfUSB_AUDIO_MODE != appconfUSB_AUDIO_RELEASE
#error Wideband mic capture is only implemented for the RAW_PAIR USB layout in release mode
#endif
#if appconfMIC_SRC_DEFAULT != appconfMIC_SRC_MICS
#error Wideband mic capture requires the PDM mics as the mic source
#endif
#endif

#if XK_VOICE_L71
#if appconfSPI_OUTPUT_ENABLED
#error SPI audio output not currently supported on XK-VOICE-L71 board
#endif
#endif

#endif /* APP_CONF_CHECK_H_ */
