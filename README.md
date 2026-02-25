
# RS485InterfaceBoard

A unit to interface various RS485 sensors with a simple serial interface via an ATMega328-based circuit. The unit also provides averaged data, controlled stepped-up supply voltage and a field-adjustable ID.

"Why do you need that?", you ask.... 

I had been wanting to use an industrial soil mositure sensor for a simple project. This device was only available as an RS3485 device. So I ordered that - it then took days of head scratching to sort basic communication with the device. This unit is meant to simplify reading of avaeraged data from RS485 sensors and make it easy to get that data into your project via a simple serial request/reply set of commands.

The other issue with a lot of the RS485 sensor is that they need higher voltages supplied (such as 12V or even 24V DC). So on this interface board I added a DC-DC step up converter to power the sensor. 

I was also concerned with energy consumption (my project was battery based). If I was to leave the DC-DC converter running all the time then my battery would run down very quickly. So I added control of the power to the sensor, so that I can power up, take the readings then power down. This saves a huge ammount of energy (well, proportional to my battery size).

## Overview

The problem with measuring sensors with values that change relatively quickly is that they constantly need to be checked. This requires a bit of microcontroller time and processing. This unit is designed to solve that.

Wire up your RS485 sensor. Power the unit up. Then it will save the averaged data for you. You can then get hold of the data through serial requests and process as you need.

![Overview](https://github.com/curiouselectric/RS485InterfaceBoard/blob/779b627fa3202d53ca6c4beaae3cd90f97ab3141/RS485%20Interface%20Board%20Instructions/Images/RS485%20Interface%20overview.png?raw=true)

I wrote this to interface to an ESP32 data logger, which sleeps most of the time. It wakes up, talks to the RS485 sensor, gets the data it needs, then goes back to sleep, knowing the RS485 Interface Board is always monitoring.

It was designed as a relatively simple interface to remove the need for monitoring pulses/dealing with RS485 requests/getting sensor data and averaging the sensor data.

The brilliant [ESPHome project](https://esphome.io/components/#environmental) has loads of sensor types you can interafce easily with a low cost ESP microcontrollers. For systems that have WiFi and good power connections then that project is a good resource to use. This project is designed for very low power systems (battery based datalogging) and for situations where easy access to Wi-Fi is not available.

There are two mode of operation, depending upon your use case:

## Response Mode

In this mode then the unit responds to serial requests made. You ask the sensor for data and this is returned. It never sends anything unless asked.

![Response](https://github.com/curiouselectric/RS485InterfaceBoard/blob/048843a24f9ec3a42a6946c3067e17d5d328104e/RS485%20Interface%20Board%20Instructions/Images/RS485%20Interface%20response.png?raw=true)

## Broadcast Mode

In this mode then the unit regularly sends data via the serial connunication. It will send the averaged data for (0) 1 second, (1) 10 second, (2) 1 min, (3) 10 min and (4) 1 hour averaged data. If this is set to (5) then the unit does not send any data. The send mdoe is stored in EEPROM, so it will start sending data again even if power is lost.

![Broadcast](https://github.com/curiouselectric/RS485InterfaceBoard/blob/048843a24f9ec3a42a6946c3067e17d5d328104e/RS485%20Interface%20Board%20Instructions/Images/RS485%20Interface%20%20broadcast.png?raw=true)

Boradcast mode works well if the logger is always listening and you only have one sensor in range. If more than one sensor is in range then the data will clash and potentially cause issues, in which case use Response mode.

The two modes work together - you can have the unit sending regular data and also responding to requests.

It runs on an ATMega328 running at 8MHz with selectable baud serial (up to 57600). It comes pre-programmed, but code can be uploaded via the Arduino IDE, using the MiniCore board add-on. See firmware for more details.


## RS485 Sensor Types

There are a huge range of RS485 sensors for many different variables. I'd like this unit to be adaptable to work with a variety of sensors with minila code changes.

This code base also works with other, non-RS485 sensors, such as wind speed sensors, wind vanes and solar irradiance sensors.


## Sensors Implemented:
This is a list of the sensor types which have been implemented with this software, Along with the version number they were added.



Sensor Name                |         Ref         | ID                 | Link                                                 | Notes  |
---------------------------|---------------------|--------------------|-----------------------------------------------------|--------|
Soil Temperature & Humidity| ZTS-3000            | ZTS-3000-TR-WS-N01 |[https://www.curiouselectric.co.uk/products/soil-moisture-sensor](https://www.curiouselectric.co.uk/products/soil-moisture-sensor)|        |






# Hardware

The PCB was designed in KiCAD and is available here. A small PCB has been designed.

There is one reset switch, one user input switch and one LED output.


## Board ID Number

Each unit can have a unique ID (using a push link 6 pin pad for 0-7 values), so multiple units can be added to a serial bus, if needed. The defalt is 0 (no links used).


## PCB User Switch and User LED

There is one user switch and one user LED on the unit.

The LED will show a regular flash every 5 seconds. This will briefly flash once every 5 seconds if the unit is in 'Response' mode. The LED will briefyl flash twice every 5 seconds if the unit is in 'Broadcast' mode. Data will be sent at the broadcast rate.

The LED will also flash whenever data is sent of the serial port. The LED will go on before data sent and then off after data is sent.

Pressing the user switch for >0.5 seconds and then releasing will result in a switch press.

A switch press will increment the mode from 0-1-2-3-4-5 then back to 0.

The unit will flash after a button press to indicate the broadcast mode (so 0 flashes if the value is 0, 1 flash if the value is 1 etc).

If this is set to 5 then the unit works in 'Response' mode.

If this is set to 0-4 then the unit is in 'Broadcast' mode and will send the data at the relevant interval (0 = 1s, 1 = 10s, 2 = 1 min, 3 = 10 min and 4 = 1 hour).

The mode can also be set with a serial request, using the "Set the unit to broadcast:" method (see below).

# Firmware

This uses an ATMega328 running at 16MHz with 3.3v or 5V supply.

## Initial bootloader installation:



You should not need to do this, as the unit should come with this already installed. This is just for information.

To upload a bootloader it then MiniCore is used:

Install MiniCore from here: https://github.com/MCUdude/MiniCore

Install the bootloader using an Arduino as an ISP. https://www.arduino.cc/en/Tutorial/BuiltInExamples/ArduinoISP

Wire up your arduino and an ISP 3x2 header pin onto the wind sensor PCB.

Choose the "ATMega328" option with the "External 8MHz Oscillator" set.

You can then use the 'Burn Bootloader' option within 'Tools' in the Arduino IDE. This will install the Minicore bootloader.

## Program via Arduino IDE

To program it then MiniCore is used:

Install MiniCore from here: https://github.com/MCUdude/MiniCore

Add to preferences and then board manager.

You can then upload code by choosing the "ATMega328" option with the "External 8MHz Oscillator" set.

# Serial Data and Commands

It returns the average values and information when requested on serial port.

At all other times then the unit is asleep.

## Wind Speed data:

Request: “aaI0WSA4#”  ("aaI0WSA4?19#" with CRC)  Where 0 is an ID from 0-7 set by solder on PCB. 4 is the averaging period (0=1s, 1=10s, 2 = 60s, 3 = 600s, 4=3600s)

Returns: "aaI0WSA4:3.00:5.67:1.23#"  // Where 4 is the averaging period, 3.00 is the data within the averaging period, 5.67 is the maximum and 1.23 is the minimum.

## Wind Speed data minimum:

Request: “aaI0WSMN#” ("aaI0WSMN?84#" with CRC) - does not matter what averaging period. min/max are just the min/max seen.

Returns: "aaI0WSMN:3.00#"  // Where 3.00 is the data + CRC if requested

## Wind Speed data maximum:

Request: “aaI0WSMX#”  ("aaI0WSMX?e6#" with CRC) - does not matter what averaging period. min/max are just the min/max seen.

Returns: "aaI0WSMX:3.00#"  // Where 3.00 is the data + CRC if requested

## What is Anemometer conversion?:

Request: "aaI0WSCON#" ("aaI0WSCON?41#" with CRC)

Returns: "aaI0STWSCONm123.4c567.89#" (from stored values) + CRC if requested

## Set the Anemometer conversion:

Request: "aaI0WSSTm123.4c567.89#"  ("aaI0WSSTm123.4c567.89?38#" with CRC) Where 123.4 is the gradient and 567.89 is the constant (y=mx+c)

Returns: "aaI0STWSSETm123.4c567.89#" (set to the new values) + CRC if requested

Request: "aaI0WSSTm1c0#" or "aaI0WSSTm1c0?da#" with CRC to set m= 1 and c=0. This is useful for initial testing.

## Wind Vane data:

Request: “aaI0WV#”  ("aaI0WV?b4#" with CRC) Where 0 is an ID from 0-7 set by solder on PCB.

Returns:    The instantaneuous direction AND the direction array data

Returns:    "aaI0WV=W:0.00:0.00:0.00:0.00:0.00:0.00:62.00:0.00#" + CRC if requested

## Reset the max, min and wind vane array:

Request: "aaI0RESET#" ("aaI0RESET?d9#" with CRC)

Returns: "aaRESET#"

## Set the unit to broadcast:

Request: "aaI0SEND?#" where ? is an int (0)= 1s data, (1)= 10s data, (2)= 60s/1 min data, (3)= 600s/10 min data, (4)= 3600s/1hr data, (5)= NO data

Returns: "aaI0SENDOK#" + CRC if requested

You can also set the unit to broadcast using the user switch. Press the button for around 0.5s or more then release. This will go through the boradcast modes from 0-1-2-3-4-5 then back round to 0. The LED will flash the number of times for the setting (so send = 0 the unit will not flash, but data will appear within 1 second!).

If the unit is in broadcast mode then the minimum and maximum wind speeds and the wind vane data are all reset each time period.

## What is baud rate?:

Request: "aaI0BD#" ("aaI0BD?dc#" with CRC)

Returns: "aaBD9600#"  // Where 9600 is the baud rate + CRC if requested

## Set Baud Rate:

Request: "aaI0STBD\*#"  Where \* is (0)1200, (1)2400, (2)9600, (3)57600, (4)115200

Returns: "aaBD9600#"   // Where 9600 is the baud rate + CRC if requested

## What is ID?:

Mentioned at start up of unit - it is solder-programmed... cannot be changed in code.

ID selection is by using a blob of solder to connect together some pads labelled A0, A1 and A2. The default is for no pads to be soldered and the ID is 0. This means the unit will respond to "I0" as the ID.

To change the ID to another number from 0-7 then we can solder the different connection pads to create a binay number. The connections are:

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

## Enter vane training mode:

Request: "aaI0VT#" ("aaI0VT?af#" with CRC)

Returns: Enter the vane training routine - use button to go through the different directions and set the values.

The serial port will show which direction the vane should be pointing at.

Move the vane to this position and press the user switch (for around 0.5 seconds).

The serial port will show then next direction and will got N, NE, E, SE, S, SW, W, NW and then end.

The unit will also send "aaI0WVOK=NW" + CRC +"#" to report back which direction the unit is now being trained.

When it ends this data is stored within the unit and the direction 'bands' are recaluclated.

## Serial 'Button' press

The command "aaI0BUTTON" + CRC + "#" will act just like a button press. This is for control via a data logger serial port without access to the physical switch.

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

"aaFAIL1#" = String too long

"aaFAIL2#" = Unit ID not correct/not a number

"aaFAIL3#" = Channel ID is not correct/not a number

"aaFAIL4#" = Average not correct/not a number

"aaFAIL5#" = Start/End chars not correct

# Overview of Connections

