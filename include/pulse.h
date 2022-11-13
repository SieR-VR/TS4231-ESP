#ifndef _PULSE_H_
#define _PULSE_H_

#include <stdint.h>

struct pulse_timestamp_t
{
    uint32_t pulse_start_time;
    uint32_t pulse_end_time;
};

struct sync_pulse_data_t
{
    pulse_timestamp_t sync_pulse;
    bool axis;
    bool data;
    bool skip;
};

struct sweep_pulse_data_t
{
    pulse_timestamp_t sweep_pulse;
};

#endif