
#include "soil_moisture_rs485.h"
#include <SoftwareSerial.h>

moistureSensor soilMoistureSensor;

/****************************************************************************/
/***       Local Variable                                                 ***/
/****************************************************************************/
#define sensorBaudRate 4800
#define sensorTx 2
#define sensorRx 3

// Request frame for the soil sensor
const byte soilSensorRequestData[] = { 0x03, 0x00, 0x00, 0x00, 0x02 };
const byte soilSensorChangeID[] = { 0x06, 0x07, 0xD0, 0x00 };  // Need to add the new device ID to then end of this request and then calc CRC-16

byte soilSensorRequestNoCRC[6];
byte soilSensorRequest[8];
byte soilSensorResponse[9];  // Always expecting 9 bytes from this sensor for data

SoftwareSerial mod(sensorRx, sensorTx);  // Software serial for RS485 communication

/****************************************************************************/
/***       Class member Functions                                         ***/
/****************************************************************************/

void moistureSensor::begin(void) {
  /* Start Serial */
  mod.begin(sensorBaudRate);  // Initialize software serial communication at 4800 baud rate
}

bool moistureSensor::readHumiture(byte ID) {
  bool returnData = false;
  if (testing == true) {
    Serial.print(F("ID is:"));
    Serial.println(ID, HEX);
  }

  soilSensorRequestNoCRC[0] = ID;
  for (int i = 0; i < sizeof(soilSensorRequestData); i++) {
    soilSensorRequestNoCRC[i + 1] = soilSensorRequestData[i];
  }

  if (testing == true) {
    for (int i = 0; i < sizeof(soilSensorRequestNoCRC); i++) {
      Serial.print(soilSensorRequestNoCRC[i], HEX);
    }
    Serial.println();
  }

  // Calculate the CRC16 value (depends upon the sensor ID)
  unsigned int CRCvalue = calc_CRC16(soilSensorRequestNoCRC, sizeof(soilSensorRequestNoCRC));

  for (int i = 0; i < sizeof(soilSensorRequestNoCRC); i++) {
    soilSensorRequest[i] = soilSensorRequestNoCRC[i];
  }
  soilSensorRequest[6] = highByte(CRCvalue);
  soilSensorRequest[7] = lowByte(CRCvalue);

  if (testing == true) {
    for (int i = 0; i < sizeof(soilSensorRequest); i++) {
      Serial.print(soilSensorRequest[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
  }
  // Send the request to the soil sensor
  mod.write(soilSensorRequest, sizeof(soilSensorRequest));
  delay(10);

  // Wait for the response from the sensor or timeout after 500 msecond
  unsigned long startTime = millis();

  while (mod.available() < 9 && millis() - startTime < 500) {
    delay(1);
  }

  if (mod.available() >= 9)  // If valid response receivedPulse sensor modules
  {
    returnData = true;  // Data is OK
    // Read the response from the sensor
    byte index = 0;
    while (mod.available() && index < 9) {
      soilSensorResponse[index] = mod.read();
      if (testing == true) {
        Serial.print(soilSensorResponse[index], HEX);  // Print the received byte in HEX format
        Serial.print(" ");
      }
      index++;
    }
    if (testing == true) {
      Serial.println();
    }

    // Parse and calculate the Moisture value
    int Moisture_Int = int(soilSensorResponse[3] << 8 | soilSensorResponse[4]);
    moisturePercent = Moisture_Int / 10.0;

    // Parse and calculate the Temperature value
    int Temperature_Int = int(soilSensorResponse[5] << 8 | soilSensorResponse[6]);
    soilTemperatureC = Temperature_Int / 10.0;

    // Check if temperature is negative and convert accordingly
    if (Temperature_Int > 0x7FFF) {
      soilTemperatureC = 0x10000 - Temperature_Int;
      soilTemperatureC = -soilTemperatureC / 10.0;
    }
  }
  return returnData;
}

void moistureSensor::changeID(byte IDold, byte IDnew) {
  // To change the ID
  soilSensorRequestNoCRC[0] = IDold;
  for (int i = 0; i < sizeof(soilSensorChangeID); i++) {
    soilSensorRequestNoCRC[i + 1] = soilSensorChangeID[i];
  }
  soilSensorRequestNoCRC[5] = IDnew;
  //soilSensorRequestNoCRC[5] = 0x00;
  // Calculate the CRC16 value (depends upon the sensor ID)
  unsigned int CRCvalue = calc_CRC16(soilSensorRequestNoCRC, sizeof(soilSensorRequestNoCRC));

  for (int i = 0; i < sizeof(soilSensorRequestNoCRC); i++) {
    soilSensorRequest[i] = soilSensorRequestNoCRC[i];
  }
  soilSensorRequest[6] = highByte(CRCvalue);
  soilSensorRequest[7] = lowByte(CRCvalue);

  if (testing == true) {
    Serial.print("ID change request:");
    for (int i = 0; i < sizeof(soilSensorRequest); i++) {
      Serial.print(soilSensorRequest[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
  }

  // Send the request frame to the soil sensor
  mod.write(soilSensorRequest, sizeof(soilSensorRequest));
  delay(10);

  // Wait for the response from the sensor or timeout after 500 msecond
  unsigned long startTime = millis();

  while (mod.available() < 8 && millis() - startTime < 500) {
    delay(1);
  }

  if (mod.available() >= 8)  // If valid response receivedPulse sensor modules
  {
    Serial.print("Device Response:");
    // Read the response from the sensor
    byte index = 0;
    while (mod.available() && index < 8) {
      soilSensorResponse[index] = mod.read();
      if (testing == true) {
        Serial.print(soilSensorResponse[index], HEX);  // Print the received byte in HEX format
        Serial.print(" ");
      }
      index++;
    }
    if (testing == true) {
      Serial.println();
    }
    // We have got data back so check it is OK:
  }
}

unsigned int moistureSensor::calc_CRC16(unsigned char *buf, int len) {

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

