#include "serial_parse.h"
#include "average_data.h"
#include "crc_check.h"
#include <EEPROM.h>  // for saving mode to EEPROM



String check_data::parseData(String &_inputString, byte _UNIT_ID, data_channel _local_my_sensor_data[]) {

  /*
  Here we parse the data we get on the serial port (where ^^ is the 8 bit CRC is added if its enabled)
  Single channel data String is:     “aaI0R01A4?^^#”
                                      => Returned: "aaI0R00A1:123.40:123.40:123.40?^^#"

  All channel data String is:        “aaI0RAAA3?^^#”
                                      => Returned: "aaI0RAAA1:123.40:567.80?^^#"

  All channel minimum data String is:“aaI0RMNA4?^^#”  - does not matter what averaging period. min/max are just the min/max seen at max data rate.
                                      => Returned: "aaI0RMN:123.40:567.80?^^#"

  All channel maximum data String is:“aaI0RMXA0?^^#”  - does not matter what averaging period. min/max are just the min/max seen at max data rate.
                                      => Returned: "aaI0RMX:123.40:567.80?^^#"

  Reset the min and max data          "aaI0RESET?^^#"
                                      => Returned: "aaI0RSTOK?^^#"   when the reset has worked OK

  What is baud rate?:                "aaI0BD?^^#" 
                                      => Returned:  "aaI0BD9600?^^#" (or whatever the Baud rate is)

  Set Baud Rate:                     "aaI0STBD*?^^#"  Where * is 0,1,2,3,4 for 1200, 2400, 9600, 57600, 115200 
                                      => Returned: "aaI0CHBD9600?^^#" (or whatever the Baud rate is)

  What is ID?:                       "aaI*ID?^^#"  The * value can be anything. Also mentioned at start up of unit - its solder-programmed... cannot be changed in code.
                                      => Returned: "aaIXID:X?^^#", where X is the actual ID of the unit

  Simulate a button press             "aaI0SWA?^^#"
                                      => Returned:  

  Request Software Version             "aaI0SWV?^^#"
                                      => Returned: 

  Request Device Type                "aaI0DT?^^#"
                                      => Returned: 

  Need to check:
  Does data have pre/end chars (aa and #)?
  Is data too long?
  Does data have correct device ID?
  Check for the CRC? - only if enabled

  Error Codes:
  aaFAILCRC: CRC check fail
  aaFAILTL: String too long
  aaFAILID: Problem with ID
  aaFAILIDX: ID not correct to device
  aaFAILPE: No aa and # on the string
  aaFAILBD: Baud rate change fail
  aaFAILCN: Channel number requested is greater than channels existing
  aaFAILCMD: Command not recognised 

  Sensor specific commands:
  Enter vane training mode:          "aaI0VT#"
*/

  String _outputString = "";  // Set output string for the output
  String _failString = F("aaFAIL");

  error_flag = false;  // reset this at beginning of checks

  // Check for pre/end chars:
  if (_inputString.charAt(0) != 'a' || _inputString.charAt(1) != 'a'
      || _inputString.charAt(_inputString.length() - 2) != '#') {
    error_flag = true;
    return (_failString + F("PE"));  //No aa and # on the string
  }

  // Check length of string:
  if (_inputString.length() > INPUT_STRING_LENGTH) {
    error_flag = true;
    return (_failString + F("TL"));  //String too long
  }

  // Check CRC (if defined)
  if (ADD_CRC_CHECK && !check_CRC(_inputString)) {
    // *************** TO DO - SORT OUT WITH CRC CHECK
    error_flag = true;
    return (_failString + F("CRC"));  // CRC check fail
  }

  // This is where we need to parse the data
  // Example input:
  // “aaI1R01A4?^^#”

  // Here we need to check if the ID is correct:
  if (_inputString.charAt(2) != 'I' || !isDigit(_inputString.charAt(3))) {
    error_flag = true;
    return (_failString + F("ID"));  // ID fail
  }

  // The request is between the ID number (char 4) and the ? point, not the #.
  // If there is a CRC check that is between the ? and the #.
  String _requestString = _inputString.substring(4, (_inputString.indexOf("?", 4)));

  _outputString = F("aaI");
  _outputString += (String)_UNIT_ID;

  // We can request the ID even if we dont know the ID number (before that check)
  //
  if (_requestString.equals(F("ID"))) {
    _outputString += F("ID");
    _outputString += DELIMITER;
    _outputString += (String)_UNIT_ID;
    return (_outputString);
  }

  // This is where we check the ID with the Unit ID
  int _unit_id = (_inputString.substring(3, 4)).toInt();
  // This checks with the global variable from set-up
  if (_unit_id != (int)_UNIT_ID) {
    error_flag = true;
    return (_failString + F("IDX"));  // ID fail - number incorrect
  }

  // Here we check the request string against the various potential requests

  // Check Baud Rate
  if (_requestString.equals(F("BD"))) {
    _outputString += F("BD");
    _outputString += baud_rates[EEPROM.read(EEPROM_SERIAL_BAUD)];
    return (_outputString);
  }

  // Set Baud rate (change it)
  if (_requestString.indexOf(F("STBD")) >= 0) {
    Serial.println(_requestString);
    // Check there is a digit, else return and error
    if (isDigit(_requestString.charAt(4))) {
      // Find the new baud rate with the end char:
      int _new_baud = (_requestString.substring(4, 5)).toInt();
      Serial.flush();
      delay(200);
      // end the old serial communication:
      Serial.end();
      delay(200);

      // Set the new baud rate:
      if (_new_baud != EEPROM.read(EEPROM_SERIAL_BAUD)) {
        if (_new_baud >= MAX_BAUD_RATES) {
          EEPROM.write(EEPROM_SERIAL_BAUD, 2);  // Update the EEPROM-Initialise to 9600 if data out of range
        } else {
          EEPROM.write(EEPROM_SERIAL_BAUD, _new_baud);  // Update the EEPROM-Initialise to 9600 if data out of range
        }
      }
      // re-initialize serial communication:
      Serial.begin(baud_rates[EEPROM.read(EEPROM_SERIAL_BAUD)]);
      Serial.flush();
      delay(200);
      _outputString += F("CHBD");
      _outputString += baud_rates[EEPROM.read(EEPROM_SERIAL_BAUD)];
      return (_outputString);
    } else {
      return (_failString + F("BD"));  // Baud rate fail
    }
  }

  // Here we parse the data we get on the serial port including the CRC check where ^^ is the 8 bit CRC
  // Single channel data String is:     “aaI0R01A4?^^#”
  // All channel data String is:        “aaI0RAAA3?^^#”
  // All channel minimum data String is:“aaI0RMN?^^#”  - does not matter what averaging period. min/max are just the min/max seen at max data rate.
  // All channel maximum data String is:“aaI0RMX?^^#”  - does not matter what averaging period. min/max are just the min/max seen at max data rate.

  if (_requestString.indexOf(F("RAA")) >= 0) {
    // RAA = Return All, but need to know what the averaging value is:
    // Data looks like: “aaI0RAAA4?^^#” this would be the 10min averaged data (A4)
    if (_requestString.charAt(_requestString.length() - 2) == 'A' && isDigit(_requestString.charAt(_requestString.length() - 1))) {
      // need to check if AVE is OK and a number
      ave_time = (_requestString.substring(_requestString.length() - 1, (_requestString.length()))).toInt();
      _outputString += F("RAAA");
      _outputString += (String)ave_time;
      _outputString += DELIMITER;
      switch (ave_time) {
        case 0:
          for (int j = 0; j < NUM_CHANNELS; j++) {
            _outputString += (String)_local_my_sensor_data[j].data_1s;
            if (j < NUM_CHANNELS - 1) _outputString += DELIMITER;
          }
          break;
        case 1:
          for (int j = 0; j < NUM_CHANNELS; j++) {
            _outputString += (String)_local_my_sensor_data[j].data_10s;
            if (j < NUM_CHANNELS - 1) _outputString += DELIMITER;
          }
          break;
        case 2:
          for (int j = 0; j < NUM_CHANNELS; j++) {
            _outputString += (String)_local_my_sensor_data[j].data_60s;
            if (j < NUM_CHANNELS - 1) _outputString += DELIMITER;
          }
          break;
        case 3:
          for (int j = 0; j < NUM_CHANNELS; j++) {
            _outputString += (String)_local_my_sensor_data[j].data_600s;
            if (j < NUM_CHANNELS - 1) _outputString += DELIMITER;
          }
          break;
        case 4:
          for (int j = 0; j < NUM_CHANNELS; j++) {
            _outputString += (String)_local_my_sensor_data[j].data_3600s;
            if (j < NUM_CHANNELS - 1) _outputString += DELIMITER;
          }
          break;
      }
    }
  }


  // Want to check R is in correct place and next two digits can be converted to an int....

  if (_requestString.charAt(0) == 'R' && isDigit(_requestString.charAt(1)) && isDigit(_requestString.charAt(2))) {
    // R = Return Single Channel, but need to know what the averaging value is:
    // Data looks like: “aaI0R01A4?^^#” this would be the 10min averaged data (A4) for just channel 1
    int _channel_number = (_requestString.substring(1, 3)).toInt();
    // Need to check that channel number is lower than max channels - if not then throw an error
    if (_channel_number >= NUM_CHANNELS) {
      return (_failString + F("CN"));  // Channel number fail
    }

    if (_requestString.charAt(_requestString.length() - 2) == 'A' && isDigit(_requestString.charAt(_requestString.length() - 1))) {
      // need to check if AVE is OK and a number
      ave_time = (_requestString.substring(_requestString.length() - 1, (_requestString.length()))).toInt();
      _outputString += F("R");
      if (_channel_number < 10) {
        _outputString += F("0");
      }
      _outputString += (String)_channel_number;
      _outputString += F("A");
      _outputString += (ave_time);
      switch (ave_time) {
        case 0:
          _outputString += DELIMITER;
          _outputString += (String)_local_my_sensor_data[_channel_number].data_1s;
          break;
        case 1:
          _outputString += DELIMITER;
          _outputString += (String)_local_my_sensor_data[_channel_number].data_10s;
          break;
        case 2:
          _outputString += DELIMITER;
          _outputString += (String)_local_my_sensor_data[_channel_number].data_60s;
          break;
        case 3:
          _outputString += DELIMITER;
          _outputString += (String)_local_my_sensor_data[_channel_number].data_600s;
          break;
        case 4:
          _outputString += DELIMITER;
          _outputString += (String)_local_my_sensor_data[_channel_number].data_3600s;
          break;
      }
      _outputString += DELIMITER;
      _outputString += (String)_local_my_sensor_data[_channel_number].data_min;
      _outputString += DELIMITER;
      _outputString += (String)_local_my_sensor_data[_channel_number].data_max;
    }
    return (_outputString);
  }

  if (_requestString.indexOf(F("RMN")) >= 0) {
    // RA = Return Minimums
    _outputString += "RMN";  // Initialise the returned string
    for (int y = 0; y < NUM_CHANNELS; y++) {
      _outputString += DELIMITER;
      _outputString += (String)_local_my_sensor_data[y].data_min;  // Print the 1 second data
    }
    return (_outputString);
  }

  if (_requestString.indexOf(F("RMX")) >= 0) {
    // RA = Return Minimums
    _outputString += "RMX";  // Initialise the returned string
    for (int y = 0; y < NUM_CHANNELS; y++) {
      _outputString += DELIMITER;
      _outputString += (String)_local_my_sensor_data[y].data_max;  // Print the 1 second data
    }
    return (_outputString);
  }

  if (_requestString.equals(F("RESET"))) {
    // Reset all the max and mins:
    // *********** RESET ALL MAXIMUMS **********************
    for (int j = 0; j < NUM_CHANNELS; j++) {
      _local_my_sensor_data[j].data_max = -99999;
    }
    // *********** RESET ALL MINIMUMS **********************
    for (int j = 0; j < NUM_CHANNELS; j++) {
      _local_my_sensor_data[j].data_min = 99999;
    }
    _outputString += F("RSTOK");
    return (_outputString);
  }


  // Software button press:
  if (_requestString.equals(F("SWA"))) {
    // if we ask: "aaI0SWA?^^#" (+CRC if needed) then this is the same as pressing on-board button - set flag high and deal with it in main sketch.
    button_press_flag = true;
    _outputString += F("RSTOK");
    return (_outputString);
  }



  return (_failString + F("CMD"));  // Command not recognised



  // // The unit ID is OK so carry on checking, otherwise dont bother checking any further
  // if (_inputString.charAt(4) == 'B' && _inputString.charAt(5) == 'D') {
  //   // if we ask for "aaI0BD#" then it will return the baud rate
  //   baud_return_flag = true;
  // } else if (_inputString.charAt(4) == 'S' && _inputString.charAt(5) == 'T' && _inputString.charAt(6) == 'B' && _inputString.charAt(7) == 'D') {
  //   // if we ask for "aaI0STBD?#" then it will return the baud rate, with ? being a number to set baud rate from array
  //   int baud_id = (_inputString.substring(8, 9)).toInt();
  //   EEPROM.write(EEPROM_SERIAL_BAUD, baud_id);
  //   baud_set_flag = true;
  // } else if (_inputString.charAt(4) == 'S' && _inputString.charAt(5) == 'E' && _inputString.charAt(6) == 'N' && _inputString.charAt(7) == 'D') {
  //   // if we ask for "aaI0SEND?#" then it will start sending regular serial data
  //   int send_id = (_inputString.substring(8, 9)).toInt();
  //   if (send_id >= 0 && send_id < 6) {
  //     EEPROM.write(EEPROM_SEND_DATA, send_id);
  //     send_sensor_data_flag = true;
  //   }
  // }
  // // else if (_inputString.charAt(4) == 'W' && _inputString.charAt(5) == 'S' && _inputString.charAt(6) == 'C' && _inputString.charAt(7) == 'O' && _inputString.charAt(8) == 'N')
  // // {
  // //   // If we ask for "aaI0WVCON#" then it will return the wind vane conversion data
  // //   conversion_return_flag = true;
  // // }
  // // else if (_inputString.charAt(4) == 'W' && _inputString.charAt(5) == 'S' && _inputString.charAt(6) == 'S' && _inputString.charAt(7) == 'T')
  // // {
  // //   // If we ask for "aaI0STWSm123.4c567.89#" then it will set the conversion values for m and c
  // //   // So need to find the data between m & c and between c and #
  // //   // Then convert the data from string to a float
  // //   String data_substring;
  // //   int start_index;
  // //   int end_index;

  // //   // start_index = _inputString.indexOf('m');
  // //   // end_index = _inputString.indexOf('c', start_index + 1);
  // //   // data_substring = _inputString.substring(start_index + 1, end_index);
  // //   // wind_speed_conv_m = data_substring.toFloat();
  // //   // start_index = end_index;
  // //   // end_index = _inputString.indexOf('#', start_index + 1);
  // //   // data_substring = _inputString.substring(start_index + 1, end_index);
  // //   // wind_speed_conv_c = data_substring.toFloat();
  // //   // conversion_set_flag = true;

  // // }
  // // else if (_inputString.charAt(4) == 'W' && _inputString.charAt(5) == 'S' && _inputString.charAt(6) == 'M' && _inputString.charAt(7) == 'N')
  // // {
  // //   // We get here is asking for minimum wind speed data "MN"
  // //   // We dont need the average number
  // //   data_min_flag = true;
  // //   data_sent_flag = true; // This flag allows the main loop to then check out the correct data value and return it.
  // // }
  // // else if (_inputString.charAt(4) == 'W' && _inputString.charAt(5) == 'S' && _inputString.charAt(6) == 'M' && _inputString.charAt(7) == 'X')
  // // {
  // //   // We get here is asking for ALL maximum channel data "MX"
  // //   // We dont need the average number
  // //   data_max_flag = true;
  // //   data_sent_flag = true; // This flag allows the main loop to then check out the correct data value and return it.
  // // }
  // // else if (_inputString.charAt(4) == 'W' && _inputString.charAt(5) == 'S')
  // // {
  // //   // We get here is asking for ALL channel data "AA"
  // //   if (_inputString.charAt(6) == 'A' && isDigit(_inputString.charAt(7)))
  // //   {
  // //     // need to check if AVE is OK and a number
  // //     ave_time = (_inputString.substring(7, 8)).toInt();
  // //     // This means we have all the parts required and can return true - the data will be correct.
  // //     data_sent_flag = true; // This flag allows the main loop to then check out the correct data value and return it.
  // //     data_all_flag = true; // This means we return ALL the data
  // //   }
  // //   else
  // //   {
  // //     error_flag = true;
  // //     data_sent_flag = false;
  // //     _outputString = "aaFAIL4";
  // //   }
  // // }
  // else if (_inputString.charAt(4) == 'R' && _inputString.charAt(5) == 'E' && _inputString.charAt(6) == 'S' && _inputString.charAt(7) == 'E' && _inputString.charAt(8) == 'T') {
  //   // We get here is asking for Min and Max data to be reset
  //   data_sent_flag = false;
  //   data_min_flag = false;
  //   data_max_flag = false;
  //   data_reset_flag = true;  // Do not need this flag - not returning anything
  // }
  // // else if (_inputString.charAt(4) == 'V' && _inputString.charAt(5) == 'T')
  // // {
  // //   // if we ask for "aaI0VT#" then it will enter the vane training mode
  // //   vane_training_mode = !vane_training_mode;
  // //   error_flag = true; // This means we will see the message (not an error!)

  // //   if (vane_training_mode == true)
  // //   {
  // //     _outputString = "Enter VT MODE";
  // //   }
  // //   else
  // //   {
  // //     _outputString = "Leave VT MODE";
  // //   }
  // // }
  // // else if (_inputString.charAt(4) == 'W' && _inputString.charAt(5) == 'V')
  // // {
  // //   // if we ask for "aaI0WV#" then it will return the wind vane data
  // //   vane_data_flag = true;
  // // }
  // else if (_inputString.charAt(4) == 'B' && _inputString.charAt(5) == 'U' && _inputString.charAt(6) == 'T' && _inputString.charAt(7) == 'T' && _inputString.charAt(8) == 'O' && _inputString.charAt(9) == 'N') {
  //   // if we ask: "aaI0BUTTON#" (+CRC if needed) then this is the same as pressing on-board button - set flag high and deal with it in main sketch.
  //   button_press_flag = true;
  // } else {
  //   error_flag = true;
  //   data_sent_flag = false;
  //   _outputString = _failString + "3";
  // }
  // return (_outputString);
}

String show_Channel_Data(int _average_time, data_channel _local_sensor_data[])
{
  
}
