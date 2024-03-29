/* ESP-01
 * TTL - only 5V incoming. If transmitting from Arduino use divider 1000 Ohm / 2000 Ohm.
 * CH_P and RST should be high.
 * Programming - raise GPIO0 and short low on RST.s
 * GPIO0 and GPIO2 should be high during boot from flash https://www.instructables.com/ESP8266-Using-GPIO0-GPIO2-as-inputs/
 * 
 * Pins from front of the chip:
 * RXD   VCC
 * GPIO0 RST
 * GPIO2 CH_P
 * GND   TXD
 * (<- chip   end->) https://arduino.stackexchange.com/questions/52255/right-pin-to-activate-a-wireless-relay-with-esp8266-esp-01
 * mac driver https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers?tab=downloads
 * or https://ftdichip.com/drivers/vcp-drivers/
 * 
 * How flash Node MCU via Arduino Serial
 * Flash Arduino with "Blink" example (no Serial output from Arduino this way)
 * Connect Arduino's TX->1 to MCU TX via 2KOhm 1KOhm divider (grounded)
 * likewise RX->1 to RX (no divider needed)
 * Connect Arduino and MCU ground
 * Press "Flash" button on MCU constntly and "Reset" once
 * 
 */
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include <Arduino.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include "index_html.h"

#include "secrets.h"
#include "settings.h"

#ifdef BATTERY_VOLTAGE
ADC_MODE(ADC_VCC);
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

AsyncWebServer server(80);
String bufferString;

int int_data[DATA_LEN_BINS];
const int data_start_index = 1; // the first value is 0
int data_len = data_start_index;

int state = HIGH;
unsigned long stateTimestampMillis = 0;
unsigned long duration = 0;

const unsigned long durations_len = 128;
unsigned long durations[durations_len];

unsigned long ledTimestamp = 0;

bool powerSaving = false;
bool wifiOn = true;
unsigned long powerSavingTimestampDelta = 0;

void WiFiOn() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
      delay(5000);
      WiFiOff();
      WiFiOn();
  }
  wifiOn = true;
}

void WiFiOff() {
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  wifiOn = false;
}

// https://gist.github.com/chaeplin/be4d9ca41a991aa327bb
// https://arduino-esp8266.readthedocs.io/en/latest/libraries.html
// ESP.deepSleepInstant(microseconds, mode) WAKE_RF_DEFAULT WAKE_RF_DISABLED
// -WAKE_RF_DEFAULT(aka deepsleepsetoption0): do or not do the radio calibration depending on the init byte 108.
// -WAKE_RFCAL(aka deepsleepsetoption1): do the radio calibration every time.-WAKE_RF_DEFAULT(aka deepsleepsetoption0): do or not do the radio calibration depending on the init byte 108.

// https://arduino.stackexchange.com/questions/43376/can-the-wifi-on-esp8266-be-disabled
// wifi.setmode(mode[, save])  mode wifi.STATION wifi.NULLMODE

void setup() {
  memset(int_data, 0 , sizeof(int_data));
  //pinMode(0, OUTPUT);
  //digitalWrite(0, LOW);

  pinMode(LED_BUILTIN, OUTPUT);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    delay(5000);
    ESP.restart();
  }

  ArduinoOTA.setHostname(OTA_HOSTNAME);
  //if (strlen(OTA_PASSWORD) > 0) ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.begin();

  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(200, "text/html", index_html);
  });
  server.on("/impulse_per_kwh", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", IMPULSE_PER_KWH);
  });
  server.on("/data", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", int_data_to_string());
  });
  server.on("/impulse", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", collect());
  });
  server.on("/pin", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", (digitalRead(SENSOR_PIN) == HIGH) ? "HIGH" : "LOW");
  });
  server.on("/battery", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", battery_percent());
  });
  server.on("/durations", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", durations_str());
  });
  server.on("/wifi_signal", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", wifi_signal());
  });
  server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", reboot());
  });
  server.on("/bin_seconds", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", BIN_SECONDS);
  });
  server.on("/power_saving_on", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", power_saving_on());
  });
  server.on("/power_saving_off", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", power_saving_off());
  });

  server.begin();
}

const char* power_saving_on() {
  powerSavingTimestampDelta = millis() % (POWER_SAVING_INTERVAL * 1000);
  powerSaving = true;
  bufferString = "power_saving_on";
  return bufferString.c_str();
}

const char* power_saving_off() {
  powerSaving = false;
  bufferString = "power_saving_off";
  return bufferString.c_str();
}

const char* reboot() {
  ESP.reset();
  return ""; // it will not be returned anyway
}

const char* battery_percent() {
  #ifdef BATTERY_VOLTAGE
  float percent = 100.0 * (ESP.getVcc() - BATTERY_0_PERCENT_VOLTAGE) / (BATTERY_100_PERCENT_VOLTAGE - BATTERY_0_PERCENT_VOLTAGE);
  bufferString = String(int(percent));
  #endif
  return bufferString.c_str();
}

const char* collect() {
  String res = "";
  for (int i = 0; i < 1000 ; i++ ) {
    int v = digitalRead(SENSOR_PIN);  // HIGH or LOW
    res.concat((v == HIGH) ? "1" : "0");
    delay(1);
  }
  String ones   = (res.indexOf("1") == -1) ? "" : "ones ";
  String zeroes = (res.indexOf("0") == -1) ? "" : "zeroes ";
  bufferString = zeroes + ones + res;
  return bufferString.c_str();
}

const char* int_data_to_string() {
  bufferString = "";
  for (int i = 0; i < data_len ; i++ )
    bufferString.concat(String(int_data[i]) + " ");
  bufferString.remove(bufferString.length() - 1); // drop last space
  return bufferString.c_str();
}

const char* durations_str() {
  bufferString = "";
  for (int i = 0; i < durations_len ; i++ )
    bufferString.concat(String(i) + " " + String(durations[i]) + "\n");
  bufferString.remove(bufferString.length() - 1); // drop last space
  return bufferString.c_str();
}

const char* wifi_signal() {
  int v = WiFi.RSSI(); // RSSI from -120 (worst) to 0 (best)
  v = round((v + 120) / 1.2);
  bufferString = String(v) + '%';
  return bufferString.c_str();
}

void loop() {
  unsigned long currentMillis = millis();

  if (!powerSaving && !wifiOn) WiFiOn();

  if (powerSaving) {
    if ( (currentMillis - powerSavingTimestampDelta / 1000) % POWER_SAVING_INTERVAL > POWER_SAVING_ACTIVE ) { // wifi off
      if (wifiOn) WiFiOff();
    } else { // wifi on
      if (!wifiOn) WiFiOn();
    }
  }

  // OTA
  ArduinoOTA.handle();

  // LED
  digitalWrite(LED_BUILTIN, (ledTimestamp < currentMillis) ? HIGH : LOW); // led pin is reversed? looks like that's the case https://forum.arduino.cc/t/led-builtin-values-are-reversed/1101541

  // current minute
  int new_data_len = currentMillis / 1000 / 60 + data_start_index;
  if (new_data_len != data_len)
    data_len = new_data_len;

  // state change
  int v = digitalRead(SENSOR_PIN);  // HIGH or LOW
  if (v != state) {
      state = v;
      int impulse_len = 1;
      if (v == LOW) {
        for (int i=0; i<50 ; i++) {
          delay(1);
          v = digitalRead(SENSOR_PIN);
          state = v;
          if (v == LOW) {
            impulse_len++;
          }
        }
        durations[impulse_len]++;
        if (impulse_len>10) {
          int_data[data_len]++;
          ledTimestamp = currentMillis + LED_BLINK_MSEC;
        }
      }
      return;
      duration = currentMillis - stateTimestampMillis;
      stateTimestampMillis = currentMillis;
      if (v == HIGH && duration > DEBOUNCE_MIN_MSEC) {
        int_data[data_len]++;
        durations[min(duration, durations_len - 1)]++;
        ledTimestamp = currentMillis + LED_BLINK_MSEC;
      }
  }


  // GPIO0 / GPIO2 and GPIO15 pins     GPIO15 L on start

  // on power up, and during reset, these pins must be pulled up or down as required
  // to have the ESP8266 module start up in normal running mode
  // with light sensor GPIO2 is usually low so it is OK
  // https://www.instructables.com/ESP8266-Using-GPIO0-GPIO2-as-inputs/

  //int adcValue = analogRead(A0);  // A0 == ADC0
}
