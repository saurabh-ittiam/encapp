//
//  Transcoder.swift
//  Encapp
//
//  Created by Binu E R on 11/06/24.
//

import Foundation
import VideoToolbox
import AVFoundation



class Transcoder {
    var definition:Test
    var statistics: Statistics!
    var trackOutput: AVAssetReaderOutput!
    var decoderSession: VTDecompressionSession!
    var encoderSession: VTCompressionSession!
    var formatDescription: CMFormatDescription!
    var lastTimeMs = 0 as Int64
    var currentTimeSec = 0 as Double
    var inputDone = false
    var scale: CMTimeScale = 0
    var framesAdded = 0
    var outputWriter: AVAssetWriter!
    var outputWriterInput: AVAssetWriterInput!
    var pts = 0
    //var lastPts: CMTime!
    var lastPts: CMTime = .zero
    var pixelPool: CVPixelBufferPool!
    var frameDurationCMTime: CMTime!
    var frameDurationMs = -1 as Float
    var frameDurationUsec = -1 as Float
    var inputFrameDurationUsec = 0 as Float
    var inputFrameDurationMs = 0 as Float
    var inputFrameCounter = 0
    var keepInterval = 1.0 as Float
    var frameDurationSec: Float!
    
    //This is the input frame rate,
    //used to figure out relations to the output frquecny
    var inputFrameRate = -1 as Float
    var outputFrameRate = -1 as Float
    var muxfps = 0 as Float
    var processFramesCounter: Int = 0
    
    var frameBuffer: [(presentationTimeStamp: CMTime, imageBuffer: CVPixelBuffer)] = []
    var shouldProcess = false
    
    init(test: Test){
        self.definition = test
    }
    func Transcode() throws -> String {
        statistics = Statistics(description: "transcoder", test: definition)
        if let dir = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask).first {
            log.info("Transcode, current test definition = \n\(definition)")
            
            // File handling
            let fileURL = dir.appendingPathComponent(definition.input.filepath)
            if FileManager.default.fileExists(atPath: fileURL.path) {
                log.info("Input media file: \(fileURL.path)")
            } else {
                log.error("Media file: \(fileURL.path) does not exist")
                return "Error: no media file"
            }
            
            let source = try AVAsset(url: fileURL)
            let inputReader = try? AVAssetReader(asset: source)
            
            let semaphore = DispatchSemaphore(value: 0)
            Task {
                let tracks = try await source.load(.tracks)
                trackOutput = AVAssetReaderTrackOutput(track: tracks[0], outputSettings: nil)
                // After initializing trackOutput
                self.muxfps = tracks[0].nominalFrameRate
                
                semaphore.signal()
            }
            semaphore.wait()
            
            log.info("Continue")
            inputReader?.add(trackOutput)
            inputReader?.startReading()
            
            
            // Callback
            let decodeCallback: VTDecompressionOutputHandler = { status, infoFlags, imageBuffer, presentationTs, duration in
                self.statistics.stopDeccoding(pts: Int64(presentationTs.seconds * 1000000.0))
                
                self.currentTimeSec = presentationTs.seconds
                self.currentTimeSec = presentationTs.seconds
                
                guard let imageBuffer = imageBuffer else {
                    log.error("Received nil imageBuffer")
                    return
                }
                
                // Store frame in the buffer
                self.frameBuffer.append((presentationTimeStamp: presentationTs, imageBuffer: imageBuffer))
                self.frameBuffer.sort { $0.presentationTimeStamp < $1.presentationTimeStamp }
                
                
                
                
                self.processFrames()
            }
            
            var frameNum = 0 as UInt32
            var inputFrameCounter = 0
            var currentLoop = 1
            
            let inputFrameRate = definition.input.hasFramerate ? definition.input.framerate : 30.0
            let outputFrameRate = definition.configure.hasFramerate ? definition.configure.framerate : inputFrameRate
            let keepInterval = inputFrameRate / outputFrameRate
            let frameDurationUsec = calculateFrameTimingUsec(frameRate: outputFrameRate)
            let inputFrameDurationUsec = calculateFrameTimingUsec(frameRate: inputFrameRate)
            
            while !inputDone {
                if inputFrameCounter % 100 == 0 {
                    log.info("\(definition.common.id) - Transcoder: input frames: \(inputFrameCounter) current loop: \(currentLoop) current time: \(currentTimeSec)")
                    if doneReading(test: definition, stream: nil, frame: inputFrameCounter, time: Float(currentTimeSec), loop: false) {
                        inputDone = true
                    }
                }
                if let sampleBuffer = trackOutput.copyNextSampleBuffer() {
                    frameNum += 1
                    var decodeInfoFlags = VTDecodeFrameFlags()
                    
                    var infoFlags = VTDecodeInfoFlags(rawValue: frameNum)
                    if decoderSession == nil {
                        setupDecoder(sampleBuffer: sampleBuffer)
                    }
                    let frameDurationMs = sampleBuffer.duration.seconds * 1000.0
                    if decoderSession != nil {
                        if definition.input.realtime {
                            self.lastTimeMs = sleepUntilNextFrame(lastTimeMs: self.lastTimeMs, frameDurationMs: frameDurationMs)
                        }
                        inputFrameCounter += 1
                        guard sampleBuffer.presentationTimeStamp.seconds.isFinite else {
                            log.debug("Decoder time is not finite, cannot decompress. Most likely end of stream.")
                            break
                        }
                        statistics.startDecoding(pts: Int64(sampleBuffer.presentationTimeStamp.seconds * 1000000.0))
                        VTDecompressionSessionDecodeFrame(decoderSession!,
                                                          sampleBuffer: sampleBuffer,
                                                          flags: decodeInfoFlags,
                                                          infoFlagsOut: &infoFlags,
                                                          outputHandler: decodeCallback)
                    } else {
                        log.error("Decoder session or sample buffer is nil")
                    }
                } else {
                    inputDone = true
                }
            }
            while !self.frameBuffer.isEmpty{
                processFrames()
                processFrames()
            }
            statistics.stop()
            // Flush
            let framePts = computePresentationTimeUsec(frameIndex: inputFrameCounter, frameTimeUsec: inputFrameDurationUsec, offset: Int64(pts))
            let lastTime = CMTime(value: Int64(framePts), timescale: CMTimeScale(scale))
            VTCompressionSessionCompleteFrames(encoderSession, untilPresentationTimeStamp: lastTime)
            
            outputWriterInput.markAsFinished()
            log.info("Wait for all pending frames")
            sleep(1)
            log.info("Call writer finish")
            outputWriter.finishWriting {
                sleep(1)
            }
            
            while outputWriter.status == .writing {
                sleep(1)
            }
            
            VTCompressionSessionInvalidate(encoderSession)
            
            return "OK"
        }
        return "Error"
    }
    
    
    private func processFrames() {
        
        processFramesCounter += 1
        log.info("Frames Encoded : \(processFramesCounter) ")
        
        while !self.frameBuffer.isEmpty {
            let firstFrame = self.frameBuffer.first!
            
            let deltaPtsTimeScale = firstFrame.presentationTimeStamp.timescale
            
            
            let deltaPtsValueFloat = Float(deltaPtsTimeScale) / self.muxfps
            
            
            let deltaPtsValue = CMTimeValue(deltaPtsValueFloat)
            
            
            let deltaPts = CMTime(value: deltaPtsValue, timescale: deltaPtsTimeScale)
            
            
            let nextExpectedPts = self.lastPts + deltaPts
            let tolerance: CMTime = CMTime(seconds: 0.01, preferredTimescale: firstFrame.presentationTimeStamp.timescale)
            let timeDifference = CMTimeSubtract(firstFrame.presentationTimeStamp, nextExpectedPts)
            
            
            let shouldProcessFrame = firstFrame.presentationTimeStamp == .zero || abs(timeDifference.seconds) <= tolerance.seconds
            
            if shouldProcessFrame {
                log.info("Processing frame from buffer with PTS: \(firstFrame.presentationTimeStamp.seconds)")
                let framePts = computePresentationTimeUsec(frameIndex: self.inputFrameCounter, frameTimeUsec: self.inputFrameDurationUsec, offset: Int64(firstFrame.presentationTimeStamp.value))
                self.scale = firstFrame.presentationTimeStamp.timescale // Assigning scale here
                var timeInfo = CMSampleTimingInfo()
                timeInfo.presentationTimeStamp = CMTime(value: Int64(framePts), timescale: CMTimeScale(self.scale))
                timeInfo.duration = CMTime(value: Int64(self.frameDurationUsec), timescale: CMTimeScale(self.scale))
                timeInfo.decodeTimeStamp = timeInfo.presentationTimeStamp
                
                var infoFlags = VTEncodeInfoFlags()
                
                self.statistics.startEncoding(pts: Int64(firstFrame.presentationTimeStamp.seconds), originalFrame: self.inputFrameCounter)
                if self.encoderSession == nil {
                    do {
                        try self.setupEncoder(pixelBuffer: firstFrame.imageBuffer)
                    } catch {
                        return
                    }
                }
                
                // Lock base addresses of image buffer
                var status = CVPixelBufferLockBaseAddress(firstFrame.imageBuffer, CVPixelBufferLockFlags(rawValue: 0))
                if status != noErr {
                    log.error("Failed to lock base address for first frame image buffer, status: \(status)")
                    return
                }
                
                var downscaledBuffer: CVPixelBuffer? = nil
                let resolution = splitX(text: definition.input.resolution)
                let width = resolution[0]
                let height = resolution[1]
                var downscaleWidth = (width >> 1) << 1 // Ensure width is even
                var downscaleHeight = (height >> 1) << 1 // Ensure height is even
                
                
                
                var pixelBufferAttributes: [String: Any] = [
                    kCVPixelBufferPixelFormatTypeKey as String: kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange,
                    kCVPixelBufferWidthKey as String: downscaleWidth,
                    kCVPixelBufferHeightKey as String: downscaleHeight
                ]
                
                status = CVPixelBufferCreate(kCFAllocatorDefault, Int(downscaleWidth), Int(downscaleHeight), kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange, pixelBufferAttributes as CFDictionary, &downscaledBuffer)
                if status != noErr {
                    log.error("Failed to create downscaled pixel buffer, status: \(status)")
                    return
                }
                
                status = CVPixelBufferLockBaseAddress(downscaledBuffer!, CVPixelBufferLockFlags(rawValue: 0))
                if status != noErr {
                    log.error("Failed to lock base address for downscaled buffer, status: \(status)")
                    return
                }
                
                // Reading input buffers row-wise and storing in arrays
                let inpBuffer_Y = CVPixelBufferGetBaseAddressOfPlane(firstFrame.imageBuffer, 0)!
                let inpStride_Y = CVPixelBufferGetBytesPerRowOfPlane(firstFrame.imageBuffer, 0)
                let height_Y = CVPixelBufferGetHeightOfPlane(firstFrame.imageBuffer, 0)
                let width_Y = CVPixelBufferGetWidthOfPlane(firstFrame.imageBuffer, 0)
                
                let inpBuffer_UV = CVPixelBufferGetBaseAddressOfPlane(firstFrame.imageBuffer, 1)!
                let inpStride_UV = CVPixelBufferGetBytesPerRowOfPlane(firstFrame.imageBuffer, 1)
                let height_UV = CVPixelBufferGetHeightOfPlane(firstFrame.imageBuffer, 1)
                let width_UV = CVPixelBufferGetWidthOfPlane(firstFrame.imageBuffer, 1)
                
                let outBuffer_Y = CVPixelBufferGetBaseAddressOfPlane(downscaledBuffer!, 0)!
                let outStride_Y = CVPixelBufferGetBytesPerRowOfPlane(downscaledBuffer!, 0)
                let outBuffer_UV = CVPixelBufferGetBaseAddressOfPlane(downscaledBuffer!, 1)!
                let outStride_UV = CVPixelBufferGetBytesPerRowOfPlane(downscaledBuffer!, 1)
                
                let inpFrameStride_Y = inpStride_Y
                let inpFrameStride_UV = inpStride_UV
                
                let outFrameStride_Y = outStride_Y
                let outFrameStride_UV = outStride_UV
                
                
                let outBuffer: [UnsafeMutableRawPointer?] = [outBuffer_Y, outBuffer_UV]
                //let inpFrameStride: [Int32] = [Int32(inpFrameStride_Y), Int32(inpFrameStride_UV)]
                let outFrameStride: [Int32] = [Int32(outFrameStride_Y), Int32(outFrameStride_UV)]
                
                let inpPixelFormat: Int32 = 21
                let outPixelFormat: Int32 = inpPixelFormat
                
                let downscaleFlag = "bicubic"
                let downscaleFlagCString = downscaleFlag.cString(using: .utf8)!
                
                // Perform downscaling to the downscaled buffer
                DownScaler(
                    inpBuffer_Y, inpBuffer_UV, nil,
                    outBuffer[0], outBuffer[1], nil,
                    Int32(width_Y), Int32(height_Y),
                    Int32(inpStride_Y), Int32(inpStride_UV),
                    Int32(downscaleWidth), Int32(downscaleHeight),
                    outFrameStride[0], outFrameStride[1],
                    inpPixelFormat, outPixelFormat,
                    downscaleFlagCString
                )
                
                
                CVPixelBufferUnlockBaseAddress(downscaledBuffer!, CVPixelBufferLockFlags(rawValue: 0))
                CVPixelBufferUnlockBaseAddress(firstFrame.imageBuffer, CVPixelBufferLockFlags(rawValue: 0))
                
                var encodeStatus = VTCompressionSessionEncodeFrame(self.encoderSession!,
                                                                   imageBuffer: downscaledBuffer!,
                                                                   presentationTimeStamp: timeInfo.presentationTimeStamp,
                                                                   duration: timeInfo.duration,
                                                                   frameProperties: nil,
                                                                   sourceFrameRefcon: Unmanaged.passUnretained(self).toOpaque(),
                                                                   infoFlagsOut: &infoFlags)
                self.lastPts = timeInfo.presentationTimeStamp
                if encodeStatus != noErr {
                    log.error("Encode frame failed, status: \(encodeStatus)")
                }
                
                self.frameBuffer.removeFirst()
                break
            }
            else {
                break
            }
        }
    }
    
    
    
    
    func setupDecoder(sampleBuffer: CMSampleBuffer) {
        if sampleBuffer.formatDescription == nil {
            log.error("no format descritpion")
            return
        }
        let imagerBufferAttributes = [
            kCVPixelBufferOpenGLCompatibilityKey: NSNumber(true),
        ]
        let compressionSessionOut = UnsafeMutablePointer<VTDecompressionSession?>.allocate(capacity: 1)
        /* let decoderSpecification = [
         kVTVideoDecoderSpecification_RequireHardwareAcceleratedVideoDecoder: NSString(string: "true"),
         ]*/
        // Create session
        log.info("Create decoder session with type: \(sampleBuffer.formatDescription!) - \(statistics.encoderName)")
        statistics.start()
        var status = VTDecompressionSessionCreate(allocator: kCFAllocatorDefault,
                                                  formatDescription: sampleBuffer.formatDescription!,
                                                  decoderSpecification: nil,
                                                  imageBufferAttributes: imagerBufferAttributes as CFDictionary?,
                                                  outputCallback: nil,
                                                  decompressionSessionOut: compressionSessionOut)
        
        if (status != noErr) {
            log.error("Failed to create decoder session, \(status) ")
            log.debug("Failed to create decoder session, \(status)")
            return
        }
        
        decoderSession = compressionSessionOut.pointee
        log.info("comp: \(decoderSession.debugDescription)")
        var gpu: CFBoolean!
        status = VTSessionCopyProperty(decoderSession, key: kVTDecompressionPropertyKey_UsingGPURegistryID, allocator: nil, valueOut: &gpu)
        if status != 0 {
            log.error("failed to check gpu statis, status: \(status)")
        } else {
            log.info("Gpu status: \(gpu)")
        }
        
        
        logVTSessionProperties(statistics: statistics, compSession: decoderSession)
    }
    
    func setupEncoder(pixelBuffer: CVPixelBuffer) throws  -> String {
        // Check input
        let resolution = splitX(text: definition.input.resolution)
        let sourceWidth = resolution[0]
        let sourceHeight = resolution[1]
        // TODO: it seems the encoder is expecting aligned planes
        let width = Int((resolution[0] >> 1) << 1)
        let height = Int((resolution[1] >> 1) << 1)
        
        // Codec type
        if !definition.configure.hasCodec {
            log.error("No codec defined")
            //return ""
        }
        let props = ListProps()
        let codecType = props.lookupCodectType(name: definition.configure.codec)
        let codecId = props.getCodecNameFromType(encoderType: codecType)
        statistics.setEncoderName(encoderName: codecId)
        
        
        //output
        let imageBufferAttributes = [
            kCVPixelBufferWidthKey: NSNumber(value: width),
            kCVPixelBufferHeightKey: NSNumber(value: height),
            kCVPixelBufferPixelFormatTypeKey: NSNumber(value: kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange)
        ]
        
        let compressionSessionOut = UnsafeMutablePointer<VTCompressionSession?>.allocate(capacity: 1)
        
        // Callback
        let encodeCallback: VTCompressionOutputCallback = { outputCallbackRefCon, sourceFrameRefCon, status, infoFlags, sampleBuffer in
            let encoder: Transcoder = Unmanaged<Transcoder>.fromOpaque(sourceFrameRefCon!).takeUnretainedValue()
            if sampleBuffer != nil && CMSampleBufferDataIsReady(sampleBuffer!) {
                log.info("Writing \(sampleBuffer!.totalSampleSize) bytes for pts:\(sampleBuffer?.presentationTimeStamp.value)")
                encoder.writeData(sampleBuffer: sampleBuffer!, infoFlags: infoFlags)
            } else {
                if (infoFlags.rawValue == VTEncodeInfoFlags.frameDropped.rawValue) {
                    log.debug("Encoder dropped frame")
                } else if (infoFlags.rawValue == VTEncodeInfoFlags.asynchronous.rawValue) {
                    log.debug("Aynchronous frame")
                } else {
                    encoder.fail(cause: "Sample buffer is not ok, \(sampleBuffer.debugDescription), \(status) ")
                }
            }
        }
        
        let encoderSpecification = [
            //kVTVideoEncoderSpecification_EncoderID: NSString(string: codecId),
            // The next property will disable any settings done at a later stage when it comes to frame order
            // frame drops, latency etc.
            kVTVideoEncoderSpecification_EnableLowLatencyRateControl: NSString(string: "false"),
            //TODO: profiles should be added.
            //kVTCompressionPropertyKey_ProfileLevel: NSString(string: kVTProfileLevel_H264_High_AutoLevel),
        ]
        // Create session
        log.info("Create \(width)x\(height) encoder session with type: \(codecType) - \(statistics.encoderName)")
        var status = VTCompressionSessionCreate(allocator: kCFAllocatorDefault,
                                                width: Int32(width),
                                                height: Int32(height),
                                                codecType: codecType, //kCMVideoCodecType_H264,
                                                encoderSpecification: encoderSpecification as CFDictionary?,
                                                imageBufferAttributes: imageBufferAttributes as CFDictionary?,
                                                compressedDataAllocator: kCFAllocatorDefault,
                                                outputCallback: encodeCallback,
                                                refcon: Unmanaged.passUnretained(self).toOpaque(),
                                                compressionSessionOut: compressionSessionOut)
        
        if (status != noErr) {
            log.error("Failed to create encoder session, \(status) ")
            return "Failed to create encoder session, \(status)"
        }
        encoderSession = compressionSessionOut.pointee
        
        // Configure encoder
        setVTEncodingSessionProperties(definition: definition, compSession: encoderSession)
        status = VTCompressionSessionPrepareToEncodeFrames(encoderSession)
        if status != 0 {
            log.error("failed prepare for encode, status: \(status)")
            
        }
        
        logVTSessionProperties(statistics: statistics, compSession: encoderSession)
        
        pixelPool = VTCompressionSessionGetPixelBufferPool(self.encoderSession)
        
        
        let frameSize = Int(Float(sourceWidth) * Float(sourceHeight) * 1.5)
        // Filehandling
        if let dir = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask).first {
            let fileURL = dir.appendingPathComponent(definition.input.filepath)
            if FileManager.default.fileExists(atPath: fileURL.path) {
                log.info("Input media file: \(fileURL.path)")
            } else {
                log.error("Media file: \(fileURL.path) does not exist")
                return "Error: no media file"
            }
            let outputURL = dir.appendingPathComponent("\(statistics.id!).mov")
            let outputPath = outputURL.path
            try? FileManager.default.removeItem(atPath: outputPath)
            
            // Nil for encoded data, only mov works
            outputWriter = try AVAssetWriter(outputURL: outputURL, fileType: AVFileType.mov)
            outputWriterInput = AVAssetWriterInput(mediaType: AVMediaType.video, outputSettings: nil)
            outputWriter.add(outputWriterInput)
            
            outputWriter.startWriting()
            outputWriter.startSession(atSourceTime: CMTime.zero)
            
            var splitname = definition.input.filepath.components(separatedBy: "/")
            statistics.setSourceFile(filename: splitname[splitname.count - 1])
            splitname = outputPath.components(separatedBy: "/")
            statistics.setEncodedFile(filename: splitname[splitname.count - 1])
            
        }
        
        return ""
        
    }
    
    func writeData(sampleBuffer: CMSampleBuffer, infoFlags: VTEncodeInfoFlags) -> Void {
        let tmp = UnsafeMutablePointer<UInt8>.allocate(capacity: sampleBuffer.totalSampleSize)
        var buffer: UnsafeMutablePointer<Int8>?
        let status = CMBlockBufferAccessDataBytes((sampleBuffer.dataBuffer)!, atOffset: 0, length: sampleBuffer.totalSampleSize, temporaryBlock: tmp, returnedPointerOut: &buffer )
        
        if status != noErr {
            log.error("Failed to get base address for blockbuffer")
            return
        }
        if let attachments = CMSampleBufferGetSampleAttachmentsArray(sampleBuffer, createIfNecessary: true) {
            let rawDic: UnsafeRawPointer = CFArrayGetValueAtIndex(attachments, 0)
            let dic: CFDictionary = Unmanaged.fromOpaque(rawDic).takeUnretainedValue()
            
            let keyFrame = !CFDictionaryContainsKey(dic, Unmanaged.passUnretained(kCMSampleAttachmentKey_NotSync).toOpaque())
            //let dependOnOther = CFDictionaryContainsKey(dic, Unmanaged.passUnretained(kCMSampleAttachmentKey_DependsOnOthers).toOpaque())
            
            statistics.stopEncoding(pts: sampleBuffer.presentationTimeStamp.value, size: Int64(sampleBuffer.totalSampleSize), isKeyFrame: keyFrame)
            currentTimeSec = Double(sampleBuffer.presentationTimeStamp.value) / Double(scale)
            framesAdded += 1
            
            //            do {
            //                try sampleBuffer.setOutputPresentationTimeStamp(CMTimeMake(value:Int64(framesAdded*6500), timescale: 25000))
            //            } catch {
            //            }
            
            if outputWriterInput.isReadyForMoreMediaData {
                if (outputWriterInput.append(sampleBuffer)) {
                    log.info("Wrote \(sampleBuffer.totalSampleSize) bytes successfully")
                } else {
                    log.error("Muxer write status: \(outputWriter.status), Error: \(outputWriter.error)")
                }
            } else {
                log.error("Writer not ready for input")
            }
        }
        tmp.deallocate()
    }
    
    func fail(cause: String) {
        log.error("Transcode failed: \(cause)")
        //inputDone = true
    }
}
