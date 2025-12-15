/*
 * input_analog.h
 *
 *  Created on: Nov 11, 2025
 *      Author: nicolas
 */

#ifndef INC_INPUT_ANALOG_H_
#define INC_INPUT_ANALOG_H_

#include <stdint.h>

/* Public API: polling-only analog acquisition (no DMA) */
void    input_analog_init(void);

/* Update cached raw values by polling ADC (call periodically in main loop) */
void    input_analog_update_polling(void);

/* Raw ADC getters (12-bit) */
uint16_t input_analog_get_raw_imes(void);   // ADC counts
uint16_t input_analog_get_raw_vbus(void);   // ADC counts

/* Physical values with integer scaling (no float / no %f) */
int32_t input_analog_get_imes_mA(void);     // milliamps
int32_t input_analog_get_vbus_adc_mV(void); // millivolts at ADC pin (NOT real bus voltage yet)

/* Optional: zero-offset calibration (call when I=0 and Vbus reference known) */
void    input_analog_calibrate_offsets(void);

void input_analog_dma_start(void);
void input_analog_dma_stop(void);
void input_analog_dma_on_new_samples_isr(void);


#endif /* INC_INPUT_ANALOG_H_ */
