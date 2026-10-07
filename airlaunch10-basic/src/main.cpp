#include <Arduino.h>

// ------------------------------------------------------------------------------------------------
// Function Prototypes
// ------------------------------------------------------------------------------------------------
void serialPrintState(void);

// State Handling Functions
void handleIdleState(void);
void handleArmState(void);
void handlePreFireState(void);
void handleFireState(void);
void handleDumpState(void);
// void handleFaultState(void);

// Read Switches
void readSwitches(void);

// ------------------------------------------------------------------------------------------------
// Declare Parameters
// ------------------------------------------------------------------------------------------------
const long preFireTime_ms = 3000; // Pre Fire Time (miliseconds)
const long fireTime_ms = 1000; // Fire Valve Open Time (miliseconds)

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

// Pin Definitions - Arm Switch
const int arm_switch_pin   = A2;  // I3
const int estop_switch_pin = A3;  // I4
const int fire_switch_pin  = A7;  // I8

// Switch Statuss
bool barmSwitch = false;
bool bfireSwitch = false;
bool bestopSwitch = false;

// Timer Variables
long pre_fire_time = 0;
long pre_fire_current = 0;
long fire_time = 0;
long fire_current = 0;

// ------------------------------------------------------------------------------------------------
// Setup
// ------------------------------------------------------------------------------------------------
void setup() {

  // Initiate Serial
  // --------------------------------------------------------------
  Serial.begin(9600);

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
