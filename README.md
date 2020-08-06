# watering system
Automatic irrigation of plants on vacation.

For this self watering system you need an Arduino microcontroller, water pump, relais module and a capacitive soil moisture sensor. For example "WayinTop Automatische Bewässerung DIY Kit". It works with Arduino Nano. With Arduino Yún log files are stored on a SD card. Also settings like threshold values and log files will be pushed
to a user defined server. An example HTML file evaluates the log file.

![watering system](doc/watering_system_small.jpg)

This software supports:
* start pump when moisture of soil is too low
* start pump after time out (e.g. after one day)
* stop pump when moisture is high enough or after a timeout (e.g. at least after 20 seconds)
* interact via terminal (seriell monitor of arduino IDE):
  - check sensor values
  - start and stop pump manualy
  - change values of variables during runtime (thresholds, time durations)
  - activate debug mode: print sensor values continously and internal states of software.
  - get log data (sensor data will be recorded every 12 hours)
  - start auto calibraton
* Internet of Things (IoT) with Arduino Yún:
  - save log files and settings on SD card
  - push files to a server
  - evaluate log files and show graphically from remote

# hardware setup
Connect digital outputs of arduino (microcontroller) with relais module inputs. Connect analog inputs of arduino with capacitive soil moisture sensors. Connect pumps with relais module switches and power supply. Make sure to use correct voltages for arduino board and pump.

![hardware setup](doc/scetch_small.jpg)

# Terminal
Type 'h' to show help.
* __d__ _debug_: Print continously debug informations
* __s__ _soft reset_ : reset timer and switch off pump
* __r__ _reset_: hard reset all values, thresholds, timer values and switch off pump
* __i__ _short info_: print value of sensors
* __t__ _terminal_: interact with software and change threshold and timer values
* __m__ _manual mode_: switch on and off each pump manually
* __l__ _read log_: read log values of each channel
* __a__ _auto calibration_: wizard to set threshold values
* __w__ _write settings_: write settings to SD card and push them to server
* __c__ _cancel_: go back to main software functionality from each menu point

_Hint_: Connect reset pin with 10 k Ohm pull up resistor and 10 nF capacitor against ground to avoid reset when connecting computer via USB cable to running system. Otherwise internal log data will be cleared. When using Arduino Yún this is not necessary: log files will be stored on SD card and on server.

## change values
Type 't' and information of all channels is printed. Type channel number to edit values for the specific channel. To change _threshold low_ for example type 'S1' and afterwards type new value. With 'c' the main program is continued. Type again 't' to check the values and 'c' to continue again.

## auto calibration
Type 'a'. Afterwards type channel number. Make sure sensor is dry. Then type '1'. Pump is switched on. When watering is sufficient then type '0' to stop pump. The sensor values are taken to set the new threshold.

# supported requirements
This software supports following requirements:
* R1: If pump is activated (on) then after time T1 there shall be a moisture change (below threshold S3). Otherwise go to error state.
* R2: If moisture is lower than threshold S1 then pump has to be activated.
* R3: If moisture is higher than threshold S2 then pump has to stop.
* R4: Maximum time for activated pump time T2.
* R5: Wait at least time T3 before activating pump again.
* R6: If after time T4 soil is too dry then start pump (maybe sensor is defect).
* R7: Leave error state after time T5 and go to ready state (new trial to water plants).
* R8: Requirements R1 - R7 works independent for each available pump.

States/Modes:
* M1: pump is ready
* M2: pump is on
* M3: pump is off
* M4: pump is in error state

# setup for Arduino Yún
A micro SD card is required for saving data and an FTP server access for pushing data to server and evaluation.

The location will be _/mnt/sd/watering_. Make sure this folder will exists.

Copy _pushToServer.sh_ script to Linux system of Arduino Yún and modify it. (FTP server name with root path, user name and password).
Upload _evaluation.html_ to the server for evaluating data.
