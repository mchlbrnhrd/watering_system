// Watering system by Michael Bernhard; 2020.04.23
// support for Arduino Uno and Arduino Yun (with IoT functionality)

#if defined (ARDUINO_AVR_YUN)
  // IoT: write data to SD card
  //      push data to server
  //      use wireless console instead of serial
  #include <FileIO.h>
  #define gd_WATERING_SYSTEM_IOT (1)
  #define gd_WATERING_SYSTEM_SERIAL (0)
#else
  #define gd_WATERING_SYSTEM_IOT (0)
  #define gd_WATERING_SYSTEM_SERIAL (1)
#endif

#if (0 == gd_WATERING_SYSTEM_SERIAL)
  #include <Console.h>
#endif

//******************************************************************************************
//******************************************************************************************
//  define constants
//******************************************************************************************
//******************************************************************************************

// number of plants (pumps, sensors and relais)
const int g_NumPlants_ic = 4;

// version to print on console
const char g_PgmVersion_pc[] PROGMEM = {"Version: 2021.05"};

// pin connection of sensor and pump (relais module) to arduino board
const int g_SensorPin_pic[g_NumPlants_ic] = {A0, A1, A2, A4};
const int g_PumpPin_pic[g_NumPlants_ic] = {2, 3, 4, 5};

const int g_LogSize_ic = 40; // number of log entries
const long g_LogInterval_lc = 60L*60L; // log interval 1 h

// store text in PROGMEM
const char g_PgmWatering_pc[] PROGMEM = {"Watering system by Michael Bernhard. Type 'h' for help."};
const char g_PgmHelp_pc[] PROGMEM = {"h: help; v: version; d: debug; s: soft reset; r: reset; i: short info; t: terminal; m: manual mode;\nl: read log; p: add log entry and push to server; a: auto calibration; w: write threshold values; c: cancel/continue"};
const char g_PgmInfo_pc[] PROGMEM = {"Info"};
const char g_PgmSensor_pc[] PROGMEM = {"Sensor "};
const char g_PgmTime_pc[] PROGMEM = {"Time: "};
const char g_PgmContinue_pc[] PROGMEM = {"Continue... "};
const char g_PgmChannel_pc[] PROGMEM = {"Channel: "};
const char g_PgmCommand_pc[] PROGMEM = {"Command: "};
const char g_PgmValue_pc[] PROGMEM = {"Value: "};
const char g_PgmManual_pc[] PROGMEM = {"[0] pump off; [1] pump on; [c] cancel"};
const char g_PgmAutoCalibration0_pc[] PROGMEM = {"[1] Confirm dry mode; [c] cancel"};
const char g_PgmAutoCalibration1_pc[] PROGMEM = {"Pump on. [0] Stop and take value; [c] cancel"};
const char g_PgmReset_pc[] PROGMEM = {"Reset"};
const char g_PgmSoftReset_pc[] PROGMEM = {"Soft reset"};
const char g_PgmDebugMode_pc[] PROGMEM = {"Debug mode "};
const char g_PgmLogStart_pc[] PROGMEM = {"--- log start --- <mode, sensor value, time stamp>"};
const char g_PgmLogEnd_pc[] PROGMEM = {"--- log end ---"};

const char g_PgmC1_pc[] PROGMEM = {"current time                   : "};
const char g_PgmT1_pc[] PROGMEM = {"[T1] timeout pump on           : "};
const char g_PgmT2_pc[] PROGMEM = {"[T2] time pump on max          : "};
const char g_PgmT3_pc[] PROGMEM = {"[T3] time wait                 : "};
const char g_PgmT4_pc[] PROGMEM = {"[T4] time out pump off         : "};
const char g_PgmT5_pc[] PROGMEM = {"[T5] time out error state      : "};
const char g_PgmS1_pc[] PROGMEM = {"[S1] threshold low             : "};
const char g_PgmS2_pc[] PROGMEM = {"[S2] threshold high            : "};
const char g_PgmS3_pc[] PROGMEM = {"[S3] threshold expected change : "};
const char g_PgmM1_pc[] PROGMEM = {"mode pump ready"};
const char g_PgmM2_pc[] PROGMEM = {"mode pump on   "};
const char g_PgmM3_pc[] PROGMEM = {"mode pump off  "};
const char g_PgmM4_pc[] PROGMEM = {"mode pump error"};

const char g_PgmErrR1_pc[] PROGMEM = {": Err R1: Switch off pump due of timeout. Check sensor. "};
const char g_PgmErrR6_pc[] PROGMEM = {": Err R6: Force pump due of timeout T4. Check sensor. "};
const char g_PgmInfoR7_pc[] PROGMEM = {": Info: Leave Error state. R7"};
const char g_PgmDebugR2_pc[] PROGMEM = {": pump on (threshold) R2"};
const char g_PgmDebugR3_pc[] PROGMEM = {": pump off (threshold) R3"};
const char g_PgmDebugR4_pc[] PROGMEM = {": pump off (time max on) R4"};
const char g_PgmDebugR5_pc[] PROGMEM = {": ready R5"};

const char g_PgmBasePath_pc[] PROGMEM = "/mnt/sd/watering/";
const char g_PgmWateringLog_pc[] PROGMEM  = "watering_log.txt";
const char g_PgmWateringLogHuman_pc[] PROGMEM  = "watering_log_human.txt";
const char g_PgmWateringThreshold_pc[] PROGMEM  = "watering_threshold.txt";
const char g_PgmWateringThresholdHuman_pc[] PROGMEM  = "watering_threshold_human.txt";
const char g_PgmDates_pc[] PROGMEM = "dates.txt";
const char g_PgmPushToServerScript_pc[] PROGMEM = "pushToServer.sh";



//******************************************************************************************
//******************************************************************************************
//  define types, enum and structs
//******************************************************************************************
//******************************************************************************************

enum ModeType {
  modePumpReady = 1,
  modePumpOn = 2,
  modePumpOff = 3,
  modePumpError = 4
};

struct typedefPlant {
  int Sensor_i;                  // Sensor value
  ModeType Mode_enm;             // current mode
  
  int ThresholdLow_i;            // if sensor value is below this threshold, pump stopps
  int ThresholdHigh_i;           // if sensor value is higher than this threshold, pump starts
  int ThresholdExpectedChange_i; // sensor value has to be lower than this value, otherwise error state
  
  long CurrTime_l;               // current time in seconds
  
  long TimeOutPumpOn_l;          // T1: timout when pump is on: after this time, a value change is expected
  long TimePumpOnMax_l;          // T2: maximum time for pump is on. After this time, pump is switched off
  long TimeWait_l;               // T3: wait this time after last pump on before start new pump on: stay at least TimeWait in pump off mode
  long TimeOutPumpOff_l;         // T4: when pump is off: start pump independent of sensor value after this timout
  long TimeOutErrorState_l;      // T5: release error state after this timeout
};

struct typedefLogEntry {
  long TotalTime_l;
  uint16_t Data_pui16[g_NumPlants_ic];
};

struct typedefLog {
  typedefLogEntry Entry_pst[g_LogSize_ic];
  int Index_i;
};

//******************************************************************************************
//******************************************************************************************
//  global variable
//******************************************************************************************
//******************************************************************************************
typedefPlant g_Plants_pst[g_NumPlants_ic];
typedefLog g_Log_st;

int g_Timer_i = 0;
long g_TotalTime_l = 0;
bool g_HeartBeat_bl = false;
bool g_PrintInfo_bl = false;
bool g_DebugMode_bl = false;
long g_LogTimer_l = 0;
bool g_LogDone_bl = false;


//******************************************************************************************
//******************************************************************************************
// functions
//******************************************************************************************
//******************************************************************************************

//******************************************************************************************
//  setup
//******************************************************************************************
void setup() 
{
  pinMode(LED_BUILTIN, OUTPUT);
  for(int i=0; i < g_NumPlants_ic; ++i) {
    pinMode(g_SensorPin_pic[i], INPUT);
    pinMode(g_PumpPin_pic[i], OUTPUT);
    pumpOff(i);
  }
  
  for (int k = 0; k < 5; ++k)
  {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(50);
    digitalWrite(LED_BUILTIN, LOW);
    delay(30);
  }
#if (0 != gd_WATERING_SYSTEM_IOT)
  // wait 3 minutes to make sure linux system is ready
  for (int k=0; k < 60*3; ++k) {
    delay(1000);
  }
  Bridge.begin();
#endif

#if (0 != gd_WATERING_SYSTEM_SERIAL)
  Serial.begin(9600);
#else
  Console.begin();
#endif

#if (0 != gd_WATERING_SYSTEM_IOT)
  FileSystem.begin();
#endif

  terminalPrintlnPgm(g_PgmWatering_pc);
  delay(1000);
  reset();

  // timer interrupt settings (1 Hz)
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;
  OCR1A = 15624; // (16*10^6) / (1*1024) - 1
  TCCR1B |=(1<<WGM12);
  TCCR1B |= (1 << CS12) | (1 << CS10);
  TIMSK1 |=(1 << OCIE1A);
  sei();
}

//******************************************************************************************
//  loop 
//******************************************************************************************
void loop() 
{
  // show heart beat
  digitalWrite(LED_BUILTIN, g_HeartBeat_bl);

  // LED always on when an error occured
  for (int i=0; i < g_NumPlants_ic; ++i) {
    if (g_Plants_pst[i].Mode_enm == modePumpError) {
      digitalWrite(LED_BUILTIN, HIGH);
    }
  }
  // read sensor values
  readSensor();

  // pump control
  pumpControl();

  // create log entry
  if ((0 == g_LogTimer_l) && !g_LogDone_bl) {
    addLogEntryAndPushToServer(true);
    g_LogDone_bl = true;
  } else if (0 != g_LogTimer_l) {
    g_LogDone_bl = false;
  }

  // check if key is pressed
  while (terminalAvailable() > 0) {
    TIMSK1 &=~(1 << OCIE1A);
    String Key_s = terminalReadString();
    Key_s.trim();
    if (Key_s.equals("h") || Key_s.equals("H")) {
      terminalPrintlnPgm(g_PgmHelp_pc);
    } else if (Key_s.equals("v") || Key_s.equals("V")) {
      terminalPrintlnPgm(g_PgmVersion_pc);
    } else if (Key_s.equals("t") || Key_s.equals("T")) {
      terminal();
    } else if (Key_s.equals("i") || Key_s.equals("I")) {
      printShortInfo();
    } else if (Key_s.equals("r") || Key_s.equals("R")) {
      terminalPrintlnPgm(g_PgmReset_pc);
      reset();
    } else if (Key_s.equals("s") || Key_s.equals("S")) {
      terminalPrintlnPgm(g_PgmSoftReset_pc);
      softReset(true);
    } else if (Key_s.equals("d") || Key_s.equals("D")) {
      g_DebugMode_bl = !g_DebugMode_bl;
      terminalPrintPgm(g_PgmDebugMode_pc);
      terminalPrintln(g_DebugMode_bl ? "on" : "off");
    } else if (Key_s.equals("m") || Key_s.equals("M")) {
      manualMode();
    } else if (Key_s.equals("l") || Key_s.equals("L")) {
      printLog();
    } else if (Key_s.equals("p") || Key_s.equals("P")) {
      addLogEntryAndPushToServer(false);
    } else if (Key_s.equals("a") || Key_s.equals("A")) {
      autoCalibration();
    } else if (Key_s.equals("w") || Key_s.equals("W")) {
#if (0 != gd_WATERING_SYSTEM_IOT)
      writeThresholdSD();
#endif
    }
    TIMSK1 |=(1 << OCIE1A);
  }
  
  // print sensor data in debug mode
  if (g_DebugMode_bl) {
    if (g_PrintInfo_bl) {
      g_PrintInfo_bl = false;
      printShortInfo();
    }
  } 
}


//******************************************************************************************
//  soft reset
//******************************************************************************************
void softReset(bool f_CurrTime_bl) 
{
  for (int i = 0; i < g_NumPlants_ic; ++i) {
    g_Plants_pst[i].Sensor_i = 0;
    g_Plants_pst[i].Mode_enm = modePumpReady;
    if (f_CurrTime_bl) {
      g_Plants_pst[i].CurrTime_l = 0;
    }
    pumpOff(i); // pump off
  }
  g_Timer_i = 0;
}


//******************************************************************************************
//  reset
//******************************************************************************************
void reset() 
{
#if (0 != gd_WATERING_SYSTEM_IOT)
  // create backup files and push to server
  Process proc;
  String date="";
  getTimeStamp(date);
  String cmd="mv ";
  stringFromPgm(g_PgmBasePath_pc, cmd);
  stringFromPgm(g_PgmWateringLog_pc, cmd);
  cmd += " ";
  stringFromPgm(g_PgmBasePath_pc, cmd);
  cmd += date;
  stringFromPgm(g_PgmWateringLog_pc, cmd);
  proc.runShellCommand(cmd);
  while(proc.running());
  delay(300);
  terminalPrintln(cmd);
  delay(300);
  cmd = date;
  stringFromPgm(g_PgmWateringLog_pc, cmd);
  pushToServer(cmd);

  cmd="mv ";
  stringFromPgm(g_PgmBasePath_pc, cmd);
  stringFromPgm(g_PgmWateringLogHuman_pc, cmd);
  cmd += " ";
  stringFromPgm(g_PgmBasePath_pc, cmd);
  cmd += date;
  stringFromPgm(g_PgmWateringLogHuman_pc, cmd);
  proc.runShellCommand(cmd);
  while(proc.running());
  delay(300);
  terminalPrintln(cmd);
  delay(300);
  cmd = date;
  stringFromPgm(g_PgmWateringLogHuman_pc, cmd);
  pushToServer(cmd);

  cmd="";
  stringFromPgm(g_PgmBasePath_pc, cmd);
  stringFromPgm(g_PgmDates_pc, cmd);
  File dateLogFile = FileSystem.open(cmd.c_str(), FILE_APPEND);
  if (dateLogFile) {
    dateLogFile.println(date);
    dateLogFile.close();
  }
  delay(300);
  pushToServer(g_PgmDates_pc);
  delay(300);
  
  // read threshold values from SD card
  readThresholdSD();  
#else
  defaultThreshold();  
#endif

  // set variables
  g_DebugMode_bl = false;
  g_PrintInfo_bl = false;
  g_LogTimer_l = 0;
  g_TotalTime_l = 0;
  g_LogDone_bl = false;

  g_Log_st.Index_i = 0;
  for (int k = 0; k < g_LogSize_ic; ++k) {
    g_Log_st.Entry_pst[k].TotalTime_l = 0;
    for (int i = 0; i < g_NumPlants_ic; ++i) {
      g_Log_st.Entry_pst[k].Data_pui16[i] = 0;
    }
  }
  softReset(true);
}

//******************************************************************************************
//  defaultThreshold
//******************************************************************************************
void defaultThreshold() 
{
//  example values:
//  S1 threshold low             : 350
//  S2 threshold high            : 450
//  S3 threshold expected change : 445
//  T1 timeout pump on           : 5
//  T2 time pump on max          : 20
//  T3 time wait                 : 100
//  T4 time out pump off         : 86400 = 24 h
//  T5 time out error state      : 86400 = 24 h

  for (int i = 0; i < g_NumPlants_ic; ++i) {  
    g_Plants_pst[i].ThresholdLow_i = 2000; // large number as default to deactive: force user defined setting
    g_Plants_pst[i].ThresholdHigh_i = 2100; //large number as default to deactivate: force user defined setting
    g_Plants_pst[i].ThresholdExpectedChange_i = 2200; // use large value to deactivate this function
    g_Plants_pst[i].TimeOutPumpOn_l = 5L; 
    g_Plants_pst[i].TimePumpOnMax_l = 20L;
    g_Plants_pst[i].TimeWait_l = 1L*60L*60L*6L; // 6 hours
    g_Plants_pst[i].TimeOutPumpOff_l = 2L*60L*60L*12L; // 2 days + TimeWait
    g_Plants_pst[i].TimeOutErrorState_l = 60L*60L*1L; // 1 hour
  } 
}


//******************************************************************************************
//  read sensor values
//******************************************************************************************
void readSensor()
{
  for (int i=0; i < g_NumPlants_ic; ++i) {
    g_Plants_pst[i].Sensor_i = analogReadMean(g_SensorPin_pic[i], 3, 50);
  }
}


//******************************************************************************************
//  pump control
//******************************************************************************************
void pumpControl()
{
  for (int i=0; i < g_NumPlants_ic; ++i) {
    bool TimerReset_bl = false;
    if (modePumpReady == g_Plants_pst[i].Mode_enm) {
      // ************* mode pump ready **************
      if (g_Plants_pst[i].Sensor_i > g_Plants_pst[i].ThresholdHigh_i) {
        g_Plants_pst[i].Mode_enm = modePumpOn;
        if (g_DebugMode_bl) {
          terminalPrint(i);
          terminalPrintlnPgm(g_PgmDebugR2_pc);
        }
      } else if (g_Plants_pst[i].CurrTime_l > g_Plants_pst[i].TimeOutPumpOff_l) {
         g_Plants_pst[i].Mode_enm = modePumpOn;
         pumpOn(i);
         terminalPrint(i);
         terminalPrintPgm(g_PgmErrR6_pc);
         terminalPrint(g_Plants_pst[i].CurrTime_l);
         terminalPrint(", ");
         terminalPrintln(g_Plants_pst[i].TimeOutPumpOff_l);
      }
      if (modePumpOn == g_Plants_pst[i].Mode_enm) {
        pumpOn(i);
        TimerReset_bl = true;
      }

      
    } else if (modePumpOn == g_Plants_pst[i].Mode_enm) {
      // ************* mode pump on **************
      if (g_Plants_pst[i].CurrTime_l > g_Plants_pst[i].TimeOutPumpOn_l) {
        if (g_Plants_pst[i].Sensor_i > g_Plants_pst[i].ThresholdExpectedChange_i) {
          g_Plants_pst[i].Mode_enm = modePumpError;
          pumpOff(i);
          terminalPrint(i);
          terminalPrintPgm(g_PgmErrR1_pc);
          terminalPrint(g_Plants_pst[i].Sensor_i);
          terminalPrint(", ");
          terminalPrintln(g_Plants_pst[i].ThresholdExpectedChange_i);
          TimerReset_bl = true;
        }
        if (g_Plants_pst[i].Sensor_i < g_Plants_pst[i].ThresholdLow_i) {
          g_Plants_pst[i].Mode_enm = modePumpOff;
          pumpOff(i);
          TimerReset_bl = true;
          if (g_DebugMode_bl) {
            terminalPrint(i);
            terminalPrintlnPgm(g_PgmDebugR3_pc);
          }
        } else if (g_Plants_pst[i].CurrTime_l  > g_Plants_pst[i].TimePumpOnMax_l) {
          g_Plants_pst[i].Mode_enm = modePumpOff;
          pumpOff(i);
          TimerReset_bl = true;
          if (g_DebugMode_bl) {
            terminalPrint(i);
            terminalPrintlnPgm(g_PgmDebugR4_pc);
          }
        }
      }

      
    } else if (modePumpOff == g_Plants_pst[i].Mode_enm) {
      // ************* mode pump off **************
      if (g_Plants_pst[i].CurrTime_l  > g_Plants_pst[i].TimeWait_l) {
        g_Plants_pst[i].Mode_enm = modePumpReady;
        TimerReset_bl = true;
        if (g_DebugMode_bl) {
          terminalPrint(i);
          terminalPrintlnPgm(g_PgmDebugR5_pc);
        }
      }

      
    } else { // modePumpError
      // ************* mode pump error **************
      if (g_Plants_pst[i].CurrTime_l > g_Plants_pst[i].TimeOutErrorState_l) {
        g_Plants_pst[i].Mode_enm = modePumpReady;
        TimerReset_bl = true;
        terminalPrint(i);
        terminalPrintlnPgm(g_PgmInfoR7_pc);
      }
    }
    
    if (TimerReset_bl) {
      g_Plants_pst[i].CurrTime_l = 0;
    }
  }
}


//******************************************************************************************
//  terminal
//******************************************************************************************
void terminal()
{
  printInfo();
  bool Finished_bl=false;
  while(!Finished_bl) {
    terminalPrintlnPgm(g_PgmChannel_pc);
    while (terminalAvailable() == 0);
    String Key_s = terminalReadString();
    Key_s.trim();
    terminalPrintln(Key_s);
    String Command_s = "";
    int Channel_i = 0;
    if (Key_s.equals("c") || Key_s.equals("C")) {
      Finished_bl = true;
    } else {
      Channel_i = Key_s.toInt();
      terminalPrintlnPgm(g_PgmCommand_pc);
      while (terminalAvailable() == 0);
      Command_s = terminalReadString();
      Command_s.trim();
      terminalPrintln(Command_s);
     if (Command_s.equals("c") || Command_s.equals("C")) {
        Finished_bl = true;
      }
    }
    if (!Finished_bl) {
      terminalPrintlnPgm(g_PgmValue_pc);
      while (terminalAvailable() == 0);
      Key_s = terminalReadString();
      Key_s.trim();
      terminalPrintln(Key_s);
      long Value_l = Key_s.toInt();
      if (Command_s.equals("S1")) {
        g_Plants_pst[Channel_i].ThresholdLow_i = (int)Value_l;
      } else if (Command_s.equals("S2")) {
        g_Plants_pst[Channel_i].ThresholdHigh_i = (int)Value_l;
      } else if (Command_s.equals("S3")) {
        g_Plants_pst[Channel_i].ThresholdExpectedChange_i = (int)Value_l;
      } else if (Command_s.equals("T1")) {
        g_Plants_pst[Channel_i].TimeOutPumpOn_l = Value_l;
      } else if (Command_s.equals("T2")) {
        g_Plants_pst[Channel_i].TimePumpOnMax_l = Value_l;
      } else if (Command_s.equals("T3")) {
        g_Plants_pst[Channel_i].TimeWait_l = Value_l;
      } else if (Command_s.equals("T4")) {
        g_Plants_pst[Channel_i].TimeOutPumpOff_l = Value_l;
      } else if (Command_s.equals("T5")) {
        g_Plants_pst[Channel_i].TimeOutErrorState_l = Value_l;
      } else {
        Finished_bl = true;
      }
      printChannel(Channel_i);
    }
  }
  terminalPrintlnPgm(g_PgmContinue_pc);
}


//******************************************************************************************
//  manual mode
//******************************************************************************************
void manualMode()
{
  bool Finished_bl=false;
  readSensor();
 
  terminalPrintlnPgm(g_PgmChannel_pc);
  while (terminalAvailable() == 0);
  String Key_s = terminalReadString();
  Key_s.trim();
  terminalPrintln(Key_s);
  int Channel_i = 0;
  if (Key_s.equals("c") || Key_s.equals("C")) {
    Finished_bl = true;
  } else {
    Channel_i = Key_s.toInt();
    terminalPrintlnPgm(g_PgmManual_pc);
  }
  while(!Finished_bl && (Channel_i < g_NumPlants_ic)) {
    if (terminalAvailable() > 0) {
      Key_s = terminalReadString();
      Key_s.trim();
      terminalPrint("Pump ");
      if (Key_s.equals("c") || Key_s.equals("C")) {
        Finished_bl = true;
      } else if (Key_s.equals("1") || Key_s.equals("1")) {
        terminalPrintln("on");
        // save current mode, change to pumpOn for log and recover mode
        ModeType ModeSave_enm = g_Plants_pst[Channel_i].Mode_enm;
        g_Plants_pst[Channel_i].Mode_enm = modePumpOn;
        pumpOn(Channel_i); // pump on
        g_Plants_pst[Channel_i].Mode_enm = ModeSave_enm;
      } else {
        terminalPrintln("off");
        // save current mode, change to pumpOff for log and recover mode
        ModeType ModeSave_enm = g_Plants_pst[Channel_i].Mode_enm;
        g_Plants_pst[Channel_i].Mode_enm = modePumpOff;
        pumpOff(Channel_i); // pump off
        g_Plants_pst[Channel_i].Mode_enm = ModeSave_enm;
      }
    }
    if (g_DebugMode_bl) {
      if (g_PrintInfo_bl) {
        g_PrintInfo_bl = false;
        printShortInfo();
      }
    }
  }
  terminalPrintlnPgm(g_PgmContinue_pc);
  softReset(false);
}


//******************************************************************************************
//  autoCalibration
//******************************************************************************************
void autoCalibration()
{
  bool Cancel_bl = false;
  terminalPrintlnPgm(g_PgmChannel_pc);
  while (terminalAvailable() == 0);
  String Key_s = terminalReadString();
  Key_s.trim();
  terminalPrintln(Key_s);
  int Channel_i = 0;
  long ThresholdLow_l = 0;
  long ThresholdHigh_l = 0;
  if (Key_s.equals("c") || Key_s.equals("C")) {
    Cancel_bl = true;
  } else {
    Channel_i = Key_s.toInt();
    if (Channel_i < g_NumPlants_ic) {
      g_Plants_pst[Channel_i].Mode_enm = modePumpOff;
      pumpOff(Channel_i);
      terminalPrintlnPgm(g_PgmAutoCalibration0_pc);
    } else {
      Cancel_bl = true;
    }
  }
  if (!Cancel_bl) {
    while (terminalAvailable() == 0);
    Key_s = terminalReadString();
    Key_s.trim();
    if (Key_s.equals("1") ) {
      delay(100);
      ThresholdHigh_l = (long)analogReadMean(g_SensorPin_pic[Channel_i], 5, 100);
    } else {
      Cancel_bl = true;
    } 
  }
  if (!Cancel_bl) {
    terminalPrintlnPgm(g_PgmAutoCalibration1_pc);
    g_Plants_pst[Channel_i].Mode_enm = modePumpReady;
    pumpOn(Channel_i);
    while (terminalAvailable() == 0);
    Key_s = terminalReadString();
    Key_s.trim();
    if (Key_s.equals("0") ) {
      g_Plants_pst[Channel_i].Mode_enm = modePumpReady;
      pumpOff(Channel_i);
      // wait short time
      delay(300);
      ThresholdLow_l = (long)analogReadMean(g_SensorPin_pic[Channel_i], 5, 100);
    } else {
      Cancel_bl = true;
    }
  }
  if (!Cancel_bl) {
    terminalPrintln(ThresholdLow_l);
    terminalPrintln(ThresholdHigh_l);
    ThresholdLow_l = (ThresholdLow_l * 105L) / 100L; // add 5 percent
    ThresholdHigh_l = (ThresholdHigh_l * 85L) / 100L; // subtract 15 percent
    g_Plants_pst[Channel_i].ThresholdLow_i = (int)ThresholdLow_l;
    g_Plants_pst[Channel_i].ThresholdHigh_i = (int)ThresholdHigh_l;
    // expected change close to threshold high
    g_Plants_pst[Channel_i].ThresholdExpectedChange_i = 2250; //(ThresholdHigh_l*98L + ThresholdLow_l*2L)/100L;
    printChannel(Channel_i);
  }
#if (0 != gd_WATERING_SYSTEM_IOT)
  writeThresholdSD();
#endif
  terminalPrintlnPgm(g_PgmContinue_pc);
  softReset(false);
}


//******************************************************************************************
//  print short info
//******************************************************************************************
void printShortInfo()
{
  terminalPrintPgm(g_PgmTime_pc);
  terminalPrintln(g_TotalTime_l);
  for (int i=0; i < g_NumPlants_ic; ++i) {
    terminalPrintPgm(g_PgmSensor_pc);
    terminalPrint(i);
    terminalPrint(": ");
    terminalPrintln(g_Plants_pst[i].Sensor_i);
  }
}


//******************************************************************************************
//  print channel info
//******************************************************************************************
void printChannel(int Channel_i)
{
  if ((0 <= Channel_i) && (g_NumPlants_ic > Channel_i)) {
    terminalPrintPgm(g_PgmSensor_pc);
    terminalPrint(Channel_i);
    terminalPrint(": ");
    terminalPrintln(g_Plants_pst[Channel_i].Sensor_i);
    switch(g_Plants_pst[Channel_i].Mode_enm) {
      case modePumpReady:
        terminalPrintlnPgm(g_PgmM1_pc);
        break;
      case modePumpOn:
        terminalPrintlnPgm(g_PgmM2_pc);
        break;
      case modePumpOff:
        terminalPrintlnPgm(g_PgmM3_pc);
        break;
      default: // modePumpError
        terminalPrintlnPgm(g_PgmM4_pc);
        break;
    }
    terminalPrintPgm(g_PgmC1_pc);
    terminalPrintln(g_Plants_pst[Channel_i].CurrTime_l);
    
    terminalPrintPgm(g_PgmS1_pc);
    terminalPrintln(g_Plants_pst[Channel_i].ThresholdLow_i);
    terminalPrintPgm(g_PgmS2_pc);
    terminalPrintln(g_Plants_pst[Channel_i].ThresholdHigh_i);
    terminalPrintPgm(g_PgmS3_pc);
    terminalPrintln(g_Plants_pst[Channel_i].ThresholdExpectedChange_i);

    terminalPrintPgm(g_PgmT1_pc);
    terminalPrintln(g_Plants_pst[Channel_i].TimeOutPumpOn_l); 
    terminalPrintPgm(g_PgmT2_pc);
    terminalPrintln(g_Plants_pst[Channel_i].TimePumpOnMax_l);
    terminalPrintPgm(g_PgmT3_pc);
    terminalPrintln(g_Plants_pst[Channel_i].TimeWait_l);
    terminalPrintPgm(g_PgmT4_pc);
    terminalPrintln(g_Plants_pst[Channel_i].TimeOutPumpOff_l);
    terminalPrintPgm(g_PgmT5_pc);
    terminalPrintln(g_Plants_pst[Channel_i].TimeOutErrorState_l);
    terminalPrintln("");
  }
}


//******************************************************************************************
//  print Info
//******************************************************************************************
void printInfo()
{
  terminalPrintlnPgm(g_PgmInfo_pc);
  printShortInfo();
  for (int i=0; i < g_NumPlants_ic; ++i) {
   printChannel(i);
  }
}


//******************************************************************************************
//  print log
//******************************************************************************************
void printLog()
{
  terminalPrintlnPgm(g_PgmLogStart_pc);
  // print time and threshold values
  terminalPrintPgm(g_PgmTime_pc);
  terminalPrintln(g_TotalTime_l);
  for (int Channel_i = 0; Channel_i < g_NumPlants_ic; ++Channel_i) {
    terminalPrintPgm(g_PgmS1_pc);
    terminalPrintln(g_Plants_pst[Channel_i].ThresholdLow_i);
    terminalPrintPgm(g_PgmS2_pc);
    terminalPrintln(g_Plants_pst[Channel_i].ThresholdHigh_i);
    terminalPrintPgm(g_PgmS3_pc);
    terminalPrintln(g_Plants_pst[Channel_i].ThresholdExpectedChange_i);
  }

  // print log data
  int StartIdx0 = g_Log_st.Index_i;
  int EndIdx0 = g_LogSize_ic;
  int StartIdx1 = 0;
  int EndIdx1 = StartIdx0;
  // print old content
  for (int k = StartIdx0; k < EndIdx0; ++k) {
    printLogEntry(k);
  }
  // print new content
  for (int k = StartIdx1; k < EndIdx1; ++k) {
    printLogEntry(k);
  }
  terminalPrintlnPgm(g_PgmLogEnd_pc);
}


//******************************************************************************************
//  print log entry
//******************************************************************************************
void printLogEntry(int Index_i)
{
  if ((0 <= Index_i) && (g_LogSize_ic > Index_i)) {
    terminalPrint(g_Log_st.Entry_pst[Index_i].TotalTime_l);
    terminalPrint(", ");
    for (int Channel_i=0; Channel_i < g_NumPlants_ic; ++Channel_i) {
      terminalPrintPgm(g_PgmChannel_pc);
      terminalPrint(Channel_i);
      terminalPrint(": ");
      uint16_t data_ui16 = g_Log_st.Entry_pst[Index_i].Data_pui16[Channel_i];
      int sensor_i = (int)(data_ui16 & 0xFFFU);
      ModeType mode_t = (ModeType)(data_ui16 >> 12);
      switch(mode_t) {
        case modePumpReady:
          terminalPrintPgm(g_PgmM1_pc);
          break;
        case modePumpOn:
          terminalPrintPgm(g_PgmM2_pc);
          break;
       case modePumpOff:
          terminalPrintPgm(g_PgmM3_pc);
          break;
       default: // modePumpError
          terminalPrintPgm(g_PgmM4_pc);
          break;
      }
      terminalPrint(", ");
      terminalPrint(sensor_i);
      terminalPrint(", ");
    }
    terminalPrintln("");
  }
}


//******************************************************************************************
//  add log entry
//******************************************************************************************
void addLogEntry()
{
  if (g_Log_st.Index_i >= g_LogSize_ic) {
    g_Log_st.Index_i = 0;
  }
  g_Log_st.Entry_pst[g_Log_st.Index_i].TotalTime_l = g_TotalTime_l;
  
  for (int Channel_i = 0; Channel_i < g_NumPlants_ic; ++Channel_i) {
    uint16_t Data_ui16 = g_Plants_pst[Channel_i].Sensor_i;
    Data_ui16 |= ((uint16_t)g_Plants_pst[Channel_i].Mode_enm << 12);
    g_Log_st.Entry_pst[g_Log_st.Index_i].Data_pui16[Channel_i] = Data_ui16;
  }
  ++g_Log_st.Index_i;
}


//******************************************************************************************
//  pump on
//******************************************************************************************
void pumpOn(int Channel_i)
{
  digitalWrite(g_PumpPin_pic[Channel_i], LOW);
  addLogEntry();
}


//******************************************************************************************
//  pump off
//******************************************************************************************
void pumpOff(int Channel_i)
{
  digitalWrite(g_PumpPin_pic[Channel_i], HIGH);
  addLogEntry();
}


//******************************************************************************************
//  analogReadMean
//******************************************************************************************
int analogReadMean(const int Pin_i, const int SampleNr_ci, const int Delay_i)
{
  int Ret_i = 0; // return 0 when parameter are invalid
  if (SampleNr_ci > 0) {
    long Value_l = 0;
    for (int i=0; i < SampleNr_ci; ++i) {
      Value_l += (long)analogRead(Pin_i);
      if (i+1  < SampleNr_ci) {
        delay(Delay_i);
      }
    }
    Value_l = Value_l / SampleNr_ci;
    Ret_i = (int) Value_l;
  }
  return Ret_i;
}


//******************************************************************************************
//  addLogEntryAndPushToServer
//******************************************************************************************
void addLogEntryAndPushToServer(bool stopIrq_bl)
{
  addLogEntry();
#if (0 != gd_WATERING_SYSTEM_IOT)
    if (stopIrq_bl) {
      TIMSK1 &=~(1 << OCIE1A);
    }
    writeLogEntrySD();
    pushToServer(g_PgmWateringLog_pc);
    pushToServer(g_PgmWateringLogHuman_pc);
    if (stopIrq_bl) {
      TIMSK1 |=(1 << OCIE1A);
    }
#endif
}

//******************************************************************************************
//  println from PROGMEM
//******************************************************************************************
void terminalPrintlnPgm(const char* string_pc) {
  String msg="";
  stringFromPgm(string_pc, msg);
  terminalPrintln(msg);
}


//******************************************************************************************
//  print from PROGMEM
//******************************************************************************************
void terminalPrintPgm(const char* string_pc) {
  String msg="";
  stringFromPgm(string_pc, msg);
  terminalPrint(msg);
}


//******************************************************************************************
//  string from PROGMEM
//******************************************************************************************
void stringFromPgm(const char* string_pc, String& value) {
  const int len = strlen_P(string_pc);
  for (int k = 0; k < len; ++k) {
    value += (char)pgm_read_byte_near(string_pc + k);
  }
}


//******************************************************************************************
//  Timer interrupt
//******************************************************************************************
ISR(TIMER1_COMPA_vect)
{
  ++g_Timer_i;
  if (g_Timer_i >= 1) {
    ++g_TotalTime_l;
    for(int i=0; i < g_NumPlants_ic; ++i) {
      ++g_Plants_pst[i].CurrTime_l;
    }
    g_HeartBeat_bl = !g_HeartBeat_bl;
    g_PrintInfo_bl = true;
    g_Timer_i=0;
  }
  if (++g_LogTimer_l >= g_LogInterval_lc) {
    g_LogTimer_l = 0;
  }
}


//******************************************************************************************
//  write log entry to SD card
//******************************************************************************************
#if (0 != gd_WATERING_SYSTEM_IOT)
void writeLogEntrySD()
{
  { // human readable version
    String file="";
    stringFromPgm(g_PgmBasePath_pc, file);
    stringFromPgm(g_PgmWateringLogHuman_pc, file);
    File logFile = FileSystem.open(file.c_str(), FILE_APPEND);
    if (logFile && g_Log_st.Index_i < g_LogSize_ic) {
      for (int k = 0; k < g_Log_st.Index_i; ++k) {
        String logEntry = "";
        logEntry += g_Log_st.Entry_pst[k].TotalTime_l;
        logEntry += ", ";
        for (int Channel_i=0; Channel_i < g_NumPlants_ic; ++Channel_i) {
          stringFromPgm(g_PgmChannel_pc, logEntry);
          logEntry += Channel_i;
          logEntry += ": ";
          uint16_t data_ui16 = g_Log_st.Entry_pst[k].Data_pui16[Channel_i];
          int sensor_i = (int)(data_ui16 & 0xFFFU);
          ModeType mode_t = (ModeType)(data_ui16 >> 12);
          switch(mode_t) {
            case modePumpReady:
              stringFromPgm(g_PgmM1_pc, logEntry);
              break;
            case modePumpOn:
              stringFromPgm(g_PgmM2_pc, logEntry);
              break;
            case modePumpOff:
              stringFromPgm(g_PgmM3_pc, logEntry);
              break;
            default: // modePumpError
              stringFromPgm(g_PgmM4_pc, logEntry);
              break;
          }
          logEntry += ", ";
          logEntry += sensor_i;
          logEntry += ", ";
        }
        logFile.println(logEntry);
      }
      logFile.close();
    }
  }
  { // mashine readable version
    String file="";
    stringFromPgm(g_PgmBasePath_pc, file);
    stringFromPgm(g_PgmWateringLog_pc, file);
    File logFile = FileSystem.open(file.c_str(), FILE_APPEND);
    if (logFile && g_Log_st.Index_i < g_LogSize_ic) {
      for (int k = 0; k < g_Log_st.Index_i; ++k) {
        String logEntry = "";
        logEntry += g_Log_st.Entry_pst[k].TotalTime_l;
        logEntry += ",";
        for (int Channel_i=0; Channel_i < g_NumPlants_ic; ++Channel_i) {
          uint16_t data_ui16 = g_Log_st.Entry_pst[k].Data_pui16[Channel_i];
          int sensor_i = (int)(data_ui16 & 0xFFFU);
          logEntry += sensor_i;
          if (Channel_i + 1 < g_NumPlants_ic) {
            logEntry += ",";
          }
        }
        logFile.println(logEntry);
      }
      logFile.close();
       // reset index of log
      g_Log_st.Index_i = 0;
    }
  }
}
#endif

//******************************************************************************************
//  write threshold values to SD card
//******************************************************************************************
#if (0 != gd_WATERING_SYSTEM_IOT)
void writeThresholdSD()
{
  { // human readable version
    String file="";
    stringFromPgm(g_PgmBasePath_pc, file);
    stringFromPgm(g_PgmWateringThresholdHuman_pc, file);
    FileSystem.remove(file.c_str());
    File thresholdFile = FileSystem.open(file.c_str(), FILE_APPEND);
    if (thresholdFile) {
      String entry="";
      // print time and threshold values
      getTimeStamp(entry);
      thresholdFile.println(entry);
      entry="";
      stringFromPgm(g_PgmTime_pc, entry);
      entry += g_TotalTime_l;
      thresholdFile.println(entry);
      entry="";
      for (int Channel_i = 0; Channel_i < g_NumPlants_ic; ++Channel_i) {
        entry="Channel: ";
        entry += Channel_i;
        thresholdFile.println(entry);
        entry="";
        stringFromPgm(g_PgmS1_pc, entry);
        entry += g_Plants_pst[Channel_i].ThresholdLow_i;
        thresholdFile.println(entry);
        entry="";
        stringFromPgm(g_PgmS2_pc, entry);
        entry += g_Plants_pst[Channel_i].ThresholdHigh_i;
        thresholdFile.println(entry);
        entry="";
        stringFromPgm(g_PgmS3_pc, entry);
        entry += g_Plants_pst[Channel_i].ThresholdExpectedChange_i;
        thresholdFile.println(entry);
        entry="";
        stringFromPgm(g_PgmT1_pc, entry);
        entry += g_Plants_pst[Channel_i].TimeOutPumpOn_l;
        thresholdFile.println(entry);
        entry="";
        stringFromPgm(g_PgmT2_pc, entry);
        entry +=g_Plants_pst[Channel_i].TimePumpOnMax_l;
        thresholdFile.println(entry);
        entry="";
        stringFromPgm(g_PgmT3_pc, entry);
        entry += g_Plants_pst[Channel_i].TimeWait_l;
        thresholdFile.println(entry);
        entry="";
        stringFromPgm(g_PgmT4_pc, entry);
        entry += g_Plants_pst[Channel_i].TimeOutPumpOff_l;
        thresholdFile.println(entry);
        entry="";
        stringFromPgm(g_PgmT5_pc, entry);
        entry += g_Plants_pst[Channel_i].TimeOutErrorState_l;
        thresholdFile.println(entry);
        thresholdFile.println("");
      }
      thresholdFile.close();
      pushToServer(g_PgmWateringThresholdHuman_pc);
    }
  }
  { // mashine readable version
    String file="";
    stringFromPgm(g_PgmBasePath_pc, file);
    stringFromPgm(g_PgmWateringThreshold_pc, file);
    FileSystem.remove(file.c_str());
    File thresholdFile = FileSystem.open(file.c_str(), FILE_APPEND);
    if (thresholdFile) {
      String entry="";
      for (int Channel_i = 0; Channel_i < g_NumPlants_ic; ++Channel_i) {
        entry="";
        entry += g_Plants_pst[Channel_i].ThresholdLow_i;
        entry += '\n';
        thresholdFile.print(entry);
        entry="";
        entry += g_Plants_pst[Channel_i].ThresholdHigh_i;
        entry += '\n';
        thresholdFile.print(entry);
        entry="";
        entry += g_Plants_pst[Channel_i].ThresholdExpectedChange_i;
        entry += '\n';
        thresholdFile.print(entry);
        entry="";
        entry += g_Plants_pst[Channel_i].TimeOutPumpOn_l;
        entry += '\n';
        thresholdFile.print(entry);
        entry="";
        entry +=g_Plants_pst[Channel_i].TimePumpOnMax_l;
        entry += '\n';
        thresholdFile.print(entry);
        entry="";
        entry += g_Plants_pst[Channel_i].TimeWait_l;
        entry += '\n';
        thresholdFile.print(entry);
        entry="";
        entry += g_Plants_pst[Channel_i].TimeOutPumpOff_l;
        entry += '\n';
        thresholdFile.print(entry);
        entry="";
        entry += g_Plants_pst[Channel_i].TimeOutErrorState_l;
        entry += '\n';
        thresholdFile.print(entry);
      }
      thresholdFile.close();
      pushToServer(g_PgmWateringThreshold_pc);
    }
  }
}
#endif

//******************************************************************************************
//  read threshold values from SD card
//******************************************************************************************
#if (0 != gd_WATERING_SYSTEM_IOT)
void readThresholdSD()
{
  String file="";
  stringFromPgm(g_PgmBasePath_pc, file);
  stringFromPgm(g_PgmWateringThreshold_pc, file);
  File thresholdFile = FileSystem.open(file.c_str(), FILE_READ);
  if (thresholdFile) {
    int Channel_i = 0;
    int Index_i = 0;
    String s="";
    while(thresholdFile.available() && Channel_i < g_NumPlants_ic) {
      char c = thresholdFile.read();
      if ('\n' != c) {
        s += String(c);
      } else {
        // newline
        long value = (long)s.toInt();
        s = "";
        switch (Index_i) {
          case 0:
            g_Plants_pst[Channel_i].ThresholdLow_i = (int)value;
            break;
          case 1:
            g_Plants_pst[Channel_i].ThresholdHigh_i = (int)value;
            break;
          case 2:
            g_Plants_pst[Channel_i].ThresholdExpectedChange_i = (int)value;
            break;
          case 3:
            g_Plants_pst[Channel_i].TimeOutPumpOn_l = value;
            break;
          case 4:
            g_Plants_pst[Channel_i].TimePumpOnMax_l = value;
            break;
          case 5:
            g_Plants_pst[Channel_i].TimeWait_l = value;
            break;
          case 6:
            g_Plants_pst[Channel_i].TimeOutPumpOff_l = value;
            break;
          case 7:
            g_Plants_pst[Channel_i].TimeOutErrorState_l = value;
            break;
          default:
            break;
        }
        if (Index_i < 7) {
          ++Index_i;
        } else {
          Index_i = 0;
          ++Channel_i;
        }
      }
    }
    thresholdFile.close();    
  } else {
    defaultThreshold();
  }
}
#endif

//******************************************************************************************
//  push to server
//******************************************************************************************
#if (0 != gd_WATERING_SYSTEM_IOT)
void pushToServer(const char* file_pc)
{
  String file = "";
  stringFromPgm(file_pc, file);
  pushToServer(file);
}
#endif

//******************************************************************************************
//  push to server
//******************************************************************************************
#if (0 != gd_WATERING_SYSTEM_IOT)
void pushToServer(const String& file)
{
  String cmd="";
  stringFromPgm(g_PgmBasePath_pc, cmd);
  stringFromPgm(g_PgmPushToServerScript_pc, cmd);
  cmd += " ";
  cmd += file;
  Process push;
  push.runShellCommand(cmd);
  // while(proc.running());
  terminalPrintln(cmd);
}
#endif

//******************************************************************************************
//  getTimeStamp
//******************************************************************************************
#if (0 != gd_WATERING_SYSTEM_IOT)
void getTimeStamp(String& value) {
  Process timeProc;
  timeProc.begin("date");
  timeProc.addParameter("+%Y.%m.%d_%H-%M-%S");
  timeProc.run();

  // read output of "date" command
  while (timeProc.available() > 0) {
    char c = timeProc.read();
    if (c != '\n') {
      value += c;
    }
  }
}
#endif

//******************************************************************************************
//  terminal: available
//******************************************************************************************
int terminalAvailable()
{
#if (0 == gd_WATERING_SYSTEM_SERIAL)
  return Console.available();
#else
  return Serial.available();
#endif  
}

//******************************************************************************************
//  terminal: readString
//******************************************************************************************
String terminalReadString()
{
#if (0 == gd_WATERING_SYSTEM_SERIAL)
  return Console.readString();
#else
  return Serial.readString();
#endif  
}

//******************************************************************************************
//  terminal: print
//******************************************************************************************
size_t terminalPrint(const String& val)
{
#if (0 == gd_WATERING_SYSTEM_SERIAL)
  return Console.print(val);
#else
  return Serial.print(val);
#endif  
}

//******************************************************************************************
//  terminal: print
//******************************************************************************************
size_t terminalPrint(const char val[])
{
#if (0 == gd_WATERING_SYSTEM_SERIAL)
  return Console.print(val);
#else
  return Serial.print(val);
#endif  
}

//******************************************************************************************
//  terminal: print
//******************************************************************************************
size_t terminalPrint(char val)
{
#if (0 == gd_WATERING_SYSTEM_SERIAL)
  return Console.print(val);
#else
  return Serial.print(val);
#endif  
}

//******************************************************************************************
//  terminal: print
//******************************************************************************************
size_t terminalPrint(int val)
{
#if (0 == gd_WATERING_SYSTEM_SERIAL)
  return Console.print(val);
#else
  return Serial.print(val);
#endif  
}

//******************************************************************************************
//  terminal: print
//******************************************************************************************
size_t terminalPrint(unsigned int val)
{
#if (0 == gd_WATERING_SYSTEM_SERIAL)
  return Console.print(val);
#else
  return Serial.print(val);
#endif  
}

//******************************************************************************************
//  terminal: print
//******************************************************************************************
size_t terminalPrint(long val)
{
#if (0 == gd_WATERING_SYSTEM_SERIAL)
  return Console.print(val);
#else
  return Serial.print(val);
#endif  
}

//******************************************************************************************
//  terminal: print
//******************************************************************************************
size_t terminalPrint(unsigned long val)
{
#if (0 == gd_WATERING_SYSTEM_SERIAL)
  return Console.print(val);
#else
  return Serial.print(val);
#endif  
}

//******************************************************************************************
//  terminal: println
//******************************************************************************************
size_t terminalPrintln(const String& val)
{
#if (0 == gd_WATERING_SYSTEM_SERIAL)
  return Console.println(val);
#else
  return Serial.println(val);
#endif  
}

//******************************************************************************************
//  terminal: println
//******************************************************************************************
size_t terminalPrintln(const char val[])
{
#if (0 == gd_WATERING_SYSTEM_SERIAL)
  return Console.println(val);
#else
  return Serial.println(val);
#endif  
}

//******************************************************************************************
//  terminal: println
//******************************************************************************************
size_t terminalPrintln(char val)
{
#if (0 == gd_WATERING_SYSTEM_SERIAL)
  return Console.println(val);
#else
  return Serial.println(val);
#endif  
}

//******************************************************************************************
//  terminal: println
//******************************************************************************************
size_t terminalPrintln(int val)
{
#if (0 == gd_WATERING_SYSTEM_SERIAL)
  return Console.println(val);
#else
  return Serial.println(val);
#endif  
}

//******************************************************************************************
//  terminal: println
//******************************************************************************************
size_t terminalPrintln(unsigned int val)
{
#if (0 == gd_WATERING_SYSTEM_SERIAL)
  return Console.println(val);
#else
  return Serial.println(val);
#endif  
}

//******************************************************************************************
//  terminal: println
//******************************************************************************************
size_t terminalPrintln(long val)
{
#if (0 == gd_WATERING_SYSTEM_SERIAL)
  return Console.println(val);
#else
  return Serial.println(val);
#endif  
}

//******************************************************************************************
//  terminal: println
//******************************************************************************************
size_t terminalPrintln(unsigned long val)
{
#if (0 == gd_WATERING_SYSTEM_SERIAL)
  return Console.println(val);
#else
  return Serial.println(val);
#endif  
}
