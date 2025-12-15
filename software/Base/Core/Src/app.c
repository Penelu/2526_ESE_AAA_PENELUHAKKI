/*
 * app.c
 *
 *  Created on: Nov 11, 2025
 *      Author: nicolas
 */

#include "app.h"

#include "user_interface/shell.h"
#include "user_interface/led.h"
#include "motor_control/motor.h"
#include "acquisition/input_encoder.h"
#include "acquisition/input_analog.h"

#define MEAS_Pin		GPIO_PIN_12
#define MEAS_GPIO_Port	GPIOA

static char shell_uart2_received_char;

void init_device(void){
	// Initialisation user interface
	// SHELL
	hshell1.drv.transmit = shell_uart2_transmit;
	hshell1.drv.receive = shell_uart2_receive;
	shell_init(&hshell1);
	HAL_UART_Receive_IT(&huart2, (uint8_t *)&shell_uart2_received_char, 1);

	// LED
	led_init();

	// BUTTON
	//	button_init();
	//
	// Initialisation motor control
	// MOTOR
	motor_init();
	// ASSERV (PID)
	//	asserv_init();
	//
	// Initialisation data acquistion
	// ANALOG INPUT
	input_analog_init();
	input_analog_dma_start();
	// ENCODER INPUT
	//input_encoder_init();
}

uint8_t shell_uart2_transmit(const char *pData, uint16_t size)
{
	HAL_UART_Transmit(&huart2, (uint8_t *)pData, size, HAL_MAX_DELAY);
	return size;
}

uint8_t shell_uart2_receive(char *pData, uint16_t size)
{
	*pData = shell_uart2_received_char;
	return 1;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart->Instance == USART2) {
		//		HAL_UART_Transmit(&huart2, (uint8_t *)&shell_uart2_received_char, 1, HAL_MAX_DELAY);
		HAL_UART_Receive_IT(&huart2, (uint8_t *)&shell_uart2_received_char, 1);
		shell_run(&hshell1);
	}
}

void HAL_TIMEx_HallSensorTriggerCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM3)
    {
        input_encoder_on_hall_event_isr();
    }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
    {
        input_analog_dma_on_new_samples_isr();

        // GPIO pulse for oscilloscope sync check (very fast toggle)
        MEAS_GPIO_Port->BSRR = (uint32_t)MEAS_Pin;               // set
        MEAS_GPIO_Port->BSRR = (uint32_t)MEAS_Pin << 16U;        // reset
    }
}


void loop(void)
{
    static uint32_t t0 = 0;

    motor_ramp_task();

    /* Update analog inputs every 10 ms */
    if ((HAL_GetTick() - t0) >= 10U)
    {
        t0 = HAL_GetTick();
        input_analog_update_polling();
    }
}

