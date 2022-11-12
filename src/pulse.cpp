#include <Arduino.h>
#include <math.h>

#include "pulse.h"

typedef struct
{
    bool valid = false;
    bool skip;
} sync_pulse_t;

sync_pulse_t newPulse, oldPulse;
static hw_timer_t *esp_timers[2];

// forward declarations (below order should help readability)
void armSyncPulse();
void measureSyncPulse();
void readSyncPulse(sync_pulse_t &pulse);
void armSweepPulse();
void measureSweepPulse(int64_t collectionTime);
bool allAreHigh();

void pulseSetup()
{
    // setup timers
    esp_timers[syncTimer] = timerBegin(syncTimer, 2, true);
    armSyncPulse();
}

static uint64_t whenRising = 0;
static uint64_t whenFalling = 0;

static int32_t syncEnd = 0;

static void onSyncPulse()
{
    if (digitalRead(14))
        whenRising = timerRead(esp_timers[syncTimer]);
    else
        whenFalling = timerRead(esp_timers[syncTimer]);

    if (whenFalling > whenRising)
    {
        syncTimerRegister = whenFalling - whenRising;
        syncEnd = esp_timer_get_time();
    }
}

void armSyncPulse()
{
    syncTimerRegister = 0;
    attachInterrupt(14, onSyncPulse, CHANGE);

    measureSyncPulse();
}

bool allAreHigh()
{
    // TODO: timer or GPIO interrupt should work with the right priority:
    // https://github.com/HiveTracker/firmware/blob/2afe9f1/Timer.cpp#L25-L30
    // https://github.com/HiveTracker/PPI/commit/d3a54ad

    // photodiodes are high by default (low when light is detected)
    bool allHigh = digitalRead(14); // & digitalRead(sensors_e[1]) &
                                    // digitalRead(sensors_e[2]) & digitalRead(sensors_e[3]);
    return allHigh;
}

void measureSyncPulse()
{
    do
    {
        attachInterrupt(14, onSyncPulse, CHANGE);
        readSyncPulse(newPulse);

        if (newPulse.valid)
        {
            detachInterrupt(14);
            measureSweepPulse(sweepEndTime);
            break;
        }
    } while (true);
}

void readSyncPulse(sync_pulse_t &pulse)
{
    pulse_data.pulse_capture = syncEnd;
    // Look for at least 1 valid pulse                        TODO: make it smarter!!!
    int pulseWidthTicks40 = syncTimerRegister; // 40 MHz

    if (pulseWidthTicks40 > minSyncPulseWidth && // ticks
        pulseWidthTicks40 < maxSyncPulseWidth)
    { // ticks

        float pulseWidthTicks48 = pulseWidthTicks40 * 1.2f; // 48 MHz

        // for the following calculation, consult "Sync pulse timings" in .h file
        pulse_data.axis = (int(round(pulseWidthTicks48 / 500.)) % 2);
        pulse.valid = true;
    }
}

static bool sweepCaptured = false;

static void captureSweep()
{
    sweepStart = esp_timer_get_time();
    sweepCaptured = true;
}

void measureSweepPulse(int64_t collectionTime)
{
    sweepStart = 0;
    int64_t measureStart = esp_timer_get_time();
    attachInterrupt(14, captureSweep, RISING);

    while (!sweepCaptured && esp_timer_get_time() - measureStart < collectionTime)
        ; // wait for sweep pulse

    detachInterrupt(14);

    if (sweepCaptured)
    {
        // Timers S and F, on 4 channels (0 to 3)
        pulse_data.sweep_capture = sweepStart;

        pulse_data.isReady = true;
        pulse_data.shouldReset = true;

        newPulse.valid = false;
        sweepCaptured = false;
    }
    else 
    {
        pulse_data.sweep_capture = sweepStart;

        pulse_data.isReady = false;
        pulse_data.shouldReset = true;

        newPulse.valid = false;
    }
}

bool pulseDataIsReady()
{
    if (pulse_data.shouldReset)
    {
        pulse_data.shouldReset = false;
        return true;
    }
    else
    {
        return false;
    }
}

pulse_data_t pulse_data;