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

  // // Wind Vane Specific
  // bool vane_training_mode;   // This flag sets if we are in vane training mode or not....
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
    error_flag = false;
    button_press_flag = false;
  }
};
