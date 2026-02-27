/*
   RS485 Interface firmware for ATMega328
   by:    Matt Little
   date:  25/02/2026
   w:     www.curiouselectric.co.uk
   e:     hello@curiouselectric.co.uk

   This code is for an ATMega328 base RS485 converter board.
   The board is designed to read RS485 sensor devices, store and average the data and then return the data based on simple serial commands.

   This uses an ATMega328 running at 8MHz (use this for 3.3V supply) or 16MHz (for 5V supply).

   It returns the average values when requested on serial port.
   Or it broadcasts wind speed and direction data at set intervals.
   A CRC check can be included on the serial data. - Set the ADD_CRC_CHECK flag to high.

   At all other times then the unit is asleep.

   More construction details are here:
   https://github.com/curiouselectric/RS485InterfaceBoard

   Details on this construction are here:

   Please see the github rpository readme for all the serial commands available.

    The ATMega382p IC can have Arduino Uno or MiniCore as the bootloader. MiniCore is slightly more efficient and allows the use of 8Mhz oscillator for 3.3V operation.
    Uno bootloader is always 16MHz.

   To program it with MiniCore bootloader:
   Easiest method is:
   Install MiniCore from here: https://github.com/MCUdude/MiniCore
   Add to preferences and then board manager.
   Use minicore to burn the bootloader with an Arduino as an ISP using the 'Burn Bootloader' option

   To program with Uno bootloader:
   Follow online instructions or use Arduino as ISP.
*/

#include <stdio.h>
#include <Arduino.h>
#include <Wire.h>
#include <EEPROM.h>   // for saving mode to EEPROM
#include <avr/wdt.h>  // for the watch dog timer

#include "Button2.h"  // Need to include this to the library
#include "config.h"
#include "serial_parse.h"
#include "average_data.h"
#include "utilitiesDL.h"
#include "crc_check.h"

// ****** This is for the buttons button2 library used and needs to be installed ********
// https://github.com/LennartHennigs/Button2
Button2 buttonA = Button2(SWA_PIN);

// ******** This is for Scheduling Tasks **************************
// Must include this library from Arduino IDE Library Manager
// https://github.com/arkhipenko/TaskScheduler
#include <TaskScheduler.h>

// #define _TASK_TIMECRITICAL      // Enable monitoring scheduling overruns
#define _TASK_SLEEP_ON_IDLE_RUN  // Enable 1 ms SLEEP_IDLE powerdowns between tasks if no callback methods were invoked during the pass
// #define _TASK_STATUS_REQUEST    // Compile with support for StatusRequest functionality - triggering tasks on status change events in addition to time only
// #define _TASK_WDT_IDS           // Compile with support for wdt control points and task ids
// #define _TASK_LTS_POINTER       // Compile with support for local task storage pointer
// #define _TASK_PRIORITY          // Support for layered scheduling priority
// #define _TASK_MICRO_RES         // Support for microsecond resolution
// #define _TASK_STD_FUNCTION      // Support for std::function (ESP8266 and ESP32 ONLY)
// #define _TASK_DEBUG             // Make all methods and variables public for debug purposes
// #define _TASK_INLINE            // Make all methods "inline" - needed to support some multi-tab, multi-file implementations
// #define _TASK_TIMEOUT           // Support for overall task timeout
// #define _TASK_OO_CALLBACKS      // Support for dynamic callback method binding
// #define _TASK_DEFINE_MILLIS     // Force forward declaration of millis() and micros() "C" style
// #define _TASK_EXPOSE_CHAIN      // Methods to access tasks in the task chain

// Callback methods prototypes
void t1Callback();
void t1SCallback();
void t10SCallback();
void t60SCallback();
void t600SCallback();
void t3600SCallback();

// Tasks
// time (mS) between tasks, number of tasks to do, call back function
/*
  Scheduling defines:
  TASK_MILLISECOND
  TASK_SECOND
  TASK_MINUTE
  TASK_HOUR
  TASK_IMMEDIATE
  TASK_FOREVER
  TASK_ONCE
  TASK_NOTIMEOUT
*/

Task t1(100 * TASK_MILLISECOND, TASK_FOREVER, &t1Callback);  // Sample as base rate of 10Hz
Task t1S(TASK_SECOND, TASK_FOREVER, &t1SCallback);
Task t10S(10 * TASK_SECOND, TASK_FOREVER, &t10SCallback);
Task t60S(TASK_MINUTE, TASK_FOREVER, &t60SCallback);
Task t600S(10 * TASK_MINUTE, TASK_FOREVER, &t600SCallback);
Task t3600S(TASK_HOUR, TASK_FOREVER, &t3600SCallback);

Scheduler runner;

// *********** This is for the serial data parsing ******************
// Create a class for the serial data to be parsed
check_data checkData;

// *********** This is for the data holders *************************
// *********** This is for the data holders *************************
// Want to create a data class for each channel, up to max of NUM_CHANNELS
data_channel my_sensor_data[NUM_CHANNELS];  // This creates our data channels

int send_sensor_data = 5;  // This int sets if the sensor data will be sent each time. If = 5 then no data sent,
                           // if = 0 then 1s, = 1 then 10s = 2 then 60s if =3 then 600s if = 4 then 3600s
                           // This is updated in EEPROM when set via button or serial command


// *********** These are global variables ***************************
String inputString = "";      // a String to hold incoming data
String returnString = "";     // A string to hold the returned data
bool stringComplete = false;  // Checks if the string is complete

byte UNIT_ID = B00000000;  // This is the unit_id of the board itself. This will be default value. Values from 0-255 can potentially be used. 4 bit binary number? = 16 channels.

// Put LED flash into utilities to clean up code
//****************TO DO***************************
uint32_t led_flash_timer = millis();
bool led_update_flag = true;
uint8_t led_flash_counter = 0;  // Number of flashes to do

volatile int data_counter_1s = 0;
volatile int data_counter_10s = 0;
volatile int data_counter_60s = 0;
volatile int data_counter_600s = 0;
volatile int data_counter_3600s = 0;

void t1Callback() {
  // This loop runs every 100mS
  // This will records the sensor value and store its it in the my_sensor_data array
  // Check the sensor for data:
  // for (int j = 0; j < NUM_CHANNELS; j++) {
  //   //****************TO DO***************************
  //   // This is a TEST
  //   my_sensor_data[0].data_1s_holder += 123.4;
  // }
  // This is a TEST
  my_sensor_data[0].data_1s_holder += 123.4;
  // This is a TEST
  my_sensor_data[1].data_1s_holder += 567.8;

  // It will then be averaged for 1s, 10s, 60s 10min and 1 hour averages
  data_counter_1s++;  // This counts the correct number of samples we take within the 1S sample period. Used for averaging.
}

void t1SCallback() {
  if (!t1S.isFirstIteration()) {

    for (int j = 0; j < NUM_CHANNELS; j++) {
      // 1 second averages put into the correct channels
      my_sensor_data[j].data_1s = my_sensor_data[j].data_1s_holder / data_counter_1s;
      // reset all the data holders
      my_sensor_data[j].data_1s_holder = 0;  // Reset the value

      // Here we set the max and min for the data
      if (my_sensor_data[j].data_1s < my_sensor_data[j].data_min) {
        my_sensor_data[j].data_min = my_sensor_data[j].data_1s;
      }
      if (my_sensor_data[j].data_1s > my_sensor_data[j].data_max) {
        my_sensor_data[j].data_max = my_sensor_data[j].data_1s;
      }
    }

    // Put all the data into the 10s average info
    for (int j = 0; j < NUM_CHANNELS; j++) {
      // Add on values for next average
      my_sensor_data[j].data_10s_holder += my_sensor_data[j].data_1s;  // This is for the 10s averages
    }

// Output Data to Serial port?:
#ifdef DEBUG_DATA_1S
    Serial.print(F("1s: "));
    for (int y = 0; y < NUM_CHANNELS; y++) {
      Serial.print((String)my_sensor_data[y].data_1s);  // Print the 1 second data
      Serial.print(F("\t :"));
    }
    Serial.print(F(" N: "));
    Serial.println(data_counter_1s);
#endif

    if (send_sensor_data == 0) {
      sendData(0);
      reset_data();
    }

    // Every second we recheck the bits of the UNIT_ID
    UNIT_ID = check_unit_id(UNIT_ID);
    data_counter_10s++;
    data_counter_1s = 0;  // Reset the counter
  }
}

void t10SCallback() {
  if (!t10S.isFirstIteration()) {

    // 10 second averages
    for (int j = 0; j < NUM_CHANNELS; j++) {
      my_sensor_data[j].data_10s = (float)my_sensor_data[j].data_10s_holder / (float)data_counter_10s;
      // reset all the data holders
      my_sensor_data[j].data_10s_holder = 0;  // Reset the value
      // Add on values for next average
      my_sensor_data[j].data_60s_holder += my_sensor_data[j].data_10s;  // This is for the 60S averages
    }

// Output Data to Serial port?:
#ifdef DEBUG_DATA_10S
    Serial.print(F("10s: "));
    for (int y = 0; y < NUM_CHANNELS; y++) {
      Serial.print((String)my_sensor_data[y].data_10s);  // Print the 1 second data
      Serial.print(F("\t :"));
    }
    Serial.print(F(" N: "));
    Serial.println(data_counter_10s);
#endif

    if (send_sensor_data == 1) {
      sendData(1);
      reset_data();
    }
    data_counter_60s++;
    data_counter_10s = 0;
  }
}

void t60SCallback() {
  if (!t60S.isFirstIteration()) {
    // 1 minute averages
    for (int j = 0; j < NUM_CHANNELS; j++) {
      my_sensor_data[j].data_60s = (float)my_sensor_data[j].data_60s_holder / (float)data_counter_60s;
      // reset all the data holders
      my_sensor_data[j].data_60s_holder = 0;  // Reset the value
      // Add on values for next average
      my_sensor_data[j].data_600s_holder += my_sensor_data[j].data_60s;  // This is for the 600S averages
    }

// Output Data to Serial port?:
#ifdef DEBUG_DATA_60S
    Serial.print(F("1s: "));
    for (int y = 0; y < NUM_CHANNELS; y++) {
      Serial.print((String)channels[y].data_60s);  // Print the 1 second data
      Serial.print(F("\t :"));
    }
    Serial.print(F(" N: "));
    Serial.println(data_counter_60s);
#endif
    // Here we want to check if we are regularly sending the data and send if needed:
    if (send_sensor_data == 2) {
      sendData(2);
      reset_data();
    }

    data_counter_600s++;
    data_counter_60s = 0;
  }
}

void t600SCallback() {
  // 10 minute averages
  if (!t600S.isFirstIteration()) {

    for (int j = 0; j < NUM_CHANNELS; j++) {
      my_sensor_data[j].data_600s = (float)my_sensor_data[j].data_600s_holder / (float)data_counter_600s;
      // reset all the data holders
      my_sensor_data[j].data_600s_holder = 0;  // Reset the value
      // Add on values for next average
      my_sensor_data[j].data_3600s_holder += my_sensor_data[j].data_600s;  // This is for the 3600S averages
    }

// Output Data to Serial port?:
#ifdef DEBUG_DATA_600S
    Serial.print(F("1s: "));
    for (int y = 0; y < NUM_CHANNELS; y++) {
      Serial.print((String)channels[y].data_600s);  // Print the 1 second data
      Serial.print(F("\t :"));
    }
    Serial.print(F(" N: "));
    Serial.println(data_counter_600s);
#endif
    // Here we want to check if we are regularly sending the data and send if needed:
    if (send_sensor_data == 3) {
      sendData(3);
      reset_data();
    }
  }
  data_counter_3600s++;
  data_counter_600s = 0;
}

void t3600SCallback() {
  // 1 hour averages
  // 10 minute averages
  if (!t3600S.isFirstIteration()) {
    // 1 hour averages
    for (int j = 0; j < NUM_CHANNELS; j++) {
      my_sensor_data[j].data_3600s = (float)my_sensor_data[j].data_3600s_holder / (float)data_counter_3600s;
      // reset all the data holders
      my_sensor_data[j].data_3600s_holder = 0;  // Reset the value
    }

// Output Data to Serial port?:
#ifdef DEBUG_DATA_3600S
    Serial.print(F("1s: "));
    for (int y = 0; y < NUM_CHANNELS; y++) {
      Serial.print((String)channels[y].data_3600s);  // Print the 1 second data
      Serial.print(F("\t :"));
    }
    Serial.print(F(" N: "));
    Serial.println(data_counter_3600s);
#endif

    // Here we want to check if we are regularly sending the data and send if needed:
    if (send_sensor_data == 4) {
      sendData(4);
      reset_data();
    }
  }
  data_counter_3600s = 0;
}

void tap(Button2& btn) {
  // In this routine we figure out which button has been pressed and deal with that:
  if (btn == buttonA) {
    // If SWA pressed then increment the "send_sensor_data" from 0-1-2-3-4-5 and back to 0.
    // This means data will be shown at 1s/10s/60s/600s/3600s timescales.
    // In this case we start to send data at regular intervals.
    send_sensor_data++;
    if (send_sensor_data >= 6) {
      send_sensor_data = 0;
    }
    EEPROM.write(120, send_sensor_data);  // Update the EEPROM
    Serial.print(F("aaSEND"));
    Serial.print(send_sensor_data);
    Serial.println("#");

    // Also want to flash the LED to show what send mode the unit is in:
    for (int i = 0; i < send_sensor_data; i++) {
      digitalWrite(LED0_PIN, true);
      delay(150);
      digitalWrite(LED0_PIN, false);
      delay(150);
    }
  }
}

void flashLED(int _LED_TIME) {
  if (millis() > (led_flash_timer + _LED_TIME)) {
    digitalWrite(LED0_PIN, true);
    led_flash_timer = millis();
    led_update_flag = true;
  }
  if (millis() > (led_flash_timer + LED_ON_TIME) && led_update_flag == true) {
    digitalWrite(LED0_PIN, !digitalRead(LED0_PIN));
    if (send_sensor_data > 4 || led_flash_counter > 2) {
      digitalWrite(LED0_PIN, false);
      led_update_flag = false;
      led_flash_counter = 0;
    } else {
      led_flash_counter++;
    }
    led_flash_timer = millis();
  }
}

// Start up
void setup() {

  // Read the serial baud rate set in EEPROM
  if (EEPROM.read(EEPROM_SERIAL_BAUD) >= MAX_BAUD_RATES) {
    EEPROM.write(EEPROM_SERIAL_BAUD, 2);  // Update the EEPROM-Initialise to 9600 if data out of range
  }
  // initialize serial communication:
  Serial.begin(baud_rates[EEPROM.read(EEPROM_SERIAL_BAUD)]);

  // reserve bytes for the Strings:
  inputString.reserve(INPUT_STRING_LENGTH);
  returnString.reserve(RETURN_STRING_LENGTH);

  // // Get the m and c wind speed conversion data - if it is NAN then set to simple values (for forst EEPROM save)
  // EEPROM.get(EEPROM_WIND_CON_M, wind_speed_data.wind_speed_conv_m);
  // if (isnan(wind_speed_data.wind_speed_conv_m)) {
  //   wind_speed_data.wind_speed_conv_m = 1.0;
  //   EEPROM.put(EEPROM_WIND_CON_M, wind_speed_data.wind_speed_conv_m);
  // }
  // EEPROM.get(EEPROM_WIND_CON_C, wind_speed_data.wind_speed_conv_c);
  // if (isnan(wind_speed_data.wind_speed_conv_c)) {
  //   wind_speed_data.wind_speed_conv_c = 0.0;
  //   EEPROM.put(EEPROM_WIND_CON_C, wind_speed_data.wind_speed_conv_c);
  // }

  // Get the control int for sending data to serial port
  // This is for constant output data
  send_sensor_data = EEPROM.read(EEPROM_SEND_DATA);

  // Read in the digital pins to check the Unit ID
  // This reads in three digital pins to set the Unit ID
  // UNIT_ID is a 8 bit byte
  // So we can have up to 8 IDs
  pinMode(GPIO_ID0, INPUT_PULLUP);
  pinMode(GPIO_ID1, INPUT_PULLUP);
  pinMode(GPIO_ID2, INPUT_PULLUP);

  // Here we set the bits of the UNIT_ID
  UNIT_ID = check_unit_id(UNIT_ID);

  analogReference(DEFAULT);

  // This is re-checked every second or after reset
  if (DEBUG_FLAG == true) {
    Serial.print(F("ID is: "));
    Serial.println(UNIT_ID);
  }

#ifdef USE_I2C
  Wire.begin();  // Start the I2C bus  - ONLY if needed?
  delay(10);
#endif

  // Flash LEDs for test
  pinMode(LED0_PIN, OUTPUT);
  digitalWrite(LED0_PIN, HIGH);
  delay(100);
  digitalWrite(LED0_PIN, LOW);

  // Set up the HC12 power output
  pinMode(HC12_PWR_PIN, OUTPUT);
  digitalWrite(HC12_PWR_PIN, LOW);

  digitalWrite(LED0_PIN, HIGH);
  delay(100);
  digitalWrite(LED0_PIN, LOW);

  pinMode(SWA_PIN, INPUT_PULLUP);
  buttonA.setDebounceTime(1000);
  buttonA.setTapHandler(tap);

  // // Sort out ISR for pulse counting
  // attachInterrupt(digitalPinToInterrupt(FREQ_PIN), ISR_PULSE_0, FALLING);

  // Set up Scheduler:
  runner.init();
  runner.addTask(t1);
  runner.addTask(t1S);
  runner.addTask(t10S);
  runner.addTask(t60S);
  runner.addTask(t600S);
  runner.addTask(t3600S);

  t1.enable();
  t1S.enable();
  t10S.enable();
  t60S.enable();
  t600S.enable();
  t3600S.enable();
  DEBUGLN(DEBUG_FLAG, "Enabled Tasks");

  // set up the watch dog timer:
  wdt_enable(WDTO_4S);  // set for 4 seconds
}

// Main Loop
void loop() {
  // Deal with schedules tasks
  runner.execute();
  // Check buttons for input
  buttonA.loop();
  // This flashes the LEDs as needed...
  flashLED(LED_FLASH_TIME);
  // feed the WDT
  wdt_reset();

  // Check for serial data
  if (stringComplete == true) {
    // This means we have had some data come in:
    // So parse it to find ID etc
    returnString = checkData.parseData(inputString, UNIT_ID, my_sensor_data);

    // Deal with any flags set:
    if (checkData.button_press_flag == true) {
      tap(buttonA);
      checkData.button_press_flag = false;  //Reset the flag
    }


    // Add the CRC if needed:
    if (ADD_CRC_CHECK) {
      // Add the CRC here: This adds the ? the CRC and the # to the end
      returnString = add_CRC(returnString);
    } else {
      returnString += F("#");
    }
    Serial.println(returnString);
    inputString = "";
    stringComplete = false;



    // if (checkData.error_flag == true) {
    //   if (ADD_CRC_CHECK) {
    //     // Add the CRC here: This adds the ? the CRC and the # to the end
    //     errorString = add_CRC(errorString);
    //   } else {
    //     errorString += "#";
    //   }
    //   Serial.println(errorString);  // This needs to be returned
    //   checkData.error_flag = false;
    // } else {
    //   if (checkData.baud_return_flag == true) {
    //     // Here we return the baud rate
    //     returnString = "aaI";
    //     returnString += (String)UNIT_ID;
    //     returnString += "BD";
    //     returnString += baud_rates[SERIAL_BAUD];
    //     checkData.baud_return_flag = false;
    //     checkData.data_sent_flag = false;
    //     // } else if (checkData.vane_data_flag == true) {
    //     //   returnString = "aaI";
    //     //   returnString += (String)UNIT_ID;
    //     //   returnString += "WV=";
    //     //   returnString += wind_vane_data.return_direction(analogRead(VANE_PIN));  // Print the instantaneous value of the wind vane as a direction

    //     //   for (int y = 0; y < 8; y++) {
    //     //     returnString += ":";
    //     //     returnString += (String)wind_vane_data.direction_array_values[y];
    //     //   }
    //     //   checkData.vane_data_flag = false;
    //     //   checkData.data_sent_flag = false;
    //     // } else if (checkData.conversion_return_flag == true) {
    //     //   // Return conversion settings "aaI0STWSCONm123.4c567.89#"
    //     //   returnString = "aaI";
    //     //   returnString += (String)UNIT_ID;
    //     //   returnString += "WVCONm";
    //     //   returnString += (String)wind_speed_data.wind_speed_conv_m;
    //     //   returnString += "c";
    //     //   returnString += (String)wind_speed_data.wind_speed_conv_c;
    //     //   checkData.conversion_return_flag = false;
    //     //   checkData.data_sent_flag = false;
    //     // } else if (checkData.conversion_set_flag == true) {
    //     //   // Return conversion settings "aaI0STWSCONm123.4c567.89#"
    //     //   wind_speed_data.wind_speed_conv_m = checkData.wind_speed_conv_m;
    //     //   wind_speed_data.wind_speed_conv_c = checkData.wind_speed_conv_c;
    //     //   // Store this data to EEPROM
    //     //   EEPROM.put(EEPROM_WIND_CON_M, wind_speed_data.wind_speed_conv_m);
    //     //   EEPROM.put(EEPROM_WIND_CON_C, wind_speed_data.wind_speed_conv_c);
    //     //   returnString = "aaI";
    //     //   returnString += (String)UNIT_ID;
    //     //   returnString += "WVSETm";
    //     //   returnString += (String)wind_speed_data.wind_speed_conv_m;
    //     //   returnString += "c";
    //     //   returnString += (String)wind_speed_data.wind_speed_conv_c;
    //     //   checkData.conversion_set_flag = false;
    //     //   checkData.data_sent_flag = false;
    //   } else if (checkData.baud_set_flag == true) {
    //     Serial.end();
    //     // Here we set the baud rate to a new value and store in EEPROM
    //     returnString = "aaI";
    //     returnString += (String)UNIT_ID;
    //     returnString += "STBD";
    //     // Read the serial baud rate set in EEPROM
    //     SERIAL_BAUD = EEPROM.read(EEPROM_SERIAL_BAUD);
    //     if (SERIAL_BAUD >= MAX_BAUD_RATES) {
    //       SERIAL_BAUD = 2;  // Initialise to 9600 if data out of range
    //     }
    //     // re-initialize serial communication:
    //     Serial.begin(baud_rates[SERIAL_BAUD]);
    //     returnString += baud_rates[SERIAL_BAUD];
    //     checkData.baud_set_flag = false;
    //     checkData.data_sent_flag = false;
    //   } else if (checkData.data_sent_flag == true) {
    //     if (checkData.data_all_flag == true) {
    //       //******** RETURN ALL DATA FOR AVE *************************
    //       DEBUGLN(DEBUG_FLAG, "Return ALL DATA");
    //       returnString = "aaI";
    //       returnString += (String)UNIT_ID;
    //       returnString += "WSA" + (String)checkData.ave_time;  // Initialise the returned string
    //       returnString += ":";
    //       for (int c = 0; c < 5; c++) {
    //         if (checkData.ave_time == c) {
    //           switch (c) {
    //             case 0:
    //               for (int y = 0; y < NUM_CHANNELS; y++) {
    //                 returnString += (String)my_sensor_data[y].data_1s;  // Print the 1 second data
    //                 returnString += F(":");
    //               }
    //               break;
    //             case 1:
    //               for (int y = 0; y < NUM_CHANNELS; y++) {
    //                 returnString += (String)my_sensor_data[y].data_10s;  // Print the 1 second data
    //                 returnString += F(":");
    //               }
    //               break;
    //             case 2:
    //               for (int y = 0; y < NUM_CHANNELS; y++) {
    //                 returnString += (String)my_sensor_data[y].data_60s;  // Print the 1 second data
    //                 returnString += F(":");
    //               }
    //               break;
    //             case 3:
    //               for (int y = 0; y < NUM_CHANNELS; y++) {
    //                 returnString += (String)my_sensor_data[y].data_600s;  // Print the 1 second data
    //                 returnString += F(":");
    //               }
    //               break;
    //             case 4:
    //               for (int y = 0; y < NUM_CHANNELS; y++) {
    //                 returnString += (String)my_sensor_data[y].data_3600s;  // Print the 1 second data
    //                 returnString += F(":");
    //               }
    //               break;
    //           }
    //         }
    //       }
    //       checkData.data_all_flag = false;
    //       checkData.data_sent_flag = false;
    //     } else if (checkData.data_min_flag == true) {
    //       //******** RETURN ALL MINIMUMS *************************
    //       DEBUGLN(DEBUG_FLAG, "Return ALL MINIMUMS");
    //       returnString = "aaI";
    //       returnString += (String)UNIT_ID;
    //       returnString += "WSMN";  // Initialise the returned string
    //       returnString += ":";
    //       for (int y = 0; y < NUM_CHANNELS; y++) {
    //         returnString += (String)my_sensor_data[y].data_min;  // Print the 1 second data
    //         returnString += F(":");
    //       }
    //       checkData.data_min_flag = false;
    //       checkData.data_sent_flag = false;
    //     } else if (checkData.data_max_flag == true) {
    //       //******** RETURN ALL MAXIMUMS  *************************
    //       DEBUGLN(DEBUG_FLAG, "Return ALL MAXIMUMS");
    //       returnString = "aaI";
    //       returnString += (String)UNIT_ID;
    //       returnString += "WSMX";  // Initialise the returned string
    //       returnString += ":";
    //       for (int y = 0; y < NUM_CHANNELS; y++) {
    //         returnString += (String)my_sensor_data[y].data_max;  // Print the 1 second data
    //         returnString += F(":");
    //       }
    //       checkData.data_max_flag = false;
    //       checkData.data_sent_flag = false;
    //     }
    //   } else if (checkData.data_reset_flag == true) {
    //     // Want to reset the wind vane direction data as previous will be wrong!
    //     reset_data();
    //     returnString = F("aaRESET");  // This needs to be returned
    //     checkData.data_reset_flag = false;
    //     checkData.data_sent_flag = false;
    //   } else if (checkData.send_sensor_data_flag == true) {
    //     // In this case we start to send data at regular intervals.
    //     send_sensor_data = EEPROM.read(120);
    //     returnString = F("aaSENDOK");  // This needs to be returned
    //     checkData.send_sensor_data_flag = false;
    //     checkData.data_sent_flag = false;
    //   } else {
    //     returnString = F("aaFAILCRC");
    //     checkData.data_sent_flag = false;
    //   }

    //   if (checkData.button_press_flag == true) {
    //     tap(buttonA);
    //   } else {
    //     if (ADD_CRC_CHECK) {
    //       // Add the CRC here: This adds the ? the CRC and the # to the end
    //       returnString = add_CRC(returnString);
    //     } else {
    //       returnString += "#";
    //     }
    //     DEBUGLN(DEBUG_FLAG, "Returned string:");
    //     Serial.println(returnString);
    //   }
    // }
    // inputString = "";
    // stringComplete = false;
  }
}

/*
  SerialEvent occurs whenever a new data comes in the hardware serial RX. This
  routine is run between each time loop() runs, so using delay inside loop can
  delay response. Multiple bytes of data may be available.
*/
void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
}



void sendData(int average_time) {
  // Send 1 second data
  digitalWrite(HC12_PWR_PIN, HIGH);
  delay(POWER_SETTLE_TIME);  // short settling delay

  returnString = checkData.showChannelData(average_time, UNIT_ID, my_sensor_data);

  if (ADD_CRC_CHECK) {
    // Add the CRC here: This adds the ? the CRC and the # to the end
    returnString = add_CRC(returnString);
  } else {
    returnString += F("#");
  }
#ifdef ADD_CRC_CHECK
  returnString = add_CRC(returnString);
#else
  returnString += F("#");
#endif

  Serial.println(returnString);

  delay(POWER_SETTLE_TIME);  // short settling delay
  digitalWrite(HC12_PWR_PIN, LOW);
}

void reset_data() {
  for (int j = 0; j < NUM_CHANNELS; j++) {
    // Here we reset the max and min for the data
    my_sensor_data[j].data_min = 999999;
    my_sensor_data[j].data_max = -999999;
  }
}