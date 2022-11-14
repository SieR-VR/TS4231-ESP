#include <Arduino.h>
#include <math.h>

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include "ts4231_mcpwm.h"

#define TICKS_PER_MICROSEC (80)
#include "ts4231.h"

// hw_timer_t *timer = NULL;
// TS4231_Interrupt ts4231(14);

const char *ssid = "Asus_Router_2.4G";
const char *password = "1810022a+";

uint8_t packet[17] = {0};

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

uint32_t mcpwm_pins[4] = {12, 14, 27, 26};
TS4231_MCPWM *ts4231s[4];
sync_pulse_data_t sync_pulse_data[4];
sweep_pulse_data_t sweep_pulse_data[4];
bool sync_from[4] = {false, false, false, false};
bool sweep_from[4] = {false, false, false, false};

void setup()
{
  Serial.begin(115200);

  for (int i = 0; i < 4; i++)
  {
    ts4231s[i] = new TS4231_MCPWM(mcpwm_pins[i], i);
    ts4231s[i]->init();
  }

  Serial.println("TS4231 Setup complete");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  ws.onEvent([](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT)
    {
      Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());
    }

    if (type == WS_EVT_DATA)
    {
      client->binary(packet, 17);
    }
  });
  server.addHandler(&ws);
  server.begin();
}

bool every(bool *arr, size_t len)
{
  for (int i = 0; i < len; i++)
  {
    if (!arr[i])
      return false;
  }
  return true;
}

void loop()
{ 
  while (true)
  {
    bool sync_ready = false;

    for (int i = 0; i <  3; i++)
    {
      sync_from[i] = ts4231s[i]->get_sync_pulse(&sync_pulse_data[i]);
      if (sync_from[i])
      {
        sync_ready = true;
        break;
      }
    }

    if (sync_ready)
      break;
  }

  sync_from[3] = ts4231s[3]->get_sync_pulse(&sync_pulse_data[3]); // 4th sensor has different timer

  int64_t sync_pulse_timestamp = esp_timer_get_time();

  while ((esp_timer_get_time() - sync_pulse_timestamp) < sweepEndTime)
  {
    for (int i = 0; i < 4; i++)
    {
      if (!sweep_from[i])
        sweep_from[i] = ts4231s[i]->get_sweep_pulse(&sweep_pulse_data[i]);
    }

    if (every(sweep_from, 4))
      break;
  }

  bool is_y;
  uint32_t sync_pulse_mcpwm_0 = 0;
  uint32_t sync_pulse_mcpwm_1 = 0;

  for (int i = 0; i < 3; i++)
  {
    if (sync_from[i])
    {
      sync_pulse_mcpwm_0 = sync_pulse_data[i].sync_pulse.pulse_start_time + interSyncTicks;
      is_y = sync_pulse_data[i].axis;
      break;
    }
  }

  if (sync_from[3])
    sync_pulse_mcpwm_1 = sync_pulse_data[3].sync_pulse.pulse_start_time + interSyncTicks;

  packet[0] = is_y ? 1 : 0;
  if (sweep_from[0])
    *(uint32_t *)&packet[1] = sweep_pulse_data[0].sweep_pulse.pulse_start_time - sync_pulse_mcpwm_0;
  if (sweep_from[1])
    *(uint32_t *)&packet[5] = sweep_pulse_data[1].sweep_pulse.pulse_start_time - sync_pulse_mcpwm_0;  
  if (sweep_from[2])
    *(uint32_t *)&packet[9] = sweep_pulse_data[2].sweep_pulse.pulse_start_time - sync_pulse_mcpwm_0;
  if (sweep_from[3])
    *(uint32_t *)&packet[13] = sweep_pulse_data[3].sweep_pulse.pulse_start_time - sync_pulse_mcpwm_1;

  for (int i = 0; i < 4; i++)
  {
    sync_from[i] = false;
    sweep_from[i] = false;
  }
}