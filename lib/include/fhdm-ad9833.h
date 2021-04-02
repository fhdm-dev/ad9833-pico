/*
 * Copyright (c) 2021 fhdm-dev
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __FHDM_AD9833__
#define __FHDM_AD9833__

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdint.h>

#ifndef NDEBUG
#define DEBUG_PRINT(fmt, args...) printf(fmt, ##args)
#else
#define DEBUG_PRINT(fmt, args...)
#endif

enum fhdm_ad9833_mode {
    FHDM_AD9833_MODE_SLEEP, FHDM_AD9833_MODE_SINE, FHDM_AD9833_MODE_TRIANGLE, FHDM_AD9833_MODE_SQUARE, FHDM_AD9833_MODE_SQUARE_DIV2
};

struct fhdm_ad9833 {
    uint16_t control_register_value;

    void (*set_mclk)(struct fhdm_ad9833 *ad9833, uint32_t mclk);
    void (*set_pins)(struct fhdm_ad9833 *ad9833, uint fsync, uint sclk, uint sdata);
    void (*start)(struct fhdm_ad9833 *ad9833);
    void (*set_frequency)(struct fhdm_ad9833 *ad9833, float freq);
    void (*set_frequency_register_value)(struct fhdm_ad9833 *ad9833, int reg_id, float freq);
    void (*select_frequency_register)(struct fhdm_ad9833 *ad9833, int reg_id);
    void (*set_phase_register_value)(struct fhdm_ad9833 *ad9833, int reg_id, float phase);
    void (*select_phase_register)(struct fhdm_ad9833 *ad9833, int reg_id);
    void (*set_phase)(struct fhdm_ad9833 *ad9833, float phase);
    void (*set_mode)(struct fhdm_ad9833 *ad9833, enum fhdm_ad9833_mode mode);

    //
    // Private functions and variables. Should not be accessed directly by users of the library - unless of course you want to :)
    //
    bool _is_started;

    uint32_t _mclk;
    uint _fsync_gpio;
    uint _sclk_gpio;
    uint _sdata_gpio;

    float _frequency0;
    float _frequency1;

    float _phase0;
    float _phase1;

    void (*_platform_init)(struct fhdm_ad9833 *ad9833);
    void (*_write_register)(struct fhdm_ad9833 *ad9833, uint16_t data);
};

//
// Private functions. Should not be called by users of the library.
//

void _fhdm_ad9833_new(struct fhdm_ad9833* ad9833);

#endif // __FHDM_AD9833__
