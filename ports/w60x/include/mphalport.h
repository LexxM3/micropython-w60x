/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Development of the code in this file was sponsored by Microbric Pty Ltd
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef INCLUDED_MPHALPORT_H
#define INCLUDED_MPHALPORT_H

#include "wm_regs.h"
#include "py/ringbuf.h"
#include "shared/runtime/interrupt_char.h"

#include "wm_osal.h"
#include "py/obj.h"
#include "wm_io.h"
#include "wm_gpio.h"
#define MP_HAL_PIN_FMT "%u"
#define mp_hal_pin_obj_t enum tls_io_name

#define CNT_START_VALUE (0xffffffff)

extern ringbuf_t stdin_ringbuf;

uint32_t mp_hal_ticks_us(void);

void mp_hal_delay_us(uint32_t);
void mp_hal_delay_us_fast(uint32_t);
void mp_hal_set_interrupt_char(int c);
uint32_t mp_hal_get_cpu_freq(void);

static inline uint32_t mp_hal_ticks_cpu(void) {
    return CNT_START_VALUE - tls_reg_read32(HR_WDG_CUR_VALUE);
}

static inline void mp_hal_pin_high(uint32_t reg, uint32_t mask) {
		*(TLS_REG *)reg |= mask;	/* write high */
}

static inline void mp_hal_pin_low(uint32_t reg, uint32_t mask) {
		*(TLS_REG *)reg &= mask;	/* write low */
}

static inline uint32_t mp_hal_ticks_bitstream(void) {
    return tls_reg_read32(HR_WDG_CUR_VALUE);
}

#define mp_hal_ticks_bitstream_start mp_hal_ticks_bitstream

#define mp_hal_quiet_timing_enter() MICROPY_BEGIN_ATOMIC_SECTION()
#define mp_hal_quiet_timing_exit(irq_state) MICROPY_END_ATOMIC_SECTION(irq_state)


// C-level pin HAL
mp_hal_pin_obj_t machine_pin_get_id(mp_obj_t pin_in);
#define mp_hal_get_pin_obj(o) machine_pin_get_id(o)
#define mp_obj_get_pin(o) machine_pin_get_id(o)
#define mp_hal_pin_name(p) (p)
// #define mp_hal_pin_low(p) tls_gpio_write(p, 0);
// #define mp_hal_pin_high(p) tls_gpio_write(p, 1);
void mp_hal_pin_input(mp_hal_pin_obj_t pin);
void mp_hal_pin_output(mp_hal_pin_obj_t pin);
void mp_hal_pin_open_drain(mp_hal_pin_obj_t pin);
void mp_hal_pin_od_low(mp_hal_pin_obj_t pin);
void mp_hal_pin_od_high(mp_hal_pin_obj_t pin);
static inline int mp_hal_pin_read(mp_hal_pin_obj_t pin) {
    return tls_gpio_read(pin);
}
void mp_hal_pin_write(mp_hal_pin_obj_t pin, int v);

#endif // INCLUDED_MPHALPORT_H
