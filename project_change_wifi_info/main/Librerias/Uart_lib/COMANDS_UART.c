#include "COMANDS_UART.h"

// #include


// Defined queue temperature NTC
QueueHandle_t TEMP_Q;


/** 
 * @brief Initializes the UART with the specified configuration.
 */
void uart_init(void) {
    /* UART configuration parameters  */
    uart_config_t uart_config = {
        .baud_rate = ECHO_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    int intr_alloc_flags = 0;// Flags to set for UART interrupt allocation

// Set the flag for UART interrupt allocation in IRAM 
#if CONFIG_UART_ISR_IN_IRAM
    intr_alloc_flags = ESP_INTR_FLAG_IRAM;
#endif

    // Initialize UART driver
    ESP_ERROR_CHECK(uart_driver_install(ECHO_UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(ECHO_UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(ECHO_UART_PORT_NUM, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS));
}

/** 
 * @brief Sends a response via UART. 
 */
void send_uart_response(const char *message) {
    // Send the response through the UART port
    uart_write_bytes(ECHO_UART_PORT_NUM, message, strlen(message)); 
}

/** 
 * @brief Converts a string to an array of command strings. 
 */
void  str_to_chars(const char *input, char ***words) {
    char copy[1025]; // Make a copy of the original string
    strncpy(copy, input, sizeof(copy) - 1);
    copy[sizeof(copy) - 1] = '\0';

    const char delimitador[] = " ";
    char *token = strtok(copy, delimitador);
    
    int count = 0;
    *words = NULL;

    while (token != NULL) {
        *words = realloc(*words, (count + 1) * sizeof(char *));
        (*words)[count] = malloc(strlen(token) + 1);
        strcpy((*words)[count], token);
        count++;
        token = strtok(NULL, delimitador);
    }

}


/** 
 * @brief Processes a received command.
 */
void process_command(char *command) 
{
    // TEMP_ONOFF=(10,sizeof(int));
   if (command != NULL){

    char **words;

    str_to_chars(command, &words);
  
    
    if (strcmp( words[0] , "open") ==0 ) {
        //setear servo en 180°
        servo_set_state(SERVO_OPEN);
        free(words);
        
    } 
    else if (strcmp(words[0],"close")==0) {
        //setear servo en 0°
        servo_set_state(SERVO_CLOSED);
        free(words);
    }
    else if (strcmp(words[0],"Reg")==0) {
        int reg_num = words[1];
        char hour = words[2];
        char min = words[3];
        char day_s = words[4];
        char str_to_save[12];
        // strcat(str_to_save, hour);
        // strcat(str_to_save, min);
        // strcat(str_to_save, day_s);
        // save_reg_data(reg_num, &str_to_save);
        // update_register(reg_num);
        ESP_LOGI("Llegó","%s",words[4]);
        free(words);
    }

    else {
            send_uart_response("ERROR: Invalid command");
            free(words);
    }


   }
}


/** 
* @brief UART task for handling communication. 
*/
void uart_task(void *arg) {
    // Initialize the UART
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);

    while (1) {
        // Read data from the UART
        int len = uart_read_bytes(ECHO_UART_PORT_NUM, data, (BUF_SIZE - 1), 20 / portTICK_PERIOD_MS);

        // If data is received
        if (len > 0) {
            // Null-terminate the string
            data[len] = '\0';  
            // ESP_LOGI("UART Recv", "Recibido: %s", (char *) data);

            // Process the command
            process_command((char *) data);

        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    
}