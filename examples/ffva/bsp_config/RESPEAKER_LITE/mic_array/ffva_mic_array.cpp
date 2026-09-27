// Copyright 2022-2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

/*
 * Mic array translation unit for the ReSpeaker Lite ffva builds.
 *
 * The fwk_rtos mic array driver (rtos_mic_array.c) drives the PDM mics through
 * lib_mic_array's "vanilla" entry points, ma_vanilla_init() and
 * ma_vanilla_task(). The library's own vanilla TU (etc/vanilla/
 * mic_array_vanilla.cpp) hard-wires the 16 kHz prefab: BasicMicArray with the
 * default 32x6 decimator, i.e. 3.072 MHz PDM -> 16 kHz.
 *
 * This file provides the same two entry points and selects the decimator from
 * appconfMIC_ARRAY_SAMPLE_RATE (a compile definition of the board target):
 *
 *   16000 (default)  mic_array::prefab::BasicMicArray - identical to the
 *                    library's vanilla behaviour.
 *   48000            mic_array::MicArray with TwoStageDecimator<2, 2, 96>
 *                    (32x2 decimation, 3.072 MHz PDM -> 48 kHz) using the
 *                    lib_mic_array "good_48k_filter" coefficient set (stage 1:
 *                    148 taps, stage 2: 96 taps, 20 kHz cutoff) that
 *                    examples/mic_aggregator also ships.
 *
 * The RESPEAKER_LITE board target compiles this file in place of the library
 * TU (see RESPEAKER_LITE.cmake); the rest of the driver is unchanged.
 *
 * Real-time note for the 48 kHz configuration: the PDM rx ISR hands the
 * decimator thread one block per output sample (20.8 us at 48 kHz) and only
 * buffers two of them, so the decimator thread must never be away from
 * GetPdmBlock() for more than ~40 us. The frame hand-off to the RTOS driver
 * (ChannelFrameTransmitter, a blocking channel transaction of
 * MIC_ARRAY_CONFIG_SAMPLES_PER_FRAME * MIC_COUNT words) happens on that
 * thread, so the board target keeps MIC_ARRAY_CONFIG_SAMPLES_PER_FRAME small
 * (48 = 1 ms, 96 words) and the application pulls a pipeline frame's worth
 * (appconfMIC_ARRAY_FRAME_ADVANCE) from the driver's ring buffer per call.
 * The 16 kHz build moves 480 words inside a 125 us budget, so 96 words in
 * 41.7 us has more margin than the stock configuration.
 */

#include <stdint.h>
#include <type_traits>
#include <xcore/channel_streaming.h>
#include <xcore/interrupt.h>

#include <platform.h>

#include "mic_array_vanilla.h"
#include "mic_array/cpp/Prefab.hpp"
#include "mic_array.h"
#include "mic_array/etc/filters_default.h"


////// Check that all the required config macros have been defined.

#ifndef MIC_ARRAY_CONFIG_MCLK_FREQ
# error Application must specify the master clock frequency by defining MIC_ARRAY_CONFIG_MCLK_FREQ.
#endif

#ifndef MIC_ARRAY_CONFIG_PDM_FREQ
# error Application must specify the PDM clock frequency by defining MIC_ARRAY_CONFIG_PDM_FREQ.
#endif

#ifndef MIC_ARRAY_CONFIG_MIC_COUNT
# error Application must specify the microphone count by defining MIC_ARRAY_CONFIG_MIC_COUNT.
#endif


////// Provide default values for optional config macros

#ifndef appconfMIC_ARRAY_SAMPLE_RATE
# define appconfMIC_ARRAY_SAMPLE_RATE    (16000)
#endif

#ifndef MIC_ARRAY_CONFIG_SAMPLES_PER_FRAME
# define MIC_ARRAY_CONFIG_SAMPLES_PER_FRAME    (1)
#else
# if ((MIC_ARRAY_CONFIG_SAMPLES_PER_FRAME) < 1)
#  error MIC_ARRAY_CONFIG_SAMPLES_PER_FRAME must be positive.
# endif
#endif

#ifndef MIC_ARRAY_CONFIG_USE_DC_ELIMINATION
# define MIC_ARRAY_CONFIG_USE_DC_ELIMINATION    (1)
#endif

#ifndef MIC_ARRAY_CONFIG_PORT_MCLK
# define MIC_ARRAY_CONFIG_PORT_MCLK   (PORT_MCLK_IN_OUT)
#endif

#ifndef MIC_ARRAY_CONFIG_PORT_PDM_CLK
# define MIC_ARRAY_CONFIG_PORT_PDM_CLK  (PORT_PDM_CLK)
#endif

#ifndef MIC_ARRAY_CONFIG_PORT_PDM_DATA
# define MIC_ARRAY_CONFIG_PORT_PDM_DATA   (PORT_PDM_DATA)
#endif

#ifndef MIC_ARRAY_CONFIG_CLOCK_BLOCK_A
# define MIC_ARRAY_CONFIG_CLOCK_BLOCK_A   (XS1_CLKBLK_1)
#endif

#ifndef MIC_ARRAY_CONFIG_CLOCK_BLOCK_B
# define MIC_ARRAY_CONFIG_CLOCK_BLOCK_B   (XS1_CLKBLK_2)
#endif

#ifndef MIC_ARRAY_CONFIG_USE_DDR
# define MIC_ARRAY_CONFIG_USE_DDR         ((MIC_ARRAY_CONFIG_MIC_COUNT)==2)
#endif

////// Additional macros derived from others

#define MIC_ARRAY_CONFIG_MCLK_DIVIDER     ((MIC_ARRAY_CONFIG_MCLK_FREQ)       \
                                              /(MIC_ARRAY_CONFIG_PDM_FREQ))

////// Allocate needed objects

#if (!(MIC_ARRAY_CONFIG_USE_DDR))
pdm_rx_resources_t pdm_res = PDM_RX_RESOURCES_SDR(
                                MIC_ARRAY_CONFIG_PORT_MCLK,
                                MIC_ARRAY_CONFIG_PORT_PDM_CLK,
                                MIC_ARRAY_CONFIG_PORT_PDM_DATA,
                                MIC_ARRAY_CONFIG_CLOCK_BLOCK_A);
#else
pdm_rx_resources_t pdm_res = PDM_RX_RESOURCES_DDR(
                                MIC_ARRAY_CONFIG_PORT_MCLK,
                                MIC_ARRAY_CONFIG_PORT_PDM_CLK,
                                MIC_ARRAY_CONFIG_PORT_PDM_DATA,
                                MIC_ARRAY_CONFIG_CLOCK_BLOCK_A,
                                MIC_ARRAY_CONFIG_CLOCK_BLOCK_B);
#endif


#if appconfMIC_ARRAY_SAMPLE_RATE == 16000

/*
 * Stock configuration: the library prefab, exactly as etc/vanilla/
 * mic_array_vanilla.cpp instantiates it.
 */

#if (MIC_ARRAY_CONFIG_PDM_FREQ) != (STAGE1_DEC_FACTOR) * (STAGE2_DEC_FACTOR) * 16000
# error The default lib_mic_array decimator produces PDM_FREQ / 192; MIC_ARRAY_CONFIG_PDM_FREQ must be 3072000 for 16 kHz.
#endif

using TMicArray = mic_array::prefab::BasicMicArray<
                        MIC_ARRAY_CONFIG_MIC_COUNT,
                        MIC_ARRAY_CONFIG_SAMPLES_PER_FRAME,
                        MIC_ARRAY_CONFIG_USE_DC_ELIMINATION>;

static TMicArray mics;


void ma_vanilla_init()
{
  mics.Init();
  mics.SetPort(pdm_res.p_pdm_mics);
  mic_array_resources_configure(&pdm_res, MIC_ARRAY_CONFIG_MCLK_DIVIDER);
  mic_array_pdm_clock_start(&pdm_res);
}


void ma_vanilla_task(
    chanend_t c_frames_out)
{
  mics.SetOutputChannel(c_frames_out);

  mics.InstallPdmRxISR();
  mics.UnmaskPdmRxISR();

  mics.ThreadEntry();
}


#elif appconfMIC_ARRAY_SAMPLE_RATE == 48000

/*
 * Wideband configuration: 3.072 MHz PDM -> 96 kHz (stage 1, /32, 148 taps)
 * -> 48 kHz (stage 2, /2, 96 taps).
 */

#include "mic_array_48k_decimator_coeffs.h"

#if (MIC_ARRAY_CONFIG_PDM_FREQ) != (STAGE1_DEC_FACTOR) * (MIC_ARRAY_CONFIG_STG2_DEC_FACTOR) * 48000
# error The 48 kHz decimator expects MIC_ARRAY_CONFIG_PDM_FREQ = 3072000.
#endif

namespace {

constexpr unsigned kMicCount = MIC_ARRAY_CONFIG_MIC_COUNT;
constexpr unsigned kStage2DecFactor = MIC_ARRAY_CONFIG_STG2_DEC_FACTOR;
constexpr unsigned kStage2TapCount = MIC_ARRAY_STAGE_2_NUM_TAPS;

const uint32_t WORD_ALIGNED stage1_coef_48k[STAGE1_WORDS] = STAGE_1_48K_COEFFS;
const int32_t WORD_ALIGNED stage2_coef_48k[kStage2TapCount] = STAGE_2_48K_COEFFS;
constexpr right_shift_t stage2_shr_48k = MIC_ARRAY_CONFIG_STG2_RIGHT_SHIFT;

/*
 * First-order DC blocker with the same structure as lib_mic_array's DCOE
 * (y[t] = R * y[t-1] + x[t] - x[t-1], state kept in Q32) but with a
 * configurable pole R = 1 - 2^-Q. The library filter has Q = 6 fixed, which
 * puts its -3 dB corner at fs / (64 * 2 * pi): ~40 Hz at 16 kHz but ~120 Hz at
 * 48 kHz, inside the voice band. Q = 8 gives ~30 Hz at 48 kHz.
 */
template <unsigned MIC_COUNT, unsigned Q>
class DcBlockSampleFilter
{
  private:
    int64_t prev_y[MIC_COUNT];

  public:
    void Init()
    {
      for (unsigned k = 0; k < MIC_COUNT; k++) {
        prev_y[k] = 0;
      }
    }

    void Filter(int32_t sample[MIC_COUNT])
    {
      for (unsigned k = 0; k < MIC_COUNT; k++) {
        const int64_t x_new = ((int64_t) sample[k]) << 32;
        prev_y[k] += x_new;
        sample[k] = (int32_t) (prev_y[k] >> 32);
        prev_y[k] -= (prev_y[k] >> Q);
        prev_y[k] -= x_new;
      }
    }
};

template <unsigned MIC_COUNT>
class NopSampleFilterWithInit
{
  public:
    void Init() {}
    void Filter(int32_t sample[MIC_COUNT]) { (void) sample; }
};

using TSampleFilter = std::conditional<(MIC_ARRAY_CONFIG_USE_DC_ELIMINATION) != 0,
                                       DcBlockSampleFilter<kMicCount, 8>,
                                       NopSampleFilterWithInit<kMicCount>>::type;

using TMicArray = mic_array::MicArray<kMicCount,
                        mic_array::TwoStageDecimator<kMicCount,
                                                     kStage2DecFactor,
                                                     kStage2TapCount>,
                        mic_array::StandardPdmRxService<kMicCount,
                                                        kMicCount,
                                                        kStage2DecFactor>,
                        TSampleFilter,
                        mic_array::FrameOutputHandler<kMicCount,
                                                      MIC_ARRAY_CONFIG_SAMPLES_PER_FRAME,
                                                      mic_array::ChannelFrameTransmitter>>;

TMicArray mics;

} // namespace


void ma_vanilla_init()
{
  mics.Decimator.Init(stage1_coef_48k, stage2_coef_48k, stage2_shr_48k);
  mics.SampleFilter.Init();
  mics.PdmRx.Init(pdm_res.p_pdm_mics);
  mic_array_resources_configure(&pdm_res, MIC_ARRAY_CONFIG_MCLK_DIVIDER);
  mic_array_pdm_clock_start(&pdm_res);
}


void ma_vanilla_task(
    chanend_t c_frames_out)
{
  mics.OutputHandler.FrameTx.SetChannel(c_frames_out);

  /*
   * If the decimator thread is ever late for a PDM block, drop the block
   * instead of trapping: on a fielded speakerphone a click is preferable to a
   * dead microphone until the next power cycle. Nominal load is ~40 MIPS on a
   * dedicated core; the bench captures are checked for glitches.
   */
  mics.PdmRx.AssertOnDroppedBlock(false);

  mics.PdmRx.InstallISR();
  mics.PdmRx.UnmaskISR();

  mics.ThreadEntry();
}

#else
# error appconfMIC_ARRAY_SAMPLE_RATE must be 16000 or 48000.
#endif
