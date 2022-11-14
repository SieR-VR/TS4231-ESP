#ifndef _TS4231_H_
#define _TS4231_H_

#include <stdint.h>

#define MARGIN (10)

#ifdef TICKS_PER_MICROSEC

const int skipThreshold = 99 * TICKS_PER_MICROSEC;
const int minSweepPulseWidth = 1 * TICKS_PER_MICROSEC;
const int maxSweepPulseWidth = 35 * TICKS_PER_MICROSEC;

const int interSyncOffset = 400;          // in microsec
const int sweepStartTime = 1222 - MARGIN; // in microsec
const int sweepEndTime = 6777 + MARGIN;   // in microsec

const int minSyncPulseWidth = (62.5 - MARGIN) * TICKS_PER_MICROSEC;
const int maxSyncPulseWidth = (135 + MARGIN) * TICKS_PER_MICROSEC;

const int sweepStartTicks = sweepStartTime * TICKS_PER_MICROSEC;
const int sweepEndTicks = sweepEndTime * TICKS_PER_MICROSEC;

const int interSyncTicks = interSyncOffset * TICKS_PER_MICROSEC;

#endif

#endif