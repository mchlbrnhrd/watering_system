// Watering system by Michael Bernhard; 2020.04.23


//******************************************************************************************
//******************************************************************************************
//  define constants
//******************************************************************************************
//******************************************************************************************

// number of plants (pumps, sensors and relais)
const int g_NumPlants_ic = 4;

// pin connection of sensor and pump (relais module) to arduino board
const int g_SensorPin_pic[g_NumPlants_ic] = {A0, A1, A2, A3};
const int g_PumpPin_pic[g_NumPlants_ic] = {2, 3, 4, 5};

// store text in PROGMEM
const char g_PgmWatering_pc[] PROGMEM = {"Watering system by Michael Bernhard"};
const char g_PgmHelp_pc[] PROGMEM = {"h: help; d: debug; s: soft reset; r: reset; i: short info; t: terminal; m: manual mode; c: cancel/continue"};
const char g_PgmInfo_pc[] PROGMEM = {"Info"};
const char g_PgmSensor_pc[] PROGMEM = {"Sensor "};
const char g_PgmContinue_pc[] PROGMEM = {"Continue... "};
const char g_PgmChannel_pc[] PROGMEM = {"Channel: "};
const char g_PgmCommand_pc[] PROGMEM = {"Command: "};
const char g_PgmValue_pc[] PROGMEM = {"Value: "};
const char g_PgmManual_pc[] PROGMEM = {"[0] pump off; [1] pump on; [c] cancel"};
const char g_PgmReset_pc[] PROGMEM = {"Reset"};
const char g_PgmSoftReset_pc[] PROGMEM = {"Soft reset"};
const char g_PgmDebugMode_pc[] PROGMEM = {"Debug mode "};

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
const char g_PgmM2_pc[] PROGMEM = {"mode pump on"};
const char g_PgmM3_pc[] PROGMEM = {"mode pump off"};
const char g_PgmM4_pc[] PROGMEM = {"mode pump error"};

const char g_PgmErrR1_pc[] PROGMEM = {": Err: Switch off pump due of timeout. Check sensor. R1"};
const char g_PgmErrR6_pc[] PROGMEM = {": Err: Force pump due of timeout T4. Check sensor. R6"};
const char g_PgmInfoR7_pc[] PROGMEM = {": Info: Leave Error state. R7"};
const char g_PgmDebugR2_pc[] PROGMEM = {": pump on (threshold) R2"};
const char g_PgmDebugR3_pc[] PROGMEM = {": pump off (threshold) R3"};
const char g_PgmDebugR4_pc[] PROGMEM = {": pump off (time max on) R4"};
const char g_PgmDebugR5_pc[] PROGMEM = {": ready R5"};



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



//******************************************************************************************
//******************************************************************************************
//  global variable
//******************************************************************************************
//******************************************************************************************
typedefPlant g_Plants_pst[g_NumPlants_ic];
int g_Timer_i = 0;
bool g_HeartBeat_bl = false;
bool g_PrintInfo_bl = false;
bool g_DebugMode_bl = false;



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
  }
  
  for (int k = 0; k < 5; ++k)
  {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(50);
    digitalWrite(LED_BUILTIN, LOW);
    delay(30);
  }
  
  Serial.begin(9600);
  serialPrintlnPgm(g_PgmWatering_pc);
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
  
  // read sensor values
  readSensor();

  // check if key is pressed
  while (Serial.available() > 0) {
    String Key_s = Serial.readString();
    Key_s.trim();
    if (Key_s.equals("h") || Key_s.equals("H")) {
      serialPrintlnPgm(g_PgmHelp_pc);
    } else if (Key_s.equals("t") || Key_s.equals("T")) {
      terminal();
    } else if (Key_s.equals("i") || Key_s.equals("I")) {
      printShortInfo();
    } else if (Key_s.equals("r") || Key_s.equals("R")) {
      serialPrintlnPgm(g_PgmReset_pc);
      reset();
    } else if (Key_s.equals("s") || Key_s.equals("S")) {
      serialPrintlnPgm(g_PgmSoftReset_pc);
      softReset();
    } else if (Key_s.equals("d") || Key_s.equals("D")) {
      g_DebugMode_bl = !g_DebugMode_bl;
      serialPrintPgm(g_PgmDebugMode_pc);
      Serial.println(g_DebugMode_bl ? "on" : "off");
    } else if (Key_s.equals("m") || Key_s.equals("M")) {
      manualMode();
    }
  }

  // print sensor data in debug mode
  if (g_DebugMode_bl) {
    if (g_PrintInfo_bl) {
      g_PrintInfo_bl = false;
      printShortInfo();
    }
  }

  // pump control
  pumpControl();
}


//******************************************************************************************
//  soft reset
//******************************************************************************************
void softReset() 
{
  for (int i = 0; i < g_NumPlants_ic; ++i) {
    g_Plants_pst[i].Sensor_i = 0;
    g_Plants_pst[i].Mode_enm = modePumpReady;
    g_Plants_pst[i].CurrTime_l = 0;
    digitalWrite(g_PumpPin_pic[i], HIGH); // pump off
  }
  g_Timer_i = 0;
}


//******************************************************************************************
//  reset
//******************************************************************************************
void reset() 
{

//  S1 threshold low             : 350
//  S2 threshold high            : 450
//  S3 threshold expected change : 600
//  T1 timeout pump on           : 5
//  T2 time pump on max          : 20
//  T3 time wait                 : 100
//  T4 time out pump off         : 86400 = 24 h
//  T5 time out error state      : 86400 = 24 h

  for (int i = 0; i < g_NumPlants_ic; ++i) {  
    g_Plants_pst[i].ThresholdLow_i = 350;
    g_Plants_pst[i].ThresholdHigh_i = 450;
    g_Plants_pst[i].ThresholdExpectedChange_i = 600; // use large value, e.g. 600 to deactivate this function
    g_Plants_pst[i].TimeOutPumpOn_l = 5; 
    g_Plants_pst[i].TimePumpOnMax_l = 20;
    g_Plants_pst[i].TimeWait_l = 100;
    g_Plants_pst[i].TimeOutPumpOff_l = 3*60*60*24; // 3 days
    g_Plants_pst[i].TimeOutErrorState_l = 60*60*24; // 24 hours
  }
  g_DebugMode_bl = false;
  g_PrintInfo_bl = false;
  softReset();
}


//******************************************************************************************
//  read sensor values
//******************************************************************************************
void readSensor()
{
  for (int i=0; i < g_NumPlants_ic; ++i) {
    g_Plants_pst[i].Sensor_i = analogRead(g_SensorPin_pic[i]);
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
          Serial.print(i);
          serialPrintlnPgm(g_PgmDebugR2_pc);
        }
      } else if (g_Plants_pst[i].CurrTime_l > g_Plants_pst[i].TimeOutPumpOff_l) {
         g_Plants_pst[i].Mode_enm = modePumpOn;
         printChannel(i);
         Serial.print(i);
         serialPrintlnPgm(g_PgmErrR6_pc);
      }
      if (modePumpOn == g_Plants_pst[i].Mode_enm) {
        digitalWrite(g_PumpPin_pic[i], LOW);
        TimerReset_bl = true;
      }

      
    } else if (modePumpOn == g_Plants_pst[i].Mode_enm) {
      // ************* mode pump on **************
      if (g_Plants_pst[i].CurrTime_l > g_Plants_pst[i].TimeOutPumpOn_l) {
        if (g_Plants_pst[i].Sensor_i > g_Plants_pst[i].ThresholdExpectedChange_i) {
          digitalWrite(g_PumpPin_pic[i], HIGH);
          g_Plants_pst[i].Mode_enm = modePumpError;
          Serial.print(i);
          serialPrintlnPgm(g_PgmErrR1_pc);
          TimerReset_bl = true;
        }
        if (g_Plants_pst[i].Sensor_i < g_Plants_pst[i].ThresholdLow_i) {
          g_Plants_pst[i].Mode_enm = modePumpOff;
          digitalWrite(g_PumpPin_pic[i], HIGH);
          TimerReset_bl = true;
          if (g_DebugMode_bl) {
            Serial.print(i);
            serialPrintlnPgm(g_PgmDebugR3_pc);
          }
        } else if (g_Plants_pst[i].CurrTime_l  > g_Plants_pst[i].TimePumpOnMax_l) {
          g_Plants_pst[i].Mode_enm = modePumpOff;
          digitalWrite(g_PumpPin_pic[i], HIGH);
          TimerReset_bl = true;
          if (g_DebugMode_bl) {
            Serial.print(i);
            serialPrintlnPgm(g_PgmDebugR4_pc);
          }
        }
      }

      
    } else if (modePumpOff == g_Plants_pst[i].Mode_enm) {
      // ************* mode pump off **************
      if (g_Plants_pst[i].CurrTime_l  > g_Plants_pst[i].TimeWait_l) {
        g_Plants_pst[i].Mode_enm = modePumpReady;
        TimerReset_bl = true;
        if (g_DebugMode_bl) {
          Serial.print(i);
          serialPrintlnPgm(g_PgmDebugR5_pc);
        }
      }

      
    } else { // modePumpError
      // ************* mode pump error **************
      if (g_Plants_pst[i].CurrTime_l > g_Plants_pst[i].TimeOutErrorState_l) {
        g_Plants_pst[i].Mode_enm = modePumpReady;
        TimerReset_bl = true;
        Serial.print(i);
        serialPrintlnPgm(g_PgmInfoR7_pc);
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
    serialPrintPgm(g_PgmChannel_pc);
    while (Serial.available() == 0);
    String Key_s = Serial.readString();
    Key_s.trim();
    Serial.println(Key_s);
    String Command_s = "";
    int Channel_i = 0;
    if (Key_s.equals("c") || Key_s.equals("C")) {
      Finished_bl = true;
    } else {
      Channel_i = Key_s.toInt();
      serialPrintPgm(g_PgmCommand_pc);
      while (Serial.available() == 0);
      Command_s = Serial.readString();
      Command_s.trim();
      Serial.println(Command_s);
     if (Command_s.equals("c") || Command_s.equals("C")) {
        Finished_bl = true;
      }
    }
    if (!Finished_bl) {
      serialPrintPgm(g_PgmValue_pc);
      while (Serial.available() == 0);
      Key_s = Serial.readString();
      Key_s.trim();
      Serial.println(Key_s);
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
  serialPrintlnPgm(g_PgmContinue_pc);
}


//******************************************************************************************
//  manual mode
//******************************************************************************************
void manualMode()
{
  printInfo();
  bool Finished_bl=false;
 
  serialPrintPgm(g_PgmChannel_pc);
  while (Serial.available() == 0);
  String Key_s = Serial.readString();
  Key_s.trim();
  Serial.println(Key_s);
  int Channel_i = 0;
  if (Key_s.equals("c") || Key_s.equals("C")) {
    Finished_bl = true;
  } else {
      Channel_i = Key_s.toInt();
      serialPrintlnPgm(g_PgmManual_pc);
  }
  while(!Finished_bl) {
    if (Serial.available() > 0) {
      Key_s = Serial.readString();
      Key_s.trim();
      Serial.print("Pump ");
      if (Key_s.equals("c") || Key_s.equals("C")) {
        Finished_bl = true;
      } else if (Key_s.equals("1") || Key_s.equals("1")) {
        Serial.println("on");
        digitalWrite(g_PumpPin_pic[Channel_i], LOW); // pump on
      } else {
        Serial.println("off");
        digitalWrite(g_PumpPin_pic[Channel_i], HIGH); // pump off
      }
    }
    if (g_DebugMode_bl) {
      if (g_PrintInfo_bl) {
        g_PrintInfo_bl = false;
        printShortInfo();
      }
    }
  }
  serialPrintlnPgm(g_PgmContinue_pc);
  softReset();
}


//******************************************************************************************
//  print short info
//******************************************************************************************
void printShortInfo()
{
  for (int i=0; i < g_NumPlants_ic; ++i) {
    serialPrintPgm(g_PgmSensor_pc);
    Serial.print(i);
    Serial.print(": ");
    Serial.println(g_Plants_pst[i].Sensor_i);
  }
}


//******************************************************************************************
//  print channel info
//******************************************************************************************
void printChannel(int Channel_i)
{
  if (Channel_i  < g_NumPlants_ic) {
    serialPrintPgm(g_PgmSensor_pc);
    Serial.print(Channel_i);
    Serial.print(": ");
    Serial.println(g_Plants_pst[Channel_i].Sensor_i);
    switch(g_Plants_pst[Channel_i].Mode_enm) {
      case modePumpReady:
      serialPrintlnPgm(g_PgmM1_pc);
      break;
      case modePumpOn:
      serialPrintlnPgm(g_PgmM2_pc);
      break;
      case modePumpOff:
      serialPrintlnPgm(g_PgmM3_pc);
      break;
      default: // modePumpError
      serialPrintlnPgm(g_PgmM4_pc);
      break;
    }
    serialPrintPgm(g_PgmC1_pc);
    Serial.println(g_Plants_pst[Channel_i].CurrTime_l);
    
    serialPrintPgm(g_PgmS1_pc);
    Serial.println(g_Plants_pst[Channel_i].ThresholdLow_i);
    serialPrintPgm(g_PgmS2_pc);
    Serial.println(g_Plants_pst[Channel_i].ThresholdHigh_i);
    serialPrintPgm(g_PgmS3_pc);
    Serial.println(g_Plants_pst[Channel_i].ThresholdExpectedChange_i);

    serialPrintPgm(g_PgmT1_pc);
    Serial.println(g_Plants_pst[Channel_i].TimeOutPumpOn_l); 
    serialPrintPgm(g_PgmT2_pc);
    Serial.println(g_Plants_pst[Channel_i].TimePumpOnMax_l);
    serialPrintPgm(g_PgmT3_pc);
    Serial.println(g_Plants_pst[Channel_i].TimeWait_l);
    serialPrintPgm(g_PgmT4_pc);
    Serial.println(g_Plants_pst[Channel_i].TimeOutPumpOff_l);
    serialPrintPgm(g_PgmT5_pc);
    Serial.println(g_Plants_pst[Channel_i].TimeOutErrorState_l);
    Serial.println("");
  }
}


//******************************************************************************************
//  print Info
//******************************************************************************************
void printInfo()
{
  serialPrintlnPgm(g_PgmInfo_pc);
  printShortInfo();
  for (int i=0; i < g_NumPlants_ic; ++i) {
   printChannel(i);
  }
}


//******************************************************************************************
//  println from PROGMEM
//******************************************************************************************
void serialPrintlnPgm(const char* string_pc) {
  const int len = strlen_P(string_pc);
  for (int k = 0; k < len-1; ++k) {
    Serial.print((char)pgm_read_byte_near(string_pc + k));
  }
  Serial.println((char)pgm_read_byte_near(string_pc + len - 1));
}


//******************************************************************************************
//  print from PROGMEM
//******************************************************************************************
void serialPrintPgm(const char* string_pc) {
  const int len = strlen_P(string_pc);
  for (int k = 0; k < len; ++k) {
    Serial.print((char)pgm_read_byte_near(string_pc + k));
  }
}


//******************************************************************************************
//  Timer interrupt
//******************************************************************************************
ISR(TIMER1_COMPA_vect)
{
  ++g_Timer_i;
  if (g_Timer_i >= 1) {
    for(int i=0; i < g_NumPlants_ic; ++i) {
      ++g_Plants_pst[i].CurrTime_l;
    }
    g_HeartBeat_bl = !g_HeartBeat_bl;
    g_PrintInfo_bl = true;
    g_Timer_i=0;
  }
}
