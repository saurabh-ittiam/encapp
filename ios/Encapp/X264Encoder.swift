//
//  X264Encoder.swift
//  Encapp
//
//  Created by Ittiam on 26/06/24.
//

import Foundation
import VideoToolbox
import AVFoundation
import CoreMedia
import CoreVideo

class X264Encoder {
    
    var cpuUsageTimer: Timer?
    var x264Wrapper: X264Wrapper?
    var useX264: Bool = false
    var statistics: Statistics!
    var definition: Test
    var scale: Int!
    var frameDurationCMTime: CMTime!
    var frameDurationMs: Float = -1
    var frameDurationUsec: Float = -1
    var inputFrameDurationUsec: Float = 0
    var inputFrameDurationMs: Float = 0
    var frameDurationSec: Float!
    var keepInterval: Float = 1.0
    var inputFrameRate: Float = -1
    var outputFrameRate: Float = -1
    var frameCount: Int = 0
    
    var outputWriterInput: AVAssetWriterInput!
    var outputWriter: AVAssetWriter!
    var outputSettings: [String: Any] = [:]
    
    
    
    var generalConfig = X264GeneralConfig(
        
        i_threads: 1,
        i_width: 0,
        i_height: 0,
        i_csp: 0,
        i_bitdepth: 8
        
    )
    
    
    init(test: Test) {
        self.definition = test
        
        let resolution = splitX(text: definition.input.resolution)
        let width = Int32((resolution[0] >> 1) << 1)
        let height = Int32((resolution[1] >> 1) << 1)
        let preset = definition.encoderX264.preset
        let colorspace = definition.encoderX264.colorSpace
        let bitDepth = definition.encoderX264.bitdepth
        let threads = definition.encoderX264.threads
        
        generalConfig.i_width = width
        generalConfig.i_height = height
        generalConfig.i_bitdepth = bitDepth
        generalConfig.i_threads = threads
        //generalConfig.i_csp = colorspace
        
        
        guard let encoder = X264Wrapper(generalConfig: generalConfig, preset: preset) else {
            log.error("x264 init failed...!")
            return
        }
        
        
        
        self.x264Wrapper = encoder
        //log.error("x264 init went through...!")
    }
    
    
    func Encode() throws -> String {
        statistics = Statistics(description: "x264 encoder", test: definition)
        
        guard let dir = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask).first else {
            log.error("Failed to get document directory")
            return ""
        }
        
        let h264FilePath = dir.appendingPathComponent("output2.h264").path
        FileManager.default.createFile(atPath: h264FilePath, contents: nil, attributes: nil)
        
        guard let fileHandle = FileHandle(forWritingAtPath: h264FilePath) else {
            log.error("Failed to create file handle for output file")
            return ""
        }
        
        log.info("Encode, current test definition = \n\(definition)")
        
        let resolution = splitX(text: definition.input.resolution)
        let width = Int((resolution[0] >> 1) << 1)
        let height = Int((resolution[1] >> 1) << 1)
        
        inputFrameRate = definition.input.hasFramerate ? definition.input.framerate : 30.0
        outputFrameRate = definition.configure.hasFramerate ? definition.configure.framerate : inputFrameRate
        
        if inputFrameRate <= 0 {
            inputFrameRate = 30.0
        }
        if outputFrameRate <= 0  {
            outputFrameRate = 30.0
        }
        
        keepInterval = inputFrameRate / outputFrameRate
        frameDurationUsec = calculateFrameTimingUsec(frameRate: outputFrameRate)
        inputFrameDurationUsec = calculateFrameTimingUsec(frameRate: inputFrameRate)
        inputFrameDurationMs = Float(frameDurationUsec) / 1000.0
        scale = 1000_000 // should be 90000?
        frameDurationMs = Float(frameDurationUsec) / 1000.0
        frameDurationSec = Float(frameDurationUsec) / 1000_000.0
        
        // Codec type
        if !definition.configure.hasCodec {
            log.error("No codec defined")
            return ""
        }
        
        let props = ListProps()
        let codecType = props.lookupCodectType(name: definition.configure.codec)
        let codecId = props.getCodecIdFromType(encoderType: codecType)
        let codecName = props.getCodecNameFromType(encoderType: codecType)
        statistics.setEncoderName(encoderName: codecName)
        
        let fileURL = dir.appendingPathComponent(definition.input.filepath)
        let outputURL = dir.appendingPathComponent("\(statistics.id!).mov")
        let outputFilePath = outputURL.path
        try? FileManager.default.removeItem(atPath: outputFilePath)
        
        var splitname = definition.input.filepath.components(separatedBy: "/")
        statistics.setSourceFile(filename: splitname.last ?? "unknown")
        splitname = outputFilePath.components(separatedBy: "/")
        statistics.setEncodedFile(filename: splitname.last ?? "unknown")
        
        do {
            outputWriter = try AVAssetWriter(outputURL: outputURL, fileType: .mov)
        } catch {
            log.error("Failed to create AVAssetWriter: \(error)")
            return ""
        }
        
        // Video settings
        let videoSettings: [String: Any] = [
            AVVideoCodecKey: AVVideoCodecType.h264,
            AVVideoWidthKey: width,
            AVVideoHeightKey: height,
            // Other settings as needed
        ]
        outputSettings = videoSettings
        
        outputWriterInput = AVAssetWriterInput(mediaType: .video, outputSettings: nil)
        
        if outputWriter.canAdd(outputWriterInput) {
            outputWriter.add(outputWriterInput)
        } else {
            log.error("Cannot add AVAssetWriterInput to AVAssetWriter")
            return ""
        }
        
        outputWriter.startWriting()
        outputWriter.startSession(atSourceTime: .zero)
        
        // Start the CPU usage timer before the frame processing loop
        cpuUsageTimer = Timer.scheduledTimer(withTimeInterval: 1.0, repeats: true) { timer in
            let cpuLoad = self.cpuUsage()
            log.info("CPU Load: \(cpuLoad)%")
        }
        
        do {
            let yuvFileHandle = try FileHandle(forReadingFrom: fileURL)
            defer {
                yuvFileHandle.closeFile()
            }
            
            let ySize = width * height
            let uvSize = (width / 2) * (height / 2)
            statistics.start()
            
            while true {
                guard let (yBuffer, uBuffer, vBuffer) = readNextFrame(yuvFileHandle: yuvFileHandle, ySize: ySize, uvSize: uvSize) else {
                    log.info("No more frames to read")
                    break
                }
                
                var outputBuffer: UnsafeMutablePointer<UInt8>? = nil
                var outputSize: Int32 = 0
                statistics.startEncoding(pts: Int64(frameCount), originalFrame: frameCount)
                
                x264Wrapper?.encodeFrame(yBuffer, uBuffer: uBuffer, vBuffer: vBuffer, outputBuffer: &outputBuffer, outputSize: &outputSize, width: Int32(width), height: Int32(height))
                
                if let buffer = outputBuffer, outputSize > 0 {
                    let data = Data(bytes: buffer, count: Int(outputSize))
                    fileHandle.write(data)
                    
                    let presentationTime = CMTime(seconds: Double(frameCount) * Double(frameDurationSec), preferredTimescale: 800000)
                    let sampleBuffer = createSampleBufferFromData(buffer: buffer, size: Int(outputSize), presentationTime: presentationTime)
                    writeSampleBuffer(sampleBuffer)
                    
                    frameCount += 1
                    //log.info("Frame \(frameCount) - CPU LOAD: \(cpuUsage())%")
                    //log.info("Frame \(frameCount) - presentationTime: \(CMTimeGetSeconds(presentationTime))")
                    
                    free(buffer)
                    outputBuffer = nil
                }
                statistics.stopEncoding(pts: Int64(frameCount), size: 0, isKeyFrame: true)
            }
            
            // Process delayed frames
            var delayedFrameCount = x264Wrapper?.delayedFrames() ?? 0
            log.info("Delayed Frames: \(delayedFrameCount)")
            
            outputWriterInput.markAsFinished()
            outputWriter.finishWriting {
                log.info("Finished writing to \(outputFilePath)")
            }
        } catch {
            log.error("Failed to open YUV file at path: \(definition.input.filepath) with error: \(error)")
            return ""
        }
        
        x264Wrapper?.closeEncoder()
        statistics.stop()
        fileHandle.closeFile()
        
        // Invalidate the CPU usage timer after the encoding process
        cpuUsageTimer?.invalidate()
        cpuUsageTimer = nil
        
        log.info("Done, leaving encoder, encoded: \(statistics.encodedFrames.count)")
        return ""
    }
    
    
    
    
    func cpuUsage() -> Double {
        var kr: kern_return_t
        var task_info_count: mach_msg_type_number_t
        
        task_info_count = mach_msg_type_number_t(TASK_INFO_MAX)
        var tinfo = [integer_t](repeating: 0, count: Int(task_info_count))
        
        kr = task_info(mach_task_self_, task_flavor_t(TASK_BASIC_INFO), &tinfo, &task_info_count)
        if kr != KERN_SUCCESS {
            return -1
        }
        
        var thread_list: thread_act_array_t? = UnsafeMutablePointer(mutating: [thread_act_t]())
        var thread_count: mach_msg_type_number_t = 0
        defer {
            if let thread_list = thread_list {
                vm_deallocate(mach_task_self_, vm_address_t(UnsafePointer(thread_list).pointee), vm_size_t(thread_count))
            }
        }
        
        kr = task_threads(mach_task_self_, &thread_list, &thread_count)
        
        if kr != KERN_SUCCESS {
            return -1
        }
        
        var tot_cpu: Double = 0
        
        if let thread_list = thread_list {
            
            for j in 0 ..< Int(thread_count) {
                var thread_info_count = mach_msg_type_number_t(THREAD_INFO_MAX)
                var thinfo = [integer_t](repeating: 0, count: Int(thread_info_count))
                kr = thread_info(thread_list[j], thread_flavor_t(THREAD_BASIC_INFO),
                                 &thinfo, &thread_info_count)
                if kr != KERN_SUCCESS {
                    return -1
                }
                
                let threadBasicInfo = convertThreadInfoToThreadBasicInfo(thinfo)
                
                if threadBasicInfo.flags != TH_FLAGS_IDLE {
                    tot_cpu += (Double(threadBasicInfo.cpu_usage) / Double(TH_USAGE_SCALE)) * 100.0
                }
            } // for each thread
        }
        
        return tot_cpu
    }
    
    func convertThreadInfoToThreadBasicInfo(_ threadInfo: [integer_t]) -> thread_basic_info {
        var result = thread_basic_info()
        result.user_time = time_value_t(seconds: threadInfo[0], microseconds: threadInfo[1])
        result.system_time = time_value_t(seconds: threadInfo[2], microseconds: threadInfo[3])
        result.cpu_usage = threadInfo[4]
        result.policy = threadInfo[5]
        result.run_state = threadInfo[6]
        result.flags = threadInfo[7]
        result.suspend_count = threadInfo[8]
        result.sleep_time = threadInfo[9]
        
        return result
    }
    
    
    func createSampleBufferFromData(buffer: UnsafeMutablePointer<UInt8>, size: Int, presentationTime: CMTime) -> CMSampleBuffer {
        // Create CMBlockBuffer from buffer
        var blockBuffer: CMBlockBuffer?
        CMBlockBufferCreateWithMemoryBlock(
            allocator: kCFAllocatorDefault,
            memoryBlock: buffer,
            blockLength: size,
            blockAllocator: kCFAllocatorNull,
            customBlockSource: nil,
            offsetToData: 0,
            dataLength: size,
            flags: 0,
            blockBufferOut: &blockBuffer
        )
        
        // Create CMFormatDescription for H.264
        var formatDescription: CMFormatDescription?
        CMVideoFormatDescriptionCreate(
            allocator: kCFAllocatorDefault,
            codecType: kCMVideoCodecType_H264,
            width: Int32(outputSettings[AVVideoWidthKey] as! Int),
            height: Int32(outputSettings[AVVideoHeightKey] as! Int),
            extensions: nil,
            formatDescriptionOut: &formatDescription
        )
        
        
        var timingInfo = CMSampleTimingInfo(duration: .invalid, presentationTimeStamp: presentationTime, decodeTimeStamp: .invalid)
        var sampleBuffer: CMSampleBuffer?
        var sampleSizeArray = [size]
        CMSampleBufferCreateReady(
            allocator: kCFAllocatorDefault,
            dataBuffer: blockBuffer,
            formatDescription: formatDescription,
            sampleCount: 1,
            sampleTimingEntryCount: 1,
            sampleTimingArray: &timingInfo,
            sampleSizeEntryCount: 1,
            sampleSizeArray: &sampleSizeArray,
            sampleBufferOut: &sampleBuffer
        )
        
        
        
        return sampleBuffer!
    }
    
    
    
    
    func writeSampleBuffer(_ sampleBuffer: CMSampleBuffer) {
        guard outputWriterInput.isReadyForMoreMediaData else {
            log.error("AVAssetWriterInput not ready for more media data")
            return
        }
        
        outputWriterInput.append(sampleBuffer)
    }
    			
    func readNextFrame(yuvFileHandle: FileHandle, ySize: Int, uvSize: Int) -> (UnsafeMutablePointer<UInt8>, UnsafeMutablePointer<UInt8>, UnsafeMutablePointer<UInt8>)? {
        let yData = yuvFileHandle.readData(ofLength: ySize)
        let uData = yuvFileHandle.readData(ofLength: uvSize)
        let vData = yuvFileHandle.readData(ofLength: uvSize)
        
        // Check if any data read is empty, indicating the end of the file
        guard !yData.isEmpty && !uData.isEmpty && !vData.isEmpty else {
            return nil
        }
        
        return yData.withUnsafeBytes { yBytes in
            uData.withUnsafeBytes { uBytes in
                vData.withUnsafeBytes { vBytes in
                    guard let yBase = yBytes.baseAddress?.assumingMemoryBound(to: UInt8.self),
                          let uBase = uBytes.baseAddress?.assumingMemoryBound(to: UInt8.self),
                          let vBase = vBytes.baseAddress?.assumingMemoryBound(to: UInt8.self) else {
                        return nil
                    }
                    return (UnsafeMutablePointer(mutating: yBase), UnsafeMutablePointer(mutating: uBase), UnsafeMutablePointer(mutating: vBase))
                }
            }
        }
    }
    
    
}
