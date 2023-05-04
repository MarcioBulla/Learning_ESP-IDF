#include <stdio.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include "blink.h"

#define LED_E 32
#define LED_I 2

const static char *TAG = "main";

void app_main(void)
{

    blink_config_t led_e_config = {
        .pin = LED_E,
        .num_repeat = 0,
        .on_time_ms = 500,
        .off_time_ms = 500};

    blink_init(&led_e_config);
    xTaskCreatePinnedToCore(&blink_infinite, "blink_task_e", 2048, &led_e_config, 2, NULL, 1);

    blink_config_t led_i_config = {
        .pin = LED_I,
        .num_repeat = 3,
        .on_time_ms = 1000,
        .off_time_ms = 2000};

    blink_init(&led_i_config);
    xTaskCreatePinnedToCore(&blink_task, "blink_task_i", 2048, &led_i_config, 2, NULL, 0);

    // vTaskDelay(pdMS_TO_TICKS(5000));
    // ESP_LOGI(TAG, "Restarting ESP...");
    // esp_restart();
}
