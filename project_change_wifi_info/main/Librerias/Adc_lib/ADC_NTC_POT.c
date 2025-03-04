#include "ADC_NTC_POT.h"


// ADC characteristics
esp_adc_cal_characteristics_t *adc_chars;

/**
 * @brief Initialize the ADC to read the NTC and potentiometer
 * 
 * This function initializes the ADC to read the NTC and potentiometer.
 * @param channel1 ADC channel for the NTC
 * @param channel2 ADC channel for the potentiometer
 */
void adc_ntc_pot_init( adc1_channel_t channel1, adc1_channel_t channel2)
{
    // Configurar el ADC1
    adc1_config_width(ADC1_ATTEN);
    adc1_config_channel_atten(channel1, ADC1_ATTEN);
    adc1_config_channel_atten(channel1, ADC2_ATTEN);
    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    // 3️⃣ Calibrar ADC
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12, 3.3, adc_chars);
}

/** @brief Read the NTC temperature and potentiometer voltage
 * 
 * This function reads the NTC temperature and sends it to a queue.
 * It also reads the potentiometer voltage and sends it to a queue.
 *  
 */ 
void read_ntc_pot(adc1_channel_t channel1, adc1_channel_t channel2)
{
    adc_ntc_pot_init(channel1,channel2);
    int temperature=0;//Temperature value
    int Pot_volt=0;//Voltage value of the potentiometer
    int ntc_adc=0;//ADC value of the NTC
    int pot_adc=0;//ADC value of the potentiometer

    // Read the NTC temperature and voltage potentiometer
    for(;;){
        // Read adc value for ntc
        ntc_adc = adc1_get_raw(channel1);
        // Read adc value for potentiometer
        pot_adc = adc1_get_raw(channel2);

        /*     Convert adc value to voltage        */

        // Convert the adc value to voltage for the NTC
        uint32_t volt_ntc = esp_adc_cal_raw_to_voltage(ntc_adc, adc_chars);
        // Convert the adc value to voltage for the potentiometer
        uint32_t volt_pot = esp_adc_cal_raw_to_voltage(pot_adc, adc_chars);

        /*     Convert voltage to temperature        */
        temperature = (-0.033324*volt_ntc)+78.22;
        // Save the potentiometer voltage
        Pot_volt = volt_pot;
        // Send the temperature to the queue
        xQueueSend(TEMP_Q , &temperature , pdMS_TO_TICKS(0));
        // Send the potentiometer voltage to the queue
        xQueueSend(POT_V , &Pot_volt , pdMS_TO_TICKS(0));
        // Delay
        vTaskDelay(100);
    }

}


