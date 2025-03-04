idf_component_register(SRCS "Librerias/Wifi_lib/wifi_app.c" "Librerias/Http_lib/http_server.c" "main.c" "Librerias/Adc_lib/ADC_NTC_POT.c" "Librerias/Uart_lib/COMANDS_UART.c" "Librerias/RGB_lib/LED_RGB.c"
                    INCLUDE_DIRS "."
                    EMBED_FILES webpage/app.css webpage/app.js webpage/favicon.ico webpage/index.html webpage/jquery-3.3.1.min.js)
