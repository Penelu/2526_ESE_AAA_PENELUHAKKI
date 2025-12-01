/*
 * motor.h
 *
 *  Created on: Nov 11, 2025
 *      Author: nicolas
 */

#ifndef MOTOR_H
#define MOTOR_H

#include "main.h"
#include "tim.h"   // Gives access to htim1 and timer registers

// -----------------------------------------------------------------------------
// Returns the maximum PWM value, which corresponds to the timer ARR.
// This value is used to limit the speed command.
// -----------------------------------------------------------------------------
uint32_t motor_get_pwm_max(void);

// -----------------------------------------------------------------------------
// Initializes the motor PWM outputs.
// - Reads ARR from TIM1
// - Sets an initial duty cycle (e.g., 60%)
// - Starts PWM on TIM1 CH1/CH1N and CH2/CH2N
// -----------------------------------------------------------------------------
void motor_init(void);

// -----------------------------------------------------------------------------
// Applies a new duty cycle to the motor.
// 'duty' must be between 0 and ARR.
// -----------------------------------------------------------------------------
void motor_set_duty(uint32_t duty);

#endif // MOTOR_H


