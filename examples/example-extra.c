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

struct fhdm_ad9833 ad9833;

bool gpio_state[29];

void gpio_callback(uint gpio, uint32_t events) {
    bool state = gpio_get(gpio);
    printf("EXAMPLE: gpio_callback(): gpio=%u, events=0x%lX, state=%d\n", gpio, (unsigned long)events, (int)state);
    if (state != gpio_state[gpio]) {
        gpio_state[gpio] = state;

        if (!state) {
            switch (gpio) {

            case 11:
                printf("set_mode(square div2)\n");
                ad9833.set_mode(&ad9833, FHDM_AD9833_MODE_SQUARE_DIV2);
                break;

            case 12:
                printf("set_mode(square)\n");
                ad9833.set_mode(&ad9833, FHDM_AD9833_MODE_SQUARE);
                break;

            case 13:
                printf("set_mode(triangle)\n");
                ad9833.set_mode(&ad9833, FHDM_AD9833_MODE_TRIANGLE);
                break;

            case 14:
                printf("set_mode(sine)\n");
                ad9833.set_mode(&ad9833, FHDM_AD9833_MODE_SINE);
                break;

            case 15:
                printf("set_mode(sleep)\n");
                ad9833.set_mode(&ad9833, FHDM_AD9833_MODE_SLEEP);
                break;

            case 16:
                printf("set_frequency(0.1)\n");
                ad9833.set_frequency(&ad9833, 0.1);
                break;

            case 17:
                printf("set_frequency(1.0)\n");
                ad9833.set_frequency(&ad9833, 1.0);
                break;

            case 18:
                printf("set_frequency(1000.0)\n");
                ad9833.set_frequency(&ad9833, 1000.0);
                break;

            case 19:
                printf("set_frequency_register_value(0, 0.1)\n");
                ad9833.set_frequency_register_value(&ad9833, 0, 0.1);
                break;

            case 20:
                printf("set_frequency_register_value(0, 1.0)\n");
                ad9833.set_frequency_register_value(&ad9833, 0, 1.0);
                break;

            case 21:
                printf("set_frequency_register_value(1, 0.1)\n");
                ad9833.set_frequency_register_value(&ad9833, 1, 0.1);
                break;

            case 22:
                printf("set_frequency_register_value(1, 1.0)\n");
                ad9833.set_frequency_register_value(&ad9833, 1, 1.0);
                break;

            case 26:
                printf("select_frequency_register(0)\n");
                ad9833.select_frequency_register(&ad9833, 0);
                break;

            case 27:
                printf("select_frequency_register(1)\n");
                ad9833.select_frequency_register(&ad9833, 1);
                break;

            default:
                break;
            }
        }
    }
}

int main() {

    sleep_ms(100);
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 1);

    stdio_init_all();
    printf("main()\n");

    fhdm_ad9833_pico_new(&ad9833);

    // initial frequency
    ad9833.set_frequency(&ad9833, 500.0);

    for (uint i = 10; i <= 28; i++) {
        if (i > 22 && i < 26) {
            continue;
        }

        gpio_init(i);
        gpio_pull_up(i);
        gpio_state[i] = true;
        gpio_set_irq_enabled_with_callback(i, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    }

    printf("  about to call start()\n");
    ad9833.start(&ad9833);

    printf("  returned from start()\n");
    while (1);
}
