#include "utilitiesDL.h"

#ifdef I2C_TEMP_HUMID
// This is if the I2C Temp and Humidity Sensor is attached
#include "DFRobot_AHT20.h"  // Must install this library
DFRobot_AHT20 aht20;
#include <BMx280I2C.h>  // Must install this library
#define I2C_ADDRESS 0x77
//create a BMx280I2C object using the I2C interface with I2C Address 0x77
BMx280I2C bmx280(I2C_ADDRESS);
#endif

void startI2Csensor() {

#ifdef I2C_TEMP_HUMID

  Wire.begin();  // Start the I2C bus  - ONLY if needed?
  delay(10);
  // AHT sensor start:
  uint8_t status;
  if ((status = aht20.begin()) != 0) {
// non-blocking
#ifdef DEBUG_FLAG
    Serial.print(F("AHT ERR:"));
    Serial.println(status);
#endif
  }
  //BMP sensor start:
  // begin() checks the Interface, reads the sensor ID (to differentiate between BMP280 and BME280)
  // and reads compensation parameters.
  if (!bmx280.begin()) {
    //non-blocking
#ifdef DEBUG_FLAG
    Serial.println(F("BMP ERR"));
#endif
  }
  //reset sensor to default parameters.
  bmx280.resetToDefaults();
  //by default sensing is disabled and must be enabled by setting a non-zero
  //oversampling setting.
  //set an oversampling setting for pressure and temperature measurements.
  bmx280.writeOversamplingPressure(BMx280MI::OSRS_P_x16);
  bmx280.writeOversamplingTemperature(BMx280MI::OSRS_T_x16);
#endif
}

String getI2Cdata() {
#ifdef I2C_TEMP_HUMID
  String _returnString = F("T");
  // if we ask: "aaI0TEMP?^^#" (+CRC if needed) then (if defined) then the I2C temp humidity and pressure sensor data will be returned
  if (aht20.startMeasurementReady(/* crcEn = */ true)) {
    _returnString += aht20.getTemperature_C();
    _returnString += F("H");
    _returnString += aht20.getHumidity_RH();
  } else {
    _returnString += F("-H-");
  }

  // Non-blocking
  _returnString += F("P");
  if (!bmx280.measure()) {
    _returnString += F("-");
  }
  uint32_t _start_time = millis();

  //Wait a little while for measurement - if no info/time out then return an error
  do {
    delay(10);
  } while (!bmx280.hasValue() && millis() < _start_time + 500);

  if (bmx280.hasValue()) {
    _returnString += (uint32_t)bmx280.getPressure();
  } else {
    _returnString += F("-");
  }
  return (_returnString);
#endif
}

byte check_unit_id(byte _unit_id) {
  // Reads the pins attached and updates the ID appropriately
  bitWrite(_unit_id, 0, !digitalRead(GPIO_ID0));
  bitWrite(_unit_id, 1, !digitalRead(GPIO_ID1));
  bitWrite(_unit_id, 2, !digitalRead(GPIO_ID2));
  return (_unit_id);
}