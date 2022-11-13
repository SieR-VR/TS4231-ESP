#ifndef _TS4231_MCPWM_H_
#define _TS4231_MCPWM_H_

#include <Arduino.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/mcpwm.h>

#include "pulse.h"

class TS4231_MCPWM
{
public:
    xQueueHandle sync_pulse_queue;  // pulse_timestamp_t
    xQueueHandle sweep_pulse_queue; // pulse_timestamp_t

    uint32_t gpio_num;
    uint32_t when_rising = 0;

    mcpwm_unit_t mcpwm_unit;
    mcpwm_capture_channel_id_t mcpwm_capture_channel;
    mcpwm_io_signals_t mcpwm_io_signal;

    TS4231_MCPWM(uint32_t gpio_num, uint32_t idx);

    esp_err_t init();
    bool get_sync_pulse(sync_pulse_data_t *sync_pulse_data);
    bool get_sweep_pulse(sweep_pulse_data_t *sweep_pulse_data);
};

static bool isr_handler(
    mcpwm_unit_t mcpwm,
    mcpwm_capture_channel_id_t cap_channel,
    const cap_event_data_t *edata,
    void *user_data);

#endif