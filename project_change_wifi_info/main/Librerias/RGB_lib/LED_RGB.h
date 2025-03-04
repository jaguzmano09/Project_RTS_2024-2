#ifndef LED_RGB_H
#define LED_RGB_H

#include "driver/ledc.h"
#include <stdint.h>
#include "esp_err.h"

// Defined configuration for timer LEDC
#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_HIGH_SPEED_MODE
#define LEDC_FREQUENCY          (4000) // Frecuency (4 kHz)

// Defined configuration for LED RGB
#define LEDC_CHANNEL_R          LEDC_CHANNEL_0
#define LEDC_CHANNEL_G          LEDC_CHANNEL_1
#define LEDC_CHANNEL_B          LEDC_CHANNEL_2

// Declared functions

/**
 * @brief Configure LEDC channels to control RGB LEDs.
 */
void configure_led_rgb(uint8_t GPIO_RED, uint8_t GPIO_GREEN, uint8_t GPIO_BLUE, ledc_timer_bit_t LEDC_DUTY_RES);

/**
 * @brief Sets the color of the RGB LED with specific values for red, green, and blue.
 * 
 * @param red   Brightness of red (0-100)%.
 * @param green Brightness of green (0-100)%.
 * @param blue  Brightness of blue (0-100)%.
 */
void set_rgb_color(uint8_t red, uint8_t green, uint8_t blue, ledc_timer_bit_t LEDC_DUTY_RES);

#endif // LED_RGB_H

