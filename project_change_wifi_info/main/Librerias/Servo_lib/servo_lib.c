#include "servo_lib.h"
#include "esp_log.h"

#define SERVO_MIN_PULSEWIDTH 500  // .5ms de pulso (0°)
#define SERVO_MAX_PULSEWIDTH 2500  // 2.5ms de pulso (180°)

static const char *TAG = "SERVOMOTOR";

/**
 * @brief Convierte un ángulo en el valor de duty cycle correspondiente
 */
static int set_angle(int angle) {
      // Calcular el pulso en microsegundos
      int pulse_width = (angle * (SERVO_MAX_PULSEWIDTH - SERVO_MIN_PULSEWIDTH) / 180) + SERVO_MIN_PULSEWIDTH;

      // Convertir a duty cycle en base a la resolución
      int max_duty = (1 << LEDC_DUTY_RES) - 1; // 2^10 - 1 = 1023
      return (pulse_width * max_duty) / 20000; // Normalizar a 20ms (período del PWM)
}

void servo_init() {
    ESP_LOGI(TAG, "Inicializando servo en GPIO %d", SERVO_PIN);

    // Configurar el temporizador LEDC
    ledc_timer_config_t timer_conf = {
        .speed_mode = LEDC_MODE,
        .duty_resolution = LEDC_DUTY_RES,
        .timer_num = LEDC_TIMER,
        .freq_hz = LEDC_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer_conf);

    // Configurar el canal LEDC
    ledc_channel_config_t ch_conf = {
        .gpio_num = SERVO_PIN,
        .speed_mode = LEDC_MODE,
        .channel = LEDC_CHANNEL,
        .timer_sel = LEDC_TIMER,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&ch_conf);
}


void servo_set_state(servo_state_t state) {
    int duty = set_angle(state);
    
    ESP_LOGI(TAG, "Moviendo servo a %d° (duty %d)", state, duty);
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
}