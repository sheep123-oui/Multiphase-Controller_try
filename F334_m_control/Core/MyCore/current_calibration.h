#pragma once

/* Measured with the power stage disabled, relative to the same analog ground.
 * These calibrate capacitor-current channels and the average-feedback DAC only.
 * DAC channel-specific offset and signal gain still require a loaded check. */
#define CAP_REFERENCE_VOLTAGE 3.324f
#define CAP_CURRENT_A_ZERO_VOLTAGE 1.657f
#define CAP_CURRENT_B_ZERO_VOLTAGE 1.658f
#define CAP_AVG_FEEDBACK_ZERO_VOLTAGE 1.668f

/* INA240A1 (20 V/V), 1 milliohm shunts. Average-stage signal gain assumed 1. */
#define CAP_CURRENT_VOLTS_PER_AMP (20.0f * 0.001f)
#define CAP_ADC_VOLTS_PER_CODE (CAP_REFERENCE_VOLTAGE / 4095.0f)
#define CAP_CURRENT_AMPS_PER_VOLT (1.0f / CAP_CURRENT_VOLTS_PER_AMP)
#define CAP_DAC_CODES_PER_VOLT (4095.0f / CAP_REFERENCE_VOLTAGE)
