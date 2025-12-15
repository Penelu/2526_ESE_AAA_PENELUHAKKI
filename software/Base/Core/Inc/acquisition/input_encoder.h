/*
 * input_encoder.h
 *
 *  Created on: Nov 11, 2025
 *      Author: nicolas
 */

#ifndef INC_ACQUISITION_INPUT_ENCODER_H_
#define INC_ACQUISITION_INPUT_ENCODER_H_

#pragma once

#include <stdint.h>
#include "user_interface/shell.h"   // h_shell_t

/* Initialize encoder acquisition and register shell commands */
void input_encoder_init(void);

/* Start/stop Hall sensor capture on TIM3 */
void input_encoder_start(void);
void input_encoder_stop(void);

/* Called from ISR callback when a new Hall event occurs */
void input_encoder_on_hall_event_isr(void);

/* Configuration: number of Hall edges per mechanical revolution.
 * Typical BLDC Hall gives 6 electrical steps per electrical revolution.
 * Mechanical rev depends on pole pairs, but for TP you can start with 6 and adjust.
 */
void input_encoder_set_pulses_per_rev(uint32_t ppr);

/* Getters (thread-safe enough for this TP) */
uint32_t input_encoder_get_last_period_ticks(void);  // TIM3 ticks between events
uint32_t input_encoder_get_last_freq_hz(void);       // event frequency (Hz)
int32_t  input_encoder_get_last_speed_rpm(void);     // mechanical speed (RPM)


#endif /* INC_ACQUISITION_INPUT_ENCODER_H_ */
