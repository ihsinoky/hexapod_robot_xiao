//
//  ProtocolTypes.swift
//  LegCtrlApp
//
//  Protocol definitions for BLE communication with hexapod robot
//  Based on legctrl_protocol v0.1
//

import Foundation

// MARK: - Protocol Constants

enum ProtocolConstants {
    static let version: UInt8 = 0x01
    
    // Service and Characteristic UUIDs
    static let serviceUUID = "12345678-1234-1234-1234-123456789ABC"
    static let commandCharacteristicUUID = "12345678-1234-1234-1234-123456789ABD"
    static let telemetryCharacteristicUUID = "12345678-1234-1234-1234-123456789ABE"
}

// MARK: - Message Types

enum MessageType: UInt8 {
    case cmdArm = 0x01
    case cmdDisarm = 0x02
    case cmdSetServoCh0 = 0x03
    case cmdPing = 0x04
    case telemetry = 0x10
}

// MARK: - System States

enum SystemState: UInt8 {
    case disarmed = 0x00
    case armed = 0x01
    case fault = 0x02
    
    var description: String {
        switch self {
        case .disarmed: return "DISARMED"
        case .armed: return "ARMED"
        case .fault: return "FAULT"
        }
    }
}

// MARK: - Error Codes

enum ErrorCode: UInt8 {
    case none = 0x00
    case deadmanTimeout = 0x01
    case lowBattery = 0x02
    case i2cFault = 0x03
    case invalidCmd = 0x04
    case unknown = 0xFF
    
    var description: String {
        switch self {
        case .none: return "None"
        case .deadmanTimeout: return "Deadman Timeout"
        case .lowBattery: return "Low Battery"
        case .i2cFault: return "I2C Fault"
        case .invalidCmd: return "Invalid Command"
        case .unknown: return "Unknown Error"
        }
    }
}

// MARK: - Protocol Messages

struct ProtocolHeader {
    let version: UInt8
    let msgType: UInt8
    let payloadLen: UInt8
    let reserved: UInt8
    
    func toData() -> Data {
        var data = Data()
        data.append(version)
        data.append(msgType)
        data.append(payloadLen)
        data.append(reserved)
        return data
    }
}

// MARK: - Command Messages

struct CommandMessage {
    static func arm() -> Data {
        let header = ProtocolHeader(
            version: ProtocolConstants.version,
            msgType: MessageType.cmdArm.rawValue,
            payloadLen: 0,
            reserved: 0
        )
        return header.toData()
    }
    
    static func disarm() -> Data {
        let header = ProtocolHeader(
            version: ProtocolConstants.version,
            msgType: MessageType.cmdDisarm.rawValue,
            payloadLen: 0,
            reserved: 0
        )
        return header.toData()
    }
    
    static func setServoCh0(pulseUs: UInt16) -> Data {
        let header = ProtocolHeader(
            version: ProtocolConstants.version,
            msgType: MessageType.cmdSetServoCh0.rawValue,
            payloadLen: 2,
            reserved: 0
        )
        
        var data = header.toData()
        // Little-endian encoding
        data.append(UInt8(pulseUs & 0xFF))
        data.append(UInt8((pulseUs >> 8) & 0xFF))
        return data
    }
    
    static func ping() -> Data {
        let header = ProtocolHeader(
            version: ProtocolConstants.version,
            msgType: MessageType.cmdPing.rawValue,
            payloadLen: 0,
            reserved: 0
        )
        return header.toData()
    }
}

// MARK: - Telemetry Message

struct TelemetryData {
    let state: SystemState
    let errorCode: ErrorCode
    let lastCmdAgeMs: UInt16
    let batteryMv: UInt16
    
    static func parse(from data: Data) -> TelemetryData? {
        // Expected: 12 bytes total (4 header + 8 payload)
        guard data.count >= 12 else { return nil }
        
        // Verify header
        guard data[0] == ProtocolConstants.version,
              data[1] == MessageType.telemetry.rawValue,
              data[2] == 8 else { return nil }
        
        // Parse payload (Little-endian)
        let stateRaw = data[4]
        let errorCodeRaw = data[5]
        let lastCmdAgeMs = UInt16(data[6]) | (UInt16(data[7]) << 8)
        let batteryMv = UInt16(data[8]) | (UInt16(data[9]) << 8)
        
        let state = SystemState(rawValue: stateRaw) ?? .fault
        let errorCode = ErrorCode(rawValue: errorCodeRaw) ?? .unknown
        
        return TelemetryData(
            state: state,
            errorCode: errorCode,
            lastCmdAgeMs: lastCmdAgeMs,
            batteryMv: batteryMv
        )
    }
}
