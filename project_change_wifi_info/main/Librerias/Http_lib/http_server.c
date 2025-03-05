/*
 * http_server.c
 * Modified by: Javier Leonardo Guzmán Olaya
 */

//MARK: INCLUDE
// Include the required header files for the libraries used in the project
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_timer.h"
#include "sys/param.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "esp_sntp.h"


// Include the header files for the libraries used in the project
#include "http_server.h"
#include "tasks_common.h"
#include "Librerias/Wifi_lib/wifi_app.h"
#include "Librerias/Adc_lib/ADC_NTC_POT.h"
#include "Librerias/Uart_lib/COMANDS_UART.h"
#include "cJSON.h"
#include "driver/gpio.h"
#include "Librerias/RGB_lib/LED_RGB.h"
#include "Librerias/Servo_lib/servo_lib.h"



#define RED_GPIO				(18)
#define GREEN_GPIO				(19)
#define BLUE_GPIO				(21)
#define LEDC_DUTY_RES 		   LEDC_TIMER_8_BIT	


// Tag_2 used for ESP serial console messages
static const char TAG_2[] = "http_server";

QueueHandle_t POT_V;
QueueHandle_t TEMP_ONOFF;
// Wifi connect status
static int g_wifi_connect_status = NONE;

// Firmware update status
static int g_fw_update_status = OTA_UPDATE_PENDING;

// HTTP server task handle
static httpd_handle_t http_server_handle = NULL;

// HTTP server monitor task handle
static TaskHandle_t task_http_server_monitor = NULL;

// Queue handle used to manipulate the main queue of events
static QueueHandle_t http_server_monitor_queue_handle;



/**
 * ESP32 timer configuration passed to esp_timer_create.
 */
const esp_timer_create_args_t fw_update_reset_args = {
		.callback = &http_server_fw_update_reset_callback,
		.arg = NULL,
		.dispatch_method = ESP_TIMER_TASK,
		.name = "fw_update_reset"
};
esp_timer_handle_t fw_update_reset;

// Embedded files: JQuery, index.html, app.css, app.js and favicon.ico files
extern const uint8_t jquery_3_3_1_min_js_start[]	asm("_binary_jquery_3_3_1_min_js_start");
extern const uint8_t jquery_3_3_1_min_js_end[]		asm("_binary_jquery_3_3_1_min_js_end");
extern const uint8_t index_html_start[]				asm("_binary_index_html_start");
extern const uint8_t index_html_end[]				asm("_binary_index_html_end");
extern const uint8_t app_css_start[]				asm("_binary_app_css_start");
extern const uint8_t app_css_end[]					asm("_binary_app_css_end");
extern const uint8_t app_js_start[]					asm("_binary_app_js_start");
extern const uint8_t app_js_end[]					asm("_binary_app_js_end");
extern const uint8_t favicon_ico_start[]			asm("_binary_favicon_ico_start");
extern const uint8_t favicon_ico_end[]				asm("_binary_favicon_ico_end");
static TaskHandle_t temp_task_handle = NULL;

uint8_t s_led_state = 0;

void toogle_led( void )
{
	s_led_state = !s_led_state;
	gpio_set_level(BLINK_GPIO, s_led_state);

}

// MARK: GET_TEMP
/**
 * @brief Task to obtain the NTC temperature
 * 
 * This function allows you to switch the temperature reading of the ntc off and on.
 */
void GET_TEMP_FUNCTION(){
	// Variables
    int status; // Variable to store the status of the UART
    char buffer[1025]; // Buffer to store the temperature
    int temp;// Variable to store the temperature
	// Loop to read the temperature
    while (1)
    {
		// Receive the temperature
       xQueueReceive(TEMP_Q,&temp, 100/ portTICK_PERIOD_MS);
	   // Receive the status of the UART
       xQueueReceive(TEMP_ONOFF,&status,100/ portTICK_PERIOD_MS);
	   // If the status is different from 0
        if (status!=NULL || status!=0){
            if (status == 1) {
				// Send the temperature
                while (status == 1) {
                    snprintf(buffer, sizeof(buffer), "\n Temperatura: %d °C\n", temp);
                    send_uart_response(buffer);
					// Receive the temperature
                    xQueueReceive(TEMP_Q,&temp, 100/ portTICK_PERIOD_MS);
					// Receive the status of the UART
                    xQueueReceive(TEMP_ONOFF, &status, pdMS_TO_TICKS(100)); 
                    vTaskDelay(pdMS_TO_TICKS(3000));
                }
				// Off send temperature
				send_uart_response("\n Temperatura apagada\n"); // Solo se envía una vez
            }   
        vTaskDelay(10);
        }
		// Eliminar la tarea después de que se apague la UART
		temp_task_handle = NULL;  // Resetear el handle de la tarea
		vTaskDelete(NULL);
    }
           
}


// MARK: NTC_READ
/**
 * @brief Task to read the NTC sensor
 * @param req HTTP request for which the uri needs to be handled
 * @return ESP_OK
 */
static esp_err_t http_server_get_ntc_sensor_readings_json_handler(httpd_req_t *req)
{
	// Log the request
    ESP_LOGI(TAG_2, "/NTCSensor.json requested");

	// Variables
    char NTCSensorJSON[100]; // Variable to store the JSON
    int temp[1][1]; // Variable to store the temperature

    // Read the value of the NTC sensor
    if (xQueueReceive(TEMP_Q, &temp, 100 / portTICK_PERIOD_MS) == pdTRUE) 
    {
        // Store the temperature in the JSON
        sprintf(NTCSensorJSON, "{\"temp\":\"%d\"}", temp[0][0]);

        // Set the type of the response
        httpd_resp_set_type(req, "application/json");

        // Send the response
        httpd_resp_send(req, NTCSensorJSON, strlen(NTCSensorJSON));
    }
    else
    {
		// If the sensor data could not be read, send an error message
        sprintf(NTCSensorJSON, "{\"error\":\"Failed to read sensor data\"}");
		// Set the type of the response
        httpd_resp_set_type(req, "application/json");

		// Send the response
        httpd_resp_send(req, NTCSensorJSON, strlen(NTCSensorJSON));
    }

    return ESP_OK;
}


// MARK: OFF_UART
/**
 * @brief Function to turn off the UART
 * @param req HTTP request for which the uri needs to be handled
 * @return ESP_OK
 */
static esp_err_t http_server_toogle_uart_off_handler(httpd_req_t *req)
{
	// Log the request
	ESP_LOGI(TAG_2, "/toogle_uart_off.json requested");
	
	// Variable to store the status
	int status;
    
    // Status to turn off the UART
    status = 2; 
	// Send the status to the queue
    xQueueSend(TEMP_ONOFF, &status, portMAX_DELAY);
	// Close the connection
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send(req, NULL, 0);
	// If the task is running, delete it
	if (temp_task_handle != NULL) {
		vTaskDelete(temp_task_handle);  // Delete the task
		temp_task_handle = NULL;        // Reset the handle
	}
	return ESP_OK;
}

// MARK: ON_UART
/**
 * @brief Function to turn on the UART
 * @param req HTTP request for which the uri needs to be handled
 * @return ESP_OK
 */
static esp_err_t http_server_toogle_uart_on_handler(httpd_req_t *req)
{
	// Log the request
	ESP_LOGI(TAG_2, "/toogle_uart_on.json requested");

	// Variable to store the status of the on send UART
	int status = 1;
	// Send the status to the queue
	xQueueSend(TEMP_ONOFF, &status, portMAX_DELAY);
	// Close the connection
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send(req, NULL, 0);
    // If the task is not running, create it
    if (temp_task_handle == NULL) 
	{
		// Create the task
        xTaskCreate(GET_TEMP_FUNCTION, "GET_TEMP_FUNCTION", 4096, NULL, configMAX_PRIORITIES - 3, NULL);
    }
	return ESP_OK;
}

// MARK: TOOGLE_LED
/**
 * @brief Function to toogle the LED
 * @param req HTTP request for which the uri needs to be handled
 * @return ESP_OK
 */
static esp_err_t http_server_toogle_led_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG_2, "/toogle_led.json requested");

	toogle_led();

	// Cerrar la conexion
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send(req, NULL, 0);
    
	return ESP_OK;
}



static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG_2, "Last error %s: 0x%x", message, error_code);
    }
}

//MARK:SERVER_UPDATE
/**
 * @brief Function to update the server
 * @param req HTTP request for which the uri needs to be handled
 * @return ESP_OK
 */
static void http_server_fw_update_reset_timer(void)
{
	if (g_fw_update_status == OTA_UPDATE_SUCCESSFUL)
	{
		ESP_LOGI(TAG_2, "http_server_fw_update_reset_timer: FW updated successful starting FW update reset timer");

		// Give the web page a chance to receive an acknowledge back and initialize the timer
		ESP_ERROR_CHECK(esp_timer_create(&fw_update_reset_args, &fw_update_reset));
		ESP_ERROR_CHECK(esp_timer_start_once(fw_update_reset, 8000000));
	}
	else
	{
		ESP_LOGI(TAG_2, "http_server_fw_update_reset_timer: FW update unsuccessful");
	}
}

// MARK: CALLBACK
/**
 * HTTP server monitor task used to track events of the HTTP server
 * @param pvParameters parameter which can be passed to the task.
 */
static void http_server_monitor(void *parameter)
{
	http_server_queue_message_t msg;

	for (;;)
	{
		if (xQueueReceive(http_server_monitor_queue_handle, &msg, portMAX_DELAY))
		{
			switch (msg.msgID)
			{
				case HTTP_MSG_WIFI_CONNECT_INIT:
					ESP_LOGI(TAG_2, "HTTP_MSG_WIFI_CONNECT_INIT");
					
					g_wifi_connect_status = HTTP_WIFI_STATUS_CONNECTING;

					break;

				case HTTP_MSG_WIFI_CONNECT_SUCCESS:
					ESP_LOGI(TAG_2, "HTTP_MSG_WIFI_CONNECT_SUCCESS");

					g_wifi_connect_status = HTTP_WIFI_STATUS_CONNECT_SUCCESS;

					break;

				case HTTP_MSG_WIFI_CONNECT_FAIL:
					ESP_LOGI(TAG_2, "HTTP_MSG_WIFI_CONNECT_FAIL");

					g_wifi_connect_status = HTTP_WIFI_STATUS_CONNECT_FAILED;

					break;

				case HTTP_MSG_OTA_UPDATE_SUCCESSFUL:
					ESP_LOGI(TAG_2, "HTTP_MSG_OTA_UPDATE_SUCCESSFUL");
					g_fw_update_status = OTA_UPDATE_SUCCESSFUL;
					http_server_fw_update_reset_timer();

					break;

				case HTTP_MSG_OTA_UPDATE_FAILED:
					ESP_LOGI(TAG_2, "HTTP_MSG_OTA_UPDATE_FAILED");
					g_fw_update_status = OTA_UPDATE_FAILED;

					break;

				default:
					break;
			}
		}
	}
}


// MARK: JQUERY_WEB
/**
 * Jquery get handler is requested when accessing the web page.
 * @param req HTTP request for which the uri needs to be handled.
 * @return ESP_OK
 */
static esp_err_t http_server_jquery_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG_2, "Jquery requested");

	httpd_resp_set_type(req, "application/javascript");
	httpd_resp_send(req, (const char *)jquery_3_3_1_min_js_start, jquery_3_3_1_min_js_end - jquery_3_3_1_min_js_start);

	return ESP_OK;
}

// MARK: INDEX_HTML
/**
 * Sends the index.html page.
 * @param req HTTP request for which the uri needs to be handled.
 * @return ESP_OK
 */
static esp_err_t http_server_index_html_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG_2, "index.html requested");

	httpd_resp_set_type(req, "text/html");
	httpd_resp_send(req, (const char *)index_html_start, index_html_end - index_html_start);

	return ESP_OK;
}


// MARK: APP_CSS
/**
 * app.css get handler is requested when accessing the web page.
 * @param req HTTP request for which the uri needs to be handled.
 * @return ESP_OK
 */
static esp_err_t http_server_app_css_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG_2, "app.css requested");

	httpd_resp_set_type(req, "text/css");
	httpd_resp_send(req, (const char *)app_css_start, app_css_end - app_css_start);

	return ESP_OK;
}


// MARK:APP_JS
/**
 * app.js get handler is requested when accessing the web page.
 * @param req HTTP request for which the uri needs to be handled.
 * @return ESP_OK
 */
static esp_err_t http_server_app_js_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG_2, "app.js requested");

	httpd_resp_set_type(req, "application/javascript");
	httpd_resp_send(req, (const char *)app_js_start, app_js_end - app_js_start);

	return ESP_OK;
}

// MARK: FAVICON_ICO
/**
 * Sends the .ico (icon) file when accessing the web page.
 * @param req HTTP request for which the uri needs to be handled.
 * @return ESP_OK
 */
static esp_err_t http_server_favicon_ico_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG_2, "favicon.ico requested");

	httpd_resp_set_type(req, "image/x-icon");
	httpd_resp_send(req, (const char *)favicon_ico_start, favicon_ico_end - favicon_ico_start);

	return ESP_OK;
}

// MARK: OTA_UPDATE
/**
 * Receives the .bin file fia the web page and handles the firmware update
 * @param req HTTP request for which the uri needs to be handled.
 * @return ESP_OK, otherwise ESP_FAIL if timeout occurs and the update cannot be started.
 */
esp_err_t http_server_OTA_update_handler(httpd_req_t *req)
{
	esp_ota_handle_t ota_handle;

	char ota_buff[1024];
	int content_length = req->content_len;
	int content_received = 0;
	int recv_len;
	bool is_req_body_started = false;
	bool flash_successful = false;

	const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);

	do
	{
		// Read the data for the request
		if ((recv_len = httpd_req_recv(req, ota_buff, MIN(content_length, sizeof(ota_buff)))) < 0)
		{
			// Check if timeout occurred
			if (recv_len == HTTPD_SOCK_ERR_TIMEOUT)
			{
				ESP_LOGI(TAG_2, "http_server_OTA_update_handler: Socket Timeout");
				continue; ///> Retry receiving if timeout occurred
			}
			ESP_LOGI(TAG_2, "http_server_OTA_update_handler: OTA other Error %d", recv_len);
			return ESP_FAIL;
		}
		printf("http_server_OTA_update_handler: OTA RX: %d of %d\r", content_received, content_length);

		// Is this the first data we are receiving
		// If so, it will have the information in the header that we need.
		if (!is_req_body_started)
		{
			is_req_body_started = true;

			// Get the location of the .bin file content (remove the web form data)
			char *body_start_p = strstr(ota_buff, "\r\n\r\n") + 4;
			int body_part_len = recv_len - (body_start_p - ota_buff);

			printf("http_server_OTA_update_handler: OTA file size: %d\r\n", content_length);

			esp_err_t err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle);
			if (err != ESP_OK)
			{
				printf("http_server_OTA_update_handler: Error with OTA begin, cancelling OTA\r\n");
				return ESP_FAIL;
			}
			else
			{
				printf("http_server_OTA_update_handler: Writing to partition subtype %d at offset 0x%lx\r\n", update_partition->subtype, update_partition->address);
			}

			// Write this first part of the data
			esp_ota_write(ota_handle, body_start_p, body_part_len);
			content_received += body_part_len;
		}
		else
		{
			// Write OTA data
			esp_ota_write(ota_handle, ota_buff, recv_len);
			content_received += recv_len;
		}

	} while (recv_len > 0 && content_received < content_length);

	if (esp_ota_end(ota_handle) == ESP_OK)
	{
		// Lets update the partition
		if (esp_ota_set_boot_partition(update_partition) == ESP_OK)
		{
			const esp_partition_t *boot_partition = esp_ota_get_boot_partition();
			ESP_LOGI(TAG_2, "http_server_OTA_update_handler: Next boot partition subtype %d at offset 0x%lx", boot_partition->subtype, boot_partition->address);
			flash_successful = true;
		}
		else
		{
			ESP_LOGI(TAG_2, "http_server_OTA_update_handler: FLASHED ERROR!!!");
		}
	}
	else
	{
		ESP_LOGI(TAG_2, "http_server_OTA_update_handler: esp_ota_end ERROR!!!");
	}

	// We won't update the global variables throughout the file, so send the message about the status
	if (flash_successful) { http_server_monitor_send_message(HTTP_MSG_OTA_UPDATE_SUCCESSFUL); } else { http_server_monitor_send_message(HTTP_MSG_OTA_UPDATE_FAILED); }

	return ESP_OK;
}


// MARK: OTA_STATUS
/**
 * OTA status handler responds with the firmware update status after the OTA update is started
 * and responds with the compile time/date when the page is first requested
 * @param req HTTP request for which the uri needs to be handled
 * @return ESP_OK
 */
esp_err_t http_server_OTA_status_handler(httpd_req_t *req)
{
	char otaJSON[100];

	ESP_LOGI(TAG_2, "OTAstatus requested");

	sprintf(otaJSON, "{\"ota_update_status\":%d,\"compile_time\":\"%s\",\"compile_date\":\"%s\"}", g_fw_update_status, __TIME__, __DATE__);

	httpd_resp_set_type(req, "application/json");
	httpd_resp_send(req, otaJSON, strlen(otaJSON));

	return ESP_OK;
}



// MARK: WIFI_CONNECT
/**
 * wifiConnect.json handler is invoked after the connect button is pressed
 * and handles receiving the SSID and password entered by the user
 * @param req HTTP request for which the uri needs to be handled.
 * @return ESP_OK
 */
static esp_err_t http_server_wifi_connect_json_handler(httpd_req_t *req)
{
    size_t header_len;
    char* header_value = NULL;
    char* ssid_str = NULL;
    char* pass_str = NULL;
    int content_length;
    char* data_buffer = NULL;
    cJSON* root = NULL;

    ESP_LOGI(TAG_2, "/wifiConnect.json requested");

    // Get the "Content-Length" header
    header_len = httpd_req_get_hdr_value_len(req, "Content-Length");
    if (header_len <= 0) {
        httpd_resp_send_err(req, HTTPD_411_LENGTH_REQUIRED, "Content-Length header is missing or invalid");
        ESP_LOGE(TAG_2, "Content-Length header is missing or invalid");
        return ESP_FAIL;
    }

    // Allocate memory for the header value
    header_value = (char*)malloc(header_len + 1);
    if (!header_value) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Memory allocation failed");
        ESP_LOGE(TAG_2, "Failed to allocate memory for header value");
        return ESP_FAIL;
    }

    // Get the Content-Length header value
    if (httpd_req_get_hdr_value_str(req, "Content-Length", header_value, header_len + 1) != ESP_OK) {
        free(header_value);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Failed to get Content-Length header value");
        ESP_LOGE(TAG_2, "Failed to get Content-Length header value");
        return ESP_FAIL;
    }

    // Convert the Content-Length header value to an integer
    content_length = atoi(header_value);
    free(header_value);

    if (content_length <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid Content-Length value");
        ESP_LOGE(TAG_2, "Invalid Content-Length value");
        return ESP_FAIL;
    }

    // Allocate memory for the data buffer
    data_buffer = (char*)malloc(content_length + 1);
    if (!data_buffer) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Memory allocation failed");
        ESP_LOGE(TAG_2, "Failed to allocate memory for data buffer");
        return ESP_FAIL;
    }

    // Read the request body
    if (httpd_req_recv(req, data_buffer, content_length) <= 0) {
        free(data_buffer);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive request body");
        ESP_LOGE(TAG_2, "Failed to receive request body");
        return ESP_FAIL;
    }

    // Null-terminate the data buffer
    data_buffer[content_length] = '\0';

    // Parse the JSON data
    root = cJSON_Parse(data_buffer);
    free(data_buffer);

    if (root == NULL) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON data");
        ESP_LOGE(TAG_2, "Invalid JSON data");
        return ESP_FAIL;
    }

    // Extract SSID and password from JSON
    cJSON* ssid_json = cJSON_GetObjectItem(root, "selectedSSID");
    cJSON* pwd_json = cJSON_GetObjectItem(root, "pwd");

    if (ssid_json == NULL || pwd_json == NULL || !cJSON_IsString(ssid_json) || !cJSON_IsString(pwd_json)) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing or invalid JSON data fields");
        ESP_LOGE(TAG_2, "Missing or invalid JSON data fields");
        return ESP_FAIL;
    }

    // Duplicate SSID and password strings
    ssid_str = strdup(ssid_json->valuestring);
    pass_str = strdup(pwd_json->valuestring);
    cJSON_Delete(root);

    if (!ssid_str || !pass_str) {
        free(ssid_str);
        free(pass_str);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Memory allocation failed");
        ESP_LOGE(TAG_2, "Failed to allocate memory for SSID or password");
        return ESP_FAIL;
    }

    // Validate SSID and password length
    if (strlen(ssid_str) > 32 || strlen(pass_str) > 64) {
        free(ssid_str);
        free(pass_str);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "SSID or password is too long");
        ESP_LOGE(TAG_2, "SSID or password is too long");
        return ESP_FAIL;
    }

    // Update WiFi configuration
    wifi_config_t* wifi_config = wifi_app_get_wifi_config();
    memset(wifi_config, 0x00, sizeof(wifi_config_t));
    strncpy((char*)wifi_config->sta.ssid, ssid_str, sizeof(wifi_config->sta.ssid) - 1);
    wifi_config->sta.ssid[sizeof(wifi_config->sta.ssid) - 1] = '\0';
    strncpy((char*)wifi_config->sta.password, pass_str, sizeof(wifi_config->sta.password) - 1);
    wifi_config->sta.password[sizeof(wifi_config->sta.password) - 1] = '\0';

    // Save credentials and reconnect
    save_wifi_credentials(ssid_str, pass_str);
    esp_wifi_disconnect();
    connect_to_wifi();

	// Send success response
    char* response_json = "{\"status\": \"success\", \"message\": \"Connected to WiFi\"}";
	httpd_resp_set_type(req, "application/json");
	httpd_resp_send(req, response_json, HTTPD_RESP_USE_STRLEN);

    // Free allocated memory
    free(ssid_str);
    free(pass_str);



    return ESP_OK;
}


// MARK: WIFI_STATUS
/**
 * wifiConnectStatus handler updates the connection status for the web page.
 * @param req HTTP request for which the uri needs to be handled.
 * @return ESP_OK
 */
static esp_err_t http_server_wifi_connect_status_json_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG_2, "/wifiConnectStatus requested");

	char statusJSON[100];

	sprintf(statusJSON, "{\"wifi_connect_status\":%d}", g_wifi_connect_status);

	httpd_resp_set_type(req, "application/json");
	httpd_resp_send(req, statusJSON, strlen(statusJSON));

	return ESP_OK;
}

//MARK: WIFI_FORGET
/**
 * @brief Function to forget the wifi credentials
 * This function is called when the user wants to forget the wifi credentials
 * @param req HTTP request for which the uri needs to be handled
 * @return ESP_OK
 */
static esp_err_t http_server_forget_wifi_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG_2, "/forget_wifi requested");
	char ssid[32];
    char password[64];
    // Cargar las credenciales desde NVS
    load_wifi_credentials(ssid, password);
	erase_wifi_credentials();
     // Crear la respuesta con el SSID eliminado
    char response[64];
    snprintf(response, sizeof(response), "Se eliminó la red: %s", ssid);

    // Enviar la respuesta al cliente
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}


//MARK: OPEN_HANDLER


/**
 * @brief Function to open the window manualy
 * @param req HTTP request for which the uri needs to be handled
 * @return ESP_OK
 */
static esp_err_t  http_server_open_window_handler(httpd_req_t *req){
	ESP_LOGI(TAG_2, "/open_window requested");
	servo_set_state(SERVO_OPEN);

	
	httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}
//MARK: CLOSE_HANDLER
/**
 * @brief Function to close the window manualy
 * @param req HTTP request for which the uri needs to be handled
 * @return ESP_OK
 */
static esp_err_t  http_server_close_window_handler(httpd_req_t *req){
	ESP_LOGI(TAG_2, "/close_window requested");
	servo_set_state(SERVO_CLOSED);


	httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}


//MARK: VIEW_WIFI
/**
 * @brief Function to view the saved wifi credentials
 * @param req HTTP request for which the uri needs to be handled
 * @return ESP_OK
 */
static esp_err_t http_server_get_saved_wifi_handler(httpd_req_t *req) {
    char ssid[32];
    char password[64];

    // Cargar las credenciales desde NVS
    load_wifi_credentials(ssid, password);

    // Crear objeto JSON con las credenciales
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "ssid", ssid);
    cJSON_AddStringToObject(root, "password", password); // Si no quieres mostrar la contraseña, omite esta línea

    // Convertir JSON a string
    char *json_str = cJSON_Print(root);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, strlen(json_str));

    // Liberar memoria
    free(json_str);
    cJSON_Delete(root);

    return ESP_OK;
}


// MARK: TIME
/**
 * @brief Function to get the current time
 * @param req HTTP request for which the uri needs to be handled
 * @return ESP_OK
 */
static esp_err_t http_server_get_time_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG_2, "/get_time.json requested");
	// init_obtain_time();
	

    char json_response[100];
    time_t now;
    struct tm timeinfo={0};

    time(&now);
    localtime_r(&now, &timeinfo);

    // Formatear la respuesta JSON con la hora
    strftime(json_response, sizeof(json_response), "{\"time\":\"%Y-%m-%d %H:%M:%S\"}", &timeinfo);


    // Enviar la respuesta JSON
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_response, strlen(json_response));

    return ESP_OK;
}

//MARK: READ_REG
/**
 * @brief Function to read the register
 * @param req HTTP request for which the uri needs to be handled
 * @return ESP_OK
 */
static esp_err_t http_server_read_register_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG_2, "/read_regs.json requested");

    // Crear un objeto JSON
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        ESP_LOGE(TAG_2, "Failed to create JSON object");
        return ESP_FAIL;
    }

    char register_information_read[12]; // Buffer para leer registros
    register_information_read[11] = '\0'; // Asegurar null terminator

    // Leer los 10 registros
    for (int i = 1; i <= 10; i++) {
        if (read_reg_data(register_information_read, i) != ESP_OK) {
            memset(register_information_read, '0', 6);
            register_information_read[6] = '\0'; // Asegurar que sea cadena válida
        }

        // Crear clave JSON (ejemplo: "reg1", "reg2", ...)
        char key[6];
        snprintf(key, sizeof(key), "reg%d", i);

        // Agregar al JSON
        cJSON_AddStringToObject(root, key, register_information_read);
    }

    // Convertir JSON a string
    char *json_response = cJSON_Print(root);
    cJSON_Delete(root); // Liberar memoria del JSON

    if (!json_response) {
        ESP_LOGE(TAG_2, "Failed to create JSON string");
        return ESP_FAIL;
    }

    // Enviar respuesta HTTP con JSON
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_response, strlen(json_response));

    free(json_response); // Liberar memoria

    return ESP_OK;
}

//MARK: CHANGE_REG
/**
 * regchange.json handler is invoked after the "enviar registro" button is pressed
 * and handles receiving the data entered by the user
 * @param req HTTP request for which the uri needs to be handled.
 * @return ESP_OK
 */
static esp_err_t http_server_register_change_handler(httpd_req_t *req)
{
    size_t header_len;
    char* header_value;
    char* hour_str = NULL;
	char* reg_str = NULL;
    char* min_str = NULL;
	char* days = NULL;
    int content_length;

    ESP_LOGI(TAG_2, "/regchange.json requested");

    // Get the "Content-Length" header to determine the length of the request body
    header_len = httpd_req_get_hdr_value_len(req, "Content-Length");
    if (header_len <= 0) {
        // Content-Length header not found or invalid
        //httpd_resp_send_err(req, HTTP_STATUS_411_LENGTH_REQUIRED, "Content-Length header is missing or invalid");
        ESP_LOGI(TAG_2, "Content-Length header is missing or invalid");
        return ESP_FAIL;
    }

    // Allocate memory to store the header value
    header_value = (char*)malloc(header_len + 1);
    if (httpd_req_get_hdr_value_str(req, "Content-Length", header_value, header_len + 1) != ESP_OK) {
        // Failed to get Content-Length header value
        free(header_value);
        //httpd_resp_send_err(req, HTTP_STATUS_BAD_REQUEST, "Failed to get Content-Length header value");
        ESP_LOGI(TAG_2, "Failed to get Content-Length header value");
        return ESP_FAIL;
    }

    // Convert the Content-Length header value to an integer
    content_length = atoi(header_value);
    free(header_value);

    if (content_length <= 0) {
        // Content length is not a valid positive integer
        //httpd_resp_send_err(req, HTTP_STATUS_BAD_REQUEST, "Invalid Content-Length value");
        ESP_LOGI(TAG_2, "Invalid Content-Length value");
        return ESP_FAIL;
    }

    // Allocate memory for the data buffer based on the content length
    char* data_buffer = (char*)malloc(content_length + 1);

    // Read the request body into the data buffer
    if (httpd_req_recv(req, data_buffer, content_length) <= 0) {
        // Handle error while receiving data
        free(data_buffer);
        //httpd_resp_send_err(req, HTTP_STATUS_INTERNAL_SERVER_ERROR, "Failed to receive request body");
        ESP_LOGI(TAG_2, "Failed to receive request body");
        return ESP_FAIL;
    }

    // Null-terminate the data buffer to treat it as a string
    data_buffer[content_length] = '\0';

    // Parse the received JSON data
    cJSON* root = cJSON_Parse(data_buffer);
    free(data_buffer);

    if (root == NULL) {
        // JSON parsing error
        //httpd_resp_send_err(req, HTTP_STATUS_BAD_REQUEST, "Invalid JSON data");
        ESP_LOGI(TAG_2, "Invalid JSON data");
        return ESP_FAIL;
    }

	cJSON* reg_number_json = cJSON_GetObjectItem(root, "selectedNumber");
    cJSON* hour_json = cJSON_GetObjectItem(root, "hours");
    cJSON* min_json = cJSON_GetObjectItem(root, "minutes");
	cJSON* selectedDays_json = cJSON_GetObjectItem(root, "selectedDays");
	
    if (hour_json == NULL || min_json == NULL || selectedDays_json == NULL|| !cJSON_IsString(hour_json) || !cJSON_IsString(min_json) || !cJSON_IsArray(selectedDays_json)) {
        cJSON_Delete(root);
        // Missing or invalid JSON fields
        //httpd_resp_send_err(req, HTTP_STATUS_BAD_REQUEST, "Missing or invalid JSON data fields");
        ESP_LOGI(TAG_2, "Missing or invalid JSON data fields");
        return ESP_FAIL;
    }

    // Extract SSID and password from JSON
	reg_str = strdup(reg_number_json->valuestring);
    hour_str = strdup(hour_json->valuestring);
    min_str = strdup(min_json->valuestring);

    

	ESP_LOGI(TAG_2, "Received reg: %s", reg_str);
    ESP_LOGI(TAG_2, "Received hour: %s", hour_str);
    ESP_LOGI(TAG_2, "Received min: %s", min_str);
 
    
	char str_to_save[12];
	//str_to_save[11]=0x00;
	memset(str_to_save, 0x00, 12);
	
	if (cJSON_IsArray(selectedDays_json)) {
    cJSON* day_item;

    // Iterate over each element in the array
	
	
	//strcat(str_to_save, reg_str);
	strcat(str_to_save, hour_str);
	strcat(str_to_save, min_str);
    cJSON_ArrayForEach(day_item, selectedDays_json) {
        // Check if the array element is a string
        if (cJSON_IsString(day_item)) {
            const char* day_str = day_item->valuestring;
			strcat(str_to_save, day_str);
            // Perform actions with the day_str
            printf("Selected Day: %s\n", day_str);
        }
    }
	
	} else {
		printf("SelectedDays is not an array\n");
	}
	printf("%s\n", str_to_save);
	save_reg_data(atoi(reg_str), &str_to_save);
	update_register(atoi(reg_str));

	// Cerrar la conexion
	free(reg_str);
	free(hour_str);
    free(min_str);
	cJSON_Delete(root);
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send(req, NULL, 0);
	

    return ESP_OK;
}

//MARK: GET_REG
/**
 * @brief Function to read the register
 * regread.json handler is invoked after the "leer registro" button is pressed
 * and handles receiving the data entered by the user
 * @param req HTTP request for which the uri needs to be handled.
 * @return ESP_OK
 */
static esp_err_t http_server_get_register_json_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG_2, "/read_regs.json requested");

	char read_regs[255];
	char register_information_read_1[12];
	register_information_read_1[11] = 0x00;
	if (read_reg_data( &register_information_read_1[0], 1 ) != ESP_OK ){
		memset(&register_information_read_1[0], "9", 6);
		sprintf(read_regs, "{\"reg1\":\"%c\"}", register_information_read_1[0]);
	}

	char register_information_read_2[12];
	register_information_read_2[11] = 0x00;
	if (read_reg_data( &register_information_read_2[0], 2 ) != ESP_OK ){
		memset(&register_information_read_2[0], "9", 6);
		sprintf(read_regs, "{\"reg2\":\"%c\"}", register_information_read_2[0]);
	}


	char register_information_read_3[12];
	register_information_read_3[11] = 0x00;
	if (read_reg_data( &register_information_read_3[0], 3 ) != ESP_OK ){
		memset(&register_information_read_3[0], "9", 6);
		sprintf(read_regs, "{\"reg3\":\"%c\"}", register_information_read_3[0]);
	}


	char register_information_read_4[12];
	register_information_read_4[11] = 0x00;
	if (read_reg_data( &register_information_read_4[0], 4 ) != ESP_OK ){
		memset(&register_information_read_4[0], "9", 6);
		sprintf(read_regs, "{\"reg4\":\"%c\"}", register_information_read_4[0]);
	}

	char register_information_read_5[12];
	register_information_read_5[11] = 0x00;
	if (read_reg_data( &register_information_read_5[0], 5 ) != ESP_OK ){
		memset(&register_information_read_5[0], "9", 6);
		sprintf(read_regs, "{\"reg5\":\"%c\"}", register_information_read_5[0]);

	}


	char register_information_read_6[12];
	register_information_read_6[11] = 0x00;
	if (read_reg_data( &register_information_read_6[0], 6 ) != ESP_OK ){
		memset(&register_information_read_6[0], "9", 6);
		sprintf(read_regs, "{\"reg6\":\"%c\"}", register_information_read_6[0]);
	}

	char register_information_read_7[12];
	register_information_read_7[11] = 0x00;
	if (read_reg_data( &register_information_read_7[0], 7 ) != ESP_OK ){
		memset(&register_information_read_7[0], "9", 6);
		sprintf(read_regs, "{\"reg7\":\"%c\"}", register_information_read_7[0]);

	}


	char register_information_read_8[12];
	register_information_read_8[11] = 0x00;
	if (read_reg_data( &register_information_read_8[0], 8 ) != ESP_OK ){
		memset(&register_information_read_8[0], "9", 6);
		sprintf(read_regs, "{\"reg8\":\"%c\"}", register_information_read_8[0]);

	}


	char register_information_read_9[12];
	register_information_read_9[11] = 0x00;
	if (read_reg_data( &register_information_read_9[0], 9 ) != ESP_OK ){
		memset(&register_information_read_9[0], "9", 6);
		sprintf(read_regs, "{\"reg9\":\"%c\"}", register_information_read_9[0]);
		// Set the type of the response
	}

	char register_information_read_10[12];
	register_information_read_10[11] = 0x00;
	if (read_reg_data( &register_information_read_10[0], 10 ) != ESP_OK ){
		memset(&register_information_read_10[0], "9", 6);
		sprintf(read_regs, "{\"reg10\":\"%c\"}", register_information_read_10[0]);

	}
	// Set the type of the response
	httpd_resp_set_type(req, "application/json");
	// Send the response
	httpd_resp_send(req,read_regs, strlen(read_regs));

	return ESP_OK;
}


// MARK: ERASE_REG
/**
 * @brief Function to erase the register
 * regerase.json handler is invoked after the "eliminar registro" button is pressed
 * and handles receiving the data entered by the user
 * @param req HTTP request for which the uri needs to be handled.
 * @return ESP_OK
 */
static esp_err_t http_server_register_erase_handler(httpd_req_t *req)
{
    size_t header_len;
    char* header_value;
    char* hour_str = NULL;
	char* reg_str = NULL;
    char* min_str = NULL;
	char* days = NULL;
    int content_length;
	

    ESP_LOGI(TAG_2, "/regerase.json requested");

    // Get the "Content-Length" header to determine the length of the request body
    header_len = httpd_req_get_hdr_value_len(req, "Content-Length");
    if (header_len <= 0) {
        // Content-Length header not found or invalid
        //httpd_resp_send_err(req, HTTP_STATUS_411_LENGTH_REQUIRED, "Content-Length header is missing or invalid");
        ESP_LOGI(TAG_2, "Content-Length header is missing or invalid");
        return ESP_FAIL;
    }

    // Allocate memory to store the header value
    header_value = (char*)malloc(header_len + 1);
    if (httpd_req_get_hdr_value_str(req, "Content-Length", header_value, header_len + 1) != ESP_OK) {
        // Failed to get Content-Length header value
        free(header_value);
        //httpd_resp_send_err(req, HTTP_STATUS_BAD_REQUEST, "Failed to get Content-Length header value");
        ESP_LOGI(TAG_2, "Failed to get Content-Length header value");
        return ESP_FAIL;
    }

    // Convert the Content-Length header value to an integer
    content_length = atoi(header_value);
    free(header_value);

    if (content_length <= 0) {
        // Content length is not a valid positive integer
        //httpd_resp_send_err(req, HTTP_STATUS_BAD_REQUEST, "Invalid Content-Length value");
        ESP_LOGI(TAG_2, "Invalid Content-Length value");
        return ESP_FAIL;
    }

    // Allocate memory for the data buffer based on the content length
    char* data_buffer = (char*)malloc(content_length + 1);

    // Read the request body into the data buffer
    if (httpd_req_recv(req, data_buffer, content_length) <= 0) {
        // Handle error while receiving data
        free(data_buffer);
        //httpd_resp_send_err(req, HTTP_STATUS_INTERNAL_SERVER_ERROR, "Failed to receive request body");
        ESP_LOGI(TAG_2, "Failed to receive request body");
        return ESP_FAIL;
    }

    // Null-terminate the data buffer to treat it as a string
    data_buffer[content_length] = '\0';

    // Parse the received JSON data
    cJSON* root = cJSON_Parse(data_buffer);
    free(data_buffer);

    if (root == NULL) {
        // JSON parsing error
        ESP_LOGI(TAG_2, "Invalid JSON data");
        return ESP_FAIL;
    }

	cJSON* reg_number_json = cJSON_GetObjectItem(root, "selectedNumber");
	
    if (reg_number_json == NULL || !cJSON_IsString(reg_number_json) ) {
        cJSON_Delete(root);
        ESP_LOGI(TAG_2, "Missing or invalid JSON data fields");
        return ESP_FAIL;
    }


	reg_str = strdup(reg_number_json->valuestring);
	erase_reg_data(atoi(reg_str));

    

	ESP_LOGI(TAG_2, "received reg: %s", reg_str);

	

    return ESP_OK;
}

// MARK:SERVER_CONFIG
/**
 * Sets up the default httpd server configuration.
 * @return http server instance handle if successful, NULL otherwise.
 */
static httpd_handle_t http_server_configure(void)
{
	// Generate the default configuration
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	// Create the message queue
	http_server_monitor_queue_handle = xQueueCreate(3, sizeof(http_server_queue_message_t));

	// Create HTTP server monitor task
	xTaskCreatePinnedToCore(&http_server_monitor, "http_server_monitor", HTTP_SERVER_MONITOR_STACK_SIZE, NULL, HTTP_SERVER_MONITOR_PRIORITY, &task_http_server_monitor, HTTP_SERVER_MONITOR_CORE_ID);

	// The core that the HTTP server will run on
	config.core_id = HTTP_SERVER_TASK_CORE_ID;

	// Adjust the default priority to 1 less than the wifi application task
	config.task_priority = HTTP_SERVER_TASK_PRIORITY;

	// Bump up the stack size (default is 4096)
	config.stack_size = HTTP_SERVER_TASK_STACK_SIZE;

	// Increase uri handlers
	config.max_uri_handlers = 25;

	// Increase the timeout limits
	config.recv_wait_timeout = 10;
	config.send_wait_timeout = 10;

	ESP_LOGI(TAG_2,
			"http_server_configure: Starting server on port: '%d' with task priority: '%d'",
			config.server_port,
			config.task_priority);

	// Start the httpd server
	if (httpd_start(&http_server_handle, &config) == ESP_OK)
	{
		ESP_LOGI(TAG_2, "http_server_configure: Registering URI handlers");


		// register OTAstatus handler
		httpd_uri_t register_change = {
			.uri = "/regchange.json",
			.method = HTTP_POST,
			.handler = http_server_register_change_handler,
			.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &register_change);

		//
		httpd_uri_t read_range_uri = {
			.uri = "/readreg.json",
			.method = HTTP_POST,
			.handler = http_server_read_register_handler,
			.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &read_range_uri );

		// register read_register handler
		httpd_uri_t read_range_uri_get = {
			.uri = "/read_regs.json",
			.method = HTTP_GET,
			.handler = http_server_read_register_handler,
			.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &read_range_uri_get );

		// register erase handler
			httpd_uri_t register_erase = {
				.uri = "/regerase.json",
				.method = HTTP_POST,
				.handler = http_server_register_erase_handler,
				.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &register_erase);


		// register get_time handler
		httpd_uri_t get_time = {
			.uri = "/get_time.json",
			.method = HTTP_GET,
			.handler = http_server_get_time_handler,
			.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &get_time);

		// get_saved_wifi handler
		httpd_uri_t get_saved_wifi = {
			.uri = "/get_saved_wifi",
			.method = HTTP_GET,
			.handler = http_server_get_saved_wifi_handler,
			.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &get_saved_wifi);		

		// register query handler
		httpd_uri_t jquery_js = {
				.uri = "/jquery-3.3.1.min.js",
				.method = HTTP_GET,
				.handler = http_server_jquery_handler,
				.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &jquery_js);

		// register index.html handler
		httpd_uri_t index_html = {
				.uri = "/",
				.method = HTTP_GET,
				.handler = http_server_index_html_handler,
				.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &index_html);

		// register app.css handler
		httpd_uri_t app_css = {
				.uri = "/app.css",
				.method = HTTP_GET,
				.handler = http_server_app_css_handler,
				.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &app_css);

		// register app.js handler
		httpd_uri_t app_js = {
				.uri = "/app.js",
				.method = HTTP_GET,
				.handler = http_server_app_js_handler,
				.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &app_js);

		// register favicon.ico handler
		httpd_uri_t favicon_ico = {
				.uri = "/favicon.ico",
				.method = HTTP_GET,
				.handler = http_server_favicon_ico_handler,
				.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &favicon_ico);

		httpd_uri_t toogle_led = {
				.uri = "/toogle_led.json",
				.method = HTTP_POST,
				.handler = http_server_toogle_led_handler,
				.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &toogle_led );


		// register wifiConnect.json handler
		httpd_uri_t wifi_connect_json = {
				.uri = "/wifiConnect.json",
				.method = HTTP_POST,
				.handler = http_server_wifi_connect_json_handler,
				.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &wifi_connect_json);
		

		// register OTAupdate handler
		httpd_uri_t OTA_update = {
				.uri = "/OTAupdate",
				.method = HTTP_POST,
				.handler = http_server_OTA_update_handler,
				.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &OTA_update);

		// register OTAstatus handler
		httpd_uri_t OTA_status = {
				.uri = "/OTAstatus",
				.method = HTTP_POST,
				.handler = http_server_OTA_status_handler,
				.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &OTA_status);

		

		// register NTCSensor.json handler
		httpd_uri_t NTC_sensor_json = {
			.uri = "/NTCSensor.json",
			.method = HTTP_GET,
			.handler = http_server_get_ntc_sensor_readings_json_handler,
			.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &NTC_sensor_json);

		//turn off UART
		httpd_uri_t toogle_uart_off = {
			.uri = "/toogle_uart_off.json",
			.method = HTTP_POST,
			.handler = http_server_toogle_uart_off_handler,
			.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &toogle_uart_off);

		//turn on UART
		httpd_uri_t toogle_uart_on = {
			.uri = "/toogle_uart_on.json",
			.method = HTTP_POST,
			.handler = http_server_toogle_uart_on_handler,
			.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &toogle_uart_on);

		// register wifiConnectStatus.json handler
		httpd_uri_t wifi_connect_status_json = {
				.uri = "/wifiConnectStatus",
				.method = HTTP_POST,
				.handler = http_server_wifi_connect_status_json_handler,
				.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &wifi_connect_status_json);

		// register forget_wifi handler
		httpd_uri_t forget_wifi = {
				.uri = "/forget_wifi",
				.method = HTTP_POST,
				.handler = http_server_forget_wifi_handler,
				.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &forget_wifi);
		//open window manualy URI
		httpd_uri_t open_window = {
				.uri = "/OPEN_WINDOW.json",
				.method = HTTP_POST,
				.handler = http_server_open_window_handler,
				.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &open_window);
		//open window manualy URI
		httpd_uri_t close_window = {
				.uri = "/CLOSE_WINDOW.json",
				.method = HTTP_POST,
				.handler = http_server_close_window_handler,
				.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &close_window);

		


		return http_server_handle;
	}

	return NULL;
}

void http_server_start(void)
{
	if (http_server_handle == NULL)
	{
		http_server_handle = http_server_configure();
	}
}

void http_server_stop(void)
{
	if (http_server_handle)
	{
		httpd_stop(http_server_handle);
		ESP_LOGI(TAG_2, "http_server_stop: stopping HTTP server");
		http_server_handle = NULL;
	}
	if (task_http_server_monitor)
	{
		vTaskDelete(task_http_server_monitor);
		ESP_LOGI(TAG_2, "http_server_stop: stopping HTTP server monitor");
		task_http_server_monitor = NULL;
	}
}

BaseType_t http_server_monitor_send_message(http_server_message_e msgID)
{
	http_server_queue_message_t msg;
	msg.msgID = msgID;
	return xQueueSend(http_server_monitor_queue_handle, &msg, portMAX_DELAY);
}

void http_server_fw_update_reset_callback(void *arg)
{
	ESP_LOGI(TAG_2, "http_server_fw_update_reset_callback: Timer timed-out, restarting the device");
	esp_restart();
}