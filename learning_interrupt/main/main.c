#include "esp_attr.h"
#include "freertos/portmacro.h"
#include "freertos/projdefs.h"
#include "hal/gpio_types.h"
#include <driver/gpio.h>
#include <driver/rtc_io.h>
#include <esp_cpu.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

static const char *TAG = "main";

static QueueHandle_t gpio_evt_queue = NULL;

#define SELECTED_PIN GPIO_NUM_0

typedef struct {
  volatile uint8_t counter;
  int64_t start;
  int64_t end;
} StopWatch_t;

StopWatch_t crono = {
    .counter = 0,
};

gpio_config_t btn = {
    .pin_bit_mask = (1ULL << SELECTED_PIN),
    .intr_type = GPIO_INTR_ANYEDGE,
    .mode = GPIO_MODE_INPUT,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .pull_up_en = GPIO_PULLUP_ENABLE,
};

static void IRAM_ATTR intr_end(void *args) {
  StopWatch_t crono_isr = *(StopWatch_t *)args;
  crono_isr.counter++;

  if (crono_isr.counter == 9) {
    crono_isr.end = esp_timer_get_time();
  }
  xQueueSendFromISR(gpio_evt_queue, &crono_isr, NULL);
}

static void IRAM_ATTR intr_start(void *args) {
  StopWatch_t crono_isr = *(StopWatch_t *)args;
  crono_isr.counter++;

  if (crono_isr.counter == 1) {
    crono_isr.start = esp_timer_get_time();
    gpio_isr_handler_remove(SELECTED_PIN);
    gpio_isr_handler_add(SELECTED_PIN, intr_end, &crono);
  }
  xQueueSendFromISR(gpio_evt_queue, &crono_isr, NULL);
}

void show_time(void *args) {
  uint64_t current_time;
  uint64_t time;
  bool stop = true;
  while (stop) {
    xQueueReceive(gpio_evt_queue, &crono, 1000 / portTICK_PERIOD_MS);
    time = esp_timer_get_time();
    if (crono.start) {
      if (crono.end) {
        current_time = crono.end - crono.start;
        ESP_LOGI(TAG, "Total Time: %" PRIu64, current_time);
        stop = false;
      } else {
        current_time = time - crono.start;
        ESP_LOGI(TAG, "time: %" PRIu64, current_time);
      }
    }
  }
  vTaskDelete(NULL);
}

void start_pin(void *args) {
  gpio_install_isr_service(0);
  gpio_isr_handler_add(SELECTED_PIN, intr_start, &crono);
  vTaskDelete(NULL);
}

void app_main(void) {
  gpio_config(&btn);
  gpio_evt_queue = xQueueCreate(10, sizeof(StopWatch_t));

  xTaskCreatePinnedToCore(start_pin, "start_pin", 4096, NULL, 5, NULL, 1);
  xTaskCreatePinnedToCore(show_time, "show_time", 4096, NULL, 4, NULL, 0);
}
