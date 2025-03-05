/**
 * Author: Javier Leonardo Guzmán Olaya
 */

#include "nvs_flash.h"
#include "Librerias/Adc_lib/ADC_NTC_POT.h"
#include "Librerias/Uart_lib/COMANDS_UART.h"
#include "Librerias/Wifi_lib/wifi_app.h"
#include "Librerias/Servo_lib/servo_lib.h"
#include "driver/gpio.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_sntp.h"


#define BLINK_GPIO				2
#define channel_ntc            ADC1_CHANNEL_6 // channel for the NTC
#define channel_pot            ADC1_CHANNEL_4 // channel for the potentiometer

QueueHandle_t Lim_min_red;
QueueHandle_t Lim_max_red;
QueueHandle_t Lim_min_green;
QueueHandle_t Lim_max_green;
QueueHandle_t Lim_min_blue;
QueueHandle_t Lim_max_blue;


static void configure_led(void)
{
    gpio_reset_pin(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
	
}

// 🔥 Callback cuando el ESP32 obtiene una IP
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        obtain_time();  // Solo se llama cuando hay internet
    }
}


void app_main(void)
{
    // Initialize NVS
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);
	

	// Configure the LED
	configure_led();
	servo_init();
	// Start Wifi
	wifi_app_start();
	init_obtain_time();
	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

	// Create the queues for the limits of the RGB LED
	Lim_min_red=xQueueCreate(10,sizeof(int));
	Lim_max_red=xQueueCreate(10,sizeof(int));
	Lim_min_green=xQueueCreate(10,sizeof(int));
	Lim_max_green=xQueueCreate(10,sizeof(int));
	Lim_min_blue=xQueueCreate(10,sizeof(int));
	Lim_max_blue=xQueueCreate(10,sizeof(int));
	// Create the queue for the temperature
	TEMP_Q= xQueueCreate(10, sizeof(int));
	TEMP_ONOFF= xQueueCreate(10, sizeof(int));
	// Create the queue for the potentiometer
	POT_V= xQueueCreate(10, sizeof(int));

	/*   Create the tasks         */ 
	xTaskCreatePinnedToCore(uart_task, "uart_task", 4096, NULL, 5, NULL,1);

	// Read NTC and POT values
	read_ntc_pot(channel_ntc, channel_pot);

	while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}