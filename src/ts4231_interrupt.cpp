#include "ts4231_interrupt.h"

#define TICKS_PER_MICROSEC (40)
#include "ts4231.h"

TS4231_Interrupt::TS4231_Interrupt(uint32_t gpio_num)
    : gpio_num(gpio_num)
{}

esp_err_t TS4231_Interrupt::init(hw_timer_t *timer)
{
    esp_err_t err = ESP_OK;

    this->sync_pulse_queue = xQueueCreate(1, sizeof(pulse_timestamp_t));
    this->sweep_pulse_queue = xQueueCreate(1, sizeof(pulse_timestamp_t));
    this->timer = timer;

    pinMode(this->gpio_num, INPUT);
    attachInterruptArg(this->gpio_num, interrupt_handler, this, CHANGE);

    return err;
}

bool TS4231_Interrupt::get_sync_pulse(sync_pulse_data_t *pulse_data)
{
    pulse_timestamp_t sync_pulse;
    bool result = xQueueReceive(this->sync_pulse_queue, &sync_pulse, 0);
    if (!result)
        return false;

    uint32_t sync_pulse_duration = sync_pulse.pulse_end_time - sync_pulse.pulse_start_time;
    uint32_t sync_pulse_duration_48mhz = sync_pulse_duration * 6 / 5;

    uint8_t sync_pulse_data = (sync_pulse_duration_48mhz - 2501) / 500;

    pulse_data->sync_pulse = sync_pulse;

    pulse_data->axis = sync_pulse_data & 0x01;
    pulse_data->data = sync_pulse_data & 0x02;
    pulse_data->skip = sync_pulse_data & 0x04;

    return true;
}

bool TS4231_Interrupt::get_sweep_pulse(sweep_pulse_data_t *pulse_data)
{
    return xQueueReceive(this->sweep_pulse_queue, &pulse_data->sweep_pulse, 0);
}

static void IRAM_ATTR interrupt_handler(void *data) 
{
    TS4231_Interrupt *ts4231_interrupt = (TS4231_Interrupt *)data;
    uint32_t pulse_start_time = timerRead(ts4231_interrupt->timer);

    if (digitalRead(ts4231_interrupt->gpio_num))
        ts4231_interrupt->when_rising = pulse_start_time;
    else
    {
        uint32_t when_falling = pulse_start_time;
        uint32_t pulse_duration = when_falling - ts4231_interrupt->when_rising;

        pulse_timestamp_t pulse = {
            .pulse_start_time = ts4231_interrupt->when_rising,
            .pulse_end_time = when_falling,
        };

        if (minSyncPulseWidth <= pulse_duration && pulse_duration <= maxSyncPulseWidth)
        {
            xQueueSendFromISR(ts4231_interrupt->sync_pulse_queue, &pulse, NULL);
        }
        else if (minSweepPulseWidth <= pulse_duration && pulse_duration <= maxSweepPulseWidth)
        {
            xQueueSendFromISR(ts4231_interrupt->sweep_pulse_queue, &pulse, NULL);
        }
    }
} 