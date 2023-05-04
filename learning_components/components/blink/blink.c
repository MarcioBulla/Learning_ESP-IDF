#include <blink.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_err.h>
#include <driver/gpio.h>

const static char *TAG = "BLINK";

esp_err_t blink_init(blink_config_t *config)
{
    gpio_reset_pin(config->pin);
    esp_err_t ret = gpio_set_direction(config->pin, GPIO_MODE_OUTPUT);
    return ret;
}

void blink(void *arg)
{
    blink_config_t *config = (blink_config_t *)arg;
    gpio_num_t pin = config->pin;
    int num_repeat = config->num_repeat;
    int time_on = pdMS_TO_TICKS(config->on_time_ms);
    int time_off = pdMS_TO_TICKS(config->off_time_ms);

    ESP_LOGI(TAG, "Blink Started:\n    Pin %d\n    Core: %d", pin, xPortGetCoreID());

    for (int i = 0; i < num_repeat; i++)
    {
        gpio_set_level(pin, 1);
        ESP_LOGI(TAG, "Led ON pin: %d", pin);
        vTaskDelay(time_on);
        gpio_set_level(pin, 0);
        ESP_LOGI(TAG, "Led OFF pin: %d", pin);
        vTaskDelay(time_off);
    }

    ESP_LOGI(TAG, "Blink Ended:\n    Pin %d\n    Core: %d", pin, xPortGetCoreID());
}

void blink_task(void *arg)
{
    blink(arg);
    vTaskDelete(NULL);
}

void blink_infinite(void *arg)
{
    blink_config_t *config = (blink_config_t *)arg;
    gpio_num_t pin = config->pin;
    int time_on = pdMS_TO_TICKS(config->on_time_ms);
    int time_off = pdMS_TO_TICKS(config->off_time_ms);

    ESP_LOGI(TAG, "Infinite Blink Started:\n    Pin %d\n    Core: %d", pin, xPortGetCoreID());

    while (1)
    {
        gpio_set_level(pin, 1);
        ESP_LOGI(TAG, "Led ON pin: %d", pin);
        vTaskDelay(time_on);
        gpio_set_level(pin, 0);
        ESP_LOGI(TAG, "Led OFF pin: %d", pin);
        vTaskDelay(time_off);
    }

    ESP_LOGI(TAG, "Blink Ended:\n    Pin %d\n    Core: %d", pin, xPortGetCoreID());
}
