/**
 * Copyright (c) 2020 panStamp S.L.U
 * 
 * This file is part of the mactracker-sim7600 project.
 * 
 * Author: Daniel Berenguer
 * Creation date: Sep 14 2020
 */

#include <EEPROM.h>

/**
 * Application name
 */
const char APP_NAME[] = "respira-indoor";

/**
 * CO2 sampling interval
 */
const uint32_t SAMPLING_INTERVAL = 10000;

/**
 * Transmission interval (msec)
 */
const uint32_t TX_INTERVAL = 300000;

/**
 * CO2 maximum range in ppm
 */
const uint16_t CO2_PPM_RANGE = 5000;

/**
 * MQTT broker settings
 */
const char MQTT_BROKER[] = "mqtt-respira-indoor.inubo.es";
const uint16_t MQTT_PORT = 1883;
const char MQTT_USERNAME[] = "";
const char MQTT_PASSWORD[] =  "";

const char MQTT_MAIN_TOPIC[] = "respira-indoor";


/**
 * Custom EEPROM addresses
 */
#define EEPROM_EMAIL_ADDR  0
#define EEPROM_EMAIL_SIZE  64
#define EEPROM_SIZE        EEPROM_EMAIL_SIZE

/**
 * Custom parameters to be saved in non-volatile space
 */
class CONFIG
{
  private:
    /**
     * e-mail address
     */
    char email[EEPROM_EMAIL_SIZE];

    /**
     * Read e-mail address from non-volatile space
     * 
     * @param email : e-mail address returned
     * 
     * @return number of bytes read
     */
    inline uint8_t readEmail(char *email)
    {      
      uint8_t i = 0;
      uint8_t b = 1;
      while(b != 0)
      {
        b = EEPROM.read(EEPROM_EMAIL_ADDR + i);
        email[i] = (char) b;
        if (b == 0)
          return i;
        i++;
      }
      return 0;
    }     

  public:
    /**
     * Initialize config space
     */
    inline void begin(void)
    {
      EEPROM.begin(EEPROM_SIZE);
      readEmail(email);
    }

    /**
     * Save e-mail address to non-volatile space
     * 
     * @param email : e-mail address to be saved
     */
    inline void saveEmail(const char *email)
    {
      bool stop = false;
      uint8_t i = 0;
      while(!stop)
      {
        EEPROM.write(EEPROM_EMAIL_ADDR + i, (uint8_t) email[i]);

        if (email[i] == 0)
          stop = true;
        i++;
      }
      EEPROM.commit();
    }

    /**
     * Get e-mail address
     * 
     * @return pointer to the e-mail address
     */
    char * getEmail(void)
    {
      return email;
    }
};
