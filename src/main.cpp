#define CONFIG_ESP_TIMER_INTERRUPT_LEVEL 3
#include <Arduino.h>
#include <math.h>

#define AXIS_X 0
#define AXIS_Y 1

#include "pulse_mcpwm.h"

void setup()
{
  Serial.begin(115200);
  while (!Serial)
    ; // wait for serial port to connect

  pinMode(14, INPUT);
  pulseSetup();
}

int rawX, rawY;
int x = 0, y = 0;

float xAngle = 0, yAngle = 0;
float prevX = 0.f, prevY = 0.f;

pulse_data_t pulse_data;

float mapf(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

float clampf(float x, float min, float max)
{
  if (x < min)
    return min;
  if (x > max)
    return max;
  return x;
}

void loop()
{
  if (pulseDataIsReady(pulse_data))
  {
    if (pulse_data.axis == AXIS_X) {
      rawX = pulse_data.sweep_pulse.pulse_start_time - pulse_data.sync_pulse.pulse_end_time;
    } 
    else {
      rawY = pulse_data.sweep_pulse.pulse_start_time - pulse_data.sync_pulse.pulse_end_time;
    }
  }

  if (rawX < 96960 || 542960 <= rawX)
    xAngle = prevX;
  else
    xAngle = mapf(rawX, 96960, 542960, 0, 120);

  if (rawY < 96960 || 542960 <= rawY)
    yAngle = prevY;
  else
    yAngle = mapf(rawY, 96960, 542960, 0, 120);

  Serial.printf("x: %f, y: %f\n", xAngle, yAngle);

  prevX = xAngle;
  prevY = yAngle;
}