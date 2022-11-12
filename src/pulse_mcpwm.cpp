#include <Arduino.h>
#include <math.h>

#include "driver/mcpwm.h"
#include "soc/mcpwm_periph.h"

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

static void IRAM_ATTR isr_handler(void *)
{
    static uint32_t whenRising = 0;
    uint32_t status = MCPWM0.int_st.val & MCPWM_CAP0_INT_ENA;

    if (status)
        whenRising = mcpwm_capture_signal_get_value(MCPWM_UNIT_0, MCPWM_SELECT_CAP0);
    else if (whenRising) {
        uint32_t whenFalling = mcpwm_capture_signal_get_value(MCPWM_UNIT_0, MCPWM_SELECT_CAP0);
        uint32_t pulse_duration = whenFalling - whenRising;

        if (minSyncPulseWidth <= pulse_duration && pulse_duration <= maxSyncPulseWidth) {
            sync_pulse_t pulse;
            pulse.pulse_duration = pulse_duration;
            pulse.pulse_timestamp.pulse_start_time = whenRising;
            pulse.pulse_timestamp.pulse_end_time = whenFalling;
            xQueueSendFromISR(sync_pulse_queue, &pulse, NULL);
        }
        else if (pulse_duration < minSweepPulseWidth) {
            pulse_timestamp_t pulse_timestamp;
            pulse_timestamp.pulse_start_time = whenRising;
            pulse_timestamp.pulse_end_time = whenFalling;
            xQueueSendFromISR(sweep_pulse_queue, &pulse_timestamp, NULL);
        }
    }
}

esp_err_t pulseSetup()
{
    esp_err_t err = ESP_OK;
    sync_pulse_queue = xQueueCreate(1, sizeof(sync_pulse_t));
    sweep_pulse_queue = xQueueCreate(1, sizeof(pulse_timestamp_t));

    err = mcpwm_gpio_init(MCPWM_UNIT_0, mcpwm_io_signals_t::MCPWM_CAP_0, 14);
    if (err != ESP_OK)
        return err;

    err = mcpwm_capture_enable(MCPWM_UNIT_0, MCPWM_SELECT_CAP0, MCPWM_BOTH_EDGE, 0);
    if (err != ESP_OK)
        return err;

    mcpwm_isr_register(MCPWM_UNIT_0, isr_handler, NULL, ESP_INTR_FLAG_IRAM, NULL);
    MCPWM0.int_ena.val = MCPWM_CAP0_INT_ENA;
}

bool pulseDataIsReady(pulse_data_t& pulse_data) {
    sync_pulse_t sync_pulse;
    pulse_timestamp_t sweep_pulse;
    bool result = true;

    // wait max 120hz
    result = xQueueReceive(sync_pulse_queue, &sync_pulse, ((1000 / 120) / portTICK_PERIOD_MS));
    if (!result)
        return false;

    uint32_t sync_pulse_duration_48mhz = sync_pulse.pulse_duration * 3 / 5;
    uint8_t sync_pulse_data = (sync_pulse_duration_48mhz - 2501) / 500;

    result = xQueueReceive(sweep_pulse_queue, &sweep_pulse, sweepEndTicks);
    if (!result)
        return false;

    pulse_data.sync_pulse = sync_pulse.pulse_timestamp;
    pulse_data.sweep_pulse = sweep_pulse;
    
    pulse_data.axis = sync_pulse_data & 0x01;
    pulse_data.data = sync_pulse_data & 0x02;
    pulse_data.skip = sync_pulse_data & 0x04;

    return true;
}