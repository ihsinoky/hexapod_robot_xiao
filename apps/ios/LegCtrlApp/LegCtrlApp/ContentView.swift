//
//  ContentView.swift
//  LegCtrlApp
//
//  Main UI for hexapod robot control
//

import SwiftUI

struct ContentView: View {
    @StateObject private var bleManager = BLEManager()
    @State private var servoPulseUs: Double = 1500
    @State private var isPeriodicMode = false
    
    var body: some View {
        NavigationView {
            VStack(spacing: 0) {
                // Connection section
                connectionSection
                
                Divider()
                
                // Control section
                if bleManager.isConnected {
                    controlSection
                } else {
                    Spacer()
                    Text("Not connected")
                        .foregroundColor(.gray)
                        .font(.title3)
                    Spacer()
                }
                
                Divider()
                
                // Telemetry section
                telemetrySection
                
                Divider()
                
                // Log section
                logSection
            }
            .navigationTitle("LegCtrl v0.1")
            .navigationBarTitleDisplayMode(.inline)
        }
        .navigationViewStyle(StackNavigationViewStyle())
    }
    
    // MARK: - Connection Section
    
    private var connectionSection: some View {
        VStack(spacing: 12) {
            HStack {
                Text("Connection")
                    .font(.headline)
                Spacer()
                Text(bleManager.connectionState)
                    .font(.caption)
                    .foregroundColor(bleManager.isConnected ? .green : .gray)
            }
            
            HStack(spacing: 12) {
                if !bleManager.isConnected {
                    Button(action: {
                        if bleManager.isScanning {
                            bleManager.stopScanning()
                        } else {
                            bleManager.startScanning()
                        }
                    }) {
                        Label(bleManager.isScanning ? "Stop Scan" : "Scan", 
                              systemImage: "antenna.radiowaves.left.and.right")
                            .frame(maxWidth: .infinity)
                    }
                    .buttonStyle(.bordered)
                    .tint(bleManager.isScanning ? .red : .blue)
                }
                
                if bleManager.isConnected {
                    Button(action: {
                        bleManager.disconnect()
                    }) {
                        Label("Disconnect", systemImage: "xmark.circle")
                            .frame(maxWidth: .infinity)
                    }
                    .buttonStyle(.bordered)
                    .tint(.red)
                }
            }
            
            if !bleManager.discoveredDevices.isEmpty && !bleManager.isConnected {
                VStack(alignment: .leading, spacing: 8) {
                    Text("Devices:")
                        .font(.caption)
                        .foregroundColor(.gray)
                    
                    ForEach(bleManager.discoveredDevices, id: \.identifier) { device in
                        Button(action: {
                            bleManager.connect(to: device)
                        }) {
                            HStack {
                                Image(systemName: "circle.fill")
                                    .font(.caption2)
                                    .foregroundColor(.blue)
                                Text(device.name ?? "Unknown Device")
                                Spacer()
                                Image(systemName: "chevron.right")
                                    .font(.caption)
                            }
                            .padding(.vertical, 8)
                            .padding(.horizontal, 12)
                            .background(Color.blue.opacity(0.1))
                            .cornerRadius(8)
                        }
                        .buttonStyle(.plain)
                    }
                }
            }
        }
        .padding()
    }
    
    // MARK: - Control Section
    
    private var controlSection: some View {
        VStack(spacing: 16) {
            // ARM/DISARM buttons
            HStack(spacing: 16) {
                Button(action: {
                    bleManager.arm()
                }) {
                    Label("ARM", systemImage: "power")
                        .frame(maxWidth: .infinity)
                        .padding()
                        .background(isArmed ? Color.green.opacity(0.3) : Color.green)
                        .foregroundColor(.white)
                        .cornerRadius(10)
                        .font(.headline)
                }
                .buttonStyle(.plain)
                .disabled(isArmed)
                
                Button(action: {
                    bleManager.stopPeriodicSending()
                    bleManager.disarm()
                    isPeriodicMode = false
                }) {
                    Label("DISARM", systemImage: "stop.circle")
                        .frame(maxWidth: .infinity)
                        .padding()
                        .background(Color.red)
                        .foregroundColor(.white)
                        .cornerRadius(10)
                        .font(.headline)
                }
                .buttonStyle(.plain)
            }
            
            Divider()
            
            // Servo control
            VStack(alignment: .leading, spacing: 8) {
                HStack {
                    Text("CH0 Servo: \(Int(servoPulseUs)) μs")
                        .font(.headline)
                    Spacer()
                    Toggle("Periodic (50Hz)", isOn: $isPeriodicMode)
                        .onChange(of: isPeriodicMode) { newValue in
                            if newValue {
                                bleManager.startPeriodicSending(pulseUs: UInt16(servoPulseUs))
                            } else {
                                bleManager.stopPeriodicSending()
                            }
                        }
                }
                
                Slider(value: $servoPulseUs, in: 500...2500, step: 50)
                    .onChange(of: servoPulseUs) { newValue in
                        if isPeriodicMode {
                            bleManager.updatePeriodicCommand(pulseUs: UInt16(newValue))
                        }
                    }
                
                HStack {
                    Text("500 μs")
                        .font(.caption)
                        .foregroundColor(.gray)
                    Spacer()
                    Text("1500 μs")
                        .font(.caption)
                        .foregroundColor(.gray)
                    Spacer()
                    Text("2500 μs")
                        .font(.caption)
                        .foregroundColor(.gray)
                }
                
                if !isPeriodicMode {
                    Button(action: {
                        bleManager.setServoCh0(pulseUs: UInt16(servoPulseUs))
                    }) {
                        Label("Send Once", systemImage: "paperplane")
                            .frame(maxWidth: .infinity)
                    }
                    .buttonStyle(.borderedProminent)
                }
            }
            
            Divider()
            
            // Manual PING
            Button(action: {
                bleManager.ping()
            }) {
                Label("Send PING", systemImage: "arrow.triangle.2.circlepath")
                    .frame(maxWidth: .infinity)
            }
            .buttonStyle(.bordered)
        }
        .padding()
    }
    
    // MARK: - Telemetry Section
    
    private var telemetrySection: some View {
        VStack(spacing: 12) {
            HStack {
                Text("Telemetry")
                    .font(.headline)
                Spacer()
            }
            
            if let telemetry = bleManager.telemetryData {
                HStack(spacing: 20) {
                    VStack(alignment: .leading, spacing: 4) {
                        Text("State")
                            .font(.caption)
                            .foregroundColor(.gray)
                        Text(telemetry.state.description)
                            .font(.body)
                            .fontWeight(.bold)
                            .foregroundColor(telemetryStateColor(telemetry.state))
                    }
                    
                    VStack(alignment: .leading, spacing: 4) {
                        Text("Error")
                            .font(.caption)
                            .foregroundColor(.gray)
                        Text(telemetry.errorCode.description)
                            .font(.body)
                            .fontWeight(telemetry.errorCode == .none ? .regular : .bold)
                            .foregroundColor(telemetry.errorCode == .none ? .primary : .red)
                    }
                    
                    VStack(alignment: .leading, spacing: 4) {
                        Text("Last Cmd")
                            .font(.caption)
                            .foregroundColor(.gray)
                        Text("\(telemetry.lastCmdAgeMs) ms")
                            .font(.body)
                            .foregroundColor(telemetry.lastCmdAgeMs > 150 ? .red : .primary)
                    }
                    
                    VStack(alignment: .leading, spacing: 4) {
                        Text("Battery")
                            .font(.caption)
                            .foregroundColor(.gray)
                        Text(String(format: "%.1f V", Double(telemetry.batteryMv) / 1000.0))
                            .font(.body)
                    }
                }
                .frame(maxWidth: .infinity)
            } else {
                Text("No telemetry data")
                    .font(.caption)
                    .foregroundColor(.gray)
            }
        }
        .padding()
    }
    
    // MARK: - Log Section
    
    private var logSection: some View {
        VStack(alignment: .leading, spacing: 8) {
            Text("Log")
                .font(.headline)
                .padding(.horizontal)
                .padding(.top, 8)
            
            ScrollViewReader { proxy in
                ScrollView {
                    VStack(alignment: .leading, spacing: 2) {
                        ForEach(bleManager.logMessages.indices, id: \.self) { index in
                            Text(bleManager.logMessages[index])
                                .font(.system(.caption, design: .monospaced))
                                .foregroundColor(.secondary)
                                .id(index)
                        }
                    }
                    .padding(.horizontal)
                }
                .frame(height: 150)
                .onChange(of: bleManager.logMessages.count) { _ in
                    if let lastIndex = bleManager.logMessages.indices.last {
                        withAnimation {
                            proxy.scrollTo(lastIndex, anchor: .bottom)
                        }
                    }
                }
            }
        }
    }
    
    // MARK: - Helpers
    
    private var isArmed: Bool {
        bleManager.telemetryData?.state == .armed
    }
    
    private func telemetryStateColor(_ state: SystemState) -> Color {
        switch state {
        case .disarmed: return .gray
        case .armed: return .green
        case .fault: return .red
        }
    }
}

struct ContentView_Previews: PreviewProvider {
    static var previews: some View {
        ContentView()
            .previewDevice("iPad Pro (11-inch)")
    }
}
