#include "freertos/portmacro.h"
#include <driver/gpio.h>
#include <driver/pulse_cnt.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

const static char *TAG = "main";

typedef struct {
  int watch_point;
  int64_t time;
} save_time_t;

int counter = 0;
int old_counter = 20;
save_time_t current_time;
save_time_t old_time;
pcnt_unit_config_t config_unit = {
    .high_limit = 20,
    .low_limit = -20,
};

pcnt_chan_config_t config_chan = {.edge_gpio_num = GPIO_NUM_25};

pcnt_unit_handle_t pcnt_unit = NULL;

pcnt_channel_handle_t pcnt_chan = NULL;

static bool example_pcnt_on_reach(pcnt_unit_handle_t pcnt_unit,
                                  const pcnt_watch_event_data_t *edata,
                                  void *user_ctx) {
  BaseType_t high_task_wakeup;
  QueueHandle_t queue = (QueueHandle_t)user_ctx;
  save_time_t temp_time = {
      .watch_point = edata->watch_point_value,
      .time = esp_timer_get_time(),
  };
  xQueueSendFromISR(queue, &temp_time, &high_task_wakeup);
  return (high_task_wakeup == pdTRUE);
};

pcnt_event_callbacks_t pcnt_event = {
    .on_reach = example_pcnt_on_reach,
};

void app_main(void) {

  QueueHandle_t queue = xQueueCreate(2, sizeof(save_time_t));
  pcnt_new_unit(&config_unit, &pcnt_unit);
  pcnt_new_channel(pcnt_unit, &config_chan, &pcnt_chan);

  pcnt_channel_set_edge_action(pcnt_chan, PCNT_CHANNEL_EDGE_ACTION_INCREASE,
                               PCNT_CHANNEL_EDGE_ACTION_HOLD);

  int watch_points[] = {4, 10};
  for (size_t i = 0; i < sizeof(watch_points) / sizeof(watch_points[0]); i++) {
    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(pcnt_unit, watch_points[i]));
  }

  pcnt_unit_register_event_callbacks(pcnt_unit, &pcnt_event, queue);

  ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit));
  ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));
  ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit));

  while (true) {
    ESP_LOGI(TAG, "count: %d", counter);
    xQueueReceive(queue, &current_time, 0);
    if (current_time.watch_point > old_time.watch_point) {
      ESP_LOGI(TAG, "current: %d; odl: %d;", current_time.watch_point,
               old_time.watch_point);
      ESP_LOGI(TAG, "time: %lld", (current_time.time - old_time.time) / 1000);
    }
    old_time = current_time;

    old_counter = counter;
    while (counter == old_counter) {
      pcnt_unit_get_count(pcnt_unit, &counter);
      vTaskDelay(pdMS_TO_TICKS(10));
    }
  }
}
