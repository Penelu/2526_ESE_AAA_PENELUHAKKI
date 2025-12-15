/*
 * input_encoder.c
 *
 *  Created on: Nov 11, 2025
 *      Author: nicolas
 */

#include "acquisition/input_encoder.h"
/*
 * input_encoder.c
 *
 *  Refactored encoder (Hall sensor) acquisition using TIM3.
 *  - Measures period between Hall events using TIM3 capture (CCR1)
 *  - Computes event frequency and estimated speed (RPM)
 *  - Provides a shell command: "enc"
 */

#include "acquisition/input_encoder.h"
#include "tim.h"                 // htim3
#include <stdio.h>

/* ---------- Shell command ---------- */
static int cmd_enc(h_shell_t* h_shell, int argc, char** argv);

/* ---------- Configuration ---------- */
/* TIM3 timer clock (Hz). If your TIM3 really runs at 170 MHz, keep this.
 * Otherwise, adjust according to your clock tree.
 */
#define TIM3_CLK_HZ   (170000000UL)

/* Default: 6 Hall events per revolution (common starting point) */
static volatile uint32_t s_pulses_per_rev = 6U;

/* ---------- Measured data (updated in ISR) ---------- */
static volatile uint32_t s_last_period_ticks = 0U;   // CCR1
static volatile uint32_t s_last_freq_hz = 0U;        // TIM3_CLK_HZ / period
static volatile int32_t  s_last_rpm = 0;             // computed RPM

void input_encoder_init(void)
{
    /* Register shell command */
    shell_add(&hshell1, "enc", cmd_enc, "Read Hall encoder period/freq/speed");

    /* Start acquisition by default (you can change this policy if needed) */
    input_encoder_start();
}

void input_encoder_start(void)
{
    /* Start Hall sensor interface with interrupts */
    HAL_TIMEx_HallSensor_Start_IT(&htim3);
}

void input_encoder_stop(void)
{
    HAL_TIMEx_HallSensor_Stop_IT(&htim3);
}

void input_encoder_set_pulses_per_rev(uint32_t ppr)
{
    if (ppr == 0U) {
        ppr = 1U;
    }
    s_pulses_per_rev = ppr;
}

uint32_t input_encoder_get_last_period_ticks(void)
{
    return s_last_period_ticks;
}

uint32_t input_encoder_get_last_freq_hz(void)
{
    return s_last_freq_hz;
}

int32_t input_encoder_get_last_speed_rpm(void)
{
    return s_last_rpm;
}

/* This must be called from the Hall sensor ISR callback */
void input_encoder_on_hall_event_isr(void)
{
    /* In Hall sensor mode, CCR1 typically captures the period between events */
    uint32_t period = __HAL_TIM_GET_COMPARE(&htim3, TIM_CHANNEL_1);

    if (period == 0U) {
        /* Avoid division by zero; keep last values */
        return;
    }

    s_last_period_ticks = period;

    /* Event frequency (Hz) = timer_clk / period_ticks */
    uint32_t f_hz = (uint32_t)((uint64_t)TIM3_CLK_HZ / (uint64_t)period);
    s_last_freq_hz = f_hz;

    /* Speed RPM = f_event * 60 / pulses_per_rev */
    uint32_t ppr = s_pulses_per_rev;
    int32_t rpm = (int32_t)((uint64_t)f_hz * 60ULL / (uint64_t)ppr);
    s_last_rpm = rpm;
}

/* ---------- Shell command implementation ---------- */
static int cmd_enc(h_shell_t* h_shell, int argc, char** argv)
{
    (void)argc; (void)argv;

    uint32_t period = input_encoder_get_last_period_ticks();
    uint32_t freq   = input_encoder_get_last_freq_hz();
    int32_t rpm     = input_encoder_get_last_speed_rpm();

    int size = snprintf(h_shell->print_buffer, SHELL_PRINT_BUFFER_SIZE,
                        "ENC: period=%lu ticks | f=%lu Hz | speed=%ld rpm | ppr=%lu\r\n",
                        (unsigned long)period,
                        (unsigned long)freq,
                        (long)rpm,
                        (unsigned long)s_pulses_per_rev);

    h_shell->drv.transmit(h_shell->print_buffer, size);
    return 0;
}
