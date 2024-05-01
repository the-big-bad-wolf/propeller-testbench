#include <Arduino.h>
#include "motor.h"
#include "HX711.h"
#include <WiFi.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <stdio.h>
#include <time.h>

// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 4;
const int LOADCELL_SCK_PIN = 5;

HX711 scale;

// Pins
#define BLDC1_PIN 6
#define BLDC2_PIN 7

// Frequencies
#define BLDC_FREQ 50

// LEDC channels esp-hal uses modulo 4 to select timers for different channels, therefore BLDC is the only odd channel
#define BLDC1_CHAN 1
#define BLDC2_CHAN 2

// Calibration values
#define CURRENT_GAIN 0.0289
#define CURRENT_OFFSET 2.7182
#define VOLTAGE_GAIN 0.0148
#define VOLTAGE_OFFSET 0.922
#define WEIGHT_GAIN -0.0040870148
#define WEIGHT_OFFSET -44.4443418099
#define WEIGHT_CALIBRATION -1249022.0 / 5000.0

// Motor structs setup
Motor motor1{BLDC1_PIN, 0, BLDC1_CHAN, 0, BLDC_FREQ, MOTOR_TYPE_BLDC};
Motor motor2{BLDC2_PIN, 0, BLDC2_CHAN, 0, BLDC_FREQ, MOTOR_TYPE_BLDC};

// Variables to store motor speeds
int motor1_speed = 0;
int motor2_speed = 0;
float target_wattage = 0;

typedef struct ForceMeasurement
{
  long time;
  float force;
} ForceMeasurement;

int run_benchmark = 0;
int benchmark_duration;
time_t benchmark_start;
time_t current_time;
int iteration = 0;

uint16_t sensor_current = 0;
uint16_t sensor_voltage = 0;
float current = 0;
float voltage = 0;

WebSocketsServer webSocket = WebSocketsServer(81);
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
  switch (type)
  {
  case WStype_DISCONNECTED:
    Serial.printf("[%u] Disconnected!\n", num);
    motor1_speed = -127;
    motor2_speed = -127;
    setMotorSpeed(-127, motor1);
    setMotorSpeed(-127, motor2);
    scale.power_down(); // put the ADC in sleep mode
    run_benchmark = 0;
    iteration = 0;
    break;
  case WStype_CONNECTED:
  {
    IPAddress ip = webSocket.remoteIP(num);
    Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
  }
  break;
  case WStype_TEXT:
  {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error)
    {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }

    if (doc.containsKey("command"))
    {
      const char *command = doc["command"];
      if (strcmp(command, "stop") == 0)
      {
        motor1_speed = -127;
        motor2_speed = -127;
        setMotorSpeed(-127, motor1);
        setMotorSpeed(-127, motor2);
        scale.power_down(); // put the ADC in sleep mode
        run_benchmark = 0;
        iteration = 0;
      }
      else if (strcmp(command, "start") == 0)
      {
        run_benchmark = 1;
        scale.power_up(); // Wake up the ADC
      }
      Serial.printf("[%u] get command: %s\n", num, command);
    }

    if (doc.containsKey("target_wattage"))
    {
      motor1_speed = -50;
      motor2_speed = -50;
      target_wattage = doc["target_wattage"];
      setMotorSpeed(motor1_speed, motor1);
      setMotorSpeed(motor2_speed, motor2);
      Serial.printf("[%u] get target_wattage: %d\n", num, target_wattage);
    }

    if (doc.containsKey("benchmark_duration"))
    {
      benchmark_duration = doc["benchmark_duration"];
      Serial.printf("[%u] get duration: %d\n", num, benchmark_duration);
    }

    // webSocket.sendTXT(num, "message here");
    benchmark_start = time(NULL);
  }
  break;
  case WStype_BIN:
    Serial.printf("[%u] get binary length: %u\n", num, length);
    // send message to client
    // webSocket.sendBIN(num, payload, length);
    break;
  case WStype_ERROR:
  case WStype_FRAGMENT_TEXT_START:
  case WStype_FRAGMENT_BIN_START:
  case WStype_FRAGMENT:
  case WStype_FRAGMENT_FIN:
    break;
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Initializing the scale");

  // Initialize library with data output pin, clock input pin and gain factor.
  // Channel selection is made by passing the appropriate gain:
  // - With a gain factor of 64 or 128, channel A is selected
  // - With a gain factor of 32, channel B is selected
  // By omitting the gain factor parameter, the library
  // default "128" (Channel A) is used here.
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(); // this value is obtained by calibrating the scale with known weights; see the README for details
  scale.tare();      // reset the scale to 0
  scale.set_scale(WEIGHT_CALIBRATION);
  scale.power_down(); // put the ADC in sleep mode

  const char *ssid = "";
  const char *password = "";

  WiFi.mode(WIFI_STA); // Optional
  WiFi.begin(ssid, password);
  Serial.println("\nConnecting");

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(100);
  }

  Serial.println("\nConnected to the WiFi network");
  Serial.print("Local ESP32 IP: ");
  Serial.println(WiFi.localIP());

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  motorInit(motor1);
  motorInit(motor2);
  delay(8000);
}

ForceMeasurement force_measurements[1];

void loop()
{
  webSocket.loop();
  current_time = time(NULL);
  sensor_voltage = analogRead(9);
  sensor_current = analogRead(10);
  printf("Voltage: %d, Current: %d\n", sensor_voltage, sensor_current);
  voltage = sensor_voltage * VOLTAGE_GAIN + VOLTAGE_OFFSET; // Convertion to volts using values measured during calibration
  current = sensor_current * CURRENT_GAIN + CURRENT_OFFSET; // Convertion to amps using values measured during calibration

  if (run_benchmark == 0)
  {
    static unsigned long lastUpdateTime = 0;
    if (current_time - lastUpdateTime >= 1)
    {
      lastUpdateTime = current_time;
      JsonDocument doc;
      doc["voltage"] = voltage;
      doc["current"] = current;
      String serialized_json;
      serializeJson(doc, serialized_json);
      webSocket.broadcastTXT(serialized_json);
    }
    return;
  }
  else if (current_time - benchmark_start > benchmark_duration)
  {
    printf("Benchmark finished\n");
    webSocket.broadcastTXT("Benchmark finished");
    setMotorSpeed(-127, motor1);
    setMotorSpeed(-127, motor2);
    scale.power_down(); // put the ADC in sleep mode
    run_benchmark = 0;
    return;
  }

  if (voltage * current > target_wattage * 1.02)
  {
    motor1_speed -= 1;
    motor2_speed -= 1;
    if (motor1_speed < -127)
    {
      motor1_speed = -127;
      motor2_speed = -127;
    }
  }
  else if (voltage * current < target_wattage * 0.98)
  {
    motor1_speed += 1;
    motor2_speed += 1;
  }
  {
    if (motor1_speed > 127)
    {
      motor1_speed = 127;
      motor2_speed = 127;
    }
  }
  setMotorSpeed(motor1_speed, motor1);
  setMotorSpeed(motor2_speed, motor2);

  ForceMeasurement force_measurement = {
      time(NULL) - benchmark_start,
      scale.get_units(1),
  };
  force_measurements[iteration] = force_measurement;

  iteration++;
  if (iteration > sizeof(force_measurements) / sizeof(force_measurements[0]) - 1)
  {
    iteration = 0;
    JsonDocument doc;
    JsonArray measurement_array = doc["force_measurements"].to<JsonArray>();
    for (size_t i = 0; i < sizeof(force_measurements) / sizeof(force_measurements[0]); i++)
    {
      JsonObject nested_object = measurement_array.add<JsonObject>();
      nested_object["time"] = force_measurements[i].time;
      nested_object["force"] = force_measurements[i].force;
    }

    doc["voltage"] = voltage;
    doc["current"] = current;

    String serialized_json;
    serializeJson(doc, serialized_json);
    webSocket.broadcastTXT(serialized_json);
  }
}
