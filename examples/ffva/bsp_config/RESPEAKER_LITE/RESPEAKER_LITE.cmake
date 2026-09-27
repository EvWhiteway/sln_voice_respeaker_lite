set(RESPEAKER_LITE_BSP_DIR ${CMAKE_CURRENT_LIST_DIR})

## Mic array driver for the ReSpeaker Lite.
##
## This is the fwk_rtos mic array driver (rtos::drivers::mic_array) with one
## substitution: the library's vanilla translation unit, which hard-wires the
## 16 kHz decimator, is replaced by mic_array/ffva_mic_array.cpp so the
## decimator can be selected per board variant (see appconfMIC_ARRAY_SAMPLE_RATE
## below). Everything else (driver sources, include paths, dependencies) mirrors
## modules/rtos/modules/drivers/mic_array/CMakeLists.txt.
set(RESPEAKER_LITE_MIC_ARRAY_DRIVER_DIR ${FRAMEWORK_RTOS_ROOT_PATH}/modules/drivers/mic_array)
set(RESPEAKER_LITE_LIB_MIC_ARRAY_DIR ${FRAMEWORK_IO_ROOT_PATH}/modules/mic_array)

add_library(sln_voice_app_ffva_respeaker_lite_mic_array INTERFACE)
target_sources(sln_voice_app_ffva_respeaker_lite_mic_array
    INTERFACE
        ${RESPEAKER_LITE_MIC_ARRAY_DRIVER_DIR}/src/rtos_mic_array.c
        ${RESPEAKER_LITE_MIC_ARRAY_DRIVER_DIR}/src/rtos_mic_array_rpc.c
        ${RESPEAKER_LITE_BSP_DIR}/mic_array/ffva_mic_array.cpp
)
target_include_directories(sln_voice_app_ffva_respeaker_lite_mic_array
    INTERFACE
        ${RESPEAKER_LITE_MIC_ARRAY_DRIVER_DIR}/api
        ${RESPEAKER_LITE_LIB_MIC_ARRAY_DIR}/etc/vanilla/
        ${RESPEAKER_LITE_BSP_DIR}/mic_array
)
target_link_libraries(sln_voice_app_ffva_respeaker_lite_mic_array
    INTERFACE
        lib_mic_array
        rtos::osal
)
target_compile_definitions(sln_voice_app_ffva_respeaker_lite_mic_array
    INTERFACE
        MIC_ARRAY_BASIC_API_ENABLE=1
)

## Create custom board targets for the ReSpeaker Lite application.
##
## Two variants differ only in the PDM mic array configuration:
##   respeaker_lite          16 kHz mics, 240-sample (15 ms) mic frames - stock
##   respeaker_lite_mic48k   48 kHz mics, 48-sample (1 ms) mic frames; the app
##                           pulls a pipeline frame (720 samples) per call and
##                           feeds the 16 kHz pipeline a decimated copy
function(respeaker_lite_board_target NAME MIC_SAMPLE_RATE MIC_SAMPLES_PER_FRAME)
    add_library(${NAME} INTERFACE)
    target_sources(${NAME}
        INTERFACE
            ${RESPEAKER_LITE_BSP_DIR}/platform/dac_port.c
            ${RESPEAKER_LITE_BSP_DIR}/platform/app_pll_ctrl.c
            ${RESPEAKER_LITE_BSP_DIR}/platform/driver_instances.c
            ${RESPEAKER_LITE_BSP_DIR}/platform/platform_init.c
            ${RESPEAKER_LITE_BSP_DIR}/platform/platform_start.c
    )
    target_include_directories(${NAME}
        INTERFACE
            ${RESPEAKER_LITE_BSP_DIR}
    )
    target_link_libraries(${NAME}
        INTERFACE
            core::general
            rtos::freertos
            rtos::drivers::general
            rtos::drivers::i2s
            sln_voice_app_ffva_respeaker_lite_mic_array
            rtos::drivers::usb
            rtos::drivers::dfu_image
            sln_voice::app::ffva::dac::aic3204
    )
    target_compile_options(${NAME}
        INTERFACE
            ${RESPEAKER_LITE_BSP_DIR}/RESPEAKER_LITE.xn
    )
    target_link_options(${NAME}
        INTERFACE
            ${RESPEAKER_LITE_BSP_DIR}/RESPEAKER_LITE.xn
    )
    target_compile_definitions(${NAME}
        INTERFACE
            RESPEAKER_LITE=1
            PLATFORM_SUPPORTS_TILE_0=1
            PLATFORM_SUPPORTS_TILE_1=1
            PLATFORM_SUPPORTS_TILE_2=0
            PLATFORM_SUPPORTS_TILE_3=0
            USB_TILE_NO=0
            USB_TILE=tile[USB_TILE_NO]
            MIC_ARRAY_CONFIG_PDM_FREQ=3072000
            MIC_ARRAY_CONFIG_SAMPLES_PER_FRAME=${MIC_SAMPLES_PER_FRAME}
            MIC_ARRAY_CONFIG_MIC_COUNT=2
            MIC_ARRAY_CONFIG_CLOCK_BLOCK_A=XS1_CLKBLK_1
            MIC_ARRAY_CONFIG_CLOCK_BLOCK_B=XS1_CLKBLK_2
            MIC_ARRAY_CONFIG_PORT_MCLK=PORT_MCLK_IN_OUT
            MIC_ARRAY_CONFIG_PORT_PDM_CLK=PORT_PDM_CLK
            MIC_ARRAY_CONFIG_PORT_PDM_DATA=PORT_PDM_DATA
            appconfMIC_ARRAY_SAMPLE_RATE=${MIC_SAMPLE_RATE}
    )
endfunction()

respeaker_lite_board_target(sln_voice_app_ffva_board_support_respeaker_lite 16000 240)
add_library(sln_voice::app::ffva::respeaker_lite ALIAS sln_voice_app_ffva_board_support_respeaker_lite)

respeaker_lite_board_target(sln_voice_app_ffva_board_support_respeaker_lite_mic48k 48000 48)
add_library(sln_voice::app::ffva::respeaker_lite_mic48k ALIAS sln_voice_app_ffva_board_support_respeaker_lite_mic48k)
