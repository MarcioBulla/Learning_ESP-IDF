#include "esp_err.h"
#include "esp_system.h"
#include "freertos/projdefs.h"
#include "hal/gpio_types.h"
#include <driver/gpio.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

const static char *TAG = "main";

#define ISR_BTN GPIO_NUM_25
#define RST_BTN GPIO_NUM_5

// For use Interrupt use extern PULL UP or DOWN
gpio_config_t btn_isr = {
    .pin_bit_mask = (1ULL << ISR_BTN),
    .intr_type = GPIO_INTR_POSEDGE,
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
};

gpio_config_t btn_rst = {
    .pin_bit_mask = (1ULL << RST_BTN),
    .intr_type = GPIO_INTR_DISABLE,
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_ENABLE,
};

uint16_t count;
nvs_handle_t my_handle;

static void IRAM_ATTR count_btn(void *args) { count++; }

void app_main(void) {

  esp_err_t err = nvs_flash_init();

  if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
      err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }

  ESP_ERROR_CHECK(err);

  // Open
  ESP_LOGI(TAG, "Opening Non-Volatile Storage (NVS) handle... ");

  err = nvs_open("storage", NVS_READWRITE, &my_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
  } else {
    ESP_LOGI(TAG, "Done\n");

    // Read
    ESP_LOGI(TAG, "Reading restart counter from NVS ... ");
    err = nvs_get_u16(my_handle, "counter", &count);
    switch (err) {
    case ESP_OK:
      ESP_LOGI(TAG, "Done\n");
      ESP_LOGI(TAG, "Restart counter = %" PRIu16 , count);
      break;
    case ESP_ERR_NVS_NOT_FOUND:
      ESP_LOGI(TAG, "The value is not initialized yet!");
      break;
    default:
      ESP_LOGE(TAG, "Error (%s) reading!", esp_err_to_name(err));
    }

    ESP_LOGI(TAG, "CLOSE NVS");
    nvs_close(my_handle);
  }

  gpio_config(&btn_isr);
  gpio_install_isr_service(0);
  gpio_isr_handler_add(ISR_BTN, count_btn, NULL);

  while (gpio_get_level(RST_BTN)) {
    vTaskDelay(pdMS_TO_TICKS(500));
    ESP_LOGI(TAG, "Count: %d", count);
  }
  // Open
  ESP_LOGI(TAG, "Opening Non-Volatile Storage (NVS) handle... ");
  err = nvs_open("storage", NVS_READWRITE, &my_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
  } else {
    ESP_LOGI(TAG, "Done\n");

    // Write
    ESP_LOGI(TAG, "Updating restart counter in NVS ... ");
    err = nvs_set_u16(my_handle, "counter", count);

    ESP_LOGI(TAG, "Update status: (%s)", esp_err_to_name(err));

    ESP_LOGI(TAG, "Committing updates in NVS ... ");
    err = nvs_commit(my_handle);
    ESP_LOGI(TAG, "Commit status: (%s)", esp_err_to_name(err));

    ESP_LOGI(TAG, "CLOSE NVS");
    nvs_close(my_handle);
  }

  ESP_LOGI(TAG, "RESTARTING....");
  vTaskDelay(pdMS_TO_TICKS(1000));
  esp_restart();
}
