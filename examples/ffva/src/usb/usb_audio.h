// Copyright 2021-2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#ifndef USB_AUDIO_H_
#define USB_AUDIO_H_

/*
 * frame_buffers format assumes:
 *   processed_audio_frame
 *   reference_audio_frame
 *   raw_mic_audio_frame
 */
void usb_audio_send(rtos_intertile_t *intertile_ctx,
                    size_t frame_count,
                    int32_t **frame_buffers,
                    size_t num_chans);

/*
* frame_buffers format assumes:
*   reference_audio_frame
*   raw_mic_audio_frame
*/
void usb_audio_recv(rtos_intertile_t *intertile_ctx,
                    size_t frame_count,
                    int32_t **frame_buffers,
                    size_t num_chans);

/*
 * Wideband mic capture (ReSpeaker Lite, appconfMIC_ARRAY_SAMPLE_RATE ==
 * appconfUSB_AUDIO_SAMPLE_RATE != pipeline rate): called on the mic tile once
 * per pipeline frame with frame_count sample-interleaved frames
 * ([frame][mic], int32) straight from the mic array driver. Converts them to
 * the USB sample format (raw gain shift, 16-bit) and ships them to the USB
 * tile, where they feed the capture interface directly. In this mode
 * usb_audio_send() sends nothing.
 */
void usb_audio_raw_mic_send(rtos_intertile_t *intertile_ctx,
                            const int32_t *mic_frame,
                            size_t frame_count);

void usb_audio_init(rtos_intertile_t *intertile_ctx, unsigned priority);


#endif /* USB_AUDIO_H_ */
