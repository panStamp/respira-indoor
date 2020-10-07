/**
 * Copyright (c) 2020 panStamp S.L.U
 * 
 * This file is part of the mactracker-sim7600 project.
 * 
 * Author: Daniel Berenguer
 * Creation date: Sep 14 2020
 */

#ifndef _MQTT_CLIENT_H
#define _MQTT_CLIENT_H

#ifdef ESP32
#include <WiFi.h>
#else
#include <WiFiClient.h>
#endif
#include <PubSubClient.h>

#define  MQTT_TOPIC_LENGTH    64  // Max topic length
#define  LED_PIN              2

enum MQTT_EVENT
{
  MQTT_EVENT_NONE = 0,
  MQTT_EVENT_TIMEOUT,
  MQTT_EVENT_CONENCTED
};

/**
 * MQTTCLIENT
 */
class MQTTCLIENT
{
  private:
    /**
     * Wifi client
     */
    WiFiClient espClient;

    /**
     * MQTT client
     */
    PubSubClient client;

    /**
     * MQTT broker
     */
    char broker[64];
    
    /**
     * MQTT port
     */
    uint16_t port;

    /**
     * MQTT client ID
     */
    char clientId[32];

    /**
     * Subscription topic
     */
    char subscriptionTopic[MQTT_TOPIC_LENGTH];

    /**
     * Reconnect to MQTT broker
     *
     * @return event code
     */
    inline MQTT_EVENT reconnect(void)
    {
      uint8_t tries = 6;
      
      // Loop until we're reconnected
      while (!client.connected())
      {       
        // Attempt to connect
        if (client.connect(clientId)) // Anonymous connection to broker
        //if (client.connect(deviceId, mqtt_user, mqtt_password)) // Authenticated connection with broker
        {         
          // Subscribe to the main topic
          client.subscribe(subscriptionTopic);

          return MQTT_EVENT_CONENCTED;
        }
        else
        {
          if (tries-- == 0)
            return MQTT_EVENT_TIMEOUT;
            
          // Wait 5 seconds before retrying
          digitalWrite(LED_PIN, HIGH);
          delay(2500);
          digitalWrite(LED_PIN, LOW);
          delay(2500);
        }    
      }

      return MQTT_EVENT_NONE;
    }

  public:
    /**
     * Class constructor
     *
     * @param mqttBroker : MQTT broker URL
     * @param mqttPort : MQTT port
     */
    inline MQTTCLIENT(const char *mqttBroker, const uint16_t mqttPort=1883) : client(espClient)
    {
      strcpy(broker, mqttBroker);
      port = mqttPort;
    }

    /**
     * begin
     *
     * Start client
     *
     * @param id : client ID
     *
     * @return True in case of connection success
     */
    bool begin(char *id);

    /**
     * attachInterrupt
     * 
     * Declare custom ISR, to be called whenever a MQTT packet is received
     * 
     * @param funct pointer to the custom function
     */
    void attachInterrupt(void (*funct)(char*, char*));

    /**
     * subscribe
     * 
     * Subscribe to topic
     * 
     * @param topic : MQTT subscription topic
     */
    inline void subscribe(char *topic)
    {
      strcpy(subscriptionTopic, topic);
    }

    /**
     * handle
     * 
     * Handle MQTT client connectivity
     *
     * @return event code
     */
    inline MQTT_EVENT handle(void)
    {
      MQTT_EVENT ret = MQTT_EVENT_NONE;

      if (!client.connected())
        ret = reconnect();
       
      client.loop();

      return ret;     
    }

    /**
     * publish
     * 
     * Publish MQTT message
     * 
     * @param topic : MQTT topic
     * @param payload : MQTT payload
     */
    void publish(char *topic, char *payload)
    { 
      // Publish MQTT message
      client.publish(topic, payload);
    }
};

#endif
