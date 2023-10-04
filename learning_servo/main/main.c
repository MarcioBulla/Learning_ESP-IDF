#include "driver/mcpwm_cmpr.h"
#include "driver/mcpwm_gen.h"
#include "driver/mcpwm_oper.h"
#include "driver/mcpwm_timer.h"
#include "driver/mcpwm_types.h"
#include "esp_err.h"
#include "hal/mcpwm_types.h"
#include "soc/clk_tree_defs.h"
#include <button.h>
#include <driver/gpio.h>
#include <driver/mcpwm_prelude.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdint.h>
#include <stdio.h>

static const char *TAG = "main";

#define SERVO_MIN_PULSEWIDTH_US 500
#define SERVO_MAX_PULSEWIDTH_US 2500
#define SERVO_MIN_DEGREE 0
#define SERVO_MAX_DEGREE 180

#define SERVO_PULSE_GPIO 13
#define SERVO_TIMEBASE_RESOLUTION_HZ 1000000
#define SERVO_TIMEBASE_PERIOD 20000

#define BUTTON_GPIO_R 14
#define BUTTON_GPIO_L 0

uint8_t current_angle = 90;

static inline uint32_t angle_to_compare(uint8_t angle) {
  return (angle - SERVO_MIN_DEGREE) *
             (SERVO_MAX_PULSEWIDTH_US - SERVO_MIN_PULSEWIDTH_US) /
             (SERVO_MAX_DEGREE - SERVO_MIN_DEGREE) +
         SERVO_MIN_PULSEWIDTH_US;
}

// Seta caracteristicas do PWM
mcpwm_timer_handle_t timer = NULL;
mcpwm_timer_config_t timer_config = {
    .group_id = 0,
    .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
    .resolution_hz = SERVO_TIMEBASE_RESOLUTION_HZ,
    .period_ticks = SERVO_TIMEBASE_PERIOD,
    .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
};

// Seta caracteristicas do operador (o que gera o proprio PWM)
mcpwm_oper_handle_t oper = NULL;
mcpwm_operator_config_t opererator_config = {
    .group_id = 0,
};

// Seta comparador onde ele verifica o estado atuual com o proximo
mcpwm_cmpr_handle_t comparator = NULL;
mcpwm_comparator_config_t comparator_config = {
    .flags.update_cmp_on_tez = true,
};

// Seta o Gerador do PWM
mcpwm_gen_handle_t generator = NULL;
mcpwm_generator_config_t generator_config = {
    .gen_gpio_num = SERVO_PULSE_GPIO,
};

// Função callback para botão onde aumenta o angulo
static void add_angle(button_t *btn, button_state_t state) {

  if (state == BUTTON_CLICKED) {

    if (current_angle < 180) {
      current_angle++;
    } else {
      current_angle = 0;
    }

    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(
        comparator, angle_to_compare(current_angle)));
    ESP_LOGI(TAG, "CURRENT\n  ANGLE: %d \n  TICK: %" PRIu32, current_angle,
             angle_to_compare(current_angle));
  }
}

// Função callback para botão onde diminui o angulo
static void rm_angle(button_t *btn, button_state_t state) {

  if (state == BUTTON_CLICKED) {

    if (current_angle != 0) {
      current_angle--;
    } else {
      current_angle = 180;
    }

    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(
        comparator, angle_to_compare(current_angle)));
    ESP_LOGI(TAG, "CURRENT\n  ANGLE: %d \n  TICK: %" PRIu32, current_angle,
             angle_to_compare(current_angle));
  }
}

// Config botão direito
static button_t btn_r = {
    .gpio = BUTTON_GPIO_R,
    .autorepeat = true,
    .pressed_level = 0,
    .internal_pull = true,
    .callback = add_angle,
};

// Config botão esquerto
static button_t btn_l = {
    .gpio = BUTTON_GPIO_L,
    .autorepeat = true,
    .pressed_level = 0,
    .internal_pull = true,
    .callback = rm_angle,
};

void app_main(void) {

  // Iniciando handles para o funcionamento do servo motor

  // Inicia e atribui o timer do PWM
  ESP_ERROR_CHECK(mcpwm_new_timer(&timer_config, &timer));
  // Inicia e atribui o operador do PWM
  ESP_ERROR_CHECK(mcpwm_new_operator(&opererator_config, &oper));
  // Connecta o Operador no Timer
  ESP_ERROR_CHECK(mcpwm_operator_connect_timer(oper, timer));
  // Inicia e atribui o comparador quue esoera zerar o contador
  ESP_ERROR_CHECK(mcpwm_new_comparator(oper, &comparator_config, &comparator));
  // Inicia e atribui o comparador quue esoera zerar o contador
  ESP_ERROR_CHECK(mcpwm_new_generator(oper, &generator_config, &generator));

  // Seleciona o valor inicial do comparador (90° centro)
  ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(
      comparator, angle_to_compare(current_angle)));

  // Set ação do gerador chamado pelo timer
  ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(
      generator, MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP,
                                              MCPWM_TIMER_EVENT_EMPTY,
                                              MCPWM_GEN_ACTION_HIGH)));

  // Set ação do gerador chamado pelo comapador
  ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(
      generator,
      MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comparator,
                                     MCPWM_GEN_ACTION_LOW)));

  ESP_ERROR_CHECK(mcpwm_timer_enable(timer));

  mcpwm_timer_start_stop(timer, MCPWM_TIMER_START_NO_STOP);

  ESP_ERROR_CHECK(button_init(&btn_r));
  ESP_ERROR_CHECK(button_init(&btn_l));
}
