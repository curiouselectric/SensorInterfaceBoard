#include "serial_parse.h"
#include "average_data.h"
#ifdef ADD_CRC_CHECK
#include "crc_check.h"
#endif
#include <EEPROM.h>  // for saving mode to EEPROM


String check_data::parseData(String &_inputString, byte _UNIT_ID, data_channel _local_my_sensor_data[]) {

  /*
  Here we parse the data we get on the serial port (where ^^ is the 8 bit CRC is added if its enabled)
  Single channel data String is:     “aaI0R00A4?^^#”
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

  Adjust the serial send time:      "aaI0SEND*?^^#" where * is the send time (0,1,2,3,4,5)
                                      => Returned:  "aaSEND*?^^#", where * is the time interval to send data (0-5, with 5 never sending)

  Simulate a button press             "aaI0SWA?^^#"
                                      => Returned:  "aaSEND*?^^#", where * is the time interval to send data (0-5, with 5 never sending)

  Request Software Version             "aaI0SWV?^^#"
                                      => Returned: "aaSWV:1.0?^^#" (where 1.0 is the software version)

  Request Device Type                "aaI0DT?^^#"
                                      => Returned: "aaDT:SM?^^#" (where SM is the device type (SM = Soil Moisture))


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
  WIND SENSOR:
  Enter vane training mode:                       "aaI0VT?^^#"    - Implemented   
  Return JUST the wind vane information:          "aaI0WV?^^#"    - Implemented
  Return the wind vane conversion data:           "aaI0WVCON?^^#"
  Set the conversion values for m and c:          "aaI0STWSm123.4c567.89?^^#"
*/

  String _outputString = "";  // Set output string for the output
  int ave_time = 0;
  error_flag = false;  // reset this at beginning of checks

  // Check for pre/end chars:
  if (_inputString.charAt(0) != 'a' || _inputString.charAt(1) != 'a'
      || _inputString.charAt(_inputString.length() - 2) != '#') {
    error_flag = true;
    return (FAIL_STRING + F("PE"));  //No aa and # on the string
  }

  // Check length of string:
  if (_inputString.length() > INPUT_STRING_LENGTH) {
    error_flag = true;
    return (FAIL_STRING + F("TL"));  //String too long
  }

#ifdef ADD_CRC_CHECK  // Check CRC (if defined)
  if (!check_CRC(_inputString)) {
    error_flag = true;
    return (FAIL_STRING + F("CRC"));  // CRC check fail
  }
#endif

  // This is where we need to parse the data
  // Example input:
  // “aaI1R01A4?^^#”

  // Here we need to check if the ID is correct:
  if (_inputString.charAt(2) != 'I' || !isDigit(_inputString.charAt(3))) {
    error_flag = true;
    return (FAIL_STRING + F("ID"));  // ID fail
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
    return (FAIL_STRING + F("IDX"));  // ID fail - number incorrect
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
      return (FAIL_STRING + F("BD"));  // Baud rate fail
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
      _outputString = showChannelData(ave_time, _UNIT_ID, _local_my_sensor_data);
      return (_outputString);
    }
  }


  // Want to check R is in correct place and next two digits can be converted to an int....

  if (_requestString.charAt(0) == 'R' && isDigit(_requestString.charAt(1)) && isDigit(_requestString.charAt(2))) {
    // R = Return Single Channel, but need to know what the averaging value is:
    // Data looks like: “aaI0R01A4?^^#” this would be the 10min averaged data (A4) for just channel 1
    int _channel_number = (_requestString.substring(1, 3)).toInt();
    // Need to check that channel number is lower than max channels - if not then throw an error
    if (_channel_number >= NUM_CHANNELS) {
      return (FAIL_STRING + F("CN"));  // Channel number fail
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
#ifdef WIND_SENSOR
    _outputString += DELIMITER;
    _outputString += F("V");
    _outputString += DELIMITER;
    _outputString += wind_vane_data.return_direction(analogRead(VANE_PIN));  // Print the instantaneous value of the wind vane as a direction
    for (int y = 0; y < 8; y++) {
      _outputString += DELIMITER;
      _outputString += String(wind_vane_data.direction_array_values[y], 1);
    }
#endif
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
// Reset the wind vane data (if its a wind sensor)
#ifdef WIND_SENSOR
    wind_vane_data.reset_vane_direction_array();
#endif
    _outputString += F("RSTOK");
    return (_outputString);
  }

  // Software button press:
  if (_requestString.equals(F("SWA"))) {
    // if we ask: "aaI0SWA?^^#" (+CRC if needed) then this is the same as pressing on-board button - set flag high and deal with it in main sketch.
    button_press_flag = true;
    return ("");  // Dont need to return anything, as we are just saying its button tap.
  }

  // Adjust the serial data send rate (like button press but directly to the correct send time):
  if (_requestString.indexOf(F("SEND")) >= 0) {
    // if we ask: "aaI0SEND*?^^#" Set the send rate to * (0,1,2,3,4,5)
    // find the send rate from the string:
    if (isDigit(_requestString.charAt(4))) {
      // R = Return Single Channel, but need to know what the averaging value is:
      // Data looks like: “aaI0R01A4?^^#” this would be the 10min averaged data (A4) for just channel 1
      send_sensor_data = (_requestString.substring(4, 5)).toInt();

      if (send_sensor_data != EEPROM.read(EEPROM_SEND_DATA)) {
        // Update the EEPROM only if it has changed
        EEPROM.write(EEPROM_SEND_DATA, send_sensor_data);
      }
      _outputString += "SEND";  // Initialise the returned string
      _outputString += (String)send_sensor_data;
      return (_outputString);  // Dont need to return anything, as we are just saying its button tap.
    } else {
      return (FAIL_STRING + F("SND"));  // Command not recognised
    }
  }

  // Software version return:
  if (_requestString.equals(F("SWV"))) {
    // if we ask: "aaI0SWV?^^#" (+CRC if needed) then return the software version
    _outputString += "SWV";  // Initialise the returned string
    _outputString += DELIMITER;
    _outputString += SOFTWARE_VERSION;
    return (_outputString);  // Dont need to return anything, as we are just saying its button tap.
  }

  // Sensor type return:
  if (_requestString.equals(F("DT"))) {
    // if we ask: "aaI0DT?^^#" (+CRC if needed) then return the Device Type
    _outputString += "DT";  // Initialise the returned string
    _outputString += DELIMITER;
    _outputString += SENSOR_TYPE;
    return (_outputString);  // Dont need to return anything, as we are just saying its button tap.
  }


#ifdef WIND_SENSOR
  // These are all Wind Sensor specific commands:
  // Enter Wind Vane Training Mode:
  if (_requestString.equals(F("VT"))) {
    // if we ask for "aaI0VT#" then it will enter the vane training mode
    wind_vane_data.vane_training_mode = !wind_vane_data.vane_training_mode;
    if (wind_vane_data.vane_training_mode == true) {
      _outputString += F("VTENTER");
    } else {
      _outputString += F("VTLEAVE");
    }
    return (_outputString);
  }

  if (_requestString.equals(F("WV"))) {
    // if we ask for "aaI0WV#" then it will return the wind vane data
    _outputString += F("WV");
    _outputString += DELIMITER;
    _outputString += wind_vane_data.return_direction(analogRead(VANE_PIN));  // Print the instantaneous value of the wind vane as a direction
    for (int y = 0; y < 8; y++) {
      _outputString += DELIMITER;
      _outputString += String(wind_vane_data.direction_array_values[y], 1);
    }
    return (_outputString);
  }

  if (_requestString.equals(F("WSCON"))) {
    // if we ask for "aaI0WSCON?^^#" then it will return the wind speed conversion values
    // Return conversion settings "aaI0STWSCONm123.4c567.89#"
    _outputString += SENSOR_TYPE;
    _outputString += F("CONm");
    _outputString += (String)_local_my_sensor_data[0].wind_speed_conv_m;
    _outputString += F("c");
    _outputString += (String)_local_my_sensor_data[0].wind_speed_conv_c;
    return (_outputString);
  }

  if (_requestString.indexOf(F("WSSET")) >= 0) {
    // If we ask for "aaI0STWSm123.4c567.89#" then it will set the conversion values for m and c
    // So need to find the data between m & c and between c and #
    // Then convert the data from string to a float
    String data_substring;
    int start_index;
    int end_index;

    start_index = _inputString.indexOf('m');
    end_index = _inputString.indexOf('c', start_index + 1);
    data_substring = _inputString.substring(start_index + 1, end_index);
    // Need to check if the data_substring is a number or not...
    ;

    // isnan returns zero if its not a number
    if (!check_if_numer(data_substring)) {
      // Check the data is a number - else return an error
      return (FAIL_STRING + F("NaN"));
    }
    _local_my_sensor_data[0].wind_speed_conv_m = data_substring.toFloat();
    start_index = end_index;
    end_index = _inputString.indexOf('#', start_index + 1);
    data_substring = _inputString.substring(start_index + 1, end_index);
    if (!check_if_numer(data_substring)) {
      // Check the data is a number - else return an error
      return (FAIL_STRING + F("NaN"));
    }
    _local_my_sensor_data[0].wind_speed_conv_c = data_substring.toFloat();
    // Store this data to EEPROM
    EEPROM.put(EEPROM_WIND_CON_M, _local_my_sensor_data[0].wind_speed_conv_m);
    EEPROM.put(EEPROM_WIND_CON_C, _local_my_sensor_data[0].wind_speed_conv_c);
    _outputString += SENSOR_TYPE;
    _outputString += F("SETm");
    _outputString += (String)_local_my_sensor_data[0].wind_speed_conv_m;
    _outputString += F("c");
    _outputString += (String)_local_my_sensor_data[0].wind_speed_conv_c;
    return (_outputString);
  }
#endif


  return (FAIL_STRING + F("CMD"));  // Command not recognised
}

String check_data::showChannelData(int _average_time, byte _UNIT_ID, data_channel _local_my_sensor_data[]) {

  String _outputString = F("aaI");  // Set output string for the output
  _outputString += (String)_UNIT_ID;
  _outputString += F("RAAA");
  _outputString += (String)_average_time;
  _outputString += DELIMITER;
  switch (_average_time) {
    case 0:
      for (int j = 0; j < NUM_CHANNELS; j++) {
        if (_average_time >= _local_my_sensor_data[j].data_send_time - 1) {
          _outputString += (String)_local_my_sensor_data[j].data_1s;
        } else {
          _outputString += NO_DATA_CHAR;  // Dont show the data as its not
        }
        if (j < NUM_CHANNELS - 1) _outputString += DELIMITER;
      }
      break;
    case 1:
      for (int j = 0; j < NUM_CHANNELS; j++) {
        if (_average_time >= _local_my_sensor_data[j].data_send_time - 1) {
          _outputString += (String)_local_my_sensor_data[j].data_10s;
        } else {
          _outputString += NO_DATA_CHAR;  // Dont show the data as its not
        }
        if (j < NUM_CHANNELS - 1) _outputString += DELIMITER;
      }
      break;
    case 2:
      for (int j = 0; j < NUM_CHANNELS; j++) {
        if (_average_time >= _local_my_sensor_data[j].data_send_time - 1) {
          _outputString += (String)_local_my_sensor_data[j].data_60s;
        } else {
          _outputString += NO_DATA_CHAR;  // Dont show the data as its not
        }
        if (j < NUM_CHANNELS - 1) _outputString += DELIMITER;
      }
      break;
    case 3:
      for (int j = 0; j < NUM_CHANNELS; j++) {
        if (_average_time >= _local_my_sensor_data[j].data_send_time - 1) {
          _outputString += (String)_local_my_sensor_data[j].data_600s;
        } else {
          _outputString += NO_DATA_CHAR;  // Dont show the data as its not
        }
        if (j < NUM_CHANNELS - 1) _outputString += DELIMITER;
      }
      break;
    case 4:
      for (int j = 0; j < NUM_CHANNELS; j++) {
        if (_average_time >= _local_my_sensor_data[j].data_send_time - 1) {
          _outputString += (String)_local_my_sensor_data[j].data_3600s;
        } else {
          _outputString += NO_DATA_CHAR;  // Dont show the data as its not
        }
        if (j < NUM_CHANNELS - 1) _outputString += DELIMITER;
      }
      break;
    default:
      // If not in this list then fail
      _outputString = FAIL_STRING + F("AVE");
      break;
  }
#ifdef WIND_SENSOR
  _outputString += DELIMITER;
  _outputString += F("V");
  _outputString += DELIMITER;
  _outputString += wind_vane_data.return_direction(analogRead(VANE_PIN));  // Print the instantaneous value of the wind vane as a direction
  for (int y = 0; y < 8; y++) {
    _outputString += DELIMITER;
    _outputString += String(wind_vane_data.direction_array_values[y], 1);
  }
#endif
  return (_outputString);
}


bool check_data::check_if_numer(String _check_string) 
{
  boolean isNum = false;
  if (!(_check_string.charAt(0) == '+' || _check_string.charAt(0) == '-' || isDigit(_check_string.charAt(0)))) return false;
  for (byte i = 1; i < _check_string.length(); i++) {
    if (!(isDigit(_check_string.charAt(i)) || _check_string.charAt(i) == '.')) return false;
  }
  return true;
}
