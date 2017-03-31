/*
 * Copyright (c) 2016, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the Intel Corporation nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL CORPORATION OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Grove Shield Electricity Sensor
 *
 * This application works with shield and Grove electricity sensor TA12-200.
 * The example assumes that the sensor is connected to A0 on Grove shield.
 *
 * More details can be found at:
 * http://www.seeedstudio.com/wiki/Grove_-_Electricity_Sensor
 *
 * This application uses a polling method to get a periodic measurement of the
 * current.
 * The number of measures is defined by the macro NUM_PRINTED_MEASURES and the
 * pace of them by the macro PRINT_INTERVAL_SEC.
 * This example uses single channel conversion.
 *
 * On the Intel(R) Quark(TM) SE this app uses the sensor subsystem.
 */

#include <string.h>
#include "qm_pinmux.h"
#include "qm_pin_functions.h"

#if (QUARK_SE)

#include "qm_common.h"
#include "qm_ss_adc.h"
#include "ss_clk.h"

#elif(QUARK_D2000)
#include "clk.h"
#include "qm_adc.h"
#endif

/* Main control constants: measures and prints every 2 seconds, ten times*/
#define NUM_PRINTED_MEASURES 10
#define PRINT_INTERVAL_SEC 2

/* Adc sample frequency control */
#define CLK_FRECUENCY 32000000
#define ADC_CLK_DIVIDER 100
#define ADC_SAMPLES_PERIOD 50
#define NUM_SAMPLES_PER_SEQ 16

#define NUM_CHANNELS 1

/* defines used for computing the ampere. */
#define VCC_VALUE (3.3)
#define DIVISOR_FOR_12_BIT_RESOLUTION (4096)
#define SAMPLING_RESISTANCE (800)
#define MAXIMUM_AC_VALUE_1000 (1414)
#define TRANSFORMATION_RATIO (2000)
#define MICROAMP 1000000
#define ROUND_100UA(uA) (((uA) + 50) / 100)

/* It calculates the number of sequences needed to fill the time requested */
/* by PRINT_INTERVAL_SEC. It is different depending on the SOC used */
#if (QUARK_SE)
#define NUM_SEQUENCES                                                          \
	(PRINT_INTERVAL_SEC * CLK_FRECUENCY / ADC_CLK_DIVIDER /                \
	 (ADC_SAMPLES_PERIOD - (QM_SS_ADC_RES_12_BITS + 3)) /                  \
	 NUM_SAMPLES_PER_SEQ)
#elif(QUARK_D2000)
#define NUM_SEQUENCES                                                          \
	(PRINT_INTERVAL_SEC * CLK_FRECUENCY / ADC_CLK_DIVIDER /                \
	 (ADC_SAMPLES_PERIOD + QM_ADC_RES_12_BITS + 2 + 9) /                   \
	 NUM_SAMPLES_PER_SEQ)
#endif

/* To store the float formatted strings */
#define MAX_FLOAT_STRING 16
/* Going to show measures in milliamp with 1 decimal place */
#define DECIMAL_PLACES 1

/* Stores the samples for every sequence */
static uint16_t samples[NUM_SAMPLES_PER_SEQ] = {0};

#if (QUARK_SE)
static int init()
{
	qm_ss_adc_config_t cfg;

	/* Enable the ADC and set the clock divisor. */
	ss_clk_adc_enable();
	ss_clk_adc_set_div(ADC_CLK_DIVIDER);

	qm_pmux_select(QM_PIN_ID_10, QM_PIN_10_FN_AIN_10);
	qm_pmux_input_en(QM_PIN_ID_10, true);

	/* Set the mode and calibrate. */
	qm_ss_adc_set_mode(QM_SS_ADC_0, QM_SS_ADC_MODE_NORM_CAL);
	qm_ss_adc_calibrate(QM_SS_ADC_0);

	/* Set up config. */
	cfg.window = ADC_SAMPLES_PERIOD;
	cfg.resolution = QM_SS_ADC_RES_12_BITS;
	qm_ss_adc_set_config(QM_SS_ADC_0, &cfg);

	return 0;
}
#elif(QUARK_D2000)
static int init()
{
	qm_adc_config_t cfg;
	/* Enable the ADC. */
	clk_periph_enable(CLK_PERIPH_CLK | CLK_PERIPH_ADC |
			  CLK_PERIPH_ADC_REGISTER);
	clk_adc_set_div(ADC_CLK_DIVIDER);

	qm_pmux_select(QM_PIN_ID_3, QM_PIN_3_FN_AIN_3);
	qm_pmux_input_en(QM_PIN_ID_3, true);

	/* Set the mode. */
	qm_adc_set_mode(QM_ADC_0, QM_ADC_MODE_NORM_CAL);
	qm_adc_calibrate(QM_ADC_0);
	/* Set up config. */
	cfg.window = ADC_SAMPLES_PERIOD;
	cfg.resolution = QM_ADC_RES_12_BITS;
	qm_adc_set_config(QM_ADC_0, &cfg);

	return 0;
}
#endif

/* Utility to get the string of a float number created by converting the micro
 * Ampere (number) into milli Ampere, with decimal_places after the point.
 */
static char *sprintf_fix_decimal(char *out_string, unsigned int number,
				 unsigned int decimal_places)
{
	char float_string[MAX_FLOAT_STRING] = "";
	unsigned int last_char = MAX_FLOAT_STRING - 2;
	unsigned int i;
	const int ASCII_0 = 48;

	if (decimal_places < 1) {
		decimal_places = 1;
	}

	float_string[MAX_FLOAT_STRING - 1] = '\0';

	for (i = 0; i < 1 || number != 0; i++) {
		if (i == decimal_places) {
			float_string[last_char - i] = '.';
			i++;
		}
		float_string[last_char - i] = ASCII_0 + number % 10;
		number /= 10;
	}

	strncpy(out_string, &(float_string[MAX_FLOAT_STRING - i - 1]), i);
	return out_string;
}

/* Launches a burst of NUM_SAMPLES_PER_SEQ conversions (samples) */
#if (QUARK_SE)
static void start_conversions()
{
	static qm_ss_adc_channel_t channels[] = {QM_SS_ADC_CH_10};
	static qm_ss_adc_xfer_t xfer = {.ch = channels,
					.ch_len = NUM_CHANNELS,
					.samples = samples,
					.samples_len = NUM_SAMPLES_PER_SEQ,
					.callback = NULL,
					.callback_data = NULL};

	if (qm_ss_adc_convert(QM_SS_ADC_0, &xfer, NULL)) {
		QM_PUTS("Error: qm_adc_convert failed");
		return;
	}
}

#elif(QUARK_D2000)
static void start_conversions()
{
	static qm_adc_channel_t channels[] = {QM_ADC_CH_3};
	static qm_adc_xfer_t xfer = {.ch = channels,
				     .ch_len = NUM_CHANNELS,
				     .samples = samples,
				     .samples_len = NUM_SAMPLES_PER_SEQ,
				     .callback = NULL,
				     .callback_data = NULL};

	if (qm_adc_convert(QM_ADC_0, &xfer, NULL)) {
		QM_PUTS("Error: qm_adc_convert failed");
		return;
	}
}
#endif

/* Runs batches (batches_qty) of sequences of conversions and gets the max
 * value (max_val) of them.
 */
static int get_max_sensor_value(int batches_qty, int *max_val)
{
	int i, j;

	for (i = 0; i < batches_qty; i++) {
		start_conversions();

		for (j = 0; j < NUM_SAMPLES_PER_SEQ; j++) {
			if (*max_val < samples[j]) {
				*max_val = samples[j];
			}
		}
	}
	return 0;
}

/* Calculates the current value in micro Ampere out from the sensor_value
 * given.
 */
static int get_current(int sensor_value)
{
	return sensor_value * (MICROAMP / DIVISOR_FOR_12_BIT_RESOLUTION) *
	       VCC_VALUE * TRANSFORMATION_RATIO / SAMPLING_RESISTANCE;
}

/* Calculate the effective value by dividing the maximum value by sqrt(2).
 * Returns micro ampere.
 */
static int get_eff_value(int microamp)
{
	return microamp * 1000 / MAXIMUM_AC_VALUE_1000;
}

int main(void)
{
	int error = 0;
	int measures_printed = 0;
	unsigned int max_current_uA = -1;
	unsigned int effective_current_uA = -1;
	char max_current_mA[MAX_FLOAT_STRING] = "-";
	char effective_current_mA[MAX_FLOAT_STRING] = "-";
	int max_sensor_value = 0;

	QM_PUTS("Starting: Electricity sensor");

	/* Init the ADC: */
	error = init();
	if (error) {
		QM_PRINTF("Init error %d\n", error);
		return 0;
	}
	QM_PRINTF("Init done\n");

	error = 0;
	/* Main loop which measures and prints NUM_PRINTED_MEASURES every
	 * approx. PRINT_INTERVAL_SEC:	 */
	while (!error && measures_printed < NUM_PRINTED_MEASURES) {
		measures_printed++;
		max_sensor_value = -1;
		error = get_max_sensor_value(NUM_SEQUENCES,
					     (int *)&max_sensor_value);
		max_current_uA = get_current(max_sensor_value);
		effective_current_uA = get_eff_value(max_current_uA);
		sprintf_fix_decimal(max_current_mA, ROUND_100UA(max_current_uA),
				    DECIMAL_PLACES);
		sprintf_fix_decimal(effective_current_mA,
				    ROUND_100UA(effective_current_uA),
				    DECIMAL_PLACES);
		QM_PRINTF(
		    "%d/%d: SensorMax= %d, I max = %s mA, I eff = %s mA\n",
		    measures_printed, NUM_PRINTED_MEASURES, max_sensor_value,
		    max_current_mA, effective_current_mA);
	}

	if (error) {
		QM_PRINTF("RUN ERROR %d\n", error);
	}

	QM_PUTS("Finished: Electricity sensor");

	return 0;
}
