#define CONFIG_ESP_TIMER_INTERRUPT_LEVEL 3
#include <Arduino.h>
#include <math.h>

#include "ts4231_mcpwm.h"

#define TICKS_PER_MICROSEC (80)
#include "ts4231.h"

// hw_timer_t *timer = NULL;
// TS4231_Interrupt ts4231(14);

TS4231_MCPWM ts4231_12(12, 0);
TS4231_MCPWM ts4231_14(14, 1);

sync_pulse_data_t sync_pulse_data_12, sync_pulse_data_14;
sweep_pulse_data_t sweep_pulse_data_12, sweep_pulse_data_14;

bool sync_from_12 = false, sync_from_14 = false;
bool sweep_from_12 = false, sweep_from_14 = false;

int rawX[2] = {0, 0}, rawY[2] = {0, 0};
float xAngle[2] = {0, 0}, yAngle[2] = {0, 0};

void setup()
{
  Serial.begin(115200);
  // timer = timerBegin(0, 2, true);

  ts4231_12.init();
  ts4231_14.init();
}

float mapf(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

bool in_range(uint32_t x, uint32_t min, uint32_t max)
{
  return (x >= min) && (x <= max);
}

void loop()
{
  while (true)
  {
    sync_from_12 = ts4231_12.get_sync_pulse(&sync_pulse_data_12);
    if (sync_from_12)
      break;
    
    sync_from_14 = ts4231_14.get_sync_pulse(&sync_pulse_data_14);
    if (sync_from_14)
      break;
  }

  int64_t sync_pulse_timestamp = esp_timer_get_time();

  while ((esp_timer_get_time() - sync_pulse_timestamp) < sweepEndTime)
  {
    if (!sweep_from_12)
      sweep_from_12 = ts4231_12.get_sweep_pulse(&sweep_pulse_data_12);
    if (!sweep_from_14)
      sweep_from_14 = ts4231_14.get_sweep_pulse(&sweep_pulse_data_14);

    if (sweep_from_12 && sweep_from_14)
      break;
  }

  bool is_y;
  uint32_t sync_pulse;

  if (sync_from_12) {
    is_y = sync_pulse_data_12.axis;
    sync_pulse = sync_pulse_data_12.sync_pulse.pulse_end_time;
  }
  else {
    is_y = sync_pulse_data_14.axis;
    sync_pulse = sync_pulse_data_14.sync_pulse.pulse_end_time;
  }
  
  if (is_y) {
    uint32_t t12 = sweep_pulse_data_12.sweep_pulse.pulse_start_time - sync_pulse;
    uint32_t t14 = sweep_pulse_data_14.sweep_pulse.pulse_start_time - sync_pulse;

    rawY[0] = in_range(t12, sweepStartTicks, sweepEndTicks) ? t12 : rawY[0];
    rawY[1] = in_range(t14, sweepStartTicks, sweepEndTicks) ? t14 : rawY[1];
  }
  else {
    uint32_t t12 = sweep_pulse_data_12.sweep_pulse.pulse_start_time - sync_pulse;
    uint32_t t14 = sweep_pulse_data_14.sweep_pulse.pulse_start_time - sync_pulse;

    rawX[0] = in_range(t12, sweepStartTicks, sweepEndTicks) ? t12 : rawX[0];
    rawX[1] = in_range(t14, sweepStartTicks, sweepEndTicks) ? t14 : rawX[1];
  }

  for (int i = 0; i < 2; i++) {
    xAngle[i] = mapf(rawX[i], sweepStartTicks, sweepEndTicks, 0, 120);
    yAngle[i] = mapf(rawY[i], sweepStartTicks, sweepEndTicks, 0, 120);
  }

  Serial.printf("X12: %f, X14: %f, Y12: %f, Y14: %f\n", xAngle[0], xAngle[1], yAngle[0], yAngle[1]);

  sync_from_12 = false;
  sync_from_14 = false;

  sweep_from_12 = false;
  sweep_from_14 = false;
}