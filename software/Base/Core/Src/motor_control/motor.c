/*
 * motor.c
 *
 *  Created on: Nov 11, 2025
 *      Author: nicolas
 */

#include "motor_control/motor.h"

static uint32_t pwm_max     = 0;  // Stores the ARR value of TIM1
static uint32_t pwm_current = 0;  // Stores the current duty cycle


uint32_t motor_get_pwm_max(void)
{
    return pwm_max;
}

uint32_t motor_get_duty(void)
{
    return pwm_current;
}

void motor_init(void)
{
    // Read the current auto-reload value (defines the PWM resolution)
    pwm_max = __HAL_TIM_GET_AUTORELOAD(&htim1);

    // Compute a 50% duty cycle
    uint32_t duty50 = (pwm_max + 1) * 50 / 100;
    if (duty50 > pwm_max) {
        duty50 = pwm_max;
    }

    pwm_current = duty50;

    // Apply the 50% duty on both motor channels (CH1 and CH2)
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, duty50);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, duty50);

    // Do NOT start PWM here.
    // PWM will be started explicitly via motor_start().
}

void motor_set_duty(uint32_t duty)
{
    // If pwm_max was not initialized yet, read ARR now
    if (pwm_max == 0) {
        pwm_max = __HAL_TIM_GET_AUTORELOAD(&htim1);
    }

    // Limit to the valid range
    if (duty > pwm_max) {
        duty = pwm_max;
    }

    pwm_current = duty;

    // Update CCR registers for both PWM channels
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, duty);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, duty);
}

void motor_start(void)
{
    // Ensure pwm_max is initialized
    if (pwm_max == 0) {
        pwm_max = __HAL_TIM_GET_AUTORELOAD(&htim1);
    }

    // Compute a 50% duty cycle
    uint32_t duty50 = (pwm_max + 1) * 50 / 100;
    if (duty50 > pwm_max) {
        duty50 = pwm_max;
    }

    // Apply 50% duty
    motor_set_duty(duty50);

    // Start PWM output for CH1 and its complementary channel CH1N
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_1);

    // Start PWM output for CH2 and its complementary channel CH2N
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
    HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_2);
}

void motor_stop(void)
{
    // Stop PWM output for CH1 and its complementary channel CH1N
    HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
    HAL_TIMEx_PWMN_Stop(&htim1, TIM_CHANNEL_1);

    // Stop PWM output for CH2 and its complementary channel CH2N
    HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_2);
    HAL_TIMEx_PWMN_Stop(&htim1, TIM_CHANNEL_2);
}
