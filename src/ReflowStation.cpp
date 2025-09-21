// ReflowStation.cpp - PlatformIO Version
#include <Arduino.h>
#include "Types.h"
#include "Profiles.h"
#include "ProfileRunner.h"
#include "SensorManager.h"
#include "HeaterController.h"
#include "FanController.h"
#include "InputEncoder.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ---- Display Setup ----
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDR 0x3C
#define I2C_SDA 21
#define I2C_SCL 22

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

// ---- Global Objects ----
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
SensorManager sensors;
HeaterController heater;
FanController fan;
ProfileRunner profRunner;
InputEncoder encoder;

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
int currentSecond = 0;

// ---- Control State Variables (moved to global scope) ----
float g_lastSetpoint = 0.0f;
bool g_inCoolingMode = false;
bool g_coolingResetDone = false;

// ---- Cooling Test Variables (moved to global scope) ----
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

// ---- Display Functions ----
void drawHeatBoxes(int x, int y) {
    display.setCursor(x, y);
    display.print("F");
    display.drawRect(x + 8, y, 8, 8, SSD1306_WHITE);
    if (heatSelection == HEAT_FRONT || heatSelection == HEAT_BOTH) {
        display.fillRect(x + 9, y + 1, 6, 6, SSD1306_WHITE);
    }
    
    display.setCursor(x + 20, y);
    display.print("B");
    display.drawRect(x + 28, y, 8, 8, SSD1306_WHITE);
    if (heatSelection == HEAT_BACK || heatSelection == HEAT_BOTH) {
        display.fillRect(x + 29, y + 1, 6, 6, SSD1306_WHITE);
    }
}

void renderMenu() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    display.setCursor(0, 0);
    display.print("Reflow Station");
    
    const char* menuItems[] = {"Plates", "Profile", "Constant", "Fan", "Test"};
    for (int i = 0; i < 5; i++) {
        int y = 12 + (i * 10);
        if (i == menuIndex) {
            display.fillTriangle(0, y-3, 0, y+3, 5, y, SSD1306_WHITE);
        }
        display.setCursor(10, y);
        display.print(menuItems[i]);
    }
    
    drawHeatBoxes(70, 12);
    
    display.setCursor(70, 24);
    if (manualFanMode) {
        display.print("Fan:");
        display.print(manualFanState ? "ON" : "OFF");
    } else {
        display.print("Auto");
    }
    
    display.setCursor(70, 38);
    display.print("F:");
    display.print((int)sensors.tempFront());
    display.print("C");
    
    display.setCursor(70, 50);
    display.print("B:");
    display.print((int)sensors.tempBack());
    display.print("C");
    
    display.display();
}

void renderProfileSetup() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    display.setCursor(0, 0);
    display.print("Profile Setup");
    
    display.setCursor(0, 20);
    display.print("Pick profile");
    
    display.setCursor(10, 32);
    display.print(PROFILES[selectedProfile].name);
    
    display.setCursor(0, 56);
    display.print("Start  Long=Back");
    
    display.display();
}

void renderConstSetup() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    display.setCursor(0, 0);
    display.print("Constant Setup");
    
    display.setCursor(0, 24);
    display.print("Setpoint: ");
    display.print(constTemp);
    display.print("C");
    
    display.setCursor(0, 56);
    display.print("Start  Long=Back");
    
    display.display();
}

void renderProfileRun() {
    if (profileDone || profileAborted) {
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(30, 25);
        display.print(profileDone ? "COMPLETE" : "ABORTED");
        display.setCursor(20, 45);
        display.print("Press to continue");
        display.display();
        return;
    }
    
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    display.setCursor(0, 0);
    display.print(PROFILES[selectedProfile].name);
    
    int remaining = profRunner.durationSec() - currentSecond;
    display.setCursor(90, 0);
    display.print(remaining);
    display.print("s");
    
    bool finished;
    float sp = profRunner.update(millis(), finished);
    
    display.setCursor(0, 15);
    display.print("SP: ");
    display.print((int)sp);
    display.print("C");
    
    display.setCursor(0, 27);
    display.print("F: ");
    display.print((int)sensors.tempFront());
    display.print("C  ");
    display.print(heater.dutyFrontPct());
    display.print("%");
    
    display.setCursor(0, 39);
    display.print("B: ");
    display.print((int)sensors.tempBack());
    display.print("C  ");
    display.print(heater.dutyBackPct());
    display.print("%");
    
    drawHeatBoxes(89, 13);
    
    display.setCursor(0, 55);
    display.print("Press=Abort");
    
    display.display();
}

void renderConstRun() {
    if (profileDone || profileAborted) {
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(30, 25);
        display.print("COMPLETE");
        display.setCursor(20, 45);
        display.print("Press to continue");
        display.display();
        return;
    }
    
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    display.setCursor(0, 0);
    display.print("Constant Run");
    
    int remaining = constDuration - currentSecond;
    display.setCursor(90, 0);
    display.print(remaining);
    display.print("s");
    
    display.setCursor(0, 15);
    display.print("SP: ");
    display.print(constTemp);
    display.print("C");
    
    display.setCursor(0, 27);
    display.print("F: ");
    display.print((int)sensors.tempFront());
    display.print("C  ");
    display.print(heater.dutyFrontPct());
    display.print("%");
    
    display.setCursor(0, 39);
    display.print("B: ");
    display.print((int)sensors.tempBack());
    display.print("C  ");
    display.print(heater.dutyBackPct());
    display.print("%");
    
    drawHeatBoxes(89, 13);
    
    display.setCursor(0, 55);
    display.print("Press=Abort");
    
    display.display();
}

void renderTest() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    display.setCursor(0, 0);
    display.print("Test Heaters");
    
    display.setCursor(0, 15);
    display.print("Enc changes duty");
    
    display.setCursor(15, 30);
    display.print("Duty: ");
    display.print(testPct);
    display.print("%");
    
    display.setCursor(0, 42);
    display.print("F:");
    display.print((int)sensors.tempFront());
    display.print("C  B:");
    display.print((int)sensors.tempBack());
    display.print("C");
    
    display.setCursor(0, 56);
    display.print("Hold=CoolTest");
    
    display.display();
}

void renderCoolTest() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    display.setCursor(0, 0);
    display.print("Cooling Test");
    
    display.setCursor(0, 20);
    display.print("F:");
    display.print((int)sensors.tempFront());
    display.print("C");
    
    display.setCursor(0, 32);
    display.print("B:");
    display.print((int)sensors.tempBack());
    display.print("C");
    
    display.setCursor(0, 50);
    display.print("Check Serial");
    
    display.display();
}

// ---- Mode Control Functions ----
void startProfile() {
    profRunner.begin(PROFILES[selectedProfile]);
    currentMode = PROFILE_RUN;
    profileRunning = true;
    profileDone = false;
    profileAborted = false;
    runStartTime = millis();
    currentSecond = 0;
    
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
    currentSecond = 0;
    
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
        currentSecond = (millis() - runStartTime) / 1000;
        
        bool finished = false;
        float setpoint = profRunner.update(millis(), finished);
        
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
                    if (!fan.isOn()) {
                        fan.set(true);
                        Serial.println("[FAN] Cooling activated");
                    }
                } else if (g_inCoolingMode && maxTemp > 60.0f) {
                    if (!fan.isOn()) {
                        fan.set(true);
                        Serial.println("[FAN] Cooling mode - fan on");
                    }
                } else if (maxTemp < 50.0f && fan.isOn()) {
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
    }
    
    if (constRunning && !profileAborted) {
        currentSecond = (millis() - runStartTime) / 1000;
        
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
    }
    
    if (currentMode == TEST_RUN) {
        heater.reset();
    }
    
    if (currentMode == COOL_TEST) {
        runCoolingTest();
    }
}

// ---- Setup & Loop ----
void setup() {
    Serial.begin(115200);
    delay(1500);                     // give the monitor time to attach
    Serial.println("Serial OK");
    Serial.println("Reflow Station Starting...");
    Serial.printf("ESP_ARDUINO_VERSION_MAJOR = %d\n", ESP_ARDUINO_VERSION_MAJOR);
    bool ok = fan.begin(FAN_PIN, /*activeLow=*/true, 25000, 8);
    Serial.printf("[FAN] begin ok=%d\n", ok);

// sanity: OFF -> ON -> OFF
Serial.println("[FAN] sanity OFF-ON-OFF");
fan.set(false); delay(400);
fan.set(true);  delay(400);
fan.set(false);



    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(400000);
    
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        Serial.println("SSD1306 allocation failed");
        while(1);
    }
    
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print("Initializing...");
    display.display();
    
    sensors.begin(THERM_FRONT, THERM_BACK);
    
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
    fan.begin(FAN_PIN, true, 25000, 8);
    fan.set(false);
    encoder.begin(ENC_A, ENC_B, ENC_BTN);
    
    pinMode(BUZZER_PIN, OUTPUT);
    
    float maxTemp = max(sensors.tempFront(), sensors.tempBack());
    if (maxTemp > 40.0f) {
        fan.set(true);
        
        Serial.println("=== HOT PLATES DETECTED ===");
        Serial.print("Temperature: ");
        Serial.print(maxTemp);
        Serial.println("C - Cooling fan activated!");
        
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(10, 10);
        display.print("!!! WARNING !!!");
        display.setCursor(0, 25);
        display.print("Hot plates detected");
        display.setCursor(0, 35);
        display.print("Temp: ");
        display.print((int)maxTemp);
        display.print("C");
        display.setCursor(0, 50);
        display.print("Auto-cooling active");
        display.display();
        
        tone(BUZZER_PIN, 800, 100);
        delay(150);
        tone(BUZZER_PIN, 800, 100);
        delay(150);
        tone(BUZZER_PIN, 800, 100);
        
        delay(3000);
    }
    
    Serial.println("Initialization complete");
    
    delay(1000);
    display.clearDisplay();
    display.display();
}

void loop() {
    sensors.update();
    handleEncoder();
    runControl();
    
    if (currentMode == MENU && !manualFanMode) {
        float maxTemp = max(sensors.tempFront(), sensors.tempBack());
        if (maxTemp < 35.0f && fan.isOn()) {
            fan.set(false);
        }
    }
    
    static unsigned long lastDisplayUpdate = 0;
    if (millis() - lastDisplayUpdate > 100) {
        switch (currentMode) {
            case MENU:        renderMenu();        break;
            case PROF_SETUP:  renderProfileSetup(); break;
            case PROFILE_RUN: renderProfileRun();   break;
            case CONST_SETUP: renderConstSetup();   break;
            case CONST_RUN:   renderConstRun();     break;
            case TEST_RUN:    renderTest();         break;
            case COOL_TEST:   renderCoolTest();     break;
        }
        lastDisplayUpdate = millis();
    }
}