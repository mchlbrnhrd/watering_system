# watering system
Automatic irrigation of plants on vacation.

For this self watering system you need an Arduino microcontroller, water pump, relais module and a capacitive soil moisture sensor. For example "WayinTop Automatische Bewässerung DIY Kit".

This software supports
* start pump when moisture of soil is too low
* start pump after time out (e.g. after one day)
* stop pump when moisture is high enough or after a timeout (e.g. at least after 20 seconds)
* interact via terminal (seriell monitor of arduino IDE)
  - check sensor values
  - start and stop pump manualy
  - change values of variables during runtime (thresholds, time durations)
  - activate debug mode: print sensor values continously and internal states of software.
  - get log data (sensor data will be recorded every 12 hours)
  - start auto calibraton

# hardware setup
Connect digital outputs of arduino (microcontroller) with relais module inputs. Connect analog inputs of arduino with capacitive soil moisture sensors. Connect pumps with relais module switches and power supply.

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
* __c__ _cancel_: go back to main software functionality from each menu point

_Hint_: Connect reset pin with 10 k Ohm pull up resistor and 10 nF capacitor against ground to avoid reset when connecting computer via USB cable to running system. Otherwise log file will be cleared.

## change values
Type 't and you get the information of all channels. Type channel number for which values should be changed. To change threshold low for example type 'S1' and afterwards type new value. With 'c' the main program is continued. Typing again 't' to check the values and 'c' to continue again.

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
