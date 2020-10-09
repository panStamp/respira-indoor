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

#include "mqttclient.h"

/**
 * Pointer to custom subscription function
 */
void (*subscriptionFunct)(char *topic, char *payload) = NULL;
    
/**
 * mqttReceive
 * 
 * Function called whenever a MQTT packet is received
 * 
 * @param topic MQTT topic
 * @param payload message
 * @param len message length
 */
void mqttReceive(char* topic, uint8_t* payload, unsigned int len)
{ 
  if (subscriptionFunct != NULL)
  {
    payload[len] = 0;
    subscriptionFunct(topic, (char*)payload);
  }
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
bool MQTTCLIENT::begin(char *id)
{
  strcpy(clientId, id);

  // Connect to MQTT broker
  client.setServer(broker, port);

  if (subscriptionFunct != NULL)
  {
    // Call this function whenever a MQTT message is received
    client.setCallback(mqttReceive);
  }

  // Handle connection
  return handle();
}

/**
 * attachInterrupt
 * 
 * Declare custom ISR, to be called whenever a MQTT packet is received
 * 
 * @param funct pointer to the custom function
 */
void MQTTCLIENT::attachInterrupt(void (*funct)(char*, char*))
{
  subscriptionFunct = funct;
}
