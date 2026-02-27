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
                                      => Returned:  "aaSEND*?^^#", where * is the time interval to send data (0-5, with 5 never sending)

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
    return ("");  // Dont need to return anything, as we are just saying its button tap.
  }
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
    default:
      // If not in this list then fail
      _outputString = FAIL_STRING + F("AVE");
      break;
  }
  return (_outputString);
}
