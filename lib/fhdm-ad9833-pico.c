/*
 * Copyright (c) 2021 fhdm-dev
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>

//
#include "hardware/gpio.h"
#include "pico/stdlib.h"

//
#include "fhdm-ad9833-pico.h"

static inline void ad9833_delay() {
    // According to the AD9833 datasheet any of the delays we need to introduce will be
    // not more than 10ns. One clock cycle at 133Mhz is approx 7.5ns so the following nops should be more than enough!
    asm volatile("nop \n nop \n nop \n nop  \n nop  \n nop  \n nop  \n nop  \n nop  \n nop");
}

static void write_register(struct fhdm_ad9833 *ad9833, uint16_t data) {

    DEBUG_PRINT("write_register(): 0x%lX\n", (unsigned long)data);

    //
    // The write logic was shamelessly copied from here:
    // https://stompville.co.uk/?p=853
    //

    gpio_put(ad9833->_fsync_gpio, 0);
    for (uint8_t i = 0; i < 16; i++) {
        if (data & 0x8000)
            gpio_put(ad9833->_sdata_gpio, 1);
        else
            gpio_put(ad9833->_sdata_gpio, 0);
        ad9833_delay();

        gpio_put(ad9833->_sclk_gpio, 0); //data is valid on falling edge
        ad9833_delay();
        gpio_put(ad9833->_sclk_gpio, 1);
        data = data << 1; //shift left by 1 bit
    }
    gpio_put(ad9833->_sdata_gpio, 0); //idle low

    ad9833_delay();
    gpio_put(ad9833->_fsync_gpio, 1);
    
    ad9833_delay();
}

static void pico_platform_init(struct fhdm_ad9833 *ad9833) {
    DEBUG_PRINT("pico_platform_init()\n");

    // FSYNC is active-low, so we'll initialise it to a driven-high state
    gpio_init(ad9833->_fsync_gpio);
    gpio_set_dir(ad9833->_fsync_gpio, GPIO_OUT);
    gpio_put(ad9833->_fsync_gpio, 1);

    gpio_init(ad9833->_sclk_gpio);
    gpio_set_dir(ad9833->_sclk_gpio, GPIO_OUT);
    gpio_put(ad9833->_fsync_gpio, 1);

    gpio_init(ad9833->_sdata_gpio);
    gpio_set_dir(ad9833->_sdata_gpio, GPIO_OUT);
    gpio_put(ad9833->_sdata_gpio, 0);

    DEBUG_PRINT("  done\n");
}

void fhdm_ad9833_pico_new(struct fhdm_ad9833 *ad9833) {
    DEBUG_PRINT("fhdm_ad9833_pico_new()\n");
    ad9833->_platform_init = pico_platform_init;
    ad9833->_write_register = write_register;

    ad9833->_fsync_gpio = 5;
    ad9833->_sclk_gpio = 6;
    ad9833->_sdata_gpio = 7;

    _fhdm_ad9833_new(ad9833);
}
