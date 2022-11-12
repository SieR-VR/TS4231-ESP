#include <Arduino.h>
#include <math.h>

#include "driver/mcpwm.h"
#include "pulse_mcpwm.h"

const int ticksPerMicrosec = 80; // because 80MHz

const int margin = 10;          // microsec
const int minSyncPulseWidth = (62.5 - margin) * ticksPerMicrosec;
const int maxSyncPulseWidth = (135  + margin) * ticksPerMicrosec;
const int skipThreshold = ((104 + ceil(93.8)) / 2) * ticksPerMicrosec;

const int interSyncOffset = 400; // microsec

/* Sync pulse timings - a 10us margins is good practice
    j0	0	0	0	3000	62.5
    k0	0	0	1	3500	72.9
    j1	0	1	0	4000	83.3
    k1	0	1	1	4500	93.8
    j2	1	0	0	5000	104
    k2	1	0	1	5500	115
    j3	1	1	0	6000	125
    k3	1	1	1	6500	135
*/

const int minSweepPulseWidth = 1  * ticksPerMicrosec;
const int maxSweepPulseWidth = 35 * ticksPerMicrosec;

const int sweepStartTime = 1222 - margin; // microsec
const int sweepEndTime   = 6777 + margin; // microsec

const int sweepStartTicks = sweepStartTime * ticksPerMicrosec;
const int sweepEndTicks   = sweepEndTime   * ticksPerMicrosec;

xQueueHandle sync_pulse_queue; // sync_pulse_t
xQueueHandle sweep_pulse_queue; // pulse_timestamp_t

static bool IRAM_ATTR isr_handler(mcpwm_unit_t mcpwm, mcpwm_capture_channel_id_t cap_channel, const cap_event_data_t *edata, void *)
{
    static uint32_t whenRising = 0;

    if (edata->cap_edge & MCPWM_POS_EDGE)
        whenRising = edata->cap_value;
    else {
        uint32_t whenFalling = edata->cap_value;
        uint32_t pulse_duration = whenFalling - whenRising;

        if (minSyncPulseWidth <= pulse_duration && pulse_duration <= maxSyncPulseWidth) {
            sync_pulse_t pulse;
            pulse.pulse_duration = pulse_duration;
            pulse.pulse_timestamp.pulse_start_time = whenRising;
            pulse.pulse_timestamp.pulse_end_time = whenFalling;
            xQueueSendFromISR(sync_pulse_queue, &pulse, 0);
        }
        // else if (pulse_duration < minSweepPulseWidth) {
        //     pulse_timestamp_t pulse_timestamp;
        //     pulse_timestamp.pulse_start_time = whenRising;
        //     pulse_timestamp.pulse_end_time = whenFalling;
        //     xQueueSendFromISR(sweep_pulse_queue, &pulse_timestamp, NULL);
        // }
        else {
            pulse_timestamp_t pulse_timestamp;
            pulse_timestamp.pulse_start_time = whenRising;
            pulse_timestamp.pulse_end_time = whenFalling;
            xQueueSendFromISR(sweep_pulse_queue, &pulse_timestamp, 0);
        }
    }

    return true;
}

esp_err_t pulseSetup()
{
    esp_err_t err = ESP_OK;
    sync_pulse_queue = xQueueCreate(1, sizeof(sync_pulse_t));
    sweep_pulse_queue = xQueueCreate(1, sizeof(pulse_timestamp_t));

    err = mcpwm_gpio_init(MCPWM_UNIT_0, mcpwm_io_signals_t::MCPWM_CAP_0, 14);
    if (err != ESP_OK)
        return err;

    mcpwm_capture_config_t cap_config;
    cap_config.cap_edge = MCPWM_BOTH_EDGE;
    cap_config.cap_prescale = 1;
    cap_config.capture_cb = isr_handler;
    cap_config.user_data = NULL;

    err = mcpwm_capture_enable_channel(MCPWM_UNIT_0, MCPWM_SELECT_CAP0, &cap_config);
    if (err != ESP_OK)
        return err;

    return err;
}

bool pulseDataIsReady(pulse_data_t& pulse_data) {
    sync_pulse_t sync_pulse;
    pulse_timestamp_t sweep_pulse;
    bool result = true;

    // wait max 120hz
    result = xQueueReceive(sync_pulse_queue, &sync_pulse, 8);
    if (!result)
        return false;

    uint32_t sync_pulse_duration_48mhz = sync_pulse.pulse_duration * 3 / 5;
    uint8_t sync_pulse_data = (sync_pulse_duration_48mhz - 2501) / 500;

    result = xQueueReceive(sweep_pulse_queue, &sweep_pulse, 7);
    if (!result)
        return false;

    pulse_data.sync_pulse = sync_pulse.pulse_timestamp;
    pulse_data.sweep_pulse = sweep_pulse;
    
    pulse_data.axis = sync_pulse_data & 0x01;
    pulse_data.data = sync_pulse_data & 0x02;
    pulse_data.skip = sync_pulse_data & 0x04;

    return true;
}