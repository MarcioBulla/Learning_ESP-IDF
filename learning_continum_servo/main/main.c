#include "driver/mcpwm_cmpr.h"
#include "driver/mcpwm_gen.h"
#include "driver/mcpwm_oper.h"
#include "driver/mcpwm_timer.h"
#include "driver/mcpwm_types.h"
#include "esp_err.h"
#include "freertos/portmacro.h"
#include "hal/mcpwm_types.h"
#include "soc/clk_tree_defs.h"
#include <button.h>
#include <driver/gpio.h>
#include <driver/mcpwm_prelude.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

static const char *TAG = "main";

#define SERVO_CLOCKWISE 2500
#define SERVO_COUNTER_CLOCKWISE 500
#define SERVO_STOP 1500

#define SERVO_PULSE_GPIO 13
#define SERVO_TIMEBASE_RESOLUTION_HZ 1000000
#define SERVO_TIMEBASE_PERIOD 20000

#define BUTTON_GPIO_R 14
#define BUTTON_GPIO_L 0

typedef enum {
  STOP,
  CLOCKWISE,
  COUNTERCLOCKWISE,
  NOTHING,
} ServoDirection_t;

ServoDirection_t current_servo_direction = STOP;

static QueueHandle_t Handle_servo = NULL;

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

// Função callback para SENTIDO HORARIO
static void set_clockwise(button_t *btn, button_state_t state) {
  ServoDirection_t command = NOTHING;
  if (state == BUTTON_PRESSED) {
    command = CLOCKWISE;
    ESP_LOGI(TAG, "Send ClockWise");
  } else if (state == BUTTON_RELEASED) {
    command = STOP;
    ESP_LOGI(TAG, "Sendo Stop");
  }
  if (command != NOTHING) {
    xQueueSend(Handle_servo, &command, 0);
  }
}

// Função callback para botão onde diminui o angulo
static void set_counterclockwise(button_t *btn, button_state_t state) {
  ServoDirection_t command = NOTHING;
  if (state == BUTTON_PRESSED) {
    command = COUNTERCLOCKWISE;
    ESP_LOGI(TAG, "Send Counter ClockWise");
  } else if (state == BUTTON_RELEASED) {
    command = STOP;
    ESP_LOGI(TAG, "Sendo Stop");
  }
  if (command != NOTHING) {
    xQueueSend(Handle_servo, &command, 0);
  }
}

// Config botão direito
static button_t btn_r = {
    .gpio = BUTTON_GPIO_R,
    .autorepeat = false,
    .pressed_level = 0,
    .internal_pull = true,
    .callback = set_clockwise,
};

// Config botão esquerto
static button_t btn_l = {
    .gpio = BUTTON_GPIO_L,
    .autorepeat = false,
    .pressed_level = 0,
    .internal_pull = true,
    .callback = set_counterclockwise,
};

inline uint32_t ServoDir_to_us(ServoDirection_t state) {
  uint32_t pulse;

  switch (state) {

  case CLOCKWISE:
    pulse = SERVO_CLOCKWISE;
    break;

  case COUNTERCLOCKWISE:
    pulse = SERVO_COUNTER_CLOCKWISE;
    break;

  default:
    pulse = STOP;
    break;
  }

  return pulse;
}

void update_servo(void *params) {
  while (true) {
    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(
        comparator, ServoDir_to_us(current_servo_direction)));
    xQueueReceive(Handle_servo, &current_servo_direction, portMAX_DELAY);
  }
}

void app_main(void) {
  Handle_servo = xQueueCreate(5, sizeof(ServoDirection_t));

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

  xTaskCreate(update_servo, "update_servo", 2048, NULL, 3, NULL);

  // Seleciona o valor inicial do comparador (90° centro)
  ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator, SERVO_STOP));

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
