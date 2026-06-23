#include "config.h"
#include <SoftwareSerial.h>

#ifdef SOLAR_RS485_SENSOR

pyr20Sensor pyr20SensorRS485;

/****************************************************************************/
/***       Local Variable                                                 ***/
/****************************************************************************/
#define sensorBaudRate 9600

// Request frame for the soil sensor
const byte pyranSensorRequestData[] = { 0x03, 0x00, 0x00, 0x00, 0x02 };
const byte pyranSensorChangeID[] = { 0x06, 0x07, 0xD0, 0x00 };  // Need to add the new device ID to then end of this request and then calc CRC-16
byte pyranSensorRequestNoCRC[6];
byte pyranSensorRequest[8];
byte pyranSensorResponse[9];  // Always expecting 9 bytes from this sensor for data

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

  pyranSensorRequestNoCRC[0] = ID;
  for (int i = 0; i < sizeof(pyranSensorRequestData); i++) {
    pyranSensorRequestNoCRC[i + 1] = pyranSensorRequestData[i];
  }

  // Calculate the CRC16 value (depends upon the sensor ID)
  unsigned int CRCvalue = calc_CRC16(pyranSensorRequestNoCRC, sizeof(pyranSensorRequestNoCRC));

  for (int i = 0; i < sizeof(pyranSensorRequestNoCRC); i++) {
    pyranSensorRequest[i] = pyranSensorRequestNoCRC[i];
  }
  pyranSensorRequest[6] = highByte(CRCvalue);
  pyranSensorRequest[7] = lowByte(CRCvalue);

  // Send the request to the pyran sensor
  mod.write(pyranSensorRequest, sizeof(pyranSensorRequest));
  delay(200);

  // Wait for the response from the sensor or timeout after 500 msecond
  unsigned long startTime = millis();
  while (mod.available() < 9 && millis() - startTime < 500) {
    delay(1);
  }

  if (mod.available() >= 9)  // If valid response received 
  {
    returnData = true;  // Data is OK
    // Read the response from the sensor
    byte index = 0;
    while (mod.available() && index < 9) {
      pyranSensorResponse[index] = mod.read();
      index++;
    }
    // Parse and calculate the pyran value
    pyranInt = int(pyranSensorResponse[3] << 8 | pyranSensorResponse[4]);
  }
  return returnData;
}

void pyr20Sensor::changeID(byte IDold, byte IDnew) {
  // To change the ID
  pyranSensorRequestNoCRC[0] = IDold;
  for (int i = 0; i < sizeof(pyranSensorChangeID); i++) {
    pyranSensorRequestNoCRC[i + 1] = pyranSensorChangeID[i];
  }
  pyranSensorRequestNoCRC[5] = IDnew;
  // Calculate the CRC16 value (depends upon the sensor ID)
  unsigned int CRCvalue = calc_CRC16(pyranSensorRequestNoCRC, sizeof(pyranSensorRequestNoCRC));

  for (int i = 0; i < sizeof(pyranSensorRequestNoCRC); i++) {
    pyranSensorRequest[i] = pyranSensorRequestNoCRC[i];
  }
  pyranSensorRequest[6] = highByte(CRCvalue);
  pyranSensorRequest[7] = lowByte(CRCvalue);

#ifdef DEBUG_PYRAN
  Serial.print(F("ID CHG:"));
  for (int i = 0; i < sizeof(pyranSensorRequest); i++) {
    Serial.print(pyranSensorRequest[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
#endif

  // Send the request frame to the pyran sensor
  mod.write(pyranSensorRequest, sizeof(pyranSensorRequest));
  delay(10);

  // Wait for the response from the sensor or timeout after 500 msecond
  unsigned long startTime = millis();

  while (mod.available() < 8 && millis() - startTime < 500) {
    delay(1);
  }

  if (mod.available() >= 8)  // If valid response received
  {
#ifdef DEBUG_PYRAN
    Serial.printF(("Res:"));
#endif
    // Read the response from the sensor
    byte index = 0;
    while (mod.available() && index < 8) {
      pyranSensorResponse[index] = mod.read();
#ifdef DEBUG_PYRAN
      Serial.print(pyranSensorResponse[index], HEX);  // Print the received byte in HEX format
      Serial.print(F(" "));
#endif
      index++;
    }
#ifdef DEBUG_PYRAN
    Serial.println();
#endif
  }
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