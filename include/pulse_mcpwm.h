#ifndef _PULSE_H_
#define _PULSE_H_

#include <math.h>
#include "esp_err.h"

typedef struct
{
    uint32_t pulse_start_time;
    uint32_t pulse_end_time;
} pulse_timestamp_t;

typedef struct
{
    uint32_t pulse_duration = 0;
    pulse_timestamp_t pulse_timestamp = {0, 0};
} sync_pulse_t;

typedef struct {
    pulse_timestamp_t sync_pulse;
    pulse_timestamp_t sweep_pulse;
    bool axis;
    bool data;
    bool skip;
} pulse_data_t;

esp_err_t pulseSetup();
bool pulseDataIsReady(pulse_data_t& pulse_data);

#endif // PULSE_H