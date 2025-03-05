#ifndef SERVO_LIB_H
#define SERVO_LIB_H

#include <stdio.h>
#include "driver/ledc.h"
#include "esp_log.h"

// Definiciones para el control del servo
#define SERVO_PIN       GPIO_NUM_5   // GPIO donde está conectado el servo
#define LEDC_CHANNEL    LEDC_CHANNEL_3
#define LEDC_TIMER      LEDC_TIMER_0
#define LEDC_MODE       LEDC_LOW_SPEED_MODE
#define LEDC_DUTY_RES   LEDC_TIMER_13_BIT  // Resolución de 10 bits
#define LEDC_FREQUENCY  50   // Frecuencia de 50Hz (20ms)

// Definir estado del servo en ángulos
typedef enum {
    SERVO_CLOSED = 0,
    SERVO_OPEN = 180
} servo_state_t;


/**
 * @brief  Initialize the servo pwm
 * 
 */
void servo_init(void);
/**
 * @brief Set the servo angle according to  its status
 * @param state The state of the servo Open or Close (0 or 180°)
 */
void servo_set_state(servo_state_t state);

#endif // SERVO_LIB_H
