/**
 * Situação de estados
 *
 * onde temo um valor binario no qual cada
 * cada bit segnifica uma parte do equipamento
 * em funcionamento, ou seja, cada numero significa
 * uma possibilidade da situação do equipamento.
 *
 * Por exemplo um equipamento que tem 2 sensore connectados.
 * Logo, 0b01 quando um deles ta connectado 0b10 quando o
 * outro ta connectado, ou seja, quando recebemos 0b00
 * todos estado desconectado e 0b11 todos estão conectados.
 **/

#include "freertos/portmacro.h"
#include "freertos/projdefs.h"
#include "hal/gpio_types.h"
#include <driver/gpio.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdint.h>
#include <stdio.h>

const static char *TAG = "main";

static TaskHandle_t taskshandle = NULL;

gpio_config_t gpio_sensors = {
    .pin_bit_mask = (1ULL << GPIO_NUM_25) | (1ULL << GPIO_NUM_26),
    .mode = GPIO_MODE_INPUT,
    .pull_down_en = GPIO_PULLDOWN_ENABLE,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .intr_type = GPIO_INTR_DISABLE,
};

void check_pin(void *params) {
  while (1) {
    xTaskNotify(taskshandle,
                (gpio_get_level(GPIO_NUM_26) << 1) |
                    (gpio_get_level(GPIO_NUM_25) << 0),
                eSetValueWithoutOverwrite);
    ESP_LOGI(TAG, "Pins checked");
    vTaskDelay(3000 / portTICK_PERIOD_MS);
  }
}

void callback_checkpin(void *params) {
  uint status;
  while (1) {
    xTaskNotifyWait(1UL, 0, &status, portMAX_DELAY);
    ESP_LOGI(TAG, "Value: %d", status);
  }
}

void app_main(void) {
  gpio_config(&gpio_sensors);
  xTaskCreate(&check_pin, "check_pin", 2048, NULL, 2, &taskshandle);
  xTaskCreate(&callback_checkpin, "callback_checkpin", 2048, NULL, 2,
              &taskshandle);
}
