#include "ts4231_mcpwm.h"

#define TICKS_PER_MICROSEC (80)
#include "ts4231.h"

static mcpwm_unit_t idx_to_unit[6]{
    MCPWM_UNIT_0,
    MCPWM_UNIT_0,
    MCPWM_UNIT_0,
    MCPWM_UNIT_1,
    MCPWM_UNIT_1,
    MCPWM_UNIT_1,
};

static mcpwm_capture_channel_id_t idx_to_capture_channel[6]{
    MCPWM_SELECT_CAP0,
    MCPWM_SELECT_CAP1,
    MCPWM_SELECT_CAP2,
    MCPWM_SELECT_CAP0,
    MCPWM_SELECT_CAP1,
    MCPWM_SELECT_CAP2,
};

static mcpwm_io_signals_t idx_to_io_signal[6]{
    MCPWM_CAP_0,
    MCPWM_CAP_1,
    MCPWM_CAP_2,
    MCPWM_CAP_0,
    MCPWM_CAP_1,
    MCPWM_CAP_2,
};

TS4231_MCPWM::TS4231_MCPWM(uint32_t gpio_num, uint32_t idx)
    : gpio_num(gpio_num)
{
    this->mcpwm_unit = idx_to_unit[idx];
    this->mcpwm_capture_channel = idx_to_capture_channel[idx];
    this->mcpwm_io_signal = idx_to_io_signal[idx];
}

esp_err_t TS4231_MCPWM::init()
{
    esp_err_t err = ESP_OK;

    this->sync_pulse_queue = xQueueCreate(1, sizeof(pulse_timestamp_t));
    this->sweep_pulse_queue = xQueueCreate(1, sizeof(pulse_timestamp_t));

    err = mcpwm_gpio_init(
        this->mcpwm_unit,
        this->mcpwm_io_signal,
        this->gpio_num);

    if (err != ESP_OK)
        return err;

    mcpwm_capture_config_t cap_config = {
        .cap_edge = MCPWM_BOTH_EDGE,
        .cap_prescale = 1,
        .capture_cb = isr_handler,
        .user_data = this,
    };

    err = mcpwm_capture_enable_channel(
        this->mcpwm_unit,
        this->mcpwm_capture_channel,
        &cap_config);

    return err;
}

bool TS4231_MCPWM::get_sync_pulse(sync_pulse_data_t *pulse_data)
{
    pulse_timestamp_t sync_pulse;
    bool result = xQueueReceive(this->sync_pulse_queue, &sync_pulse, 0);
    if (!result)
        return false;

    uint32_t sync_pulse_duration = sync_pulse.pulse_end_time - sync_pulse.pulse_start_time;
    uint32_t sync_pulse_duration_48mhz = sync_pulse_duration * 3 / 5;

    uint8_t sync_pulse_data = (sync_pulse_duration_48mhz - 2501) / 500;

    pulse_data->sync_pulse = sync_pulse;

    pulse_data->axis = sync_pulse_data & 0x01;
    pulse_data->data = sync_pulse_data & 0x02;
    pulse_data->skip = sync_pulse_data & 0x04;

    return true;
}

bool TS4231_MCPWM::get_sweep_pulse(sweep_pulse_data_t *pulse_data)
{
    return xQueueReceive(this->sweep_pulse_queue, &pulse_data->sweep_pulse, 0);
}

static bool IRAM_ATTR isr_handler(
    mcpwm_unit_t mcpwm,
    mcpwm_capture_channel_id_t channel,
    const cap_event_data_t *cap_event_data,
    void *user_data)
{
    TS4231_MCPWM *ts4231_mcpwm = (TS4231_MCPWM *)user_data;

    if (cap_event_data->cap_edge & MCPWM_POS_EDGE)
        ts4231_mcpwm->when_rising = cap_event_data->cap_value;
    else
    {
        uint32_t when_falling = cap_event_data->cap_value;
        uint32_t pulse_duration = when_falling - ts4231_mcpwm->when_rising;

        pulse_timestamp_t pulse = {
            .pulse_start_time = ts4231_mcpwm->when_rising,
            .pulse_end_time = when_falling,
        };

        if (minSyncPulseWidth <= pulse_duration && pulse_duration <= maxSyncPulseWidth)
        {
            xQueueSendFromISR(ts4231_mcpwm->sync_pulse_queue, &pulse, NULL);
        }
        else
        {
            xQueueSendFromISR(ts4231_mcpwm->sweep_pulse_queue, &pulse, NULL);
        }
    }

    return true;
}