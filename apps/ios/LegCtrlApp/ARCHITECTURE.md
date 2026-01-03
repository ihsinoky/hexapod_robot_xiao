# LegCtrlApp - Architecture Diagram

## System Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                          iPad (iOS 15+)                         │
│                                                                 │
│  ┌───────────────────────────────────────────────────────────┐ │
│  │                    LegCtrlApp (SwiftUI)                   │ │
│  │                                                           │ │
│  │  ┌─────────────────────────────────────────────────────┐ │ │
│  │  │           ContentView (UI Layer)                    │ │ │
│  │  │  ┌─────────────────────────────────────────────┐   │ │ │
│  │  │  │  Connection Section                         │   │ │ │
│  │  │  │  • Scan Button                              │   │ │ │
│  │  │  │  • Device List                              │   │ │ │
│  │  │  │  • Connect/Disconnect                       │   │ │ │
│  │  │  └─────────────────────────────────────────────┘   │ │ │
│  │  │  ┌─────────────────────────────────────────────┐   │ │ │
│  │  │  │  Control Section                            │   │ │ │
│  │  │  │  • ARM / DISARM Buttons                     │   │ │ │
│  │  │  │  • Servo Slider (500-2500 μs)               │   │ │ │
│  │  │  │  • Periodic Mode Toggle (50Hz)              │   │ │ │
│  │  │  │  • Send Once Button                         │   │ │ │
│  │  │  │  • PING Button                              │   │ │ │
│  │  │  └─────────────────────────────────────────────┘   │ │ │
│  │  │  ┌─────────────────────────────────────────────┐   │ │ │
│  │  │  │  Telemetry Section                          │   │ │ │
│  │  │  │  • State (DISARMED/ARMED/FAULT)             │   │ │ │
│  │  │  │  • Error Code                               │   │ │ │
│  │  │  │  • Last Cmd Age (ms)                        │   │ │ │
│  │  │  │  • Battery Voltage (V)                      │   │ │ │
│  │  │  └─────────────────────────────────────────────┘   │ │ │
│  │  │  ┌─────────────────────────────────────────────┐   │ │ │
│  │  │  │  Log Section                                │   │ │ │
│  │  │  │  • Scrollable log viewer (100 entries)      │   │ │ │
│  │  │  └─────────────────────────────────────────────┘   │ │ │
│  │  └─────────────────────────────────────────────────────┘ │ │
│  │                             │                             │ │
│  │                             │ @StateObject                │ │
│  │                             ▼                             │ │
│  │  ┌─────────────────────────────────────────────────────┐ │ │
│  │  │           BLEManager (Business Logic)               │ │ │
│  │  │                                                     │ │ │
│  │  │  • CBCentralManager (CoreBluetooth)               │ │ │
│  │  │  • Scan / Connect / Disconnect                    │ │ │
│  │  │  • Service/Characteristic Discovery               │ │ │
│  │  │  • Command Sending (Write Without Response)       │ │ │
│  │  │  • Telemetry Receiving (Notify)                   │ │ │
│  │  │  • Periodic Timer (50Hz)                          │ │ │
│  │  │  • Log Management                                 │ │ │
│  │  │                                                     │ │ │
│  │  │  @Published Properties:                            │ │ │
│  │  │  • isScanning, isConnected                        │ │ │
│  │  │  • connectionState                                │ │ │
│  │  │  • discoveredDevices                              │ │ │
│  │  │  • telemetryData                                  │ │ │
│  │  │  • logMessages                                    │ │ │
│  │  └─────────────────────────────────────────────────────┘ │ │
│  │                             │                             │ │
│  │                             │ uses                        │ │
│  │                             ▼                             │ │
│  │  ┌─────────────────────────────────────────────────────┐ │ │
│  │  │         ProtocolTypes (Protocol Layer)              │ │ │
│  │  │                                                     │ │ │
│  │  │  • ProtocolConstants (UUIDs, version)             │ │ │
│  │  │  • MessageType (enum)                             │ │ │
│  │  │  • SystemState (enum)                             │ │ │
│  │  │  • ErrorCode (enum)                               │ │ │
│  │  │  • CommandMessage (builder)                       │ │ │
│  │  │    - arm(), disarm()                              │ │ │
│  │  │    - setServoCh0(pulseUs)                         │ │ │
│  │  │    - ping()                                       │ │ │
│  │  │  • TelemetryData (parser)                         │ │ │
│  │  │    - parse(from: Data)                            │ │ │
│  │  └─────────────────────────────────────────────────────┘ │ │
│  └───────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
                                │
                                │ BLE (Bluetooth Low Energy)
                                │ CoreBluetooth Framework
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────┐
│                   XIAO nRF52840 (Peripheral)                    │
│                                                                 │
│  ┌───────────────────────────────────────────────────────────┐ │
│  │            BLE GATT Server (Zephyr Firmware)              │ │
│  │                                                           │ │
│  │  Service: 12345678-1234-1234-1234-123456789abc          │ │
│  │                                                           │ │
│  │  ┌─────────────────────────────────────────────────────┐ │ │
│  │  │  Command Characteristic                             │ │ │
│  │  │  UUID: ...789abd                                    │ │ │
│  │  │  Properties: Write, Write Without Response          │ │ │
│  │  │  Direction: iOS → FW                                │ │ │
│  │  │                                                      │ │ │
│  │  │  Receives:                                          │ │ │
│  │  │  • ARM (0x01)                                       │ │ │
│  │  │  • DISARM (0x02)                                    │ │ │
│  │  │  • SET_SERVO_CH0 (0x03)                             │ │ │
│  │  │  • PING (0x04)                                      │ │ │
│  │  └─────────────────────────────────────────────────────┘ │ │
│  │                                                           │ │
│  │  ┌─────────────────────────────────────────────────────┐ │ │
│  │  │  Telemetry Characteristic                           │ │ │
│  │  │  UUID: ...789abe                                    │ │ │
│  │  │  Properties: Notify                                 │ │ │
│  │  │  Direction: FW → iOS                                │ │ │
│  │  │  Frequency: 10 Hz (100ms)                           │ │ │
│  │  │                                                      │ │ │
│  │  │  Sends:                                             │ │ │
│  │  │  • state (DISARMED/ARMED/FAULT)                    │ │ │
│  │  │  • error_code                                       │ │ │
│  │  │  • last_cmd_age_ms                                  │ │ │
│  │  │  • battery_mv                                       │ │ │
│  │  └─────────────────────────────────────────────────────┘ │ │
│  └───────────────────────────────────────────────────────────┘ │
│                                │                               │
│                                │ I2C                           │
│                                ▼                               │
│  ┌───────────────────────────────────────────────────────────┐ │
│  │                    PCA9685 PWM Driver                     │ │
│  │                                                           │ │
│  │  • 16 channels (CH0-CH15)                                │ │
│  │  • 50 Hz PWM frequency                                   │ │
│  │  • 500-2500 μs pulse width                               │ │
│  └───────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
                                │
                                │ PWM Signal
                                ▼
                        ┌───────────────┐
                        │  Servo Motor  │
                        │  (CH0)        │
                        └───────────────┘
```

## Data Flow

### Command Flow (iOS → Firmware)
```
User Input (Slider/Button)
    │
    ▼
ContentView Action
    │
    ▼
BLEManager.setServoCh0(pulseUs: 1500)
    │
    ▼
CommandMessage.setServoCh0(pulseUs: 1500)
    │   [Encoding: 01 03 02 00 DC 05]
    ▼
CBPeripheral.writeValue(..., type: .withoutResponse)
    │
    │ BLE Write Without Response
    ▼
Firmware: ble_service.c (cmd_write_cb)
    │
    ▼
Command Processing (workqueue)
    │
    ▼
PCA9685 I2C Update
    │
    ▼
Servo Movement
```

### Telemetry Flow (Firmware → iOS)
```
Firmware: Telemetry Timer (10 Hz)
    │
    ▼
ble_service_send_telemetry()
    │   [Encoding: 01 10 08 00 + payload]
    ▼
BLE Notify
    │
    ▼
BLEManager: didUpdateValueFor characteristic
    │
    ▼
TelemetryData.parse(from: data)
    │
    ▼
@Published telemetryData updated
    │
    ▼
ContentView UI Auto-Update (SwiftUI)
    │
    ▼
Display: State, Error, Battery, Last Cmd Age
```

### Periodic Sending Flow (Deadman Prevention)
```
User: Enable "Periodic (50Hz)"
    │
    ▼
BLEManager.startPeriodicSending(pulseUs: 1500)
    │
    ▼
Timer.scheduledTimer(interval: 0.02, repeats: true)
    │
    ├─ Every 20ms ─┐
    │              │
    │              ▼
    │     sendCommand(lastCommandData)
    │              │
    │              │ BLE Write Without Response
    │              ▼
    │         Firmware receives command
    │              │
    │              ▼
    │         Deadman timer reset (< 200ms)
    │              │
    │              ▼
    │         System stays ARMED
    │              │
    └──────────────┘ (loop continues)

User: Disable "Periodic (50Hz)" OR Disconnect
    │
    ▼
Timer stops
    │
    ▼
No commands for >200ms
    │
    ▼
Firmware: Deadman timeout
    │
    ▼
System → FAULT state
    │
    ▼
Servos stop (safe mode)
```

## Safety Mechanisms

### 1. Deadman Timeout (Firmware)
- **Timeout**: 200ms
- **Reset**: Every valid command in ARMED state
- **Action**: Transition to FAULT state, stop all servos

### 2. Periodic Sending (iOS App)
- **Frequency**: 50 Hz (20ms interval)
- **Purpose**: Keep last_cmd_age_ms < 200ms
- **Auto-stop**: On disconnect or user toggle

### 3. State Machine (Firmware)
```
DISARMED → ARM → ARMED → DISARM → DISARMED
              ↓
         (timeout/error)
              ↓
            FAULT → DISARM → DISARMED
```

### 4. UI State Awareness (iOS App)
- ARM button disabled when already ARMED
- Visual feedback for FAULT state (red)
- Connection state monitoring
- Error display

## Communication Protocol

### Message Format
```
All messages: [Header: 4 bytes] + [Payload: 0-N bytes]

Header:
  [version: 1] [msg_type: 1] [payload_len: 1] [reserved: 1]

Encoding: Little-endian for all multi-byte values
Max Size: Fits in MTU 23 (effective 20 bytes)
```

### UUIDs (128-bit Custom)
```
Service:    12345678-1234-1234-1234-123456789abc
Command:    12345678-1234-1234-1234-123456789abd
Telemetry:  12345678-1234-1234-1234-123456789abe
```

## File Structure Summary
```
LegCtrlApp/
├── LegCtrlApp.xcodeproj/
│   └── project.pbxproj              # Xcode project (14KB)
├── LegCtrlApp/
│   ├── LegCtrlAppApp.swift          # Entry point (17 lines)
│   ├── ProtocolTypes.swift          # Protocol layer (178 lines)
│   ├── BLEManager.swift             # BLE logic (300 lines)
│   ├── ContentView.swift            # UI layer (336 lines)
│   ├── Info.plist                   # App config
│   └── Assets.xcassets/             # Icons
├── IMPLEMENTATION_SUMMARY.md        # Feature summary
├── QUICKSTART.md                    # Quick reference
└── .gitignore                       # Git exclusions

Total Swift Code: 831 lines
Total Documentation: ~400 lines
```

---

**Implementation Status**: ✅ Complete  
**Hardware Testing**: ⏳ Pending (requires macOS + iPad + XIAO)  
**Production Ready**: Yes (for Sprint 1 scope)
