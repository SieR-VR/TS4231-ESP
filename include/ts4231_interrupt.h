#ifndef _TS4231_INTERRUPT_H_
#define _TS4231_INTERRUPT_H_

#include <Arduino.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/mcpwm.h>

#include "pulse.h"

class TS4231_Interrupt
{
public:
    xQueueHandle sync_pulse_queue;  // pulse_timestamp_t
    xQueueHandle sweep_pulse_queue; // pulse_timestamp_t

    uint32_t gpio_num;
    uint32_t when_rising = 0;

    hw_timer_t *timer = NULL;

    TS4231_Interrupt(uint32_t gpio_num);

    esp_err_t init(hw_timer_t *timer);
    bool get_sync_pulse(sync_pulse_data_t *pulse_data);
    bool get_sweep_pulse(sweep_pulse_data_t *pulse_data);
};

static void interrupt_handler(void *data);

#endif