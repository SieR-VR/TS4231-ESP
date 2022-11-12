#ifndef _PULSE_H_
#define _PULSE_H_

#include <math.h>

typedef struct {
    int pulse_capture;
    int sweep_capture;
    bool baseID;
    bool axis; // 0 horizontal (x), 1 vertical (y) - TODO: check it!
    bool isReady;
    bool shouldReset;
} pulse_data_t;

static const int syncTimer = 0;

static uint64_t syncTimerRegister = 0;

static int sweepStart = 0;

const int ticksPerMicrosec = 40; // because 40MHz

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

void armSyncPulse();
void pulseSetup();
bool pulseDataIsReady();

extern pulse_data_t pulse_data;

#endif // PULSE_H