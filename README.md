# watering system

You need an Arduino, water pump, relais module and a humidity sensor. For example "WayinTop Automatische Bewässerung DIY Kit".

This software supports
* start pump when humidity is too low
* start pump after time out (e.g. after one day)
* stop pump when humidity is high enough or after a timeout (e.g. after 20 seconds)
* interact via terminal (seriell monitor of arduino IDE)
  - start and stop pump manualy
  - change values of variables (thresholds, times)
  
# requirements
* R1: pump is on. after time T1 humidity has to change (threshold S3), otherwise go to error mode
* R2: if humidity lower than S1 then activate pump
* R3: if humidity is higher than S2 then stop pump
* R4: maximum time for pump is on is time T2
* R5: wait time T3 after last time pump was on
* R6: If after time T4 earth is to dry then start pump (maybe sensor is defect)
* R7: After time T5 new trial if in error state


