#pragma once

#include <Arduino.h>
#include <Wire.h>

// Define which sensor the unit is connected to.
// Please see https://github.com/curiouselectric/RS485InterfaceBoard for list of options
#define NUM_CHANNELS    2    // This will depends upon the sensor

// Hardware Definitions
#define SOFTWARE_VERSION  "1.0" // Defines which version of software - to control updates if needed   

#define LED0_PIN          7     // Indication LED
#define SWA_PIN           8     // On board user switch (can also be called by serial command)


#define HC12_PWR_PIN      4     // This is a power output for the HC12 unit - set to LED0 at present
#define POWER_SETTLE_TIME 50    // Time in mS to wait after power is applied before sending serial data and time to wait after data set before switching off power.


#define LED_FLASH_TIME    5000    // time in mS between LED flashes in normal operation
#define LED_ON_TIME       100      // time in mS of the flash length
#define LED_BLINK_TIME    250     // Time (in mS) between LED flashes when mode changes


// Serial Communication control:
#define MAX_BAUD_RATES      5       // Serial comms baud rates available are: (0) 1200, (1) 24800, (2)(Default) 9600, (3) 57600, (4) 115200 
const long int baud_rates[MAX_BAUD_RATES] = { 1200, 4800, 9600, 57600, 115200 };  // This holds the available baud rates
#define EEPROM_SERIAL_BAUD  4       // EEPROM location of serial baud rate value

// ******** TO DO add CRC check as a #def ***************************************************************
#define ADD_CRC_CHECK         false        // Use this to add CRC check to incomming and outgoing messages

// Sending data info
#define EEPROM_SEND_DATA      120    // EEPROM location of this value
#define INPUT_STRING_LENGTH   25
#define RETURN_STRING_LENGTH  100
#define DELIMITER             ":"

#define DEBUG_FLAG        true   // Use this for debugging. Not need for roll out.
//#define DEBUG_DATA_1S     // Use this for debugging. Not need for roll out.
//#define DEBUG_DATA_10S    // Use this for debugging. Not need for roll out.
//#define DEBUG_DATA_60S    // Use this for debugging. Not need for roll out.
//#define DEBUG_DATA_600S   // Use this for debugging. Not need for roll out.
//#define DEBUG_DATA_3600S  // Use this for debugging. Not need for roll out.
                      
// These three digital pins are for the Device ID selection:
#define  GPIO_ID0      A2  //  A0 Digital 4 on Arduino
#define  GPIO_ID1      A1  //  A1 Digital 5 on Arduino
#define  GPIO_ID2      A0  //  A3 Digital 6 on Arduino

// I2C Options
// I2C ADC is on standard I2C Pins: A4 (SDA) and A5 (SCL)
// #define USE_I2C



// // Definitions for Wind Speed and Wind Vane Sensor version
// #define FREQ_PIN          2
// #define VANE_PIN          A3
// #define DEBOUNCE_DELAY    5       // debounce delay time in milli-Seconds This also gives max pulses - 5mS = 200 pulses per second   

// #define EEPROM_WIND_CON_M  100    // EEPROM location of this value
// #define EEPROM_WIND_CON_C  110    // EEPROM location of this value
