
# Sensor Interface Board

A unit to interface various sensors (digital/analog/RS485) with a simple serial interface via an ATMega328-based circuit. The unit also provides averaged data, controlled stepped-up supply voltage and a field-adjustable ID.

"Why do you need that?", you ask.... 

I had been wanting to use an industrial soil mositure sensor for a simple project. This device was only available as an RS3485 device. So I ordered that - it then took days of head scratching to sort basic communication with the device. This unit is meant to simplify reading of avaeraged data from RS485 sensors and make it easy to get that data into your project via a simple serial request/reply set of commands.

The other issue with a lot of the RS485 sensor is that they need higher voltages supplied (such as 12V or even 24V DC). So on this interface board I added a DC-DC step up converter to power the sensor. 

I was also concerned with energy consumption (my project was battery based). If I was to leave the DC-DC converter running all the time then my battery would run down very quickly. So I added control of the power to the sensor, so that I can power up, take the readings then power down. This saves a huge ammount of energy (well, proportional to my battery size).

## Overview

The problem with measuring sensors with values that change relatively quickly is that they constantly need to be checked. This requires a bit of microcontroller time and processing. This unit is designed to solve that.

Wire up your RS485 sensor. Power the unit up. Then it will save the averaged data for you. You can then get hold of the data through serial requests and process as you need.

![Overview](https://github.com/curiouselectric/SensorInterfaceBoard/blob/1cc6fdd7eef303f40e9d5f870216e7cde911cf6a/RS485%20Interface%20Board%20Instructions/Images/Sensor%20Interface%20overview.png)

I wrote this to interface to an ESP32 data logger, which sleeps most of the time. It wakes up, talks to the RS485 sensor, gets the data it needs, then goes back to sleep, knowing the RS485 Interface Board is always monitoring.

It was designed as a relatively simple interface to remove the need for monitoring pulses/dealing with RS485 requests/getting sensor data and averaging the sensor data.

The brilliant [ESPHome project](https://esphome.io/components/#environmental) has loads of sensor types you can interafce easily with a low cost ESP microcontrollers. For systems that have WiFi and good power connections then that project is a good resource to use. This project is designed for very low power systems (battery based datalogging) and for situations where easy access to Wi-Fi is not available.

There are two mode of operation, depending upon your use case:

## Response Mode

In this mode then the unit responds to serial requests made. You ask the sensor for data and this is returned. It never sends anything unless asked.

![Response](https://github.com/curiouselectric/SensorInterfaceBoard/blob/1cc6fdd7eef303f40e9d5f870216e7cde911cf6a/RS485%20Interface%20Board%20Instructions/Images/Sensor%20Interface%20response.png)

## Broadcast Mode

In this mode then the unit regularly sends data via the serial connunication. It will send the averaged data for (0) 1 second, (1) 10 second, (2) 1 min, (3) 10 min and (4) 1 hour averaged data. If this is set to (5) then the unit does not send any data. The send mode is stored in EEPROM, so it will start sending data again even if power is lost.

![Broadcast](https://github.com/curiouselectric/SensorInterfaceBoard/blob/1cc6fdd7eef303f40e9d5f870216e7cde911cf6a/RS485%20Interface%20Board%20Instructions/Images/Sensor%20Interface%20%20broadcast.png)

Boradcast mode works well if the logger is always listening and you only have one sensor in range. If more than one sensor is in range then the data will clash and potentially cause issues, in which case use Response mode.

The two modes work together - you can have the unit sending regular data and also responding to requests.

It runs on an ATMega328 running at 16MHz with selectable baud serial (up to 57600). It comes pre-programmed, but new code can be uploaded via the Arduino IDE, with the Uno bootloader. See firmware for more details.


## Sensor Types

There are a huge range of RS485 sensors for many different variables. I'd like this unit to be adaptable to work with a variety of sensors with minila code changes.

This code base also works with other, non-RS485 sensors, such as wind speed sensors, wind vanes and solar irradiance sensors.

## Sensors Implemented:
This is a list of the sensor types which have been implemented with this software, Along with the version number they were added.

Sensor Name                |  Ref  |  Version | Device Type  |   ID                   | Link                                                 | Notes  |
---------------------------|-------|----------|--------------|------------------------|------------------------------------------------------|--------|
Soil Temperature & Humidity|ZTS-3000|1.0      | SM           |ZTS-3000-TR-WS-N01     |[https://www.curiouselectric.co.uk/products/soil-moisture-sensor](https://www.curiouselectric.co.uk/products/soil-moisture-sensor)|        |
Wind Sensor                |       |          | WT           |                         |[https://www.curiouselectric.co.uk/collections/monitoring/products/wind-sensor-interface](https://www.curiouselectric.co.uk/collections/monitoring/products/wind-sensor-interface)  |        |
Irradiance Sensor          |       |          | IR           |                         | |        |
PAR Sensor                 |       |          | PR           |                         | |        |


# Hardware

A number of PCBs have been designed in KiCAD. This repository holds the RS485 sensor and is available here. A small PCB has been designed.

There is one reset switch, one user input switch and one LED output. 

There is a step-up DC-DC converter with power control (to power higher voltage sensors). 

There is a TTL to RS485 converter (to connect to the RS485 sensors)

There is a 4 pin 'Grove'-type connector for direct connection I2C (code for this is not yet implemented) 

## Board ID Number

Each unit can have a unique ID (using a push link 6 pin pad for 0-7 values), so multiple units can be added to a serial bus, if needed. The defalt is 0 (no links used).

## PCB User Switch and User LED

There is one user switch and one user LED on the unit.

The LED will show a regular flash every 5 seconds. This will briefly flash once every 5 seconds if the unit is in 'Response' mode. The LED will briefly flash twice every 5 seconds if the unit is in 'Broadcast' mode. Data will be sent at the broadcast rate.

The LED will also flash whenever data is sent of the serial port. The LED will go on before data sent and then off after data is sent.

Pressing the user switch for >0.5 seconds and then releasing will result in a switch press.

A switch press will increment the mode from 0-1-2-3-4-5 then back to 0.

The unit will flash after a button press to indicate the broadcast mode (so 0 flashes if the value is 0, 1 flash if the value is 1 etc).

If this is set to 5 then the unit works in 'Response' mode.

If this is set to 0-4 then the unit is in 'Broadcast' mode and will send the data at the relevant interval (0 = 1s, 1 = 10s, 2 = 1 min, 3 = 10 min and 4 = 1 hour).

The mode can also be set with a serial request, using the "Set the unit to broadcast:" method (see below).

# Firmware

This uses an ATMega328 with the MiniCore Bootloader. This uses an oscillator running at 16MHz for 5V supply or 8MHz for 3.3V supply. Check the correct oscillator is chosen when uploading.

## Initial bootloader installation:

You should not need to do this, as the unit should come with this already installed. This is just for information.

To upload a bootloader it then MiniCore is used:

Install MiniCore from here: https://github.com/MCUdude/MiniCore

Install the bootloader using an Arduino as an ISP. https://www.arduino.cc/en/Tutorial/BuiltInExamples/ArduinoISP

Wire up your arduino and an ISP 3x2 header pin onto the wind sensor PCB.

Choose the "ATMega328" option with the "External 8MHz Oscillator" set for the 3.3V version or "External 16MHz Oscillator" set for the 5V version, depending upon the crystal supplied.

The ATmega328 datasheet says that an 8MHz crystal should be used at 3.3V for reliable serial comms. YMMV. 

You can then use the 'Burn Bootloader' option within 'Tools' in the Arduino IDE. This will install the Minicore bootloader.


## Program via Arduino IDE

You can only do this when the bootloader is programmed (this will be programmed for versions supplied by me).

To program it then MiniCore is used:

Install MiniCore from here: https://github.com/MCUdude/MiniCore

Add to preferences and then board manager.

Choose the "ATMega328" option with the "External 8MHz Oscillator" set for the 3.3V version or "External 16MHz Oscillator" set for the 5V version, depending upon the crystal supplied.

The ATmega328 datasheet says that an 8MHz crystal should be used at 3.3V for reliable serial comms. YMMV. 

You can then upload code using the 6pin serial/FTDI interface on the board.

# Serial Data and Commands

It returns the average values and information when requested on serial port.

If the 8-bit CRC (Cyclic Redundancy Check) is used then each request must have a 2 char CRC added between the ? and # within these commands (labelled ^^ here).
For the responses, if no CRC enabled then the ?^^ within these commands is NOT returned. You must use the ? for a command, but the response will not contain it.

At all other times then the unit is asleep.

## Set the unit to broadcast:

Request: "aaI0SEND*?^^#" where * is an int (0)= 1s data, (1)= 10s data, (2)= 60s/1 min data, (3)= 600s/10 min data, (4)= 3600s/1hr data, (5)= NO data

Returns: "aaI0SEND*?^^#" + CRC if requested where * is an int (0)= 1s data, (1)= 10s data, (2)= 60s/1 min data, (3)= 600s/10 min data, (4)= 3600s/1hr data, (5)= NO data

You can also set the unit to broadcast using the user switch. Press the button for around 0.5s or more then release. This will go through the boradcast modes from 0-1-2-3-4-5 then back round to 0. The LED will flash the number of times for the setting (so send = 0 the unit will not flash, but data will appear within 1 second!).

If the unit is in broadcast mode then the minimum and maximum wind speeds and the wind vane data are all reset each time period.

## What is baud rate?:

Request: "aaI0STBD?#" ("aaI0BD?dc#" with CRC)

Returns: "aI0CHBD9600?^^#"  // Where 9600 is the baud rate 

## Set Baud Rate:

Request: "aaI0STBD\*?^^#"  Where \* is (0)1200, (1)2400, (2)9600, (3)57600, (4)115200

Returns: "aaI0BD9600?^^#"   // Where 9600 is the baud rate + CRC if requested

## What is ID?:

Mentioned at start up of unit - it is hardware set... and cannot be changed in code.

ID selection is by using a shorting link on the pads labelled 1, 2 and 4. The default is for no pads to be connected and the ID is 0. This means the unit will respond to "I0" as the ID.

To change the ID to another number from 0-7 then use a shorting link on the pins to create a binay number. The connections are:

1    |2    |4     | ID
-------|-------|-------|----
NC     |NC     |NC     | 0
CONN |NC     |NC     | 1
NC     |CONN |NC     | 2
CONN |CONN |NC     | 3
NC     |NC     |CONN | 4
CONN |NC     |CONN | 5
NC     |CONN |CONN | 6
CONN |CONN |CONN | 7

You can request the ID:

Request: "aaI*ID?^^#"  The * value can be anything.

Returns: "aaIXID:X?^^#"   // Where X is the device ID + CRC if requested

## Request Data from ONE channel

Request: “aaI0R00A*?^^#” where 00 is the channel number (00, 01 etc) and * is the averaging period (* = 0 for 1 sec data, 1 for 10 sec data, 2 for 60 sec (1 min) data, 3 for 600 sec (10 min) data and 4 for 3600 sec (1 hr) data 

Returns:  "aaI0R00A*:123.40:123.40:123.40?^^#"  // This wil return the average data then : then the minimum value then : then the maxximum value.

## Request Data from ALL channels

Request: “aaI0RAAA3?^^#”

Returns: "aaI0RAAA1:123.40:567.80?^^#" where the first item of data is channel 0, the next is channel 1. This will depend upon the number of channels.

## Request ALL Minimum data

Request: “aaI0RMNA4?^^#”  - does not matter what averaging period. min/max are just the min/max seen at max data rate.

Returns: "aaI0RMN:123.40:567.80?^^#"  where the first item of data is channel 0, the next is channel 1. This will depend upon the number of channels.

## Request ALL Maximum data

Request: “aaI0RMXA0?^^#”  - does not matter what averaging period. min/max are just the min/max seen at max data rate.

Returns: "aaI0RMX:123.40:567.80?^^#"  where the first item of data is channel 0, the next is channel 1. This will depend upon the number of channels.

## Reset the Min and Max value:

The min and max of each channel are logged. These are reset if the data is sent in broadcast mode. If the unit is in Response mode then you need to reset the min/max when you have taken the data.

Request: "aaI0RESET?#" ("aaI0RESET?d9#" with CRC)

Returns: "aaI0RSTOK#"

## Serial 'Button' press

You can simulate a button press with a serial command. This might be useful for some logging applications

The command "aaI0SWA?^^#"  (Where ^^ is the CRC of the data before the ?) will act just like a button press. This is for control via a data logger serial port without access to the physical switch.

Request: "aaI0SWA?^^#" 

Returns: "aaSEND*?^^#", where * is the time interval to send data (0->1->2->3->4->5->0 etc, with 5 never sending)

## What is the software version?:

Request: "aaI0SWV?^^#"

Returns: "aaI0SWV:1.0?^^#" (where 1.0 is the software version)

## What is the device type?:

Request: "aaI0DT?^^#""

Returns: "aaI0DT:SM?^^#" where SM is the device type (SM = Soil Moisture) - check the sensor list for these 2 char codes.

## Add CRC check:

Within the config of the firmware a CRC (Cyclic Redundancy Check) can be added to the data (or not!).

Set this true using the flag in the config.h file:

  #define ADD\_CRC\_CHECK     true    // Use this to add CRC check to incomming and outgoing messages
  
This uses the CRC routines from Rob Tillaart, available here: https://github.com/RobTillaart/CRC

A 'CRC-8/SMBUS' is perfromed on the data and a 2 byte CRC code is added to all replys (and expected on all enquiries). This is added between a ? and # symbol.

If no CRC then the end char is a #.

For example: aaI0RESET?d9# has the CRC check d9 added to the reset request.

Remember: Capitalisation will affect the results: D is not the same as d!

You can use this online calculator to check your CRC: https://crccalc.com/ The type of CRC is CRC-8/SMBUS.

## Failure codes:

If data is not that length or does not have 'aa' and '#' at start/end then return with send "aaFAIL\*\*#" error code. All will have CRC on these codes, if requested.

+ "aaFAILCRC?^^#": CRC check fail
+  "aaFAILTL?^^#": String too long
+  "aaFAILID?^^#": Problem with ID
+  "aaFAILIDX?^^#": ID not correct to device
+  "aaFAILPE?^^#": No aa and # on the string
+  "aaFAILBD?^^#": Baud rate change fail
+  "aaFAILCN?^^#": Channel number requested is greater than channels existing
+  "aaFAILCMD?^^#": Command not recognised 


# Sensor Specific Commands
For each sensor type there are additional commands. These are only available if the unit is in the correct mode.
They are listed here.

## 'SM' Soil Mositure Sensor

There are no extra commands.


## 'WS' Wind Speed Sensor (Vane and Anemometer)

### Wind Speed data:

Request: “aaI0WSA4#”  ("aaI0WSA4?19#" with CRC)  Where 0 is an ID from 0-7 set by solder on PCB. 4 is the averaging period (0=1s, 1=10s, 2 = 60s, 3 = 600s, 4=3600s)

Returns: "aaI0WSA4:3.00:5.67:1.23#"  // Where 4 is the averaging period, 3.00 is the data within the averaging period, 5.67 is the maximum and 1.23 is the minimum.

### Wind Speed data minimum:

Request: “aaI0RMN#” ("aaI0RMN?^^#" with CRC) - does not matter what averaging period. min/max are just the min/max seen.

Returns: "aaI0RMN:3.00#"  // Where 3.00 is the data + CRC if requested

### Wind Speed data maximum:

Request: “aaI0RMX#”  ("aaI0RMX?^^#" with CRC) - does not matter what averaging period. min/max are just the min/max seen.

Returns: "aaI0RMX:3.00#"  // Where 3.00 is the data + CRC if requested

### What is Anemometer conversion?:

Request: "aaI0WSCON#" ("aaI0WSCON?41#" with CRC)

Returns: "aaI0STWSCONm123.4c567.89#" (from stored values) + CRC if requested

### Set the Anemometer conversion:

Request: "aaI0WSSTm123.4c567.89#"  ("aaI0WSSTm123.4c567.89?38#" with CRC) Where 123.4 is the gradient and 567.89 is the constant (y=mx+c)

Returns: "aaI0STWSSETm123.4c567.89#" (set to the new values) + CRC if requested

Request: "aaI0WSSTm1c0#" or "aaI0WSSTm1c0?da#" with CRC to set m= 1 and c=0. This is useful for initial testing.

### Wind Vane data:

Request: “aaI0WV#”  ("aaI0WV?b4#" with CRC) Where 0 is an ID from 0-7 set by solder on PCB.

Returns:    The instantaneuous direction AND the direction array data

Returns:    "aaI0WV=W:0.00:0.00:0.00:0.00:0.00:0.00:62.00:0.00#" + CRC if requested

### Reset the max, min and wind vane array:

Request: "aaI0RESET#" ("aaI0RESET?d9#" with CRC)

Returns: "aaRESET#"

### Enter vane training mode:

Request: "aaI0VT?#" ("aaI0VT?af#" with CRC) OR long press of the button (>1 second) will enter this mode.

Returns: Enter the vane training routine - use button to go through the different directions and set the values.

The serial port will show which direction the vane should be pointing at.

Move the vane to this position and press the user switch (for around 0.5 seconds).

The serial port will show then next direction and will got N, NE, E, SE, S, SW, W, NW and then end.

The unit will also send "aaI0WVOK=NW" + CRC +"#" to report back which direction the unit is now being trained.

When it ends this data is stored within the unit and the direction 'bands' are recaluclated.

# Overview of Connections

