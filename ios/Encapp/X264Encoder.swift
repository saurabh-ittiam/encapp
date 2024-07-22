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
        cpu: 34234,
        i_threads: 1,
        i_lookahead_threads: 1,
        b_sliced_threads: 1,
        b_deterministic: 0,
        b_cpu_independent: 0,
        i_sync_lookahead: -1,
        i_width: 0,
        i_height: 0,
        i_csp: 2,
        i_bitdepth: 8,
        i_level_idc: 9,
        i_frame_total: 15,
        i_nal_hrd: 0,
        i_frame_reference: 1,
        i_dpb_size: 1,
        i_keyint_max: 250,
        i_keyint_min: 0,
        i_scenecut_threshold: 40,
        b_intra_refresh: 0,
        i_bframe: 0,
        i_bframe_adaptive: 1,
        i_bframe_bias: 0,
        i_bframe_pyramid: 2,
        b_open_gop: 0,
        b_bluray_compat: 0,
        i_avcintra_class: 0,
        b_deblocking_filter: 0,
        i_deblocking_filter_alphac0: 0,
        i_deblocking_filter_beta: 0,
        b_cabac: 1,
        i_cabac_init_idc: 0,
        b_interlaced: 0,
        i_log_level: 0,
        b_full_recon: 0
    )
    
    var vuiConfig = X264VUIConfig(
        i_sar_height: 1,
        i_sar_width: 1,
        i_overscan: 0,
        i_vidformat: 5,
        b_fullrange: -1,
        i_colorprim: 2,
        i_transfer: 2,
        i_colmatrix: -1,
        i_chroma_loc: 0
    )
    
    var analysisConfig = X264AnalysisConfig(
        intra: 3,
        inter: 275,
        b_transform_8x8: 1,
        i_weighted_pred: 2,
        b_weighted_bipred: 1,
        i_direct_mv_pred: 1,
        i_chroma_qp_offset: 0,
        i_me_method: 1,
        i_me_range: 16,
        i_mv_range: -1,
        i_mv_range_thread: -1,
        i_subpel_refine: 7,
        b_chroma_me: 1,
        b_mixed_references: 1,
        i_trellis: 1,
        b_fast_pskip: 1,
        b_dct_decimate: 1,
        i_noise_reduction: 0,
        f_psy_rd: 1.0,
        f_psy_trellis: 0.0,
        b_psy: 1,
        b_mb_info: 0,
        b_mb_info_update: 0,
        b_psnr: 0,
        b_ssim: 0
    )
    
    var cropRect = X264CropRect(
        i_left: 0,
        i_top: 0,
        i_right: 0,
        i_bottom: 0
    )
    
    init(test: Test) {
        self.definition = test
        
        let resolution = splitX(text: definition.input.resolution)
        let width = Int32((resolution[0] >> 1) << 1)
        let height = Int32((resolution[1] >> 1) << 1)
        
        generalConfig.i_width = width
        generalConfig.i_height = height
        
        guard let encoder = X264Wrapper(generalConfig: generalConfig, vuiConfig: vuiConfig, analysisConfig: analysisConfig, cropRect: cropRect) else {
            log.error("x264 init failed...!")
            return
        }
        self.x264Wrapper = encoder
        log.error("x264 init went through...!")
    }
    
    
    func Encode() throws -> String {
        statistics = Statistics(description: "x264 encoder", test: definition)
        
        guard let dir = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask).first else {
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
        inputFrameRate = (definition.input.hasFramerate) ? definition.input.framerate: 30.0
        outputFrameRate = (definition.configure.hasFramerate) ? definition.configure.framerate: inputFrameRate
        
        if inputFrameRate <= 0 {
            inputFrameRate = 30.0
        }
        if outputFrameRate <= 0  {
            outputFrameRate = 30.0
        }
        keepInterval = inputFrameRate / outputFrameRate;
        frameDurationUsec = calculateFrameTimingUsec(frameRate: outputFrameRate);
        inputFrameDurationUsec = calculateFrameTimingUsec(frameRate: inputFrameRate);
        inputFrameDurationMs = Float(frameDurationUsec) / 1000.0
        scale = 1000_000 //should be 90000?
        frameDurationUsec =  calculateFrameTimingUsec(frameRate: outputFrameRate);
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
        statistics.setSourceFile(filename: splitname[splitname.count - 1])
        splitname = outputFilePath.components(separatedBy: "/")
        statistics.setEncodedFile(filename: splitname[splitname.count - 1])
        
        
        //let outputFilePath = dir.appendingPathComponent("output.mov").path
        //FileManager.default.createFile(atPath: outputFilePath, contents: nil, attributes: nil)
        
        do {
            outputWriter = try AVAssetWriter(outputURL: outputURL, fileType: AVFileType.mov)
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
        
        outputWriterInput = AVAssetWriterInput(mediaType: AVMediaType.video, outputSettings: nil)
        //outputWriterInput = AVAssetWriterInput(mediaType: .video, outputSettings: outputSettings)
        //outputWriterInput.expectsMediaDataInRealTime = false
        
        if outputWriter.canAdd(outputWriterInput) {
            outputWriter.add(outputWriterInput)
        } else {
            log.error("Cannot add AVAssetWriterInput to AVAssetWriter")
            return ""
        }
        
        outputWriter.startWriting()
        //outputWriter.startSession(atSourceTime: .zero)
        outputWriter.startSession(atSourceTime: CMTime.zero)
        
        
        do {
            
            let yuvFileHandle = try FileHandle(forReadingFrom: fileURL)
            let ySize = width * height
            let uvSize = (width / 2) * (height / 2)
            statistics.start()
            while true {
                
                guard let (yBuffer, uBuffer, vBuffer) = readNextFrame(yuvFileHandle: yuvFileHandle, ySize: ySize, uvSize: uvSize) else {
                    log.info("No more frames to read")
                    break
                }
                
                //log.info("Read a frame")
                
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
                    //log.info("Encoded frame count: \(frameCount)")
                    log.info("Frame \(frameCount) - presentationTime: \(CMTimeGetSeconds(presentationTime))")
                    free(buffer)
                    outputBuffer = nil
                    statistics.stopEncoding(pts: Int64(frameCount), size: 0, isKeyFrame: true)
                }
                
                //                yBuffer.deallocate()
                //                uBuffer.deallocate()
                //                vBuffer.deallocate()
                
            }
            
            // Process delayed frames
            var delayedFrameCount = x264Wrapper?.delayedFrames() ?? 0
            
            
            while delayedFrameCount > 0 {
                //yuvFileHandle.seek(toFileOffset: 0)
                let yuvFileHandle = try FileHandle(forReadingFrom: fileURL)
                let ySize = width * height
                let uvSize = (width / 2) * (height / 2)
                
                for _ in 0..<delayedFrameCount {
                    
                    //                    guard let (yBuffer, uBuffer, vBuffer) = readNextFrame(yuvFileHandle: yuvFileHandle, ySize: ySize, uvSize: uvSize) else {
                    //                        log.info("No more frames to read")
                    //                        break
                    //                    }
                    //log.info("Read a frame")
                    
                    var outputBuffer: UnsafeMutablePointer<UInt8>? = nil
                    var outputSize: Int32 = 0
                    statistics.startEncoding(pts: Int64(frameCount), originalFrame: frameCount)
                    //x264Wrapper?.encodeFrame(yBuffer, uBuffer: uBuffer, vBuffer: vBuffer, outputBuffer: &outputBuffer, outputSize: &outputSize, width: Int32(width), height: Int32(height))
                    x264Wrapper?.encodeFrame(nil, uBuffer: nil, vBuffer: nil, outputBuffer: &outputBuffer, outputSize: &outputSize, width: Int32(width), height: Int32(height))
                    if let buffer = outputBuffer, outputSize > 0 {
                        let data = Data(bytes: buffer, count: Int(outputSize))
                        fileHandle.write(data)
                        
                        
                        let presentationTime = CMTime(seconds: Double(frameCount) * Double(frameDurationSec), preferredTimescale: 800000)
                        
                        
                        let sampleBuffer = createSampleBufferFromData(buffer: buffer, size: Int(outputSize), presentationTime: presentationTime)
                        writeSampleBuffer(sampleBuffer)
                        
                        frameCount += 1
                        //log.info("Flushed frame count: \(frameCount)")
                        log.info("Flushed Frame \(frameCount) - presentationTime: \(CMTimeGetSeconds(presentationTime))")
                        free(buffer)
                        outputBuffer = nil
                        statistics.stopEncoding(pts: Int64(frameCount), size: 0, isKeyFrame: true)
                    }
                    
                    //                    yBuffer.deallocate()
                    //                    uBuffer.deallocate()
                    //                    vBuffer.deallocate()
                    
                    delayedFrameCount -= 1
                }
                
            }
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
        
        log.info("Done, leaving encoder, encoded: \(statistics.encodedFrames.count)")
        return ""
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
