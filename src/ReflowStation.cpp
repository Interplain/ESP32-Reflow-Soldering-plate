// ReflowStation.cpp - PlatformIO Version with Integrated Profile Display
#include <Arduino.h>
#include "Types.h"
#include "Profiles.h"
#include "ProfileRunner.h"
#include "SensorManager.h"
#include "HeaterController.h"
#include "FanController.h"
#include "InputEncoder.h"
#include "DisplayUI.h"

// ---- Hardware Pins ----
#define THERM_FRONT 32
#define THERM_BACK  33
#define SSR_FRONT   18
#define SSR_BACK    5
#define FAN_PIN     19
#define BUZZER_PIN  23
#define ENC_A       34
#define ENC_B       35
#define ENC_BTN     25
#define I2C_SDA     21
#define I2C_SCL     22

// ---- Global Objects ----
SensorManager sensors;
HeaterController heater;
FanController fan;
ProfileRunner profRunner;
InputEncoder encoder;
DisplayUI ui;

// ---- State Variables ----
Mode currentMode = MENU;
int menuIndex = 0;
bool manualFanMode = false;
bool manualFanState = false;
HeatState heatSelection = HEAT_BOTH;
HeatState heatActive = HEAT_OFF;
uint8_t selectedProfile = 0;
int constTemp = 150;
int constDuration = 300;
int testPct = 0;
bool profileRunning = false;
bool constRunning = false;
bool profileDone = false;
bool profileAborted = false;
unsigned long runStartTime = 0;

// ---- Control State Variables ----
float g_lastSetpoint = 0.0f;
bool g_inCoolingMode = false;
bool g_coolingResetDone = false;

// ---- Cooling Test Variables ----
bool g_testStarted = false;
unsigned long g_lastLog = 0;
float g_startTemp = 0;
unsigned long g_testStart = 0;

// ---- Heat Selection Functions ----
void nextHeatSelection() {
    switch (heatSelection) {
        case HEAT_OFF:   heatSelection = HEAT_BOTH;  break;
        case HEAT_BOTH:  heatSelection = HEAT_FRONT; break;
        case HEAT_FRONT: heatSelection = HEAT_BACK;  break;
        default:         heatSelection = HEAT_OFF;   break;
    }
}

bool hasHeatersSelected() {
    return heatSelection != HEAT_OFF;
}

void startHeatFromSelection() {
    heatActive = heatSelection;
}

void stopHeatAndReset() {
    heatActive = HEAT_OFF;
    heatSelection = HEAT_BOTH;
}

// ---- Mode Control Functions ----
void startProfile() {
    profRunner.begin(PROFILES[selectedProfile]);
    ui.setupProfileDisplay(PROFILES[selectedProfile], profRunner.durationSec());
    
    currentMode = PROFILE_RUN;
    profileRunning = true;
    profileDone = false;
    profileAborted = false;
    runStartTime = millis();
    
    // Reset cooling state variables
    g_lastSetpoint = 0.0f;
    g_inCoolingMode = false;
    g_coolingResetDone = false;
    
    startHeatFromSelection();
    manualFanMode = false;
    
    PIDGains gains = {3.0f, 0.13f, 8.0f, 150.0f};
    heater.setGains(gains);
    heater.reset();
    fan.set(false);
    
    Serial.print("Started profile: ");
    Serial.println(PROFILES[selectedProfile].name);
}

void startConstant() {
    currentMode = CONST_RUN;
    constRunning = true;
    profileDone = false;
    profileAborted = false;
    runStartTime = millis();
    
    startHeatFromSelection();
    manualFanMode = false;
    
    PIDGains gains = {10.0f, 0.1f, 100.0f, 150.0f};
    heater.setGains(gains);
    heater.reset();
    fan.set(false);
    
    Serial.print("Started constant: ");
    Serial.print(constTemp);
    Serial.println("C");
}

void returnToMenu() {
    profileDone = false;
    profileAborted = false;
    profileRunning = false;
    constRunning = false;
    currentMode = MENU;
    heater.reset();
    manualFanMode = false;
    fan.set(true);
    stopHeatAndReset();
    
    // Reset global state variables
    g_lastSetpoint = 0.0f;
    g_inCoolingMode = false;
    g_coolingResetDone = false;
}

void handleButtonPress() {
    switch (currentMode) {
        case MENU:
            switch (menuIndex) {
                case 0:
                    nextHeatSelection();
                    tone(BUZZER_PIN, 1500, 60);
                    break;
                case 1:
                    if (!hasHeatersSelected()) {
                        tone(BUZZER_PIN, 800, 200);
                        break;
                    }
                    currentMode = PROF_SETUP;
                    break;
                case 2:
                    if (!hasHeatersSelected()) {
                        tone(BUZZER_PIN, 800, 200);
                        break;
                    }
                    currentMode = CONST_SETUP;
                    break;
                case 3:
                    if (!manualFanMode) {
                        manualFanMode = true;
                        manualFanState = false;
                        fan.set(false);
                        tone(BUZZER_PIN, 1500, 60);
                    } else {
                        manualFanState = !manualFanState;
                        fan.set(manualFanState);
                        tone(BUZZER_PIN, 1500, 60);
                    }
                    break;
                case 4:
                    currentMode = TEST_RUN;
                    testPct = 0;
                    manualFanMode = false;
                    fan.set(false);
                    break;
            }
            break;
            
        case PROF_SETUP:
            startProfile();
            break;
            
        case CONST_SETUP:
            startConstant();
            break;
            
        case PROFILE_RUN:
            if (profileDone || profileAborted) {
                returnToMenu();
            } else {
                profileAborted = true;
                tone(BUZZER_PIN, 1200, 80);
            }
            break;
            
        case CONST_RUN:
            if (profileDone || profileAborted) {
                returnToMenu();
            } else {
                profileAborted = true;
                tone(BUZZER_PIN, 1200, 80);
            }
            break;
            
        case TEST_RUN:
            heater.reset();
            fan.set(false);
            currentMode = MENU;
            tone(BUZZER_PIN, 1200, 80);
            break;
            
        default:
            break;
    }
}

void handleLongPress() {
    if (currentMode == TEST_RUN) {
        currentMode = COOL_TEST;
        // Reset cooling test variables
        g_testStarted = false;
        g_lastLog = 0;
        g_startTemp = 0;
        g_testStart = 0;
        Serial.println("=== STARTING COOLING TEST ===");
        tone(BUZZER_PIN, 1500, 200);
        return;
    }
    
    heater.reset();
    fan.set(false);
    manualFanMode = false;
    profileRunning = false;
    constRunning = false;
    currentMode = MENU;
    stopHeatAndReset();
    tone(BUZZER_PIN, 1200, 80);
}

// ---- Input Handling ----
void handleEncoder() {
    InputEvents events = encoder.poll();
    
    if (events.steps != 0) {
        switch (currentMode) {
            case MENU:
                menuIndex += events.steps;
                if (menuIndex < 0) menuIndex = 4;
                if (menuIndex > 4) menuIndex = 0;
                break;
                
            case PROF_SETUP:
                selectedProfile += events.steps;
                if ((int)selectedProfile < 0) selectedProfile = PROFILE_COUNT - 1;
                if (selectedProfile >= PROFILE_COUNT) selectedProfile = 0;
                break;
                
            case CONST_SETUP:
                constTemp += events.steps;
                if (constTemp < 0) constTemp = 0;
                if (constTemp > 220) constTemp = 220;
                break;
                
            case TEST_RUN:
                testPct += events.steps * 5;
                if (testPct < 0) testPct = 0;
                if (testPct > 100) testPct = 100;
                break;
                
            default:
                break;
        }
    }
    
    if (events.click) {
        handleButtonPress();
    }
    
    if (events.longPress) {
        handleLongPress();
    }
}

// ---- Cooling Test ----
void runCoolingTest() {
    if (!g_testStarted) {
        heater.setMaxOutput(90);
        PIDGains gains = {6.0f, 0.15f, 8.0f, 150.0f};
        heater.setGains(gains);
        
        float maxTemp = max(sensors.tempFront(), sensors.tempBack());
        heater.control(HEAT_BOTH, 200.0f, sensors.tempFront(), sensors.tempBack());
        
        if (maxTemp >= 200.0f) {
            g_testStarted = true;
            g_startTemp = maxTemp;
            g_testStart = millis();
            g_lastLog = millis();
            
            heater.reset();
            fan.set(true);
            
            Serial.println("\nTime,Temp,Rate");
            Serial.print("0,");
            Serial.print(g_startTemp, 1);
            Serial.println(",0.0");
        }
    } else {
        float maxTemp = max(sensors.tempFront(), sensors.tempBack());
        unsigned long elapsed = millis() - g_testStart;
        
        if (millis() - g_lastLog >= 30000) {
            float elapsedMin = elapsed / 60000.0f;
            float tempDrop = g_startTemp - maxTemp;
            float ratePerMin = tempDrop / elapsedMin;
            
            Serial.print(elapsed / 1000);
            Serial.print(",");
            Serial.print(maxTemp, 1);
            Serial.print(",");
            Serial.println(ratePerMin, 1);
            
            g_lastLog = millis();
        }
        
        if (maxTemp < 40.0f) {
            float totalMin = elapsed / 60000.0f;
            float avgRate = (g_startTemp - maxTemp) / totalMin;
            
            Serial.print("\nAvg: ");
            Serial.print(avgRate, 1);
            Serial.print(" C/min | Time: ");
            Serial.print(totalMin, 1);
            Serial.println(" min");
            
            fan.set(false);
            g_testStarted = false;
            currentMode = MENU;
            tone(BUZZER_PIN, 600, 1000);
        }
    }
}

// ---- Main Control ----
void runControl() {
    if (profileRunning && !profileAborted) {
        int currentSecond = (millis() - runStartTime) / 1000;
        
        bool finished = false;
        float setpoint = profRunner.update(millis(), finished);
        int elapsed = profRunner.elapsedSec(millis());
        int remaining = profRunner.durationSec() - elapsed;
        
        if (finished) {
            profileDone = true;
            profileRunning = false;
            heater.reset();
            fan.set(true);
            tone(BUZZER_PIN, 600, 2000);
        } else {
            float maxTemp = max(sensors.tempFront(), sensors.tempBack());
            
            // Detect cooling mode entry
            if (setpoint < g_lastSetpoint - 0.5f) {
                if (!g_inCoolingMode) {
                    g_inCoolingMode = true;
                    g_coolingResetDone = false;
                    Serial.println("[CONTROL] Entering cooling mode");
                }
            }
            
            // Exit cooling mode when temp drops sufficiently
            if (g_inCoolingMode && maxTemp < setpoint - 3.0f) {
                g_inCoolingMode = false;
                Serial.println("[CONTROL] Exiting cooling mode");
            }
            
            g_lastSetpoint = setpoint;
            
            uint16_t coolingStart = profRunner.coolingStartSec();
            bool inCoolingPhase = (currentSecond >= coolingStart);
            
            // Heater control
            if (g_inCoolingMode && !g_coolingResetDone) {
                heater.reset();
                heater.setMaxOutput(0);
                g_coolingResetDone = true;
                Serial.println("[CONTROL] Heaters disabled for cooling");
            } else if (!g_inCoolingMode) {
                g_coolingResetDone = false;
                
                if (maxTemp > setpoint - 5.0f && !inCoolingPhase) {
                    heater.setMaxOutput(50);
                } else {
                    heater.setMaxOutput(90);
                }
                
                heater.control(heatActive, setpoint, sensors.tempFront(), sensors.tempBack());
            }
            
        // Fan control
if (!manualFanMode) {
    if ((g_inCoolingMode && maxTemp > 80.0f) || (inCoolingPhase && maxTemp > setpoint + 2.0f)) {
        fan.set(true);
        Serial.println("[FAN] Cooling activated");
    } else if (g_inCoolingMode && maxTemp > 60.0f) {
        fan.set(true);
        Serial.println("[FAN] Cooling mode - fan on");
    } else if (maxTemp < 50.0f) {
        fan.set(false);
        Serial.println("[FAN] Cool enough - fan off");
    }
}
            
            // Safety override
            if (maxTemp >= 80.0f && !fan.isOn() && !manualFanMode) {
                fan.set(true);
                Serial.println("[FAN] Safety override (>80C)");
            }
        }
        
        // Update profile display with condensed graph + data
        ui.showProfileRun(PROFILES[selectedProfile], setpoint, 
                          sensors.tempFront(), sensors.tempBack(),
                          heater.dutyFrontPct(), heater.dutyBackPct(),
                          elapsed, remaining, profileDone, profileAborted);
    }
    
    if (constRunning && !profileAborted) {
        int currentSecond = (millis() - runStartTime) / 1000;
        
        if (currentSecond >= constDuration) {
            profileDone = true;
            constRunning = false;
            heater.reset();
            fan.set(true);
            tone(BUZZER_PIN, 600, 2000);
        } else {
            heater.control(heatActive, constTemp, sensors.tempFront(), sensors.tempBack());
            float maxTemp = max(sensors.tempFront(), sensors.tempBack());
            
            if (currentSecond >= constDuration && maxTemp > constTemp + 2.0f) {
                if (!fan.isOn() && !manualFanMode) {
                    fan.set(true);
                    Serial.println("[FAN] Constant mode cooling");
                }
            }
        }
        
        // Update constant display (uses simple layout)
        int remaining = constDuration - currentSecond;
        ui.showRun(constTemp, sensors.tempFront(), sensors.tempBack(),
                   heater.dutyFrontPct(), heater.dutyBackPct(), CONST_RUN);
    }
    
    // Manual heater control for TEST_RUN mode
    if (currentMode == TEST_RUN) {
        // Use manual control by bypassing PID and directly setting a constant setpoint
        float manualSetpoint = (float)testPct * 2.0f;  // Convert 0-100% to 0-200°C range
        heater.control(heatSelection, manualSetpoint, sensors.tempFront(), sensors.tempBack());
    }
    
    if (currentMode == COOL_TEST) {
        runCoolingTest();
    }
}

// ---- Display Update for Non-Running Modes ----
void updateDisplay() {
    if (!profileRunning && !constRunning) {
        switch (currentMode) {
            case MENU:
                ui.showMenu(menuIndex, sensors.tempFront(), sensors.tempBack(), 
                           heatSelection, manualFanMode, manualFanState);
                break;
                
            case PROF_SETUP:
                ui.showProfileSetup(PROFILES[selectedProfile], 2);
                break;
                
            case CONST_SETUP:
                ui.showConstantSetup(constTemp, constDuration);
                break;
                
            case TEST_RUN:
                ui.showTest(testPct, sensors.tempFront(), sensors.tempBack(), heatSelection);
                break;
                
            case COOL_TEST:
                ui.showCoolTest(sensors.tempFront(), sensors.tempBack());
                break;
                
            default:
                break;
        }
    }
}

// ---- Setup & Loop ----
void setup() {
    Serial.begin(115200);
    delay(1500);
    Serial.println("Serial OK");
    Serial.println("Reflow Station Starting...");
    Serial.printf("ESP_ARDUINO_VERSION_MAJOR = %d\n", ESP_ARDUINO_VERSION_MAJOR);
    
    // Initialize all modules
    bool ok = fan.begin(FAN_PIN, true, 25000, 8);
    Serial.printf("[FAN] begin ok=%d\n", ok);

    // Fan sanity check: OFF -> ON -> OFF
    Serial.println("[FAN] sanity OFF-ON-OFF");
    fan.set(false); delay(400);
    fan.set(true);  delay(400);
    fan.set(false);

    // Initialize display using DisplayUI
    ui.begin(I2C_SDA, I2C_SCL);
    
    sensors.begin(THERM_FRONT, THERM_BACK);
    // Add these lines in setup() after sensors.begin():
    sensors.setFrontCal(-120);
    sensors.setBackCal(-120);
    // Let sensors stabilize
    for (int i = 0; i < 20; i++) {
        sensors.update();
        delay(50);
    }
    
    float avgTemp = (sensors.tempFront() + sensors.tempBack()) / 2.0f;
    if (avgTemp > 20.0f && avgTemp < 30.0f) {
        sensors.calibrateAtRoomTemp(23.5);
        Serial.println("Applied room temperature calibration");
    } else {
        Serial.print("Skipped calibration - current temp: ");
        Serial.print(avgTemp);
        Serial.println("C");
    }
    
    heater.begin(SSR_FRONT, SSR_BACK, 1000);
    encoder.begin(ENC_A, ENC_B, ENC_BTN);
    
    pinMode(BUZZER_PIN, OUTPUT);
    
    // Hot plate detection and warning
    float maxTemp = max(sensors.tempFront(), sensors.tempBack());
    if (maxTemp > 40.0f) {
        fan.set(true);
        
        Serial.println("=== HOT PLATES DETECTED ===");
        Serial.print("Temperature: ");
        Serial.print(maxTemp);
        Serial.println("C - Cooling fan activated!");
        
        // Use DisplayUI for warning message
        ui.clear();
        
        tone(BUZZER_PIN, 800, 100);
        delay(150);
        tone(BUZZER_PIN, 800, 100);
        delay(150);
        tone(BUZZER_PIN, 800, 100);
        
        delay(3000);
    }
    
    Serial.println("Initialization complete");
    ui.clear();
}

void loop() {
    sensors.update();
    handleEncoder();
    runControl();
    
    // Auto-cooling for hot plates in menu mode
    if (currentMode == MENU && !manualFanMode) {
        float maxTemp = max(sensors.tempFront(), sensors.tempBack());
        if (maxTemp < 35.0f && fan.isOn()) {
            fan.set(false);
        }
    }
    
    // Update display for non-running modes only
    static unsigned long lastDisplayUpdate = 0;
    if (millis() - lastDisplayUpdate > 100) {
        updateDisplay();
        lastDisplayUpdate = millis();
    }
}