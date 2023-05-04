#include <stdio.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <sdkconfig.h>
#include <hd44780.h>
#include <pcf8574.h>
#include <esp_system.h>

static const char *TAG = "main";

static i2c_dev_t pcf8574;

static esp_err_t write_lcd_data(const hd44780_t *lcd, uint8_t data)
{
    return pcf8574_port_write(&pcf8574, data);
}

hd44780_t lcd = {
    .write_cb = write_lcd_data,
    .font = HD44780_FONT_5X8,
    .lines = 4,
    .pins = {
        .rs = 0,
        .e = 2,
        .d4 = 4,
        .d5 = 5,
        .d6 = 6,
        .d7 = 7,
        .bl = 3,
    }};

void start_lcd(void)
{
    ESP_ERROR_CHECK(i2cdev_init());
    ESP_ERROR_CHECK(pcf8574_init_desc(&pcf8574, CONFIG_I2C_ADDR, 0, CONFIG_I2C_SDA, CONFIG_I2C_SCL));
    ESP_ERROR_CHECK(hd44780_init(&lcd));

    hd44780_switch_backlight(&lcd, true);
    ESP_LOGI(TAG, "LCD ON");
}

void app_main(void)
{
    start_lcd();
    for (int i = 0; i < 4; i++)
    {
        vTaskDelay(pdMS_TO_TICKS(500));
        hd44780_gotoxy(&lcd, 0, i);
        hd44780_puts(&lcd, "HELLO WORLD!!!");
        ESP_LOGI(TAG, "HELLO WORLD!!! in position %d", i);
        vTaskDelay(pdMS_TO_TICKS(500));
    };
    vTaskDelay(pdMS_TO_TICKS(2000));
    ESP_LOGI(TAG, "Restarting ESP...");
    esp_restart();
}
