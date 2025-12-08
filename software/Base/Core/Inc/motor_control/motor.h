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

uint32_t motor_get_pwm_max(void);
uint32_t motor_get_duty(void);
void motor_init(void);
void motor_set_duty(uint32_t duty);
void motor_set_target(uint32_t duty_target);
void motor_ramp_task(void);
void motor_start(void);
void motor_stop(void);


#endif // MOTOR_H


