*************************
Far-field Voice Assistant
*************************

This is the far-field voice assistant example design firmware.  See the full documentation for more information on configuring, modifying, building, and running the firmware.

Supported Hardware and pre-requisites
=====================================

This example is supported on the XK_VOICE_L71 board.

Make sure that your XTC tools environment is activated.

It is recommended to use `Ninja` or `xmake` as the make system under Windows.
`Ninja` has been observed to be faster than `xmake`, however `xmake` comes natively with XTC tools.
This firmware has been tested with `Ninja` version v1.11.1.

To install Ninja, activate your python environment, and run the following command:

::

   $ pip install ninja

Before building the host application, you will need to add the path to the XTC Tools to your environment.

  set "XMOS_TOOL_PATH=<path-to-xtc-tools>"

Building the Host Applications
==============================

This application requires a host application to create the flash data partition. Run the following commands in the root folder to build the host application using your native Toolchain:

.. note::

    Permissions may be required to install the host applications.

On Linux and Mac run:

    cmake -B build_host
    cd build_host
    make install

The host applications will be installed at ``/opt/xmos/bin``, and may be moved if desired.  You may wish to add this directory to your ``PATH`` variable.

On Windows run:

Before building the host application, you will need to add the path to the XTC Tools to your environment:

::

    set "XMOS_TOOL_PATH=<path-to-xtc-tools>"

Then build the host application:

::

    cmake -G Ninja -B build_host
    cd build_host
    ninja install

The host applications will be installed at ``%USERPROFILE%\.xmos\bin``, and may be moved if desired.  You may wish to add this directory to your ``PATH`` variable.

Building the Firmware
=====================

After having your python environment activated, run the following commands in the root folder to build the firmware:

On Linux and Mac run:

::

    pip install -r requirements.txt
    cmake -B build --toolchain xmos_cmake_toolchain/xs3a.cmake
    cd build

    make example_ffva_ua_adec_altarch
    make example_ffva_int_fixed_delay
    make example_ffva_int_cyberon_fixed_delay

On Windows run:

::

    pip install -r requirements.txt
    cmake -G Ninja -B build --toolchain xmos_cmake_toolchain/xs3a.cmake
    cd build

    ninja example_ffva_ua_adec_altarch
    ninja example_ffva_int_fixed_delay
    ninja example_ffva_int_cyberon_fixed_delay

From the build folder, create the data partition containing the filesystem and
flash the device with the appropriate command to the desired configuration:

On Linux and Mac run:

::

    make flash_app_example_ffva_ua_adec_altarch
    make flash_app_example_ffva_int_fixed_delay
    make flash_app_example_ffva_int_cyberon_fixed_delay

On Windows run:

::

    ninja flash_app_example_ffva_ua_adec_altarch
    ninja flash_app_example_ffva_int_fixed_delay
    ninja flash_app_example_ffva_int_cyberon_fixed_delay

Once flashed, the application will run.

If changes are made to the data partition components, the application must be
re-flashed.

ReSpeaker Lite: 48 kHz microphone capture (``example_ffva_ua_*``)
=================================================================

The stock firmware decimates the two PDM microphones to 16 kHz (the rate of
the XMOS voice pipeline), which caps the captured audio at 8 kHz. The
``ffva_ua`` targets in this fork can instead decimate the mics at 48 kHz and
send both raw capsules to the host at 48 kHz:

::

    cmake -B build --toolchain xmos_cmake_toolchain/xs3a.cmake -DFFVA_UA_MIC_SAMPLE_RATE=48000   # default
    cmake -B build --toolchain xmos_cmake_toolchain/xs3a.cmake -DFFVA_UA_MIC_SAMPLE_RATE=16000   # stock

With ``FFVA_UA_MIC_SAMPLE_RATE=48000``:

* The mic array uses a custom two-stage decimator (32x2, lib_mic_array's
  ``good_48k_filter`` coefficients: 148 + 96 taps, 20 kHz cutoff) instead of
  the 16 kHz prefab. See ``bsp_config/RESPEAKER_LITE/mic_array/``.
* USB capture channels 0/1 are raw mic0/mic1 at 48 kHz (the ``RAW_PAIR``
  layout, ``appconfRESPEAKER_LITE_USB_LAYOUT``), taken straight from the
  decimator with the usual ``appconfRESPEAKER_LITE_RAW_MIC_GAIN_SHIFT``. The
  processed pipeline outputs are 16 kHz and are not put on the wire, so the
  ``STOCK`` and ``PROC_RAW`` layouts are only available at 16 kHz.
* The on-chip 16 kHz pipeline (AEC/IC/NS/AGC, and the playback reference)
  keeps running on a 3:1 decimated copy of the mics (lib_src ``ff3_96t_ds``).
* The USB interface runs at 48 kHz in both directions (one UAC2 clock is
  shared). Playback is decimated 3:1 on the device to the 16 kHz DAC path, so
  speaker output is unchanged.
* The mic frames are 1 ms (``MIC_ARRAY_CONFIG_SAMPLES_PER_FRAME=48``) and the
  application pulls 720 samples per 15 ms pipeline frame from the driver's
  ring buffer, which keeps the decimator thread inside the PDM interrupt's
  timing budget.

Host side, the device enumerates as 48 kHz only; capture clients that want
16 kHz must resample (ALSA ``plug`` with a proper rate converter).

Running the Firmware
====================

Run the following commands in the build folder:

::

    xrun --xscope example_ffva_ua_adec_altarch.xe
    xrun --xscope example_ffva_int_fixed_delay.xe
    xrun --xscope example_ffva_int_cyberon_fixed_delay.xe


Debugging the firmware with `xgdb`
=================================

Run the following commands in the build folder:

::

    xgdb -ex "conn --xscope" -ex "r" example_ffva_ua_adec_altarch.xe
    xgdb -ex "conn --xscope" -ex "r" example_ffva_int_fixed_delay.xe
    xgdb -ex "conn --xscope" -ex "r" example_ffva_int_cyberon_fixed_delay.xe


Running the Firmware With WAV Files
===================================

This application supports USB audio input and output debug configuration.

To enable USB audio debug, configure cmake with:

After having your python environment activated, run the following commands in the root folder to build the firmware:

On Linux and Mac run::

::

    pip install -r requirements.txt
    cmake -B build --toolchain xmos_cmake_toolchain/xs3a.cmake -DDEBUG_FFVA_USB_MIC_INPUT=1
    cd build

    make example_ffva_ua_adec_altarch

On Windows run:

::

    pip install -r requirements.txt
    cmake -G Ninja -B build --toolchain xmos_cmake_toolchain/xs3a.cmake -DDEBUG_FFVA_USB_MIC_INPUT=1
    cd build

    ninja example_ffva_ua_adec_altarch

After rebuilding the firmware, run the application.

In a separate terminal, run the usb audio host utility provided in the tools/audio folder:

::

    process_wav.sh -c4 input.wav output.wav

This application requires the input audio wav file to be 4 channels in the order MIC 0, MIC 1, REF L, REF R.  Output is ASR, ignore, REF L, REF R, MIC 0, MIC 1, where the reference and microphone are passthrough.
