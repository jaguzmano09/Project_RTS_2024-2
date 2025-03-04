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
void str_to_chars(const char *input, char comm[3][1025]) {
    // Copy the original string
    char copy[1025]; 
    strncpy(copy, input, 1025 - 1);
    copy[1025 - 1] = '\0';

    // Tokenize the string
    const char delim[] = "_";
    char *token = strtok(copy, delim);

    // Copy the tokens to the output array
    for (int i = 0; i < 3; i++) {
        if (token != NULL) {
            // Copy the token to the output array
            strncpy(comm[i], token, 1025 - 1);
            // Null-terminate the string
            comm[i][1025 - 1] = '\0';
            // Get the next token
            token = strtok(NULL, delim);
        } else {
            // Null-terminate the string
            comm[i][0] = '\0';
        }
    }
}


/** 
 * @brief Processes a received command.
 */
void process_command(char *command) 
{
    // TEMP_ONOFF=(10,sizeof(int));
    int temp;// Variable to store the temperature

    /*Variables to store the limits of the RGB LED*/ 
    int red_min; // Variable to store the minimum red limit
    int red_max; // Variable to store the maximum red limit
    int green_min; // Variable to store the minimum green limit
    int green_max; // Variable to store the maximum green limit
    int blue_min; // Variable to store the minimum blue limit
    int blue_max; // Variable to store the maximum blue limit

    // If the temperature is received
    if (xQueueReceive(TEMP_Q, &temp, pdMS_TO_TICKS(500))) {
        // If the string is not null
        if (command!= NULL){ 
        //if string is not null
        char (*comm)[1025] = malloc(3 * 1025); // Allocate memory for the command
        str_to_chars(command,comm);// Convert the string to an array of characters
            /*If the command is "GET_TEMP" */    
            if (strcmp(command, "TEMP_ON") == 0) {
                // Send the temperature through the UART
                int status=1;
                xQueueSend(TEMP_ONOFF,&status,pdMS_TO_TICKS(0));

            } 
            else if (strcmp(command, "TEMP_OFF") == 0){
                int status=2;
                xQueueSend(TEMP_ONOFF,&status,pdMS_TO_TICKS(0));
            }
            /* If command contain "RED" */
            else if (strcmp(comm[1],"RED")==0)
            {
                // If the command is "MIN"
                if (strcmp(comm[0],"MIN")==0){
                    // Store the minimum red limit
                    red_min=atoi(comm[2]);
                    // Send message to UART
                    send_uart_response("Red minimum limit set\n");
                    // Send the minimum red limit to the queue
                    xQueueSend(Lim_min_red,&red_min,pdMS_TO_TICKS(0));
                    // free(comm);// Free the memory
                }
                // If the command is "MAX"
                else if (strcmp(comm[0],"MAX")==0){
                    // Store the maximum red limit
                    red_max=atoi(comm[2]);
                    // Send message to UART
                    send_uart_response("Red maximum limit set\n");
                    // Send the maximum red limit to the queue
                    xQueueSend(Lim_max_red,&red_max,pdMS_TO_TICKS(0));
                    free(comm);// Free the memory
                }
                else{
                    // Send a response through the UART
                    send_uart_response("Command invalid\n");
                    free(comm);// Free the memory
                }
            }
            /* If command contain "GREEN" */
            else if (strcmp(comm[1],"GREEN")==0)
            {
                // If the command is "MIN"
                if (strcmp(comm[0],"MIN")==0){
                    // Store the minimum green limit
                    green_min=atoi(comm[2]);
                    // Send message to UART
                    send_uart_response("Green minimum limit set\n");
                    // Send the minimum green limit to the queue
                    xQueueSend(Lim_min_green,&green_min,pdMS_TO_TICKS(0));
                    free(comm);// Free the memory
                }
                // If the command is "MAX"
                else if (strcmp(comm[0],"MAX")==0){
                    // Store the maximum green limit
                    green_max=atoi(comm[2]);
                    // Send message to UART
                    send_uart_response("Green maximum limit set\n");
                    // Send the maximum green limit to the queue
                    xQueueSend(Lim_max_green,&green_max,pdMS_TO_TICKS(0));
                    free(comm);// Free the memory
                }
                else{
                    // Send a response through the UART
                    send_uart_response("Command invalid\n");
                    free(comm);// Free the memory
                }
            }
            /* If command contain "BLUE" */
            else if (strcmp(comm[1],"BLUE")==0)
            {
                // If the command is "MIN"
                if (strcmp(comm[0],"MIN")==0){
                    // Store the minimum blue limit
                    blue_min=atoi(comm[2]);
                    // Send message to UART
                    send_uart_response("Blue minimum limit set\n");
                    // Send the minimum blue limit to the queue
                    xQueueSend(Lim_min_blue,&blue_min,pdMS_TO_TICKS(0));
                    free(comm); // Free the memory
                }
                // If the command is "MAX"
                else if (strcmp(comm[0],"MAX")==0){
                    // Store the maximum blue limit
                    blue_max=atoi(comm[2]);
                    // Send message to UART
                    send_uart_response("Blue maximum limit set\n");
                    // Send the maximum blue limit to the queue
                    xQueueSend(Lim_max_blue,&blue_max,pdMS_TO_TICKS(0));
                    free(comm); // Free the memory
                }
                else{
                    // Send a response through the UART
                    send_uart_response("Command invalid\n");
                    free(comm); // Free the memory
                }
            }
            else {
                // Send a response through the UART
                send_uart_response("Command invalid\n");
            }
        }
        else {
            // Send a response through the UART
            send_uart_response("Command invalid\n");
        }
    }
    else {
        // Send a response through the UART
        send_uart_response("Temperature could not be received\n");
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
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