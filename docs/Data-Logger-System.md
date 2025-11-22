# Data Logger System
## Aircraft Instrumentation - Subsystem 3

**Document Version:** 1.0
**Date:** 2025-01-18
**Project:** N2-Arduino Aircraft Instrumentation
**System:** Data Logger (CAN ID Range: 0x300-0x3FF)

---

## Executive Summary

The Data Logger System provides persistent storage of all flight data for post-flight analysis, trend monitoring, and maintenance tracking. Running on a Raspberry Pi Zero 2 W, this system records all CAN bus traffic to CSV files on a microSD card and provides WiFi access for data retrieval.

**Platform:** Raspberry Pi Zero 2 W
**Primary Function:** Data logging and retrieval
**Storage:** MicroSD card (32-128GB)
**CAN ID Range:** 0x300-0x3FF (256 message IDs)

**Key Benefits:**
- Gigabytes of flight data storage
- WiFi access for wireless data retrieval
- Automatic flight session detection
- Compressed archives for space efficiency
- Safe shutdown to prevent SD card corruption
- Real-time web interface for status

---

## System Overview

### Hardware Platform

**Board:** Raspberry Pi Zero 2 W
**SoC:** BCM2837 (4× ARM Cortex-A53 @ 1GHz)
**RAM:** 512MB
**Storage:** MicroSD card (32GB minimum, 128GB recommended)
**Connectivity:** WiFi 802.11n (on-chip), Bluetooth (not used)

### Key Components

- **CAN Interface:** MCP2515 CAN controller (SPI) + TJA1050 transceiver
- **Storage:** MicroSD card (high-endurance recommended)
- **Real-Time Clock:** DS3231 with battery backup (I2C)
- **Safe Shutdown Button:** GPIO input with hardware debounce
- **Status LEDs:**
  - Green: Logging active
  - Yellow: SD card write in progress
  - Red: Fault/error condition
- **Power:** 5V regulated supply (2A minimum)

### GPIO Pin Assignments

**SPI (MCP2515 CAN):**
- GPIO 10 (MOSI): SPI data to CAN controller
- GPIO 9 (MISO): SPI data from CAN controller
- GPIO 11 (SCLK): SPI clock
- GPIO 8 (CE0): SPI chip select
- GPIO 25: CAN interrupt (optional)

**I2C (RTC):**
- GPIO 2 (SDA): I2C data
- GPIO 3 (SCL): I2C clock

**Status/Control:**
- GPIO 17: Logging LED (green)
- GPIO 27: SD write LED (yellow)
- GPIO 22: Fault LED (red)
- GPIO 23: Safe shutdown button (input, pull-up)

---

## Processing Responsibilities

### Primary Functions

**1. CAN Bus Monitoring (Promiscuous Mode)**
- Listen to all CAN traffic (0x100-0x4FF range)
- No message filtering (record everything)
- Buffer messages in RAM before writing
- Handle bus errors gracefully
- Monitor bus utilization

**2. CSV File Logging**
- Format: Timestamp, Message ID, Data bytes, Decoded values
- Write buffering (10-second intervals to reduce SD wear)
- New file per flight session
- File naming: `flight_YYYYMMDD_HHMMSS.csv`
- Header row with column names

**3. Flight Session Detection**
- Detect engine start (RPM > 500 from 0x103)
- Create new log file on engine start
- Close file on engine shutdown (RPM = 0 for >60 seconds)
- Rotate files every 2 hours (even if engine running)

**4. Log Compression and Archiving**
- Compress completed flight logs with gzip
- Move to `compressed/` subdirectory
- Automatic cleanup: delete logs >1 year old (configurable)
- Preserve current session uncompressed

**5. WiFi Access Point**
- SSID: `N2-Logger-XXXX` (last 4 of MAC address)
- IP: 192.168.42.1
- Web server on port 5000 (Flask)
- Browse and download logs via web interface
- System status display (disk space, uptime, last flight)

**6. Safe Shutdown**
- Detect shutdown button press (GPIO 23)
- Flush all buffered data to SD card
- Close CSV file properly
- Sync filesystem (`sync` command)
- Unmount SD card
- Power off system (`shutdown -h now`)

**7. Time Synchronization**
- Sync RTC from GPS time (via CAN message 0x203)
- Provides accurate timestamps even when offline
- Battery backup maintains time during power loss

**8. SD Card Health Monitoring**
- Track bad blocks and remapped sectors
- Monitor write cycles (estimate wear)
- Alert when card nearing end of life
- Log card health to separate file

---

## CAN Bus Protocol

### Transmitted Messages

| Message ID | Name | Frequency | Data Layout |
|------------|------|-----------|-------------|
| 0x300 | Logger Status | 1Hz | Byte 0: Status (0=idle, 1=logging, 2=fault), 1: SD free % (0-100), 2-3: File count |
| 0x30F | Heartbeat | 1Hz | Byte 0: Node ID (0x03), 1: Uptime (minutes), 2-7: Reserved |

### Received Messages (Logged)

**All messages from other subsystems:**
- 0x100-0x1FF: Engine Monitor data
- 0x200-0x2FF: Instrument System data
- 0x400-0x4FF: Pilot Information commands/config

**Logging Format:**
- Every received message logged to CSV
- No filtering or selective logging (capture everything)
- Timestamp with millisecond precision

---

## Storage Strategy

### File System Structure

```
/media/sdcard/
├── logs/
│   ├── flight_20250118_093042.csv       (current/recent sessions)
│   ├── flight_20250118_110523.csv
│   ├── flight_20250118_143015.csv
│   └── compressed/
│       ├── flight_20250117_083015.csv.gz (archived sessions)
│       ├── flight_20250117_113020.csv.gz
│       └── flight_20250116_091500.csv.gz
├── config/
│   └── logger_config.json               (configuration settings)
├── status/
│   ├── health_log.txt                   (SD card health monitoring)
│   └── error_log.txt                    (system errors)
└── README.txt                           (instructions for users)
```

### CSV File Format

**Header Row:**
```
Timestamp,Message_ID,Node,Data_Hex,EGT1,EGT2,EGT3,EGT4,CHT1,CHT2,CHT3,CHT4,Oil_Temp,Oil_Press,RPM,Altitude,Pitch,Roll,Heading,Latitude,Longitude,Fuel_Percent,Battery_Volts
```

**Data Rows:**
```
2025-01-18 09:30:42.123,0x100,Engine,A3070012080912098A,685,690,688,692,,,,,,,,,,,,,,
2025-01-18 09:30:42.124,0x101,Engine,C3000C50CA00CE00,,,,,195,198,196,199,,,,,,,,,,,
2025-01-18 09:30:42.223,0x200,Instruments,1015C727E42B,,,,,,,,,,,1679,,,,,,,
```

**Timestamp Format:**
- ISO 8601: `YYYY-MM-DD HH:MM:SS.mmm`
- Millisecond precision
- UTC timezone (synced from GPS)

**Message ID:**
- Hexadecimal format: `0x100`, `0x201`, etc.
- Easy to correlate with CAN protocol documentation

**Data Encoding:**
- Hex string of raw CAN data (e.g., `A3070012080912098A`)
- Decoded values in human-readable columns (e.g., `685` for EGT1 in °C)
- Blank columns for data not in this message

### File Rotation Strategy

**Triggers for New File:**
1. Engine start detected (RPM goes from 0 to >500)
2. 2 hours elapsed since current file opened
3. File size exceeds 100MB (shouldn't happen in 2 hours)

**File Naming:**
- `flight_YYYYMMDD_HHMMSS.csv`
- Example: `flight_20250118_093042.csv` = Jan 18, 2025, 09:30:42

**Compression:**
- After flight ends (engine shutdown for >60 seconds)
- gzip compression (typically 90% reduction)
- Original file removed after successful compression
- Compressed files in `compressed/` subdirectory

### Retention Policy

**Default:**
- Keep all logs from last 90 days uncompressed
- Keep compressed logs from last 365 days
- Delete logs older than 1 year

**Configurable via `logger_config.json`:**
```json
{
  "retention_days_uncompressed": 90,
  "retention_days_compressed": 365,
  "auto_compression": true,
  "compression_delay_seconds": 300
}
```

---

## Software Architecture

### Operating System

**Raspberry Pi OS Lite:**
- Minimal installation (no desktop environment)
- Reduced memory footprint (~180MB RAM used)
- Faster boot time (~15-20 seconds)
- Debian-based: familiar package management (apt)

**Filesystem:**
- ext4 with journaling (data integrity)
- Reduce write amplification: `noatime` mount option
- Regular `sync` to flush buffers

### Software Stack

**Programming Language:** Python 3.9+

**Key Libraries:**
- `python-can`: CAN bus interface (socketcan backend)
- `Flask`: Web server for log browser
- `gzip`: Log compression
- `smbus2`: I2C for RTC
- `RPi.GPIO`: GPIO control for LEDs/button

**Services:**
- `systemd` service for logging daemon (auto-start on boot)
- `hostapd`: WiFi access point
- `dnsmasq`: DHCP/DNS for WiFi clients

### Python Logging Daemon Structure

```python
#!/usr/bin/env python3
"""
CAN Logger Daemon
Logs all CAN bus traffic to CSV files with automatic session detection
"""

import can
import csv
import time
from datetime import datetime
from pathlib import Path

class CANLogger:
    def __init__(self, log_dir='/media/sdcard/logs'):
        self.log_dir = Path(log_dir)
        self.log_dir.mkdir(parents=True, exist_ok=True)

        # Initialize CAN bus
        self.bus = can.interface.Bus(
            bustype='socketcan',
            channel='can0',
            bitrate=500000
        )

        # Buffer for batched writes
        self.buffer = []
        self.buffer_size = 100  # Write every 100 messages or 10 seconds
        self.last_flush = time.time()

        # Current log file
        self.csv_file = None
        self.csv_writer = None
        self.current_filename = None

        # Session detection
        self.engine_running = False
        self.last_rpm_time = 0

    def start_new_file(self):
        """Create new CSV log file"""
        if self.csv_file:
            self.flush()
            self.csv_file.close()

        timestamp = datetime.utcnow().strftime('%Y%m%d_%H%M%S')
        self.current_filename = self.log_dir / f'flight_{timestamp}.csv'

        self.csv_file = open(self.current_filename, 'w', newline='')
        self.csv_writer = csv.writer(self.csv_file)

        # Write header
        self.csv_writer.writerow([
            'Timestamp', 'Message_ID', 'Node', 'Data_Hex',
            'EGT1', 'EGT2', 'EGT3', 'EGT4',
            'CHT1', 'CHT2', 'CHT3', 'CHT4',
            'Oil_Temp', 'Oil_Press', 'RPM', 'Altitude',
            'Pitch', 'Roll', 'Heading',
            'Latitude', 'Longitude',
            'Fuel_Percent', 'Battery_Volts'
        ])

        print(f"Started new log file: {self.current_filename}")

    def parse_message(self, msg):
        """Parse CAN message and extract values"""
        timestamp = datetime.utcnow().strftime('%Y-%m-%d %H:%M:%S.%f')[:-3]
        msg_id = f'0x{msg.arbitration_id:03X}'
        data_hex = msg.data.hex().upper()

        # Determine node
        if 0x100 <= msg.arbitration_id <= 0x1FF:
            node = 'Engine'
        elif 0x200 <= msg.arbitration_id <= 0x2FF:
            node = 'Instruments'
        elif 0x300 <= msg.arbitration_id <= 0x3FF:
            node = 'Logger'
        elif 0x400 <= msg.arbitration_id <= 0x4FF:
            node = 'PilotInfo'
        else:
            node = 'Unknown'

        # Decode based on message ID
        values = self.decode_message(msg)

        # Check for engine start/stop (0x103 contains RPM)
        if msg.arbitration_id == 0x103 and len(msg.data) >= 2:
            rpm = int.from_bytes(msg.data[0:2], 'little')
            self.handle_rpm_change(rpm)

        return [timestamp, msg_id, node, data_hex] + values

    def decode_message(self, msg):
        """Decode message data into human-readable values"""
        # Initialize all fields as empty
        values = [''] * 19  # 19 data columns

        # Decode based on message ID
        if msg.arbitration_id == 0x100:  # EGT Temperatures
            if len(msg.data) == 8:
                values[0] = int.from_bytes(msg.data[0:2], 'little') / 10  # EGT1
                values[1] = int.from_bytes(msg.data[2:4], 'little') / 10  # EGT2
                values[2] = int.from_bytes(msg.data[4:6], 'little') / 10  # EGT3
                values[3] = int.from_bytes(msg.data[6:8], 'little') / 10  # EGT4

        elif msg.arbitration_id == 0x101:  # CHT Temperatures
            if len(msg.data) == 8:
                values[4] = int.from_bytes(msg.data[0:2], 'little') / 10  # CHT1
                values[5] = int.from_bytes(msg.data[2:4], 'little') / 10  # CHT2
                values[6] = int.from_bytes(msg.data[4:6], 'little') / 10  # CHT3
                values[7] = int.from_bytes(msg.data[6:8], 'little') / 10  # CHT4

        elif msg.arbitration_id == 0x102:  # Oil & Manifold
            if len(msg.data) >= 4:
                values[8] = int.from_bytes(msg.data[0:2], 'little') / 10   # Oil Temp
                values[9] = int.from_bytes(msg.data[2:4], 'little') / 10   # Oil Pressure

        elif msg.arbitration_id == 0x103:  # Tach & Hours
            if len(msg.data) >= 2:
                values[10] = int.from_bytes(msg.data[0:2], 'little')  # RPM

        elif msg.arbitration_id == 0x200:  # Altitude & Pressure
            if len(msg.data) >= 2:
                values[11] = int.from_bytes(msg.data[0:2], 'little', signed=True)  # Altitude

        elif msg.arbitration_id == 0x201:  # Attitude
            if len(msg.data) >= 6:
                values[12] = int.from_bytes(msg.data[0:2], 'little', signed=True) / 10  # Pitch
                values[13] = int.from_bytes(msg.data[2:4], 'little', signed=True) / 10  # Roll
                values[14] = int.from_bytes(msg.data[4:6], 'little')  # Heading

        elif msg.arbitration_id == 0x202:  # GPS Position
            if len(msg.data) == 8:
                values[15] = int.from_bytes(msg.data[0:4], 'little', signed=True) / 1e6  # Latitude
                values[16] = int.from_bytes(msg.data[4:8], 'little', signed=True) / 1e6  # Longitude

        elif msg.arbitration_id == 0x204:  # Fuel & Battery
            if len(msg.data) >= 3:
                values[17] = msg.data[0]  # Fuel %
                values[18] = int.from_bytes(msg.data[1:3], 'little') / 100  # Battery Volts

        return values

    def handle_rpm_change(self, rpm):
        """Detect engine start/stop and manage log files"""
        current_time = time.time()

        if rpm > 500 and not self.engine_running:
            # Engine just started
            print(f"Engine start detected (RPM: {rpm})")
            self.engine_running = True
            self.start_new_file()

        elif rpm == 0 and self.engine_running:
            # Engine stopped, but wait 60 seconds to confirm
            if current_time - self.last_rpm_time > 60:
                print("Engine shutdown confirmed")
                self.engine_running = False
                # Compress log file in background
                # (actual implementation would use subprocess)

        if rpm > 0:
            self.last_rpm_time = current_time

    def flush(self):
        """Write buffered data to CSV file"""
        if self.buffer and self.csv_writer:
            self.csv_writer.writerows(self.buffer)
            self.csv_file.flush()
            self.buffer = []
            self.last_flush = time.time()

    def run(self):
        """Main loop: receive CAN messages and log"""
        print("CAN Logger started")
        self.start_new_file()  # Create initial log file

        try:
            while True:
                # Receive message (1 second timeout)
                msg = self.bus.recv(timeout=1.0)

                if msg:
                    # Parse and buffer
                    row = self.parse_message(msg)
                    self.buffer.append(row)

                # Flush buffer every 100 messages or 10 seconds
                if len(self.buffer) >= self.buffer_size or \
                   (time.time() - self.last_flush) > 10:
                    self.flush()

        except KeyboardInterrupt:
            print("\nShutting down...")
            self.flush()
            if self.csv_file:
                self.csv_file.close()
            print("Shutdown complete")

if __name__ == '__main__':
    logger = CANLogger()
    logger.run()
```

### Web Interface (Flask)

**Minimal web server for log browsing:**

```python
#!/usr/bin/env python3
"""
Web interface for log browsing and download
"""

from flask import Flask, render_template, send_file
from pathlib import Path
import os

app = Flask(__name__)
LOG_DIR = Path('/media/sdcard/logs')

@app.route('/')
def index():
    """Main page: list all log files"""
    files = []

    # List uncompressed files
    for f in sorted(LOG_DIR.glob('flight_*.csv'), reverse=True):
        files.append({
            'name': f.name,
            'size': f.stat().st_size,
            'date': f.stat().st_mtime,
            'compressed': False
        })

    # List compressed files
    compressed_dir = LOG_DIR / 'compressed'
    if compressed_dir.exists():
        for f in sorted(compressed_dir.glob('flight_*.csv.gz'), reverse=True):
            files.append({
                'name': f.name,
                'size': f.stat().st_size,
                'date': f.stat().st_mtime,
                'compressed': True
            })

    return render_template('index.html', files=files)

@app.route('/download/<path:filename>')
def download(filename):
    """Download a log file"""
    if filename.endswith('.gz'):
        filepath = LOG_DIR / 'compressed' / filename
    else:
        filepath = LOG_DIR / filename

    return send_file(filepath, as_attachment=True)

@app.route('/status')
def status():
    """System status page"""
    import subprocess

    # Get disk usage
    df = subprocess.check_output(['df', '-h', '/media/sdcard']).decode()

    # Get uptime
    uptime = subprocess.check_output(['uptime']).decode()

    return render_template('status.html', df=df, uptime=uptime)

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
```

---

## Power Budget

### Power Consumption

**Idle (WiFi off):**
- RPi Zero 2W: 150mA @ 5V = 750mW
- MCP2515 CAN: 5mA @ 5V = 25mW
- DS3231 RTC: 1mA @ 5V = 5mW
- **Total Idle: 156mA @ 5V = 780mW**

**Active Logging:**
- RPi Zero 2W: 200mA @ 5V = 1000mW
- SD write (burst): +50mA = 250mW (bursts <1 second)
- **Total Active: 250mA @ 5V = 1250mW**

**WiFi Active:**
- WiFi: +50mA average = 250mW
- **Total with WiFi: 300mA @ 5V = 1500mW**

### From 12V Aircraft Bus
- With 95% efficient regulator: ~132mA @ 12V (active logging)
- Acceptable for aircraft electrical system
- Dedicated 5V/2A supply recommended (isolated from other systems)

---

## Development Plan

### Phase 1: Hardware Setup (Week 1)

**Tasks:**
- Set up Raspberry Pi Zero 2 W
- Install Raspberry Pi OS Lite
- Configure MCP2515 CAN interface (enable SPI in config.txt)
- Connect DS3231 RTC (enable I2C)
- Wire status LEDs and shutdown button
- Test microSD write performance

**Deliverables:**
- Raspberry Pi boots to console
- CAN interface recognized (`ip link show can0`)
- RTC keeps time after power cycle
- LEDs controllable from GPIO

### Phase 2: CAN Interface and Logging (Week 2)

**Tasks:**
- Install python-can library (`pip3 install python-can`)
- Configure socketcan interface
- Write basic CAN receive script
- Implement CSV logging with buffering
- Test with simulated CAN data (from Arduino)

**Deliverables:**
- Python daemon receives CAN messages
- CSV file created with correct format
- Buffering works (writes every 10 seconds)

### Phase 3: Advanced Features (Week 3)

**Tasks:**
- Implement flight session detection
- Add log file compression (gzip)
- Implement safe shutdown procedure
- Add status LED control
- Create systemd service for auto-start

**Deliverables:**
- New file created on engine start
- Old files compressed automatically
- Shutdown button triggers safe power-off
- Logger starts on boot

### Phase 4: WiFi and Web Interface (Week 4)

**Tasks:**
- Configure WiFi access point (hostapd)
- Set up DHCP server (dnsmasq)
- Develop Flask web interface
- Create log browser page
- Add download functionality

**Deliverables:**
- WiFi AP broadcasting (N2-Logger-XXXX)
- Web interface accessible at 192.168.42.1:5000
- Logs browsable and downloadable

### Testing Milestones

**CAN Reception:**
- All messages from Engine and Instruments received
- No dropped messages (<0.1% loss acceptable)
- Timing accurate (±10ms)

**Logging Performance:**
- 1-hour continuous logging with zero data loss
- File size reasonable (~10MB/hour estimated)
- SD card write speed adequate (>2 MB/s)

**Compression:**
- gzip achieves >80% size reduction
- Compressed files readable (gunzip works)
- Compression doesn't interfere with logging

**WiFi Access:**
- Range covers cockpit area (10+ feet)
- Download speed acceptable (>100 KB/s)
- Multiple clients can connect (2+ devices)

**Safe Shutdown:**
- Button press triggers shutdown within 5 seconds
- All data flushed before power off
- SD card survives 100 power cycles without corruption

---

## WiFi Configuration

### Access Point Setup

**hostapd.conf:**
```
interface=wlan0
driver=nl80211
ssid=N2-Logger-
hw_mode=g
channel=6
wmm_enabled=0
macaddr_acl=0
auth_algs=1
ignore_broadcast_ssid=0
wpa=2
wpa_passphrase=YourPasswordHere
wpa_key_mgmt=WPA-PSK
wpa_pairwise=TKIP
rsn_pairwise=CCMP
```

**dnsmasq.conf:**
```
interface=wlan0
dhcp-range=192.168.42.10,192.168.42.50,255.255.255.0,24h
```

**Static IP for wlan0:**
```
# /etc/dhcpcd.conf
interface wlan0
static ip_address=192.168.42.1/24
nohook wpa_supplicant
```

### Web Interface Access

**URL:** `http://192.168.42.1:5000`

**Features:**
- Browse all log files (sorted by date, newest first)
- Download individual files
- System status (disk space, uptime, last flight)
- Delete old files (with confirmation)
- View current logging status

**Security:**
- Basic HTTP (no HTTPS needed for local network)
- Optional: HTTP basic auth (username/password)
- Firewall: Only allow port 5000 on wlan0

---

## SD Card Management

### Card Selection

**Recommended:**
- SanDisk High Endurance (rated for dashcams/security)
- Samsung PRO Endurance
- Minimum: Class 10, UHS-I
- Size: 32GB minimum, 64-128GB recommended

**Avoid:**
- Generic/no-name brands (high failure rate)
- Ultra-cheap cards (counterfeit risk)
- Very old cards (worn out)

### Health Monitoring

**Check bad blocks:**
```bash
sudo badblocks -v /dev/mmcblk0
```

**Monitor SMART data:**
```bash
sudo apt install smartmontools
sudo smartctl -a /dev/mmcblk0
```

**Track writes:**
- Log total bytes written
- Estimate remaining life (based on card specs)
- Alert when >80% of rated endurance used

### Backup Strategy

**Automatic WiFi Upload (future enhancement):**
- When on home WiFi network
- Rsync logs to NAS or cloud storage
- Delete local copies after successful upload

**Manual Download:**
- Connect to WiFi AP
- Browse to web interface
- Download flight logs of interest

---

## Troubleshooting

### Common Issues

| Symptom | Cause | Solution |
|---------|-------|----------|
| No CAN messages | Interface not up | `sudo ip link set can0 up type can bitrate 500000` |
| SD card full | Old logs not deleted | Run cleanup script or download and delete |
| WiFi not visible | hostapd not running | `sudo systemctl start hostapd` |
| Logging daemon crashed | Python error | Check logs: `sudo journalctl -u canlogger` |
| Time incorrect after reboot | RTC battery dead | Replace CR2032 battery in DS3231 |
| Corrupt CSV file | Unexpected shutdown | Use safe shutdown button |

### Diagnostic Commands

**Check CAN interface:**
```bash
ip link show can0
candump can0  # View live CAN traffic
```

**Check logging daemon:**
```bash
sudo systemctl status canlogger
sudo journalctl -u canlogger -f  # Follow log output
```

**Check disk space:**
```bash
df -h /media/sdcard
```

**Check WiFi AP:**
```bash
sudo systemctl status hostapd
iwconfig wlan0
```

### Log Files

**System logs:**
- `/var/log/canlogger.log`: Logger daemon output
- `/media/sdcard/status/error_log.txt`: Application errors
- `/var/log/syslog`: System-wide logs

---

## Bill of Materials

| Component | Quantity | Est. Cost | Source | Notes |
|-----------|----------|-----------|--------|-------|
| Raspberry Pi Zero 2 W | 1 | $15 | Adafruit, Pimoroni | Includes WiFi |
| MCP2515 CAN Module | 1 | $8 | Amazon | With TJA1050 |
| MicroSD Card (64GB) | 1 | $15 | SanDisk High Endurance | Rated for continuous write |
| DS3231 RTC Module | 1 | $5 | Amazon | With battery |
| LEDs (R/Y/G) | 3 | $2 | Local electronics | Status indicators |
| Pushbutton | 1 | $1 | Local electronics | Safe shutdown |
| Resistors | 5 | $1 | Local electronics | LED current limit, pull-ups |
| Power Supply (5V/2A) | 1 | $8 | Amazon | Dedicated for RPi |
| Enclosure | 1 | $12 | Hammond | Small plastic case |
| MicroSD Card Reader | 1 | $5 | Amazon | For initial setup |
| Connectors & Wire | - | $10 | DigiKey | Various |
| **Total** | | **$82** | | Approximate |

---

## Document Control

**Version:** 1.0
**Date:** 2025-01-18
**Related Documents:**
- Engine-Monitor-System.md
- Instrument-System.md
- Pilot-Information-System.md
- Multi-System-Architecture-Plan.md (complete system)

**Change History:**

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2025-01-18 | Initial document created from multi-system plan |
