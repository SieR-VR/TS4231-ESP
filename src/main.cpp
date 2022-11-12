#define CONFIG_ESP_TIMER_INTERRUPT_LEVEL 3
#include <Arduino.h>
#include <math.h>

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
  if (pulseDataIsReady())
  {
    if (pulse_data.isReady)
    {
      if (pulse_data.axis == 0)
      {
        rawX = (pulse_data.sweep_capture - pulse_data.pulse_capture);
      }
      else
      {
        rawY = (pulse_data.sweep_capture - pulse_data.pulse_capture);
      }
    }
    armSyncPulse();
  }

  if (1222 > rawX || rawX >= 6787)
    xAngle = prevX;
  else
    xAngle = mapf(rawX, 1222, 6787, 0, 120);

  if (1222 > rawY || rawY >= 6787)
    yAngle = prevY;
  else
    yAngle = mapf(rawY, 1222, 6787, 0, 120);

  Serial.printf("x: %f, y: %f\n", xAngle, yAngle);

  prevX = xAngle;
  prevY = yAngle;
}