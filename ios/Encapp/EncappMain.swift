////
////  EncappMain.swift
////  Encapp
////
////  Created by Johan Blome on 12/5/22.
////
//
import Foundation
#if os(iOS)
import UIKit
#endif
import SwiftUI

    class EncappMain {
        @State var logText: String = "Encapp"
        let runningLockName = "running.lock"
        
        
        
        func run() {
            log.info("Starting encapp: \(CommandLine.arguments)")
            if CommandLine.arguments.count <= 2 {
                log.error("No commands")
                log.info("No command given")
                UIDevice.current.isBatteryMonitoringEnabled = true
                logBatteryLevel(with: "Before running test")
    #if os(iOS)
                UIDevice.current.isBatteryMonitoringEnabled = true
                NotificationCenter.default.addObserver(
                    self,
                    selector: #selector(batteryLevelDidChange(notification:)),
                    name: UIDevice.batteryLevelDidChangeNotification,
                    object: nil
                )
    #endif
                let runner = TestRunner(filename: "phase0_surface_transcode_nv12.pbtxt", completion: completion)
                runner.start()
                
                
                
                
            }else {
                // We are assuming things about the arguments depending on type
                let command = CommandLine.arguments[1] as String
                overview.updateTestsLog(text: "Running command: \'\(command)\'")
                if command == "list_codecs" {
                    log.info("Retrieve codecs")
                    let props = ListProps()
                    let output = props.retrieveProps()
                    log.info(output)
                    //TODO: write to file
                    let io = FileIO()
                    log.info("Write codecx.txt")
                    io.writeData(filename: "codecs.txt", data: output)
                    completion()
                } else if command == "reset" {
                    let io = FileIO()
                    io.deleteEncappoutputFiles()
                    completion()
                } else if command.trimmingCharacters(in: .whitespacesAndNewlines).lowercased() == "test" {
                    log.info("Running a test suite.")
                    let io = FileIO()
                    io.writeData(filename: runningLockName, data: "Running")
                    if CommandLine.arguments.count < 3 {
                        log.error("Command 'test' without test name")
                        completion()
                    }
                    
                    let testToRun = (CommandLine.arguments[2] as String).trimmingCharacters(in: .whitespacesAndNewlines)
                    if !io.fileExist(filename: testToRun) {
                        log.error("Non existing test file: \(testToRun)")
                        completion()
                    }
                    log.info("Start testing: '\(testToRun)'")
                    overview.updateTestsLog(text: "Starting test: '\(testToRun)'")
                    let runner = TestRunner(filename: testToRun, completion: completion)
                    runner.start()
                } else if command == "standby" {
                    // This is only to keep the screen on while doing slow io
                }
            }
        }
        
        
        
    
    
    
    
    @objc func batteryLevelDidChange(notification: NSNotification) {
#if os(iOS)
        let batteryLevel = UIDevice.current.batteryLevel
        let batteryState = UIDevice.current.batteryState
        let stateString: String
        
        switch batteryState {
        case .unknown:
            stateString = "Unknown"
        case .unplugged:
            stateString = "Unplugged"
        case .charging:
            stateString = "Charging"
        case .full:
            stateString = "Full"
        @unknown default:
            stateString = "Unknown"
        }
        
        let logMessage = "Battery level changed - Raw Battery Level: \(batteryLevel * 100)% - Battery State: \(stateString)"
        log.info(logMessage)
#endif
    }
    func logBatteryLevel(with message: String) {
#if os(iOS)
        let batteryLevel = UIDevice.current.batteryLevel
        let batteryState = UIDevice.current.batteryState
        let stateString: String
        
        switch batteryState {
        case .unknown:
            stateString = "Unknown"
        case .unplugged:
            stateString = "Unplugged"
        case .charging:
            stateString = "Charging"
        case .full:
            stateString = "Full"
        @unknown default:
            stateString = "Unknown"
        }
        
        let logMessage = "\(message) - Battery Level: \(batteryLevel )% - Battery State: \(stateString)"
        log.info(logMessage)
#endif
    }
    
    func completion() {
        let io = FileIO()
        log.info("EncappMain: complete, close down")
        logBatteryLevel(with: "After running test")
        log.release()
        overview.updateTestsLog(text: "Done with all tests")
        sleep(1)
        io.deleteFile(filename: runningLockName)
        sleep(1)
        exit(0)
    }
}


