#include "config.h"
#include <SoftwareSerial.h>

#ifdef SOLAR_RS485_SENSOR

 pyr20Sensor pyr20SensorRS485;

/****************************************************************************/
/***       Local Variable                                                 ***/
/****************************************************************************/
#define sensorBaudRate 4800

// Request frame for the soil sensor
const byte soilSensorRequestData[] = { 0x03, 0x00, 0x00, 0x00, 0x02 };
const byte soilSensorChangeID[] = { 0x06, 0x07, 0xD0, 0x00 };  // Need to add the new device ID to then end of this request and then calc CRC-16
byte soilSensorRequestNoCRC[6];
byte soilSensorRequest[8];
byte soilSensorResponse[9];  // Always expecting 9 bytes from this sensor for data

SoftwareSerial mod(RS485_RX_PIN, RS485_TX_PIN);  // Software serial for RS485 communication

/****************************************************************************/
/***       Class member Functions                                         ***/
/****************************************************************************/

void pyr20Sensor::begin(void) {
  /* Start Serial */
  mod.begin(sensorBaudRate);  // Initialize software serial communication at 4800 baud rate
}

bool pyr20Sensor::readIrradiance(byte ID) {
  bool returnData = false;

  return returnData;
}

void pyr20Sensor::changeID(byte IDold, byte IDnew) {
  // To change the ID
 
}

unsigned int pyr20Sensor::calc_CRC16(unsigned char *buf, int len) {
  unsigned int crc = 0xFFFF;
  for (int pos = 0; pos < len; pos++) {
    crc ^= (unsigned int)buf[pos];
    for (int i = 8; i != 0; i--) {
      if ((crc & 0x0001) != 0) {
        crc >>= 1;
        crc ^= 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  crc = ((crc & 0x00ff) << 8) | ((crc & 0xff00) >> 8);
  return crc;
}

#endif