# Pilot Information System
## Aircraft Instrumentation - Subsystem 4

**Document Version:** 1.0
**Date:** 2025-01-18
**Project:** N2-Arduino Aircraft Instrumentation
**System:** Pilot Information (CAN ID Range: 0x400-0x4FF)

---

## Executive Summary

The Pilot Information System provides a rich graphical user interface for displaying all flight and engine data, configuring system parameters, and managing alerts. Running on a Raspberry Pi 4 with a 7" touchscreen, this system serves as the primary pilot interface with real-time graphs, trend monitoring, and intuitive configuration.

**Platform:** Raspberry Pi 4 Model B (2GB RAM)
**Primary Function:** Display interface and configuration
**Display:** Official 7" touchscreen (800×480) or HDMI
**CAN ID Range:** 0x400-0x4FF (256 message IDs)

**Key Benefits:**
- Large, graphical display (vs 16×2 LCD)
- Touch interface for easy configuration
- Real-time trend graphs (EGT, CHT, altitude)
- Multi-page layout (8+ screens)
- Night mode for night flying
- Alert management and logging
- Export data to USB drive

---

## System Overview

### Hardware Platform

**Board:** Raspberry Pi 4 Model B (2GB RAM)
**SoC:** BCM2711 (4× ARM Cortex-A72 @ 1.5GHz)
**RAM:** 2GB LPDDR4-3200
**Storage:** MicroSD card (16GB minimum)
**Display:** Official 7" touchscreen (800×480, DSI interface) or HDMI monitor

### Key Components

- **Display Options:**
  - **Primary:** Official 7" touchscreen (DSI connector, 800×480)
  - **Alternative:** HDMI monitor/display
  - Capacitive touch (10-point multitouch)
  - On-screen keyboard for text input

- **CAN Interface:**
  - **Option 1:** MCP2515 CAN module (SPI, same as other subsystems)
  - **Option 2:** USB-CAN adapter (easier installation, powered USB)

- **Input Devices:**
  - Touchscreen (primary input)
  - USB keyboard (optional, for development/configuration)
  - USB mouse (optional, for development)

- **Power:**
  - Official USB-C power supply (5V/3A recommended)
  - Power button with safe shutdown circuit
  - Auto-start on power application

### GPIO Pin Assignments (if using MCP2515)

**SPI (MCP2515 CAN):**
- GPIO 10 (MOSI): SPI data to CAN controller
- GPIO 9 (MISO): SPI data from CAN controller
- GPIO 11 (SCLK): SPI clock
- GPIO 8 (CE0): SPI chip select
- GPIO 25: CAN interrupt

**Display:**
- DSI connector (dedicated, no GPIO needed)
- I2C for touch controller (on DSI connector)

**Status:**
- GPIO 17: Status LED (optional)
- GPIO 23: Power button (safe shutdown)

---

## Processing Responsibilities

### Primary Functions

**1. Data Reception and Storage**
- Receive all CAN messages (0x100-0x3FF ranges)
- Store in RAM circular buffers (last 60 minutes)
- Update rate: Process messages as received (up to 100 msg/s)
- Data validation: Range checking, timeout detection

**2. Real-Time Display**
- Update screens at 5Hz (200ms refresh)
- Smooth graphical updates (no flicker)
- Multi-page layout with swipe navigation
- Alert overlays (always visible when active)

**3. Historical Trending**
- Store last 60 minutes of data in RAM (~100MB)
- Line graphs for EGT, CHT, altitude, etc.
- Zoom controls (15/30/60 minute views)
- Markers for events (takeoff, landing, alerts)

**4. Configuration Management**
- QNH setting with numeric keypad
- Date/time configuration (sync to GPS or manual)
- Units selection (metric/imperial, °C/°F, hPa/inHg)
- Alert threshold customization
- Display brightness and night mode

**5. Alert Management**
- Visual alerts (red banner across top of screen)
- Audio alerts (beep/tone, configurable)
- Alert history log (stored in file)
- Acknowledge/silence functionality
- Priority-based display (critical > warning)

**6. Data Export**
- Export configuration to USB drive (JSON)
- Export recent flight data (CSV, last session)
- Screenshot capture (for documentation)
- Firmware update from USB drive

**7. CAN Transmission**
- Configuration messages to other subsystems
- QNH to Instrument System
- Alert limits to Engine Monitor
- Time sync to all systems (1/minute)

---

## Screen Layout Design

### Screen 1: Engine Monitor

**Purpose:** Real-time engine parameter display

**Layout:**
```
┌─────────────────────────────────────────────────────────────┐
│ ENGINE MONITOR                    14:32:15 UTC    2450 RPM  │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  EGT 1: 685°C    CHT 1: 195°C         Oil Temp: 95°C        │
│  EGT 2: 690°C    CHT 2: 198°C         Oil Pressure: 55 PSI  │
│  EGT 3: 688°C    CHT 3: 196°C         Manifold: 25.2 inHg   │
│  EGT 4: 692°C    CHT 4: 199°C         Engine Hours: 124.3   │
│                                                               │
│  [========================================] EGT Bar Chart    │
│  [========================================] CHT Bar Chart    │
│                                                               │
│  ⚠ CHT 2 approaching warning limit (198°C / 200°C warn)     │
│                                                               │
│  ← Prev Screen                            Next Screen →      │
└─────────────────────────────────────────────────────────────┘
```

**Elements:**
- Large, readable numbers (critical for quick scan)
- Color coding: Green (normal), Yellow (warn), Red (critical)
- Bar charts for cylinder comparison
- Alert text at bottom (if active)
- Navigation arrows (or swipe gestures)

### Screen 2: Flight Instruments

**Purpose:** Altitude, attitude, heading, speed

**Layout:**
```
┌─────────────────────────────────────────────────────────────┐
│ FLIGHT INSTRUMENTS                                5,420 ft  │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│     ┌─────────┐              ┌─────────┐                    │
│     │ Artif.  │              │ Heading │                    │
│     │ Horizon │              │  Ind.   │                    │
│     │         │              │         │                    │
│     │ +5° ∧ │              │   285°  │                    │
│     │    <-2° │              │    N    │                    │
│     └─────────┘              └─────────┘                    │
│                                                               │
│  Altitude: 5,420 ft          Vertical Speed: +250 ft/min    │
│  QNH: 1013 hPa  [Edit]       GPS Alt: 5,445 ft              │
│                                                               │
│  Fuel: 45%  [████████░░░░░]  Battery: 13.8V / 5.2A          │
│                                                               │
│  ← Prev Screen                            Next Screen →      │
└─────────────────────────────────────────────────────────────┘
```

**Elements:**
- Graphical artificial horizon (pitch/roll)
- Heading indicator (compass rose)
- Digital readouts for altitude, V/S
- QNH setting button (tap to edit)
- Fuel and battery gauges

### Screen 3: Navigation

**Purpose:** GPS position, track, speed

**Layout:**
```
┌─────────────────────────────────────────────────────────────┐
│ NAVIGATION                                      GS: 95 kt   │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  Position:      52.5235° N, 13.4115° E                      │
│  Ground Speed:  95 knots                                     │
│  Track:         285° (W-NW)                                  │
│  GPS Fix:       3D, 12 satellites (Good)                    │
│  GPS Altitude:  5,445 ft                                     │
│                                                               │
│  ┌───────────────────────────────────┐                      │
│  │                                     │                      │
│  │    [Map display - future]          │                      │
│  │                                     │                      │
│  │                                     │                      │
│  └───────────────────────────────────┘                      │
│                                                               │
│  Time: 14:32:15 UTC                                          │
│                                                               │
│  ← Prev Screen                            Next Screen →      │
└─────────────────────────────────────────────────────────────┘
```

**Elements:**
- GPS coordinates (lat/lon)
- Ground speed and track
- Satellite count and fix quality
- Reserved space for future map display
- UTC time from GPS

### Screen 4: Temperature Trends

**Purpose:** Historical EGT/CHT graphs

**Layout:**
```
┌─────────────────────────────────────────────────────────────┐
│ TEMPERATURE TRENDS                          Last 60 minutes │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  EGT (°C)                                                    │
│  900 ┤                                                       │
│  800 ┤                  ╱────────────                        │
│  700 ┤          ╱──────╯                                     │
│  600 ┤  ───────╯                                             │
│  500 ┤                                                       │
│      └────────────────────────────────────────────────→ Time │
│       [15min] [30min] [60min] ← Zoom                        │
│                                                               │
│  CHT (°C)                                                    │
│  250 ┤                                                       │
│  200 ┤              ╱────────────                            │
│  150 ┤      ╱──────╯                                         │
│  100 ┤  ───╯                                                 │
│   50 ┤                                                       │
│      └────────────────────────────────────────────────→ Time │
│                                                               │
│  ← Prev Screen                            Next Screen →      │
└─────────────────────────────────────────────────────────────┘
```

**Elements:**
- Line graphs for all 4 EGT traces
- Line graphs for all 4 CHT traces
- Zoom buttons (15/30/60 minute windows)
- Event markers (T/O, cruise, landing)
- Scrollable (view older data)

### Screen 5: Settings

**Purpose:** System configuration

**Layout:**
```
┌─────────────────────────────────────────────────────────────┐
│ SETTINGS                                                     │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  ┌─ Display ───────────────────────────────────────┐        │
│  │  Brightness: [======░░░░] 60%                   │        │
│  │  Night Mode: [ OFF ] [ ON ]                     │        │
│  └─────────────────────────────────────────────────┘        │
│                                                               │
│  ┌─ Units ─────────────────────────────────────────┐        │
│  │  Temperature:  [  °C  ] [ °F ]                  │        │
│  │  Altitude:     [  ft  ] [  m ]                  │        │
│  │  Pressure:     [ hPa  ] [ inHg ]                │        │
│  │  Speed:        [ knots ] [ km/h ]               │        │
│  └─────────────────────────────────────────────────┘        │
│                                                               │
│  ┌─ Alerts ────────────────────────────────────────┐        │
│  │  EGT Warning:   750°C   [Edit]                  │        │
│  │  EGT Critical:  800°C   [Edit]                  │        │
│  │  CHT Warning:   200°C   [Edit]                  │        │
│  │  CHT Critical:  230°C   [Edit]                  │        │
│  │  Audio Alerts:  [ OFF ] [ ON ]                  │        │
│  └─────────────────────────────────────────────────┘        │
│                                                               │
│  ← Prev Screen                [Save Settings]               │
└─────────────────────────────────────────────────────────────┘
```

**Elements:**
- Slider controls (brightness)
- Toggle buttons (night mode, units)
- Edit buttons (numeric input for thresholds)
- Save button (transmits config via CAN)

### Screen 6: System Status

**Purpose:** Subsystem health and diagnostics

**Layout:**
```
┌─────────────────────────────────────────────────────────────┐
│ SYSTEM STATUS                                                │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  Subsystem           Status    Last Msg    Uptime           │
│  ─────────────────────────────────────────────────────────  │
│  Engine Monitor      ●  OK     0.1s ago    125 min          │
│  Instrument System   ●  OK     0.2s ago    125 min          │
│  Data Logger         ●  OK     1.0s ago    125 min          │
│                                 (SD: 78% free, 24 files)    │
│  Pilot Info (self)   ●  OK     -           125 min          │
│                                                               │
│  CAN Bus Utilization: 2.3% (93 msg/s)                       │
│  CAN Errors: 0 (good)                                        │
│                                                               │
│  ┌─ Recent Alerts ──────────────────────────────────┐       │
│  │  14:28:15  CHT 2 warning (200°C)  [Acknowledged] │       │
│  │  14:15:03  GPS fix lost           [Cleared]      │       │
│  │  13:42:10  Engine start detected                 │       │
│  └───────────────────────────────────────────────────┘       │
│                                                               │
│  ← Prev Screen                [Export Logs to USB]          │
└─────────────────────────────────────────────────────────────┘
```

**Elements:**
- Subsystem status (green dot = OK, red = offline, yellow = warning)
- Last message time (detect timeouts)
- Uptime for each subsystem
- CAN bus health metrics
- Recent alert history
- Export button

### Additional Screens (Future)

- **Screen 7:** Fuel Management (fuel flow, endurance, range)
- **Screen 8:** Engine Analysis (leaning, power settings)
- **Screen 9:** Maintenance (engine hours, next service, checklist)
- **Screen 10:** Data Export/Import

---

## CAN Bus Protocol

### Transmitted Messages

| Message ID | Name | Frequency | Data Layout |
|------------|------|-----------|-------------|
| 0x400 | System Config | On change | Byte 0-1: QNH (hPa×10), 2: Units (0=metric, 1=imperial), 3-7: Reserved |
| 0x401 | Time Sync | 1/minute | Byte 0-3: Unix timestamp (seconds since epoch) |
| 0x402 | Alert Config | On change | Byte 0: Alert ID, 1-2: Warning threshold, 3-4: Critical threshold |
| 0x403 | Alert Acknowledge | On event | Byte 0: Alert ID being acknowledged, 1-7: Reserved |
| 0x40F | Heartbeat | 1Hz | Byte 0: Node ID (0x04), 1: Uptime (minutes), 2-7: Reserved |

### Received Messages

**All messages from other subsystems:**
- 0x100-0x1FF: Engine Monitor data
- 0x200-0x2FF: Instrument System data
- 0x300-0x3FF: Data Logger status

**Processing:**
- Parse and store in RAM buffers
- Update display (5Hz refresh)
- Check for alerts (critical values)
- Log timeouts (no message for >3× expected interval)

---

## Software Architecture

### Operating System

**Raspberry Pi OS (with Desktop):**
- Full desktop environment (for GUI)
- Wayland or X11 display server
- Boot to GUI (no login prompt)
- Auto-start application on boot

**Lite Alternative (headless + GUI app):**
- Raspberry Pi OS Lite (minimal)
- Start Qt app directly (no desktop environment)
- Faster boot, lower memory usage
- Recommended for production

### Software Stack

**Programming Language:** Python 3.9+

**GUI Framework:** PyQt5 (Qt for Python)
- Cross-platform (develop on PC, deploy on RPi)
- Rich widget library (buttons, sliders, graphs)
- Touch-friendly controls
- Hardware-accelerated rendering (OpenGL)

**Key Libraries:**
- `PyQt5`: GUI framework
- `pyqtgraph`: Real-time plotting (EGT/CHT trends)
- `python-can`: CAN bus interface
- `numpy`: Data processing and buffering
- `json`: Configuration file handling

### Application Structure

```python
#!/usr/bin/env python3
"""
Pilot Information System - Main Application
"""

from PyQt5.QtWidgets import QApplication, QMainWindow, QStackedWidget
from PyQt5.QtCore import QTimer
import can
import numpy as np
from datetime import datetime, timedelta

class PilotInfoSystem(QMainWindow):
    def __init__(self):
        super().__init__()

        # Initialize CAN bus
        self.bus = can.interface.Bus(
            bustype='socketcan',
            channel='can0',
            bitrate=500000
        )

        # Data buffers (60 minutes @ 10Hz = 36000 samples per parameter)
        self.buffer_size = 36000
        self.timestamps = np.zeros(self.buffer_size)
        self.egt = np.zeros((4, self.buffer_size))  # 4 EGT channels
        self.cht = np.zeros((4, self.buffer_size))  # 4 CHT channels
        self.altitude = np.zeros(self.buffer_size)
        self.rpm = np.zeros(self.buffer_size)
        # ... more buffers for other parameters

        self.buffer_index = 0

        # Configuration
        self.config = {
            'qnh': 1013.25,
            'units_temp': 'C',
            'units_altitude': 'ft',
            'brightness': 60,
            'night_mode': False,
            'alert_egt_warn': 750,
            'alert_egt_crit': 800,
            'alert_cht_warn': 200,
            'alert_cht_crit': 230
        }

        # Active alerts
        self.alerts = []

        # Initialize UI
        self.init_ui()

        # Timers
        self.can_timer = QTimer()
        self.can_timer.timeout.connect(self.read_can)
        self.can_timer.start(10)  # Read CAN every 10ms

        self.display_timer = QTimer()
        self.display_timer.timeout.connect(self.update_display)
        self.display_timer.start(200)  # Update display at 5Hz

    def init_ui(self):
        """Initialize user interface"""
        self.setWindowTitle('N2 Aircraft Instrumentation')
        self.setGeometry(0, 0, 800, 480)

        # Create stacked widget for multiple screens
        self.screens = QStackedWidget()

        # Add screens
        from screens import (
            EngineMonitorScreen,
            FlightInstrumentsScreen,
            NavigationScreen,
            TrendScreen,
            SettingsScreen,
            StatusScreen
        )

        self.screen_engine = EngineMonitorScreen(self)
        self.screen_instruments = FlightInstrumentsScreen(self)
        self.screen_navigation = NavigationScreen(self)
        self.screen_trends = TrendScreen(self)
        self.screen_settings = SettingsScreen(self)
        self.screen_status = StatusScreen(self)

        self.screens.addWidget(self.screen_engine)
        self.screens.addWidget(self.screen_instruments)
        self.screens.addWidget(self.screen_navigation)
        self.screens.addWidget(self.screen_trends)
        self.screens.addWidget(self.screen_settings)
        self.screens.addWidget(self.screen_status)

        self.setCentralWidget(self.screens)

        # Touch gestures for screen switching
        self.installEventFilter(self)

        # Fullscreen mode
        self.showFullScreen()

    def read_can(self):
        """Read CAN messages (called every 10ms)"""
        msg = self.bus.recv(timeout=0)  # Non-blocking

        if msg:
            self.process_can_message(msg)

    def process_can_message(self, msg):
        """Process received CAN message and update buffers"""
        timestamp = datetime.now()

        if msg.arbitration_id == 0x100:  # EGT Temperatures
            if len(msg.data) == 8:
                self.egt[0][self.buffer_index] = int.from_bytes(msg.data[0:2], 'little') / 10
                self.egt[1][self.buffer_index] = int.from_bytes(msg.data[2:4], 'little') / 10
                self.egt[2][self.buffer_index] = int.from_bytes(msg.data[4:6], 'little') / 10
                self.egt[3][self.buffer_index] = int.from_bytes(msg.data[6:8], 'little') / 10

                # Check for alerts
                self.check_egt_alerts()

        elif msg.arbitration_id == 0x101:  # CHT Temperatures
            if len(msg.data) == 8:
                self.cht[0][self.buffer_index] = int.from_bytes(msg.data[0:2], 'little') / 10
                self.cht[1][self.buffer_index] = int.from_bytes(msg.data[2:4], 'little') / 10
                self.cht[2][self.buffer_index] = int.from_bytes(msg.data[4:6], 'little') / 10
                self.cht[3][self.buffer_index] = int.from_bytes(msg.data[6:8], 'little') / 10

                # Check for alerts
                self.check_cht_alerts()

        # ... process other message IDs ...

        # Update timestamp and increment circular buffer index
        self.timestamps[self.buffer_index] = timestamp.timestamp()
        self.buffer_index = (self.buffer_index + 1) % self.buffer_size

    def check_egt_alerts(self):
        """Check EGT values against thresholds"""
        for i in range(4):
            egt_value = self.egt[i][self.buffer_index - 1]

            if egt_value >= self.config['alert_egt_crit']:
                self.raise_alert(f'EGT {i+1} CRITICAL', 'critical', egt_value)
            elif egt_value >= self.config['alert_egt_warn']:
                self.raise_alert(f'EGT {i+1} Warning', 'warning', egt_value)

    def raise_alert(self, message, severity, value):
        """Raise an alert (visual + audio)"""
        alert = {
            'time': datetime.now(),
            'message': message,
            'severity': severity,
            'value': value,
            'acknowledged': False
        }

        # Add to alert list if not already present
        if not any(a['message'] == message and not a['acknowledged'] for a in self.alerts):
            self.alerts.append(alert)

            # Audio alert (if enabled)
            if self.config.get('audio_alerts', True):
                self.play_alert_sound(severity)

    def update_display(self):
        """Update current screen (called at 5Hz)"""
        current_screen = self.screens.currentWidget()
        if hasattr(current_screen, 'update_data'):
            current_screen.update_data()

    def send_config_to_subsystems(self):
        """Transmit configuration via CAN"""
        # QNH to Instrument System (0x400)
        qnh_value = int(self.config['qnh'] * 10)
        units = 1 if self.config['units_temp'] == 'F' else 0

        msg = can.Message(
            arbitration_id=0x400,
            data=[
                qnh_value & 0xFF,
                (qnh_value >> 8) & 0xFF,
                units,
                0, 0, 0, 0, 0
            ],
            is_extended_id=False
        )

        self.bus.send(msg)

if __name__ == '__main__':
    app = QApplication([])
    window = PilotInfoSystem()
    app.exec_()
```

---

## Power Budget

### Power Consumption

**Idle (display off):**
- RPi 4: 400mA @ 5V = 2000mW
- MCP2515 CAN: 5mA @ 5V = 25mW
- **Total Idle: 405mA @ 5V = 2025mW (~2W)**

**Active (display on, normal brightness):**
- RPi 4: 600mA @ 5V = 3000mW
- 7" Display: 400mA @ 5V = 2000mW
- MCP2515 CAN: 5mA @ 5V = 25mW
- **Total Active: 1005mA @ 5V = 5025mW (~5W)**

**Peak (display max brightness, high CPU load):**
- RPi 4: 700mA @ 5V = 3500mW
- 7" Display: 450mA @ 5V = 2250mW
- **Total Peak: 1155mA @ 5V = 5775mW (~5.8W)**

### From 12V Aircraft Bus
- With 95% efficient regulator: ~506mA @ 12V (active)
- Acceptable for aircraft electrical system
- Dedicated 5V/3A USB-C supply recommended

### Power Saving Features

**Auto-dim:**
- Reduce brightness after 5 minutes of inactivity
- Save ~30% display power (1.5W)

**Screen off:**
- Turn off display after 15 minutes (configurable)
- Retain data processing and CAN communication
- Save 2W

**CPU Governor:**
- Use `powersave` or `ondemand` governor
- Scale CPU frequency based on load
- Save ~500mW during low activity

---

## Development Plan

### Phase 1: Hardware Setup (Weeks 1-2)

**Week 1: Assembly**
- Set up Raspberry Pi 4
- Connect 7" touchscreen (DSI cable)
- Install MCP2515 CAN module or USB-CAN adapter
- Test touchscreen calibration

**Week 2: Software Setup**
- Install Raspberry Pi OS
- Install PyQt5: `sudo apt install python3-pyqt5`
- Install python-can: `pip3 install python-can`
- Install pyqtgraph: `pip3 install pyqtgraph`
- Configure CAN interface (same as Data Logger)
- Test basic GUI app

**Deliverables:**
- RPi 4 with touchscreen operational
- CAN interface receiving messages
- Basic PyQt5 app running

### Phase 2: GUI Development (Weeks 3-6)

**Week 3: Basic Screens**
- Create main window with screen switching
- Implement Engine Monitor screen
- Implement Flight Instruments screen
- Touch navigation (swipe gestures)

**Week 4: Advanced Screens**
- Implement Navigation screen
- Implement Trends screen (with pyqtgraph)
- Test real-time graphing performance

**Week 5: Configuration**
- Implement Settings screen
- Numeric keypad for QNH entry
- Unit conversion logic
- Save/load configuration (JSON file)

**Week 6: Alerts and Status**
- Implement alert detection and display
- Alert overlay (always visible)
- Audio alerts (system beep)
- Implement System Status screen

**Deliverables:**
- All 6 screens functional
- Navigation working (touch or keyboard)
- Configuration persists across reboots
- Alerts display and acknowledge correctly

### Phase 3: Integration and Polish (Weeks 7-8)

**Week 7: CAN Integration**
- Receive all CAN message types
- Parse and buffer data correctly
- Transmit configuration messages
- Test with real data from other subsystems

**Week 8: Polish and Optimization**
- Optimize rendering performance
- Implement night mode
- Add brightness control
- Auto-start on boot (systemd service)
- Full-screen mode, hide cursor

**Deliverables:**
- Complete system integrated with CAN bus
- Smooth performance (no lag)
- Professional appearance

### Testing Milestones

**Display Performance:**
- 5Hz refresh rate achieved
- No visible lag or stutter
- Touch response <100ms

**Data Accuracy:**
- All CAN messages parsed correctly
- Values match source (Engine Monitor, Instruments)
- Graphs update smoothly

**Configuration:**
- QNH setting transmitted to Instruments
- Alert thresholds transmitted to Engine Monitor
- Settings persist across reboots

**Reliability:**
- 24-hour continuous operation without crash
- Memory usage stable (no leaks)
- Auto-restart on failure

---

## Night Mode Implementation

### Color Scheme

**Day Mode:**
- Background: White (#FFFFFF)
- Text: Black (#000000)
- Normal values: Dark blue (#000080)
- Warning: Orange (#FF8C00)
- Critical: Red (#FF0000)

**Night Mode:**
- Background: Black (#000000)
- Text: Dim red (#800000)
- Normal values: Red (#FF0000)
- Warning: Bright red (#FF4500)
- Critical: Flashing red (#FF0000)
- Reduced brightness (20-30%)

**Automatic Switching:**
- Ambient light sensor (future enhancement)
- Manual toggle in Settings
- Time-based (if local time known, sunset→night mode)

---

## Configuration File Format

**File:** `/home/pi/.n2-config.json`

```json
{
  "version": "1.0",
  "display": {
    "brightness": 60,
    "night_mode": false,
    "auto_dim_timeout": 300,
    "screen_off_timeout": 900
  },
  "units": {
    "temperature": "C",
    "altitude": "ft",
    "pressure": "hPa",
    "speed": "knots"
  },
  "altimeter": {
    "qnh": 1013.25,
    "auto_sync_gps": true
  },
  "alerts": {
    "egt_warn": 750,
    "egt_crit": 800,
    "cht_warn": 200,
    "cht_crit": 230,
    "oil_temp_warn": 120,
    "oil_temp_crit": 130,
    "oil_press_warn": 30,
    "oil_press_crit": 20,
    "audio_enabled": true,
    "audio_volume": 50
  },
  "magnetic_declination": 0.0,
  "fuel_capacity_gallons": 20.0
}
```

---

## Bill of Materials

| Component | Quantity | Est. Cost | Source | Notes |
|-----------|----------|-----------|--------|-------|
| Raspberry Pi 4 (2GB) | 1 | $45 | Adafruit, Pimoroni | 2GB RAM sufficient |
| Official 7" Touchscreen | 1 | $75 | Adafruit, Pimoroni | 800×480, capacitive |
| MCP2515 CAN Module | 1 | $8 | Amazon | With TJA1050, SPI |
| USB-C Power Supply (5V/3A) | 1 | $12 | Amazon | Official recommended |
| MicroSD Card (32GB) | 1 | $10 | SanDisk | Class 10 |
| Touchscreen Stand | 1 | $15 | 3D printed or mount | Panel mount |
| Power Button | 1 | $3 | Adafruit | Shutdown button |
| Enclosure | 1 | $25 | Custom or SmartiPi | Touchscreen case |
| Cables & Connectors | - | $15 | Various | HDMI, USB, GPIO |
| **Total** | | **$208** | | Approximate |

**Alternative (lower cost):**
- Use HDMI monitor instead of touchscreen: -$60
- Use USB mouse/keyboard: +$20
- **Total: ~$168**

---

## Document Control

**Version:** 1.0
**Date:** 2025-01-18
**Related Documents:**
- Engine-Monitor-System.md
- Instrument-System.md
- Data-Logger-System.md
- Multi-System-Architecture-Plan.md (complete system)

**Change History:**

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2025-01-18 | Initial document created from multi-system plan |
