#pragma once

#include <stdio.h>
#include <Arduino.h>
#include "config.h"
#include "average_data.h"

// This class takes in an input string, parses it and returns the ID and AVE values if they are OK
// If not it returns an error message

class check_data {
public:
  bool error_flag;
  bool button_press_flag;

  int   send_sensor_data;   // This int sets if the sensor data will be sent each time. If = 5 then no data sent,
                           // if = 0 then 1s, = 1 then 10s = 2 then 60s if =3 then 600s if = 4 then 3600s
                           // This is updated in EEPROM and set via button or serial command

  // // Wind Vane Specific
  
  // 
  // bool vane_data_flag;
  // bool conversion_return_flag;
  // bool conversion_set_flag;
  // float wind_speed_conv_m;
  // float wind_speed_conv_c;

  // Member functions
  String parseData(String &dataString, byte unit_ID, data_channel local_my_sensor_data[]);
  String showChannelData(int my_average_time, byte unit_ID, data_channel local_my_sensor_data[]);

  // This is the constructor
  check_data() {
    send_sensor_data = 5;   // ititalise to NOT send data on serial port
    error_flag = false;
    button_press_flag = false;
  }
};
