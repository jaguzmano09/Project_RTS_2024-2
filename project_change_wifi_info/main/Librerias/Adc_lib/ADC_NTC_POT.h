
#ifndef ADC_NTC_POT_H
#define ADC_NTC_POT_H

//include libraries
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "hal/adc_types.h"
#include "esp_adc/adc_oneshot.h" 
#include "math.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"


/*       ADC configuration       */ 
// ADC attenuation level
#define ADC1_ATTEN ADC_ATTEN_DB_12

#define ADC2_ATTEN ADC_ATTEN_DB_12



// Tag for the ADC
#define TAG "ADC_NTC_POT"
// Queue for the temperature
extern QueueHandle_t TEMP_Q;
// Queue for the potentiometer voltage
extern QueueHandle_t POT_V;

/**
 * @brief function to read the NTC temperature and potentiometer voltage
 * 
 * This function reads the NTC temperature and sends it to a queue.
 * It also reads the potentiometer voltage and sends it to a queue.
 * @param channel: ADC channel for the NTC
 * @param channel2: ADC channel for the potentiometer
 */

void read_ntc_pot(adc1_channel_t channel1, adc1_channel_t channel2);

/**
 * @brief initialize the ADC for the NTC and potentiometer
 * 
 * This function initializes the ADC for the NTC and potentiometer.
 * @param channel1: ADC channel for the NTC
 * @param channel2: ADC channel for the potentiometer
 */
void adc_ntc_pot_init(adc1_channel_t channel1, adc1_channel_t channel2);


#endif // ADC_LIB_H
