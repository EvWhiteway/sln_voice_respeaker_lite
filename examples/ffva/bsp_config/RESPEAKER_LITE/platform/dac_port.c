// Copyright 2022-2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

/* System headers */
#include <platform.h>

/* FreeRTOS headers */
#include "FreeRTOS.h"

/* App headers */
#include "platform_conf.h"
#include "platform/driver_instances.h"
#include "aic3204.h"

static void gpo_update(uint32_t mask, int set)
{
    rtos_gpio_port_id_t gpo_port = rtos_gpio_port(PORT_GPO);
    uint32_t val = rtos_gpio_port_in(gpio_ctx_t0, gpo_port);

    if (set) {
        val |= mask;
    } else {
        val &= ~mask;
    }

    rtos_gpio_port_out(gpio_ctx_t0, gpo_port, val);
}

int aic3204_reg_write(uint8_t reg, uint8_t val)
{
    i2c_regop_res_t ret;

    ret = rtos_i2c_master_reg_write(i2c_master_ctx, AIC3204_I2C_DEVICE_ADDR, reg, val);

    if (ret == I2C_REGOP_SUCCESS) {
        return 0;
    } else {
        return -1;
    }
}

void aic3204_codec_reset(void)
{
    gpo_update(PIN_RST_DAC_OUT, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpo_update(PIN_RST_DAC_OUT, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
}

void aic3204_wait(uint32_t wait_ms)
{
    vTaskDelay(pdMS_TO_TICKS(wait_ms));
}