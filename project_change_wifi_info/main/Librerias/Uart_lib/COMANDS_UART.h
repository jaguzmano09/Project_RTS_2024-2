#ifndef COMANDS_UART_H
#define COMANDS_UART_H

// Include libraries
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include <string.h>
#include <stdlib.h>
#include "Librerias/Adc_lib/ADC_NTC_POT.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

// Define the UART configuration
#define ECHO_UART_PORT_NUM UART_NUM_0           // UART port number
#define ECHO_UART_BAUD_RATE 115200              // Baud rate
#define ECHO_TEST_TXD    (1)                    // Pin TX
#define ECHO_TEST_RXD    (3)                    // Pin RX
#define ECHO_TEST_RTS (UART_PIN_NO_CHANGE)      // Pin RTS
#define ECHO_TEST_CTS (UART_PIN_NO_CHANGE)      // Pin CTS
#define BUF_SIZE (1025)                         // buffer size

// Define extern queues for the limits of the RGB LED
extern QueueHandle_t Lim_min_red;
extern QueueHandle_t Lim_max_red;
extern QueueHandle_t Lim_min_green;
extern QueueHandle_t Lim_max_green;
extern QueueHandle_t Lim_min_blue;
extern QueueHandle_t Lim_max_blue;
extern QueueHandle_t TEMP_ONOFF;

// Define the function prototypes
/**
 * @brief Initializes the UART.
 */
void uart_init(void); 

/**
 * @brief UART task for handling communication.
 *
 * @param arg Task argument.
 */
void uart_task(void *arg);

/**
 * @brief Processes a received command.
 *
 * @param command The command to process.
 */
void process_command(char *command);

/**
 * @brief Converts a string to an array of characters.
 *
 * @param input The input string.
 * @param comm The output array of characters.
 */
void str_to_chars(const char *input, char comm[3][1025]);

/**
 * @brief Sends a response through the UART.
 *
 * @param message The message to send.
 */
void send_uart_response(const char *message);



#endif // COMANDS_UART_H