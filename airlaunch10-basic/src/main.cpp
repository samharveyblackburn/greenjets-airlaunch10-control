#include <Arduino.h>
#include <ArduinoRS485.h> // RS485
#include "StatusPacket.h" // RS485 Status Packet

// ------------------------------------------------------------------------------------------------
// Function Prototypes
// ------------------------------------------------------------------------------------------------
// USB Serial Debugging
void serialPrintState(void);

// RS485 Status Packet
void sendStatusPacket(void);

// State Handling Functions
void handleIdleState(void);
void handleArmState(void);
void handlePreFireState(void);
void handleFireState(void);
void handleDumpState(void);
// void handleFaultState(void);

// Read Switches
void readSwitches(void);

// Read Pressure / Temperature
void readSensors(void);

// ------------------------------------------------------------------------------------------------
// Declare Parameters
// ------------------------------------------------------------------------------------------------
const long preFireTime_ms = 3000; // Pre Fire Time (miliseconds)
const long fireTime_ms = 1000; // Fire Valve Open Time (miliseconds)
const unsigned long UPDATE_INTERVAL_MS = 1000; // RS485 Signal Interval (miliseconds)

const float pressure_cal = (40.0/10.0); // Pressure Calibration
const float temperature_cal = 0.0; // Temperature Calibration

// ------------------------------------------------------------------------------------------------
// Declare variables
// ------------------------------------------------------------------------------------------------
// Definition of states
enum class State{
  IDLE, 
  ARM, 
  PREFIRE,
  FIRE, 
  COMPLETE, 
  DUMP, 
  FAULT
};

// Definition of state names
const char* stateNames[] = {
    "IDLE",
    "ARM",
    "PREFIRE",
    "FIRE",
    "COMPLETE",
    "DUMP",
    "FAULT"
};

const int nStates = sizeof(stateNames) / sizeof(stateNames[0]);


// Initial state
State state = State::IDLE;

// Pin Definitions - Arm Valve
const int arm_valve_open_pin = D0;
const int arm_valve_open_led = LED_D0;

// Pin Definitions - Fire Valve
const int fire_valve_open_pin = D1;
const int fire_valve_open_led = LED_D1;

// Pin Definitions - Dump Valve
const int dump_valve_close_pin = D2;
const int dump_valve_close_led = LED_D2;

// Pin Definitions - Switches
const int arm_switch_pin   = A2;  // I3
const int estop_switch_pin = A3;  // I4
const int fire_switch_pin  = A7;  // I8

// Pin Definitions - Pressure / Temp Sensors
const int pressure_pin = A0; // I1
const int temp_pin = A1; // I2 

// Switch Statuss
bool barmSwitch = false;
bool bfireSwitch = false;
bool bestopSwitch = false;

// Timer Variables
long pre_fire_time = 0;
long pre_fire_current = 0;
long fire_time = 0;
long fire_current = 0;
unsigned long lastRS485Update = 0;
uint32_t RS485counter = 0;

// Pressure / Temperature Variables
uint16_t pAccumulator_raw = 0;
uint16_t TAccumulator_raw = 0;
float pAccumulator = 0.0;
float pAccumulator_target = 0.0;
float TAccumulator = 0.0;

// RS485 Status Packet
StatusPacket packet;

// ------------------------------------------------------------------------------------------------
// Setup
// ------------------------------------------------------------------------------------------------
void setup() {

  // Initiate Serial
  // --------------------------------------------------------------
  Serial.begin(9600);

  // Initiate RS485
  // --------------------------------------------------------------
  RS485.begin(9600);
  RS485.setDelays(0, 1200);

  // Setup Pin Modes
  // --------------------------------------------------------------
  // Set Pin Modes - Output
  pinMode(arm_valve_open_pin, OUTPUT);
  pinMode(arm_valve_open_led, OUTPUT);
  pinMode(fire_valve_open_pin, OUTPUT);
  pinMode(fire_valve_open_led, OUTPUT);
  pinMode(dump_valve_close_pin, OUTPUT);
  pinMode(dump_valve_close_led, OUTPUT);

  // Set Pin Modes - Input
  pinMode(arm_switch_pin, INPUT);
  pinMode(fire_switch_pin, INPUT);
  pinMode(estop_switch_pin, INPUT);
  pinMode(temp_pin, INPUT);
  pinMode(pressure_pin, INPUT);
  
  // Set Initial Valve States
  // --------------------------------------------------------------
  // Arm Valve
  digitalWrite(arm_valve_open_pin, false);
  digitalWrite(arm_valve_open_led, false);

  // Fire Valve
  digitalWrite(fire_valve_open_pin, false);
  digitalWrite(fire_valve_open_led, false);

  // Dump Valve
  digitalWrite(dump_valve_close_pin, true);
  digitalWrite(dump_valve_close_led, true);
  
}

// ------------------------------------------------------------------------------------------------
// Main Loop
// ------------------------------------------------------------------------------------------------
void loop() {

    // Send RS485 Update
    // --------------------------------------------------------------
    if(millis() - lastRS485Update > UPDATE_INTERVAL_MS){
      sendStatusPacket();
      lastRS485Update = millis();
    }

    // Read Sensors
    // --------------------------------------------------------------
    readSensors();

    // Read Switch Statues
    // --------------------------------------------------------------
    readSwitches();

    // State Machine Switch
    // --------------------------------------------------------------
    switch (state) {

        case State::IDLE:
            handleIdleState();
            break;

        case State::ARM:
            handleArmState();
            break;

        case State::PREFIRE:
            handlePreFireState();
            break;

        case State::FIRE:
            handleFireState();
            break;

        // case State::FAULT:
        //     handleFaultState();
        //     break;

        case State::DUMP:
            handleDumpState();
            break;

        default:
            // Safe fallback
            state = State::IDLE;
            break;
    }


    // Serial Output
    // --------------------------------------------------------------
    delay(10);
    serialPrintState();

}

// ------------------------------------------------------------------------------------------------
// Function Definitions 
// ------------------------------------------------------------------------------------------------

void sendStatusPacket(void){
    // Update Packet - State
    int iState = static_cast<int>(state);
    packet.state = iState;

    // Update Packet - Switches
    packet.barmSwitch = barmSwitch;
    packet.bfireSwitch = bfireSwitch;
    packet.bestopSwitch = bfireSwitch;

    // Update Packet - Pressures / Temperatures
    packet.pAccumulator_target = pAccumulator_target;
    packet.pAccumulator = pAccumulator;
    packet.TAccumulator = TAccumulator;

    // Create buffer
    uint8_t buffer[StatusPacket::SIZE];

    // Encode packet
    packet.encode(buffer);

    // Transmit packet
    RS485.beginTransmission();
    RS485.write(buffer, StatusPacket::SIZE);
    RS485.endTransmission();

    packet.sequence++;
};

void serialPrintState() {
    // Print Switches
    // --------------------------------------------------------------
    Serial.print("barm:");
    Serial.print(barmSwitch);
    Serial.print("    ");
    Serial.print("bfire:");
    Serial.print(bfireSwitch);
    Serial.print("    ");
    Serial.print("bstop:");
    Serial.print(bestopSwitch);

    // Print State
    // --------------------------------------------------------------
    int iState = static_cast<int>(state);

    Serial.print("State: ");

    if (iState >= 0 && iState < nStates) {
        Serial.println(stateNames[iState]);
    }
    else {
        Serial.println("UNKNOWN");
    }
}

void readSwitches(){
  // Switch Statuss
  barmSwitch = digitalRead(arm_switch_pin);
  bfireSwitch = digitalRead(fire_switch_pin);
  bestopSwitch = digitalRead(estop_switch_pin);
};

void readSensors(void){
  
  // Read and calibrate pressure
  pAccumulator_raw = analogRead(pressure_pin);
  pAccumulator = pAccumulator_raw*pressure_cal;

  // Read and calibrate temperature
  TAccumulator_raw = analogRead(temp_pin);
  TAccumulator = TAccumulator_raw*temperature_cal;
};

// ------------------------------------------------------------------------------------------------
// Function Definitions - State Handling
// ------------------------------------------------------------------------------------------------

// IDLE State
// --------------------------------------------------------------
void handleIdleState(){
  // --------------------------------------------
  // Set Valves
  // --------------------------------------------
  // Arm Valve
  digitalWrite(arm_valve_open_pin, false);
  digitalWrite(arm_valve_open_led, false);

  // Fire Valve
  digitalWrite(fire_valve_open_pin, false);
  digitalWrite(fire_valve_open_led, false);

  // Dump Valve
  digitalWrite(dump_valve_close_pin, true);
  digitalWrite(dump_valve_close_led, true);

  
  // --------------------------------------------
  // State Transitions
  // --------------------------------------------
  // State Transition: ARM -> DUMP
  if(!bestopSwitch){
    state = State::DUMP;
  }
  // State Transition: IDLE -> ARM 
  else if(barmSwitch && !bfireSwitch && bestopSwitch){
    state = State::ARM;
  }

  // State Transition: IDLE -> FAULT

};

// ARM State
// --------------------------------------------------------------
void handleArmState(){

  // --------------------------------------------
  // Set Valves
  // --------------------------------------------
  // Arm Valve
  digitalWrite(arm_valve_open_pin, true);
  digitalWrite(arm_valve_open_led, true);

  // Fire Valve
  digitalWrite(fire_valve_open_pin, false);
  digitalWrite(fire_valve_open_led, false);

  // Dump Valve
  digitalWrite(dump_valve_close_pin, true);
  digitalWrite(dump_valve_close_led, true);

  // --------------------------------------------
  // State Transitions
  // --------------------------------------------
  // State Transition: ARM -> DUMP
  if(!bestopSwitch){
    state = State::DUMP;
  }
  // State Transition: ARM -> IDLE
  else if(!barmSwitch){
    state = State::IDLE;
  }
  // State Transition: ARM -> PREFIRE
  else if(barmSwitch && bfireSwitch && bestopSwitch){
    state = State::PREFIRE;

    // Set prefire timer
    pre_fire_time = millis();
  }
};

// PREFIRE State
// --------------------------------------------------------------
void handlePreFireState(void){
  // --------------------------------------------
  // Set Valves
  // --------------------------------------------
  // Arm Valve
  digitalWrite(arm_valve_open_pin, true);
  digitalWrite(arm_valve_open_led, true);

  // Fire Valve
  digitalWrite(fire_valve_open_pin, false);
  digitalWrite(fire_valve_open_led, false);

  // Dump Valve
  digitalWrite(dump_valve_close_pin, true);
  digitalWrite(dump_valve_close_led, true);
  
  // --------------------------------------------
  // Evaulate Timer
  // --------------------------------------------
  pre_fire_current = millis();

  // --------------------------------------------
  // State Transitions
  // --------------------------------------------
  // State Transition: PRE-FIRE -> DUMP
  if(!bestopSwitch){
    state = State::DUMP;
  }
  // State Transition: PREFIRE -> ARM
  else if(
    barmSwitch &&
    !bfireSwitch &&
    bestopSwitch
  )
  {
    state = State::ARM;
  }
  // State Transition: PREFIRE -> FIRE
  else if(
    barmSwitch &&
    bfireSwitch && 
    bestopSwitch &&
    (pre_fire_current > pre_fire_time + preFireTime_ms) ){

      fire_time = millis();

      state = State::FIRE;
    };
}
// FIRE State
// --------------------------------------------------------------
void handleFireState(){
  // --------------------------------------------
  // Set Valves
  // --------------------------------------------
  // Arm Valve
  digitalWrite(arm_valve_open_pin, true);
  digitalWrite(arm_valve_open_led, true);

  // Fire Valve
  digitalWrite(fire_valve_open_pin, true);
  digitalWrite(fire_valve_open_led, true);

  // Dump Valve
  digitalWrite(dump_valve_close_pin, true);
  digitalWrite(dump_valve_close_led, true);

  // --------------------------------------------
  // Evaulate Timer
  // --------------------------------------------
  fire_current = millis();

  // --------------------------------------------
  // State Transitions
  // --------------------------------------------
    // State Transition: FIRE -> DUMP
  if(!bestopSwitch){
    state = State::DUMP;
  }
  // State Transition: FIRE -> IDLE
  else if (fire_current > fire_time + fireTime_ms){
    state = State::IDLE;
  }
};

// DUMP State
// --------------------------------------------------------------
void handleDumpState(){
  // --------------------------------------------
  // Set Valves
  // --------------------------------------------
  // Arm Valve
  digitalWrite(arm_valve_open_pin, false);
  digitalWrite(arm_valve_open_led, false);

  // Fire Valve
  digitalWrite(fire_valve_open_pin, false);
  digitalWrite(fire_valve_open_led, false);

  // Dump Valve
  digitalWrite(dump_valve_close_pin, false);
  digitalWrite(dump_valve_close_led, false);

  // --------------------------------------------
  // State Transitions
  // --------------------------------------------
  // State Transition: DUMP -> IDLE
  if (
    !barmSwitch &&
    bestopSwitch &&
    !bfireSwitch){
    state = State::IDLE;
  }
}
