/*
 * motor.c
 *
 *  Created on: Nov 11, 2025
 *      Author: nicolas
 */
// motor.c
#include "motor_control/motor.h"

static uint32_t pwm_max     = 0;  // ARR
static uint32_t pwm_current = 0;  // current duty
static uint32_t pwm_target  = 0;  // target duty for ramp

// How often (ms) to update the ramp
#define MOTOR_RAMP_DT_MS   1U
// Step per tick (duty increment/decrement)
#define MOTOR_RAMP_STEP    1U

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
    pwm_max = __HAL_TIM_GET_AUTORELOAD(&htim1);

    uint32_t duty50 = (pwm_max + 1U) * 50U / 100U;
    if (duty50 > pwm_max) {
        duty50 = pwm_max;
    }

    pwm_current = duty50;
    pwm_target  = duty50;

    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, duty50);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, duty50);
}

void motor_set_duty(uint32_t duty)
{
    if (pwm_max == 0U) {
        pwm_max = __HAL_TIM_GET_AUTORELOAD(&htim1);
    }

    if (duty > pwm_max) {
        duty = pwm_max;
    }

    pwm_current = duty;

    uint32_t ccr1 = duty;
    uint32_t ccr2 = pwm_max - duty;

    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, ccr1);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, ccr2);
}

// Set target for ramp (called from shell)
void motor_set_target(uint32_t duty_target)
{
    if (pwm_max == 0U) {
        pwm_max = __HAL_TIM_GET_AUTORELOAD(&htim1);
    }
    if (duty_target > pwm_max) {
        duty_target = pwm_max;
    }
    pwm_target = duty_target;
}

// Small task to be called periodically in main loop
void motor_ramp_task(void)
{
    static uint32_t last_ms = 0U;
    uint32_t now = HAL_GetTick();

    // Only update every MOTOR_RAMP_DT_MS ms
    if ((now - last_ms) < MOTOR_RAMP_DT_MS) {
        return;
    }
    last_ms = now;

    if (pwm_current < pwm_target) {
        motor_set_duty(pwm_current + MOTOR_RAMP_STEP);
    } else if (pwm_current > pwm_target) {
        motor_set_duty(pwm_current - MOTOR_RAMP_STEP);
    }
    // If equal: nothing to do
}

void motor_start(void)
{
    if (pwm_max == 0U) {
        pwm_max = __HAL_TIM_GET_AUTORELOAD(&htim1);
    }

    uint32_t duty50 = (pwm_max + 1U) * 50U / 100U;
    if (duty50 > pwm_max) {
        duty50 = pwm_max;
    }

    motor_set_duty(duty50);
    pwm_target = duty50;

    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_1);

    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
    HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_2);

    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);
}

void motor_stop(void)
{
    HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
    HAL_TIMEx_PWMN_Stop(&htim1, TIM_CHANNEL_1);

    HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_2);
    HAL_TIMEx_PWMN_Stop(&htim1, TIM_CHANNEL_2);
}
