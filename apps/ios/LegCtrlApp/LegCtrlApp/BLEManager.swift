//
//  BLEManager.swift
//  LegCtrlApp
//
//  CoreBluetooth manager for BLE communication
//

import Foundation
import CoreBluetooth
import Combine

class BLEManager: NSObject, ObservableObject {
    
    // MARK: - Published Properties
    
    @Published var isScanning = false
    @Published var isConnected = false
    @Published var connectionState: String = "Disconnected"
    @Published var discoveredDevices: [CBPeripheral] = []
    @Published var telemetryData: TelemetryData?
    @Published var logMessages: [String] = []
    
    // MARK: - Private Properties
    
    private var centralManager: CBCentralManager!
    private var connectedPeripheral: CBPeripheral?
    private var commandCharacteristic: CBCharacteristic?
    private var telemetryCharacteristic: CBCharacteristic?
    
    private var periodicTimer: Timer?
    private var lastCommandData: Data?
    private var isPeriodicSendingEnabled = false
    
    // Periodic sending at 50 Hz (20ms interval) to prevent deadman timeout
    private let periodicTimerInterval: TimeInterval = 0.02
    
    // Maximum number of log messages to keep
    private let maxLogMessages = 100
    
    // Reusable date formatter for log timestamps
    private lazy var logDateFormatter: DateFormatter = {
        let formatter = DateFormatter()
        formatter.dateFormat = "HH:mm:ss.SSS"
        return formatter
    }()
    
    // MARK: - Initialization
    
    override init() {
        super.init()
        centralManager = CBCentralManager(delegate: self, queue: nil)
    }
    
    // MARK: - Public Methods
    
    func startScanning() {
        guard centralManager.state == .poweredOn else {
            addLog("Bluetooth not ready")
            return
        }
        
        discoveredDevices.removeAll()
        addLog("Scanning for devices...")
        
        let serviceUUID = CBUUID(string: ProtocolConstants.serviceUUID)
        centralManager.scanForPeripherals(
            withServices: [serviceUUID],
            options: [CBCentralManagerScanOptionAllowDuplicatesKey: false]
        )
        isScanning = true
    }
    
    func stopScanning() {
        centralManager.stopScan()
        isScanning = false
        addLog("Scanning stopped")
    }
    
    func connect(to peripheral: CBPeripheral) {
        stopScanning()
        addLog("Connecting to \(peripheral.name ?? "Unknown")...")
        connectionState = "Connecting..."
        centralManager.connect(peripheral, options: nil)
    }
    
    func disconnect() {
        stopPeriodicSending()
        
        if let peripheral = connectedPeripheral {
            addLog("Disconnecting...")
            centralManager.cancelPeripheralConnection(peripheral)
        }
    }
    
    func sendCommand(_ data: Data) {
        guard let peripheral = connectedPeripheral,
              let characteristic = commandCharacteristic else {
            addLog("Cannot send: Not connected")
            return
        }
        
        // Use writeWithoutResponse for low latency
        peripheral.writeValue(data, for: characteristic, type: .withoutResponse)
        
        // Log command (abbreviated)
        let hexString = data.map { String(format: "%02X", $0) }.joined(separator: " ")
        addLog("TX: \(hexString)")
    }
    
    func arm() {
        sendCommand(CommandMessage.arm())
    }
    
    func disarm() {
        sendCommand(CommandMessage.disarm())
    }
    
    func setServoCh0(pulseUs: UInt16) {
        sendCommand(CommandMessage.setServoCh0(pulseUs: pulseUs))
    }
    
    func ping() {
        sendCommand(CommandMessage.ping())
    }
    
    func startPeriodicSending(pulseUs: UInt16) {
        stopPeriodicSending()
        
        lastCommandData = CommandMessage.setServoCh0(pulseUs: pulseUs)
        isPeriodicSendingEnabled = true
        
        periodicTimer = Timer.scheduledTimer(withTimeInterval: periodicTimerInterval, repeats: true) { [weak self] _ in
            guard let self = self, let data = self.lastCommandData else { return }
            self.sendCommand(data)
        }
        
        addLog("Periodic sending started at 50 Hz")
    }
    
    func updatePeriodicCommand(pulseUs: UInt16) {
        guard isPeriodicSendingEnabled else { return }
        lastCommandData = CommandMessage.setServoCh0(pulseUs: pulseUs)
    }
    
    func stopPeriodicSending() {
        let wasEnabled = isPeriodicSendingEnabled
        periodicTimer?.invalidate()
        periodicTimer = nil
        isPeriodicSendingEnabled = false
        lastCommandData = nil
        if wasEnabled {
            addLog("Periodic sending stopped")
        }
    }
    
    // MARK: - Private Methods
    
    private func addLog(_ message: String) {
        let timestamp = Date()
        let logEntry = "[\(logDateFormatter.string(from: timestamp))] \(message)"
        
        DispatchQueue.main.async {
            self.logMessages.append(logEntry)
            // Keep only last maxLogMessages
            if self.logMessages.count > self.maxLogMessages {
                self.logMessages.removeFirst()
            }
        }
    }
}

// MARK: - CBCentralManagerDelegate

extension BLEManager: CBCentralManagerDelegate {
    
    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        switch central.state {
        case .poweredOn:
            addLog("Bluetooth powered on")
        case .poweredOff:
            addLog("Bluetooth powered off")
        case .unsupported:
            addLog("Bluetooth not supported")
        case .unauthorized:
            addLog("Bluetooth unauthorized")
        case .resetting:
            addLog("Bluetooth resetting")
        case .unknown:
            addLog("Bluetooth state unknown")
        @unknown default:
            addLog("Bluetooth state: unknown default")
        }
    }
    
    func centralManager(_ central: CBCentralManager, didDiscover peripheral: CBPeripheral, advertisementData: [String : Any], rssi RSSI: NSNumber) {
        
        if !discoveredDevices.contains(where: { $0.identifier == peripheral.identifier }) {
            discoveredDevices.append(peripheral)
            addLog("Discovered: \(peripheral.name ?? "Unknown") (RSSI: \(RSSI))")
        }
    }
    
    func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral) {
        addLog("Connected to \(peripheral.name ?? "Unknown")")
        connectionState = "Discovering services..."
        isConnected = true
        
        connectedPeripheral = peripheral
        peripheral.delegate = self
        
        let serviceUUID = CBUUID(string: ProtocolConstants.serviceUUID)
        peripheral.discoverServices([serviceUUID])
    }
    
    func centralManager(_ central: CBCentralManager, didDisconnectPeripheral peripheral: CBPeripheral, error: Error?) {
        addLog("Disconnected")
        connectionState = "Disconnected"
        isConnected = false
        
        connectedPeripheral = nil
        commandCharacteristic = nil
        telemetryCharacteristic = nil
        telemetryData = nil
        
        stopPeriodicSending()
    }
    
    func centralManager(_ central: CBCentralManager, didFailToConnect peripheral: CBPeripheral, error: Error?) {
        addLog("Failed to connect: \(error?.localizedDescription ?? "Unknown error")")
        connectionState = "Connection failed"
        isConnected = false
    }
}

// MARK: - CBPeripheralDelegate

extension BLEManager: CBPeripheralDelegate {
    
    func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: Error?) {
        guard error == nil else {
            addLog("Service discovery error: \(error!.localizedDescription)")
            return
        }
        
        guard let services = peripheral.services else { return }
        
        let serviceUUID = CBUUID(string: ProtocolConstants.serviceUUID)
        
        for service in services {
            if service.uuid == serviceUUID {
                addLog("Found LegCtrl service")
                connectionState = "Discovering characteristics..."
                
                let cmdUUID = CBUUID(string: ProtocolConstants.commandCharacteristicUUID)
                let telemetryUUID = CBUUID(string: ProtocolConstants.telemetryCharacteristicUUID)
                
                peripheral.discoverCharacteristics([cmdUUID, telemetryUUID], for: service)
            }
        }
    }
    
    func peripheral(_ peripheral: CBPeripheral, didDiscoverCharacteristicsFor service: CBService, error: Error?) {
        guard error == nil else {
            addLog("Characteristic discovery error: \(error!.localizedDescription)")
            return
        }
        
        guard let characteristics = service.characteristics else { return }
        
        let cmdUUID = CBUUID(string: ProtocolConstants.commandCharacteristicUUID)
        let telemetryUUID = CBUUID(string: ProtocolConstants.telemetryCharacteristicUUID)
        
        for characteristic in characteristics {
            if characteristic.uuid == cmdUUID {
                commandCharacteristic = characteristic
                addLog("Found Command characteristic")
            } else if characteristic.uuid == telemetryUUID {
                telemetryCharacteristic = characteristic
                peripheral.setNotifyValue(true, for: characteristic)
                addLog("Found Telemetry characteristic, enabling notify")
            }
        }
        
        if commandCharacteristic != nil && telemetryCharacteristic != nil {
            connectionState = "Ready"
            addLog("Ready to send commands")
        }
    }
    
    func peripheral(_ peripheral: CBPeripheral, didUpdateValueFor characteristic: CBCharacteristic, error: Error?) {
        guard error == nil else {
            addLog("Characteristic update error: \(error!.localizedDescription)")
            return
        }
        
        let telemetryUUID = CBUUID(string: ProtocolConstants.telemetryCharacteristicUUID)
        
        if characteristic.uuid == telemetryUUID, let data = characteristic.value {
            if let telemetry = TelemetryData.parse(from: data) {
                DispatchQueue.main.async {
                    self.telemetryData = telemetry
                }
            }
        }
    }
    
    func peripheral(_ peripheral: CBPeripheral, didUpdateNotificationStateFor characteristic: CBCharacteristic, error: Error?) {
        if let error = error {
            addLog("Notification state error: \(error.localizedDescription)")
        } else {
            addLog("Notifications \(characteristic.isNotifying ? "enabled" : "disabled") for \(characteristic.uuid)")
        }
    }
}
