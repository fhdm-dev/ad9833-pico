/*
 * Copyright (c) 2021 fhdm-dev
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/stdlib.h"
#include <stdio.h>

//
#include "fhdm-ad9833-pico.h"

const uint LED_PIN = 25;

int main() {

    sleep_ms(100);
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 1);

    stdio_init_all();
    printf("example-simple\n");

    struct fhdm_ad9833 ad9833;
    fhdm_ad9833_pico_new(&ad9833);

    // Use GPIO 13, 14 and 15 instead of the defaults.
    //ad9833.set_pins(&ad9833, 13, 14, 15);

    ad9833.start(&ad9833);

    while (1);
}
