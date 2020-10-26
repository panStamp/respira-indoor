/**
 * Copyright (c) 2020 panStamp <contact@panstamp.com>
 * 
 * This file is part of the RESPIRA project.
 * 
 * panStamp  is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 * 
 * panStamp is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with panStamp; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301
 * USA
 * 
 * Author: Daniel Berenguer
 * Creation date: Sep 14 2020
 */

#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager

#include <Wire.h>
#include <SparkFun_Si7021_Breakout_Library.h>

#include "config.h"
#include "mqttclient.h"

/**
 * Pin definition
 */
#define LED          2
#define CO2_PWM_PIN  14

// Sensor object
Weather si7021;

// Wifi manager
WiFiManager wifiManager;
const char wmPassword[] = "panstamp";

// Device MAC address
char deviceMac[16];

// Description string
char deviceId[32];

/**
 * MQTT client
 */
MQTTCLIENT mqtt(MQTT_BROKER, MQTT_PORT);

/**
 * MQTT topics
 */
char systemTopic[64];
char networkTopic[64];
char controlTopic[64];

// Timestamp of last sample
uint32_t lastSamplingTime = 0;

// WiFi manager custom e-mail field
WiFiManagerParameter emailField("email", "e-mail", "", 64,"placeholder=\"your e-mail address\" type=\"email\"");

// Config space
CONFIG cfg;

/**
 * Registered CO2 values
 */
const uint16_t NUM_CO2_REGS = TX_INTERVAL / SAMPLING_INTERVAL;
uint16_t regCo2[NUM_CO2_REGS];
uint16_t numCo2Regs = 0;


/**
 * Restart board
 */
void restart(void)
{
  Serial.println("Restarting system");
  
  // Restart ESP
  ESP.restart();  
}

/**
 * WiFi manager completed
 */
void saveConfigCallback ()
{
  cfg.saveEmail(emailField.getValue());
  restart();
}

/**
 * MQTT packet received
 * 
 * @param topic MQTT topic
 * @param MQTT payload
 */
void mqttReceived(char *topic, char *payload)
{
  Serial.print("MQTT command received: "); Serial.println(payload);

  // Process command
  if (!strcasecmp(payload, "restart"))
    restart();
  if (!strcasecmp(payload, "factory-reset"))
  {
    wifiManager.resetSettings();
    restart();
  }
}

/**
 * Register CO2 value
 * 
 * @return true in case of success
 */
bool registerCo2(void)
{
  // Measurements
  static uint32_t co2 = 0;
  uint8_t tries = 0;    
  uint32_t th = 0;
  
  while ((th = pulseIn(CO2_PWM_PIN, HIGH)) == 0)
  {
    delay(1000);
    if ((++tries) == 10)
    {
      mqtt.publish(systemTopic, "No response from CO2 sensor");
      return false;
    }
  }
 
  co2 = CO2_PPM_RANGE/1000 * th - 2*CO2_PPM_RANGE;
  co2 /= 1000; // ppm

  // Register CO2 reading
  regCo2[numCo2Regs++] = co2;

  Serial.print("CO2 (ppm): "); Serial.println(co2);

  return true;
}

/**
 * Get computed CO2 value
 */
uint16_t getCo2(void)
{
  uint16_t min = 0xFFFF;
  uint16_t max = 0;
  uint16_t indMin = 0, indMax = 0;
  uint32_t sum = 0;

  // Detect minimum and maximum values
  for(uint16_t i=0 ; i<numCo2Regs ; i++)
  {
    if (regCo2[i] < min)
    {
      min = regCo2[i];
      indMin = i;
    }
    else if (regCo2[i] > max)
    {
      max = regCo2[i];
      indMax = i;
    }    
  }

  // Calculate mean, not including minimum and maximum values
  for(uint16_t i=0 ; i<numCo2Regs ; i++)
  {
    if ((i != indMin) && (i != indMax))
      sum += regCo2[i];
  }

  // Return mean
  uint16_t mean = (uint16_t) sum / (numCo2Regs-2);
  numCo2Regs = 0;

  return mean;
}

/**
 * Transmit MQTT packet with measurements
 */
void mqttTransmit(void)
{
  // Read temperature and humidity
  float temperature = si7021.getTemp();
  float humidity = si7021.getRH();
     
  // Get computed CO2 value
  uint16_t co2 = getCo2();

  digitalWrite(LED, LOW);

  Serial.print("Temperature (ºC): "); Serial.println(temperature);
  Serial.print("Humidity (%): "); Serial.println(humidity);
  Serial.print("Computed CO2 (ppm): "); Serial.println(co2);

  char json[256];
  sprintf(json, "{\"owner\":\"%s\",\"device\":\"%s\",\"temperature\":%.2f,\"humidity\":%.2f,\"co2\":%d}",
        cfg.getEmail(), deviceMac, temperature, humidity, co2);
          
  mqtt.publish(networkTopic, json);
    
  digitalWrite(LED, HIGH);
}

/**
 * Arduino setup function
 */
void setup()
{
  // Config LED pin
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);

  // Config CO2 PWM pin
  pinMode(CO2_PWM_PIN, INPUT);
  
  Serial.begin(115200);
  Serial.println("Starting...");

  // Get MAC
  uint8_t mac[WL_MAC_ADDR_LENGTH];
  WiFi.softAPmacAddress(mac);
  sprintf(deviceMac, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  // Set device ID
  sprintf(deviceId, "%s %s", APP_NAME, deviceMac);

  // Initialize config space
  cfg.begin();

  // Fill MQTT topics
  sprintf(systemTopic, "%s/%s/%s/system", MQTT_MAIN_TOPIC, cfg.getEmail(), deviceMac);
  sprintf(networkTopic, "%s/%s/%s/network", MQTT_MAIN_TOPIC, cfg.getEmail(), deviceMac);
  sprintf(controlTopic, "%s/%s/%s/control", MQTT_MAIN_TOPIC, cfg.getEmail(), deviceMac);  
  
  digitalWrite(LED, LOW);
  
  //wifiManager.resetSettings();
  // WiFi Manager timeout
  wifiManager.setConfigPortalTimeout(300);

  // WiFi manager custom e-mail field
  wifiManager.addParameter(&emailField);
  
  // Set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  // WiFi Manager autoconnect
  if (!wifiManager.autoConnect(deviceId))
  {
    Serial.println("failed to connect and hit timeout");
    restart();
    delay(1000);
  }
  else
  {
    Serial.println("");
    Serial.print("MAC address: ");
    Serial.println(deviceMac);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("e-mail address: ");
    Serial.println(cfg.getEmail());
  }

  // MQTT subscription
  mqtt.subscribe(controlTopic);
  mqtt.attachInterrupt(mqttReceived);  

  // Connect to MQTT server
  Serial.println("Connecting to MQTT broker");
  if (mqtt.begin(deviceMac))
  {
    Serial.println("Connected to MQTT broker");
    mqtt.publish(systemTopic, "connected");
  }
 
  // Initialize sensor
  si7021.begin();

  digitalWrite(LED, HIGH);
}

/**
 * Endless loop
 */
void loop()
{
  if (!lastSamplingTime || ((millis() - lastSamplingTime) >= SAMPLING_INTERVAL))
  {
    lastSamplingTime = millis();

    // Register CO2 value
    registerCo2();

    // Send MQTT packet containing measurements once the CO2 registers are completed
    if (numCo2Regs == NUM_CO2_REGS)
      mqttTransmit();    
  }
  else
    mqtt.handle();
}
