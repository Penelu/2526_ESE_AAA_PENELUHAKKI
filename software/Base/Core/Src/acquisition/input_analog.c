/*
 * analog_input.c
 *
 *  Created on: Nov 11, 2025
 *      Author: nicolas
 */
#include "acquisition/input_analog.h"
#include "user_interface/shell.h"
#include "adc.h"
#include <stdio.h>

/* -------------------------------------------------------------------------- */
/* ADC channel mapping (ADJUST HERE if needed)                                */
/* -------------------------------------------------------------------------- */
#define ADC_CH_IMES   ADC_CHANNEL_2   /* ADC1_IN2  */
#define ADC_CH_VBUS   ADC_CHANNEL_8   /* ADC1_IN8  */

/* -------------------------------------------------------------------------- */
/* ADC conversion constants                                                   */
/* -------------------------------------------------------------------------- */
#define ADC_COUNTS_MAX     4095
#define ADC_VREF_mV        3300

/* -------------------------------------------------------------------------- */
/* GO 10-SME/SP3 current sensor parameters                                    */
/* Typical: offset ~ Vref/2, sensitivity ~ 50 mV/A                            */
/* -------------------------------------------------------------------------- */
#define GO_OFFSET_mV_DEFAULT   1650   /* 1.65 V default (can be calibrated) */
#define GO_SENS_mV_PER_A       50     /* 50 mV/A (GO 10-SME/SP3) */

/* Cached raw values */
static volatile uint16_t s_raw_imes = 0;
static volatile uint16_t s_raw_vbus = 0;

/* Offsets (mV at ADC pin) */
static volatile int32_t s_imes_offset_mV = GO_OFFSET_mV_DEFAULT;
/* For Vbus, offset depends on the analog chain. We'll store and allow calibration. */
static volatile int32_t s_vbus_offset_mV = 0;

static volatile uint16_t s_dma_buf[2]; // [IMES, VBUS]

/* Private shell commands */
static int cmd_imes(h_shell_t* h_shell, int argc, char** argv);
static int cmd_vbus(h_shell_t* h_shell, int argc, char** argv);
static int cmd_measraw(h_shell_t* h_shell, int argc, char** argv);
static int cmd_calib(h_shell_t* h_shell, int argc, char** argv);

/* -------------------------------------------------------------------------- */
/* Helpers                                                                    */
/* -------------------------------------------------------------------------- */
static int32_t adc_to_mV(uint16_t adc)
{
    return ((int32_t)adc * ADC_VREF_mV) / ADC_COUNTS_MAX;
}

static uint16_t adc1_read_channel(uint32_t channel)
{
    ADC_ChannelConfTypeDef sConfig;
    uint16_t v = 0;

    sConfig.Channel      = channel;
    sConfig.Rank         = ADC_REGULAR_RANK_1;
    /* Use long sampling time to reduce PWM noise during tests */
    sConfig.SamplingTime = ADC_SAMPLETIME_247CYCLES_5;
    sConfig.SingleDiff   = ADC_SINGLE_ENDED;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.Offset       = 0;

    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
    {
        return 0;
    }

    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK)
    {
        v = (uint16_t)HAL_ADC_GetValue(&hadc1);
    }
    HAL_ADC_Stop(&hadc1);

    return v;
}

/* -------------------------------------------------------------------------- */
/* Public functions                                                           */
/* -------------------------------------------------------------------------- */
void input_analog_init(void)
{
    /* Optional calibration for better accuracy */
    (void)HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);

    /* Register shell commands */
    shell_add(&hshell1, "imes",   cmd_imes,   "Measure IMES current (mA)");
    shell_add(&hshell1, "vbus",   cmd_vbus,   "Measure VBUS ADC voltage (mV)");
    shell_add(&hshell1, "measraw",cmd_measraw,"Print raw ADC counts (IMES/VBUS)");
    shell_add(&hshell1, "calib",  cmd_calib,  "Calibrate offsets at I=0 / Vref");
}

void input_analog_update_polling(void)
{
    /* Read IMES on IN2 */
    s_raw_imes = adc1_read_channel(ADC_CH_IMES);

    /* Read VBUS on IN8 */
    s_raw_vbus = adc1_read_channel(ADC_CH_VBUS);
}

uint16_t input_analog_get_raw_imes(void)
{
    return s_raw_imes;
}

uint16_t input_analog_get_raw_vbus(void)
{
    return s_raw_vbus;
}

int32_t input_analog_get_imes_mA(void)
{
    /* Convert ADC to mV at pin */
    int32_t v_mV = adc_to_mV(s_raw_imes);

    /* Remove offset */
    int32_t vdiff_mV = v_mV - s_imes_offset_mV;

    /* GO 10-SME: 50 mV/A => I(A) = vdiff_mV / 50
       I(mA) = (vdiff_mV * 1000) / 50 = vdiff_mV * 20 */
    return (vdiff_mV * 20);
}

int32_t input_analog_get_vbus_adc_mV(void)
{
    /* This is the voltage at the ADC pin (not the real bus voltage yet) */
    int32_t v_mV = adc_to_mV(s_raw_vbus);
    return (v_mV - s_vbus_offset_mV);
}

void input_analog_calibrate_offsets(void)
{
    /* Use current cached samples (call when IMES=0 and VBUS reference known).
       For IMES: motor stopped / no current => offset = measured pin voltage.
       For VBUS: if you want, calibrate at Vbus=0 => offset = measured pin voltage. */
    s_imes_offset_mV = adc_to_mV(s_raw_imes);
    s_vbus_offset_mV = adc_to_mV(s_raw_vbus);
}

/* -------------------------------------------------------------------------- */
/* Shell commands                                                             */
/* -------------------------------------------------------------------------- */
static int cmd_measraw(h_shell_t* h_shell, int argc, char** argv)
{
    (void)argc; (void)argv;

    int size = snprintf(h_shell->print_buffer, SHELL_PRINT_BUFFER_SIZE,
                        "RAW: IMES=%u | VBUS=%u\r\n",
                        (unsigned int)s_raw_imes, (unsigned int)s_raw_vbus);
    h_shell->drv.transmit(h_shell->print_buffer, size);
    return 0;
}

static int cmd_imes(h_shell_t* h_shell, int argc, char** argv)
{
    (void)argc; (void)argv;

    int32_t i_mA = input_analog_get_imes_mA();
    int size = snprintf(h_shell->print_buffer, SHELL_PRINT_BUFFER_SIZE,
                        "IMES = %ld mA\r\n", (long)i_mA);
    h_shell->drv.transmit(h_shell->print_buffer, size);
    return 0;
}

static int cmd_vbus(h_shell_t* h_shell, int argc, char** argv)
{
    (void)argc; (void)argv;

    int32_t v_mV = input_analog_get_vbus_adc_mV();
    int size = snprintf(h_shell->print_buffer, SHELL_PRINT_BUFFER_SIZE,
                        "VBUS(ADC) = %ld mV\r\n", (long)v_mV);
    h_shell->drv.transmit(h_shell->print_buffer, size);
    return 0;
}

static int cmd_calib(h_shell_t* h_shell, int argc, char** argv)
{
    (void)argc; (void)argv;

    input_analog_calibrate_offsets();

    int size = snprintf(h_shell->print_buffer, SHELL_PRINT_BUFFER_SIZE,
                        "Offsets calibrated: IMES_off=%ld mV | VBUS_off=%ld mV\r\n",
                        (long)s_imes_offset_mV, (long)s_vbus_offset_mV);
    h_shell->drv.transmit(h_shell->print_buffer, size);
    return 0;
}

void input_analog_dma_start(void)
{
    // Start ADC1 in DMA circular mode (2 conversions: IN2 then IN8)
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)s_dma_buf, 2);
}

void input_analog_dma_stop(void)
{
    HAL_ADC_Stop_DMA(&hadc1);
}

void input_analog_dma_on_new_samples_isr(void)
{
    // Copy DMA buffer to cached raw values (fast, ISR-safe)
    s_raw_imes = s_dma_buf[0];
    s_raw_vbus = s_dma_buf[1];
}

