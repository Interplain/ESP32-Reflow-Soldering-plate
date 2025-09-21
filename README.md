<<<<<<< HEAD
# ESP32 Reflow Soldering Station

A professional-grade dual-zone reflow soldering station with precise temperature control, customizable profiles, and an intuitive user interface.

![Project Status](https://img.shields.io/badge/status-active-brightgreen)
![Platform](https://img.shields.io/badge/platform-ESP32-blue)
![License](https://img.shields.io/badge/license-MIT-green)

## 🔥 Features

- **Dual-zone heating control** - Independent front and back plate temperature management
- **PID temperature control** - Precise heating with anti-windup protection
- **Multiple operation modes**:
  - **Profile Mode** - Automated reflow profiles for different solder types
  - **Constant Mode** - Manual temperature setting
  - **Test Mode** - Component and system testing
  - **Plates Mode** - Individual plate control
- **Real-time monitoring** - Live temperature display and control feedback
- **Rotary encoder interface** - Intuitive navigation with click and long-press support
- **PWM fan control** - Variable speed cooling fan
- **Safety features** - Temperature limits and controlled heating/cooling cycles

## 🛠️ Hardware Requirements

### Core Components
- **ESP32 Development Board** (any variant with sufficient GPIO)
- **SSD1306 OLED Display** (128x64, I2C)
- **Rotary Encoder** with push button
- **2x Solid State Relays (SSR)** for heater control
- **2x NTC Thermistors** (100kΩ @ 25°C, β=3950)
- **12V PC Fan** with PWM control
- **2x 6N137 Opto Isolators** (Optocoupler isolation Module GPIO19 for fan Control, GPIO18 for Tacho)
- **Heating Elements** (compatible with your soldering plates)

### Circuit Components
- **2x 100kΩ Resistors** (thermistor voltage dividers)
- **4.7kΩ Resistor** (fan transistor base)
- **12v to 7805 Regulator** (fan driver)
- **Pull-up resistors** for encoder (if needed)

## 📋 Pin Configuration

```cpp
// Display (I2C)
SDA: GPIO21
SCL: GPIO22
Address: 0x3C

// Heater Control (SSR)
Front Heater: GPIO (define in HeaterController)
Back Heater:  GPIO (define in HeaterController)

// Temperature Sensors (ADC)
Front Sensor: GPIO (define in SensorManager)
Back Sensor:  GPIO (define in SensorManager)

// Fan Control
Fan PWM: GPIO19
Fan TACHO: GPIO18 (Wired in Reverse Opto Isolator)

// User Interface
Encoder A: GPIO (define in InputEncoder)
Encoder B: GPIO (define in InputEncoder)
Button:    GPIO (define in InputEncoder)
```

## 🚀 Installation

### Prerequisites
- **Arduino IDE** or **PlatformIO**
- **ESP32 Board Package** installed
- Required libraries:
  - `Adafruit SSD1306`
  - `Adafruit GFX Library`
  - `Wire` (built-in)

### Setup Steps

1. **Clone or download** this repository
2. **Install dependencies** through Arduino Library Manager
3. **Configure pin assignments** in respective header files
4. **Calibrate temperature sensors** (optional - see Calibration section)
5. **Upload** to your ESP32
6. **Test safety systems** before connecting heaters

### Wiring Diagram

```
ESP32 Pin Connections:
├── Display (SSD1306)
│   ├── VCC → 3.3V
│   ├── GND → GND
│   ├── SDA → GPIO21
│   └── SCL → GPIO22
├── Heater SSRs
│   ├── Front SSR Control → GPIO (configurable)
│   └── Back SSR Control → GPIO (configurable)
├── Temperature Sensors
│   ├── Front NTC → ADC Pin + 100kΩ pullup
│   └── Back NTC → ADC Pin + 100kΩ pullup
├── Fan Control
│   └── GPIO19 → Opto Isolator IN GND/POS+, GND Out Isolation 12v/GND
│   └── GPIO18 → Opto Isolator, GND Isolation
└── Rotary Encoder
    ├── A → GPIO (configurable)
    ├── B → GPIO (configurable)
    └── Button → GPIO (configurable)
```

## 🎯 Usage

### Operation Modes

#### **Main Menu Navigation**
- **Rotate encoder** to select menu items
- **Short press** to enter selected mode
- **Long press** (900ms) for special functions

#### **Plates Mode**
- Control individual heating plates
- Toggle Front (F) and/or Back (B) plates
- Real-time temperature monitoring
- Manual on/off control

#### **Profile Mode**
- Automated reflow soldering profiles
- Pre-programmed temperature curves
- Automatic heating and cooling cycles
- Progress monitoring

#### **Constant Mode**
- Set and maintain specific temperature
- Both plates at same setpoint
- Manual temperature adjustment
- Continuous PID control

#### **Test Mode**
- Component testing and diagnostics
- Sensor verification
- Heater functionality check

### Display Information

Main Menu Display
```
┌─────────────────────────────┐
│ Reflow Station              │
│                             │
│ > Plates          F■ B□     │ ← Selected menu item
│   Profile         F:185C    │
│   Constant        B:182C    │
│   Test                      │
└─────────────────────────────┘
```
Icon Legend:

F■ - Front plate ACTIVE (filled box)
F□ - Front plate inactive (empty box)
B■ - Back plate ACTIVE (filled box)
B□ - Back plate inactive (empty box)
```
┌─────────────────────────────┐
│ Reflow Station    SP:200C   │
│ Front: 180C  85%  [████░░]  │
│ Back : 175C  90%  [█████░]  │
│ [██████████████████████]    │ ← Front heater bar
│ [██████████████████████]    │ ← Back heater bar
└─────────────────────────────┘
```

- **SP** - Current setpoint temperature
- **Temperature readings** - Live sensor data
- **Duty percentages** - PID output (0-100%)
- **Progress bars** - Visual heater activity

## ⚙️ Configuration

### PID Tuning
Modify PID gains in `HeaterController.h`:
```cpp
struct PIDGains {
    float P = 50.0f;    // Proportional gain
    float I = 0.5f;     // Integral gain  
    float D = 10.0f;    // Derivative gain
    float iMax = 50.0f; // Integral windup limit
};
```

### Temperature Calibration
1. **Using LUT** (recommended): Create `thermProf.h` with calibration table
2. **Using Beta model**: Adjust constants in `SensorManager.cpp`

### Safety Settings
- **Window time**: Adjust SSR switching period (default: suitable for most SSRs)
- **Temperature limits**: Modify maximum temperatures in code
- **Cooling band**: Set temperature offset for heater shutoff

## 📊 System Architecture

```
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│   User Input    │    │   Control Logic  │    │   Hardware      │
│                 │    │                  │    │                 │
│ ┌─────────────┐ │    │ ┌──────────────┐ │    │ ┌─────────────┐ │
│ │ Rotary      │ │◄──►│ │ State        │ │◄──►│ │ Heating     │ │
│ │ Encoder     │ │    │ │ Machine      │ │    │ │ Elements    │ │
│ └─────────────┘ │    │ └──────────────┘ │    │ └─────────────┘ │
│                 │    │ ┌──────────────┐ │    │ ┌─────────────┐ │
│ ┌─────────────┐ │    │ │ PID          │ │    │ │ Temperature │ │
│ │ OLED        │ │◄──►│ │ Controllers  │ │◄──►│ │ Sensors     │ │
│ │ Display     │ │    │ │ (Dual Zone)  │ │    │ └─────────────┘ │
│ └─────────────┘ │    │ └──────────────┘ │    │ ┌─────────────┐ │
└─────────────────┘    │ ┌──────────────┐ │    │ │ Cooling     │ │
                       │ │ Profile      │ │    │ │ Fan         │ │
                       │ │ Manager      │ │    │ └─────────────┘ │
                       │ └──────────────┘ │    └─────────────────┘
                       └──────────────────┘
```

### Key Components

- **`DisplayUI`** - OLED interface and menu system
- **`HeaterController`** - Dual-zone PID temperature control
- **`SensorManager`** - Temperature sensing with filtering
- **`InputEncoder`** - Rotary encoder with debouncing
- **`ProfileRunner`** - Automated reflow profile execution
- **Fan Control** - PWM-based cooling management

## ⚠️ Safety Considerations

### **Critical Safety Warnings**
- ⚡ **High temperature operation** - Heating elements can exceed 250°C
- ⚡ **Electrical safety** - Ensure proper SSR ratings and heat sinking
- ⚡ **Fire hazard** - Never leave unattended during operation
- ⚡ **Ventilation** - Use in well-ventilated area due to flux fumes

### **Recommended Safety Features**
- **Thermal cutoffs** on heating elements
- **Ground fault protection** on AC circuits
- **Temperature monitoring** with audible alarms
- **Emergency stop** button
- **Proper enclosure** with warning labels

### **Before First Use**
1. ✅ Verify all wiring connections
2. ✅ Test temperature sensors with known references
3. ✅ Confirm SSR operation at low temperatures
4. ✅ Check fan operation and airflow
5. ✅ Test emergency shutdown procedures

## 🔧 Troubleshooting

### **Temperature Reading Issues**
- Check thermistor connections and pullup resistors
- Verify ADC pin configuration
- Test with multimeter for proper resistance values

### **Heating Problems**
- Confirm SSR triggering with multimeter
- Check heater element continuity
- Verify power supply adequacy

### **Display/Interface Issues**
- Check I2C connections (SDA/SCL)
- Verify encoder wiring and pullup resistors
- Test with simple I2C scanner sketch

### **PID Oscillation**
- Reduce proportional gain
- Increase derivative term
- Check for noise in temperature readings

## 📈 Performance Specifications

- **Temperature Range**: 25°C to 250°C+ (limited by sensors/heaters)
- **Temperature Accuracy**: ±2°C (with proper calibration)
- **Control Resolution**: 0.1°C
- **Response Time**: < 30 seconds to setpoint
- **Thermal Stability**: ±1°C at steady state
- **Update Rate**: 10Hz control loop

## 🤝 Contributing

Contributions are welcome! Please:

1. **Fork** the repository
2. **Create** a feature branch
3. **Test** thoroughly (especially safety systems)
4. **Submit** a pull request with detailed description

### **Areas for Improvement**
- Additional reflow profiles
- Web interface for remote monitoring
- Data logging capabilities
- Advanced safety features
- Mobile app integration

## 📄 License

This project is licensed under the MIT License - see the LICENSE file for details.

## 🙏 Acknowledgments

- ESP32 community for excellent documentation
- Contributors to Arduino libraries used
- Safety guidance from professional reflow equipment manufacturers

---

**⚠️ DISCLAIMER**: This project involves high temperatures and electrical systems. Build and use at your own risk. The authors are not responsible for any damage, injury, or loss resulting from the use of this information.
=======

>>>>>>> ace7779 (Test commit after setup)
