/*
 * Copyright (c) 2021 fhdm-dev
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <math.h>

//
#include "fhdm-ad9833.h"
//#include "pico/stdlib.h"

#define AD9833_CTRL_REG_D15 15
#define AD9833_CTRL_REG_D14 14
#define AD9833_CTRL_REG_B28 13
#define AD9833_CTRL_REG_HLB 12
#define AD9833_CTRL_REG_FSL 11
#define AD9833_CTRL_REG_PSL 10
#define AD9833_CTRL_REG_D09 9
#define AD9833_CTRL_REG_RST 8
#define AD9833_CTRL_REG_SL1 7
#define AD9833_CTRL_REG_SL2 6
#define AD9833_CTRL_REG_OPB 5
#define AD9833_CTRL_REG_D04 4
#define AD9833_CTRL_REG_DV2 3
#define AD9833_CTRL_REG_D02 2
#define AD9833_CTRL_REG_MOD 1
#define AD9833_CTRL_REG_D00 0

#define AD_2POW28 (1UL << 28) // For calculating freqreg value

static inline void ad9833_set_bit(uint16_t *x, int bitNum) {
    *x |= (1U << bitNum);
}

static inline void ad9833_clear_bit(uint16_t *x, int bitNum) {
    *x &= ~(1U << bitNum);
}

static inline void ad9833_set_mclk(struct fhdm_ad9833 *ad9833, uint32_t mclk) {
    ad9833->_mclk = mclk;
}

static void ad9833_set_pins(struct fhdm_ad9833 *ad9833, uint fsync, uint sclk, uint sdata) {
    ad9833->_fsync_gpio = fsync;
    ad9833->_sclk_gpio = sclk;
    ad9833->_sdata_gpio = sdata;
}

static void ad9833_write_control_register(struct fhdm_ad9833 *ad9833) {
    //DEBUG_PRINT("ad9833_write_control_register()\n");
    ad9833->_write_register(ad9833, ad9833->control_register_value);
}

static uint32_t ad9833_get_freqreg_value(struct fhdm_ad9833 *ad9833, float f) {
    // freq = fMCLK/2^28 × FREQREG
    return (uint32_t)((f * AD_2POW28 / ad9833->_mclk) + 0.5);
}

static void ad9833_write_frequency_register(struct fhdm_ad9833 *ad9833, int reg_id) {
    //DEBUG_PRINT("ad9833_write_frequency_register()\n");

    uint16_t freq_reg_select_bits = reg_id == 0 ? 0x4000 : 0x8000;
    float freq_hz = reg_id == 0 ? ad9833->_frequency0 : ad9833->_frequency1;
    uint32_t freqreg_value = ad9833_get_freqreg_value(ad9833, freq_hz);

    // LSB
    uint16_t data = freq_reg_select_bits | (freqreg_value & 0x3fff);
    ad9833->_write_register(ad9833, data);

    // MSB
    data = freq_reg_select_bits | ((freqreg_value >> 14) & 0x3fff);
    ad9833->_write_register(ad9833, data);
}

static void ad9833_set_frequency_register_value(struct fhdm_ad9833 *ad9833, int reg_id, float freq) {
    //DEBUG_PRINT("ad9833_set_frequency_register_value()\n");

    if (reg_id == 0) {
        ad9833->_frequency0 = freq;
    } else {
        ad9833->_frequency1 = freq;
    }

    if (ad9833->_is_started) {
        // According to 'Figure 28. Data Writes' we need to write to the control register before writing the
        // frequency register - or is this only required if B28 (D13) != 1 ?
        ad9833_write_control_register(ad9833);
        ad9833_write_frequency_register(ad9833, reg_id);
    }
}

static void ad9833_select_frequency_register(struct fhdm_ad9833 *ad9833, int reg_id) {
    if (reg_id == 0) {
        ad9833_clear_bit(&ad9833->control_register_value, AD9833_CTRL_REG_FSL);
    } else {
        ad9833_set_bit(&ad9833->control_register_value, AD9833_CTRL_REG_FSL);
    }

    if (ad9833->_is_started) {
        ad9833_write_control_register(ad9833);
    }
}

static void ad9833_set_frequency(struct fhdm_ad9833 *ad9833, float freq) {
    if (ad9833->control_register_value & (1 << AD9833_CTRL_REG_FSL)) {
        // Frequency register 1 is selected. Select register 0.
        ad9833_select_frequency_register(ad9833, 0);
    }
    ad9833_set_frequency_register_value(ad9833, 0, freq);
}

uint32_t ad9833_get_phasereg_value(float radians) {

    // From the datasheet:
    // This signal is phase shifted by
    // 2π/4096 × PHASEREG
    // where PHASEREG is the value contained in the selected phase
    return (radians * 4096) / (2 * M_PI);
}

static void ad9833_write_phase_register(struct fhdm_ad9833 *ad9833, int reg_id) {
    //DEBUG_PRINT("ad9833_write_phase_register()\n");

    float phase = reg_id == 0 ? ad9833->_phase0 : ad9833->_phase1;
    uint32_t phasereg_value = ad9833_get_phasereg_value(phase);

    // PHASEREG0: D15=1, D14=1, D13=0
    // PHASEREG1: D15=1, D14=1, D13=1
    uint16_t phase_reg_select_bits = reg_id == 0 ? 0xC000 : 0xD000;
    uint16_t data = phase_reg_select_bits | phasereg_value;
    ad9833->_write_register(ad9833, data);
}

static void ad9833_set_phase_register_value(struct fhdm_ad9833 *ad9833, int reg_id, float phase) {
    //DEBUG_PRINT("ad9833_set_phase_register_value()\n");

    if (reg_id == 0) {
        ad9833->_phase0 = phase;
    } else {
        ad9833->_phase1 = phase;
    }

    if (ad9833->_is_started) {
        ad9833_write_phase_register(ad9833, reg_id);
    }
}

static void ad9833_select_phase_register(struct fhdm_ad9833 *ad9833, int reg_id) {
    if (reg_id == 0) {
        ad9833_clear_bit(&ad9833->control_register_value, AD9833_CTRL_REG_PSL);
    } else {
        ad9833_set_bit(&ad9833->control_register_value, AD9833_CTRL_REG_PSL);
    }

    if (ad9833->_is_started) {
        ad9833_write_control_register(ad9833);
    }
}

static void ad9833_set_phase(struct fhdm_ad9833 *ad9833, float phase) {
    if (ad9833->control_register_value & (1 << AD9833_CTRL_REG_PSL)) {
        // Phase register 1 is selected. Select regsiter 0.
        ad9833_select_phase_register(ad9833, 0);
    }
    ad9833_set_phase_register_value(ad9833, 0, phase);
}

static void ad9833_set_mode(struct fhdm_ad9833 *ad9833, enum fhdm_ad9833_mode mode) {
    if (mode == FHDM_AD9833_MODE_SLEEP) {
        ad9833_set_bit(&ad9833->control_register_value, AD9833_CTRL_REG_SL1);
        ad9833_set_bit(&ad9833->control_register_value, AD9833_CTRL_REG_SL2);
    } else {
        ad9833_clear_bit(&ad9833->control_register_value, AD9833_CTRL_REG_SL1);
        ad9833_clear_bit(&ad9833->control_register_value, AD9833_CTRL_REG_SL2);

        if (mode == FHDM_AD9833_MODE_SINE) {
            ad9833_clear_bit(&ad9833->control_register_value, AD9833_CTRL_REG_OPB);
            ad9833_clear_bit(&ad9833->control_register_value, AD9833_CTRL_REG_MOD);
        } else if (mode == FHDM_AD9833_MODE_TRIANGLE) {
            ad9833_clear_bit(&ad9833->control_register_value, AD9833_CTRL_REG_OPB);
            ad9833_set_bit(&ad9833->control_register_value, AD9833_CTRL_REG_MOD);
        } else if (mode == FHDM_AD9833_MODE_SQUARE) {
            ad9833_set_bit(&ad9833->control_register_value, AD9833_CTRL_REG_OPB);
            ad9833_clear_bit(&ad9833->control_register_value, AD9833_CTRL_REG_DV2);
            ad9833_clear_bit(&ad9833->control_register_value, AD9833_CTRL_REG_MOD);
        } else if (mode == FHDM_AD9833_MODE_SQUARE_DIV2) {
            ad9833_set_bit(&ad9833->control_register_value, AD9833_CTRL_REG_OPB);
            ad9833_set_bit(&ad9833->control_register_value, AD9833_CTRL_REG_DV2);
            ad9833_clear_bit(&ad9833->control_register_value, AD9833_CTRL_REG_MOD);
        }
    }

    if (ad9833->_is_started) {
        ad9833_write_control_register(ad9833);
    }
}

static void ad9833_enter_reset(struct fhdm_ad9833 *ad9833) {
    //DEBUG_PRINT("ad9833_enter_reset()\n");
    ad9833_set_bit(&ad9833->control_register_value, AD9833_CTRL_REG_RST);
    ad9833_write_control_register(ad9833);
}

static void ad9833_exit_reset(struct fhdm_ad9833 *ad9833) {
    //DEBUG_PRINT("ad9833_exit_reset()\n");
    ad9833_clear_bit(&ad9833->control_register_value, AD9833_CTRL_REG_RST);
    ad9833_write_control_register(ad9833);
}

static void ad9833_start(struct fhdm_ad9833 *ad9833) {
    //DEBUG_PRINT("ad9833_start()\n");

    ad9833->_platform_init(ad9833);

    //
    // See Figure 27. Flowchart for Initialization.
    //

    ad9833_enter_reset(ad9833);

    // starting freq
    ad9833_write_frequency_register(ad9833, 0);
    ad9833_write_frequency_register(ad9833, 1);

    // starting phase
    ad9833_write_phase_register(ad9833, 0);
    ad9833_write_phase_register(ad9833, 1);

    ad9833_exit_reset(ad9833);

    ad9833->_is_started = true;
}

void _fhdm_ad9833_new(struct fhdm_ad9833 *ad9833) {
    //DEBUG_PRINT("_fhdm_ad9833_new()\n");

    // Initialise function pointers
    ad9833->set_mclk = ad9833_set_mclk;
    ad9833->set_pins = ad9833_set_pins;
    ad9833->start = ad9833_start;
    ad9833->set_frequency_register_value = ad9833_set_frequency_register_value;
    ad9833->select_frequency_register = ad9833_select_frequency_register;
    ad9833->set_frequency = ad9833_set_frequency;
    ad9833->set_phase_register_value = ad9833_set_phase_register_value;
    ad9833->select_phase_register = ad9833_select_phase_register;
    ad9833->set_phase = ad9833_set_phase;
    ad9833->set_mode = ad9833_set_mode;

    //
    ad9833->_is_started = false;

    // Set defaults

    ad9833->_mclk = 25000000UL;

    ad9833->control_register_value =
        (0 << AD9833_CTRL_REG_D15) | // sending to control register
        (0 << AD9833_CTRL_REG_D14) | // sending to control register
        (1 << AD9833_CTRL_REG_B28) | // two writes required to load freq register
        (0 << AD9833_CTRL_REG_HLB) | // ignored when B28 is 1
        (0 << AD9833_CTRL_REG_FSL) | // Freq register used by the phase accumulator
        (0 << AD9833_CTRL_REG_PSL) | // Phase register used by the phase accumulator
        (0 << AD9833_CTRL_REG_D09) | // reserved
        (0 << AD9833_CTRL_REG_RST) | // reset (1=reset, 0=not reset)
        (0 << AD9833_CTRL_REG_SL1) | // SLEEP1 (1=sleep MCLK)
        (0 << AD9833_CTRL_REG_SL2) | // SLEEP12 (1=sleep DAC)
        (0 << AD9833_CTRL_REG_OPB) | // OPBITEN (0=Output from DAC)
        (0 << AD9833_CTRL_REG_D04) | // reserved
        (0 << AD9833_CTRL_REG_DV2) | // not used then OPBITEN=0?
        (0 << AD9833_CTRL_REG_D02) | // reserved
        (0 << AD9833_CTRL_REG_MOD) | // Mode (0=sine,1=triangle)
        (0 << AD9833_CTRL_REG_D00)   // reserved
        ;

    // Hz
    ad9833->_frequency0 = 100.0;
    ad9833->_frequency1 = 0.1;

    // Radians
    ad9833->_phase0 = 0.0;
    ad9833->_phase1 = M_PI;
}
