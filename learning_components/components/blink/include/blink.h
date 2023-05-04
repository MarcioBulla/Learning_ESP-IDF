#ifndef BLINK_H_
#define BLINK_H_

#include <driver/gpio.h>
#include <esp_err.h>

typedef struct
{
    gpio_num_t pin;
    int num_repeat;
    int on_time_ms;
    int off_time_ms;
} blink_config_t;

esp_err_t blink_init(blink_config_t *arg);

void blink(void *arg);

void blink_task(void *arg);

void blink_infinite(void *arg);

#endif // BLINK_H_