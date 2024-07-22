
#import "X264Wrapper.h"
#include <stdio.h>
#include <stdint.h>
#include <assert.h>

@implementation X264Wrapper

- (instancetype)initWithGeneralConfig:(X264GeneralConfig)generalConfig
                            vuiConfig:(X264VUIConfig)vuiConfig
                       analysisConfig:(X264AnalysisConfig)analysisConfig
                             cropRect:(X264CropRect)cropRect {
    self = [super init];
    if (self) {
        // Initialize x264 parameters
        x264_param_t params;
        x264_param_default(&params);
        
        const char *preset = "ultrafast";  
                if (x264_param_default_preset(&params, preset, "zerolatency") < 0) {
                    NSLog(@"Failed to set preset: %s", preset);
                    return nil;
                }
        
        
        // Mapping generalConfig to x264Params
        params.cpu = generalConfig.cpu;
        params.i_threads = generalConfig.i_threads;
        params.i_lookahead_threads = generalConfig.i_lookahead_threads;
        params.b_sliced_threads = generalConfig.b_sliced_threads;
        params.b_deterministic = generalConfig.b_deterministic;
        params.b_cpu_independent = generalConfig.b_cpu_independent;
        params.i_sync_lookahead = generalConfig.i_sync_lookahead;
        params.i_width = generalConfig.i_width;
        params.i_height = generalConfig.i_height;
        params.i_csp = generalConfig.i_csp;
        params.i_bitdepth = generalConfig.i_bitdepth;
        params.i_level_idc = generalConfig.i_level_idc;
        params.i_frame_total = generalConfig.i_frame_total;
        params.i_nal_hrd = generalConfig.i_nal_hrd;
        params.i_frame_reference = generalConfig.i_frame_reference;
        params.i_dpb_size = generalConfig.i_dpb_size;
        params.i_keyint_max = generalConfig.i_keyint_max;
        params.i_keyint_min = generalConfig.i_keyint_min;
        params.i_scenecut_threshold = generalConfig.i_scenecut_threshold;
        params.b_intra_refresh = generalConfig.b_intra_refresh;
        params.i_bframe = generalConfig.i_bframe;
        params.i_bframe_adaptive = generalConfig.i_bframe_adaptive;
        params.i_bframe_bias = generalConfig.i_bframe_bias;
        params.i_bframe_pyramid = generalConfig.i_bframe_pyramid;
        params.b_open_gop = generalConfig.b_open_gop;
        params.b_bluray_compat = generalConfig.b_bluray_compat;
        params.i_avcintra_class = generalConfig.i_avcintra_class;
        params.b_deblocking_filter = generalConfig.b_deblocking_filter;
        params.i_deblocking_filter_alphac0 = generalConfig.i_deblocking_filter_alphac0;
        params.i_deblocking_filter_beta = generalConfig.i_deblocking_filter_beta;
        params.b_cabac = generalConfig.b_cabac;
        params.i_cabac_init_idc = generalConfig.i_cabac_init_idc;
        params.b_interlaced = generalConfig.b_interlaced;
        params.i_log_level = generalConfig.i_log_level;
        params.b_full_recon = generalConfig.b_full_recon;
        
        // Mapping vuiConfig to x264Params.vui
        params.vui.i_sar_height = vuiConfig.i_sar_height;
        params.vui.i_sar_width = vuiConfig.i_sar_width;
        params.vui.i_overscan = vuiConfig.i_overscan;
        params.vui.i_vidformat = vuiConfig.i_vidformat;
        params.vui.b_fullrange = vuiConfig.b_fullrange;
        params.vui.i_colorprim = vuiConfig.i_colorprim;
        params.vui.i_transfer = vuiConfig.i_transfer;
        params.vui.i_colmatrix = vuiConfig.i_colmatrix;
        params.vui.i_chroma_loc = vuiConfig.i_chroma_loc;
        
        // Mapping analysisConfig to x264Params.analyse
        params.analyse.intra = analysisConfig.intra;
        params.analyse.inter = analysisConfig.inter;
        params.analyse.b_transform_8x8 = analysisConfig.b_transform_8x8;
        params.analyse.i_weighted_pred = analysisConfig.i_weighted_pred;
        params.analyse.b_weighted_bipred = analysisConfig.b_weighted_bipred;
        params.analyse.i_direct_mv_pred = analysisConfig.i_direct_mv_pred;
        params.analyse.i_chroma_qp_offset = analysisConfig.i_chroma_qp_offset;
        params.analyse.i_me_method = analysisConfig.i_me_method;
        params.analyse.i_me_range = analysisConfig.i_me_range;
        params.analyse.i_mv_range = analysisConfig.i_mv_range;
        params.analyse.i_mv_range_thread = analysisConfig.i_mv_range_thread;
        params.analyse.i_subpel_refine = analysisConfig.i_subpel_refine;
        params.analyse.b_chroma_me = analysisConfig.b_chroma_me;
        params.analyse.b_mixed_references = analysisConfig.b_mixed_references;
        params.analyse.i_trellis = analysisConfig.i_trellis;
        params.analyse.b_fast_pskip = analysisConfig.b_fast_pskip;
        params.analyse.b_dct_decimate = analysisConfig.b_dct_decimate;
        params.analyse.i_noise_reduction = analysisConfig.i_noise_reduction;
        params.analyse.f_psy_rd = analysisConfig.f_psy_rd;
        params.analyse.f_psy_trellis = analysisConfig.f_psy_trellis;
        params.analyse.b_psy = analysisConfig.b_psy;
        params.analyse.b_mb_info = analysisConfig.b_mb_info;
        params.analyse.b_mb_info_update = analysisConfig.b_mb_info_update;
        // params.analyse.i_luma_deadzone[0] = analysisConfig.i_luma_deadzone[0];
        // params.analyse.i_luma_deadzone[1] = analysisConfig.i_luma_deadzone[1];
        params.analyse.b_psnr = analysisConfig.b_psnr;
        params.analyse.b_ssim = analysisConfig.b_ssim;
        
        
        params.crop_rect.i_left = cropRect.i_left;
        params.crop_rect.i_top = cropRect.i_top;
        params.crop_rect.i_right = cropRect.i_right;
        params.crop_rect.i_bottom = cropRect.i_bottom;
        
        // Open the x264 encoder
        encoder = x264_encoder_open(&params);
        if (!encoder) {
            NSLog(@"Failed to open encoder");
            return nil;
        }
        NSLog(@"Encoder opened successfully");
        
        // Retrieve encoder parameters
        x264_encoder_parameters(encoder, &params);
        // Retrieve headers
        size_of_headers = x264_encoder_headers(encoder, &nals, &nnal);
     
        if (size_of_headers < 0) {
            NSLog(@"Failed to get encoder headers");
            x264_encoder_close(encoder);
            return nil;
        }
        NSLog(@"Encoder headers obtained. Size: %d", size_of_headers);
      
        // Allocate memory for extra_data
        extra_data = (uint8_t *)malloc(size_of_headers + 64);
        uint8_t *p = extra_data;
        if (!extra_data) {
            NSLog(@"Failed to allocate memory for extra_data");
            x264_encoder_close(encoder);
            return nil;
        }
        
        // Copy NAL units to extra_data
        for (int i = 0; i < nnal; i++) {
            if (nals[i].i_type == NAL_SEI) {
                sei_size = nals[i].i_payload;
                sei = (uint8_t *)malloc(sei_size);
                if (!sei) {
                    NSLog(@"Failed to allocate memory for SEI");
                    return nil;
                }
                memcpy(sei, nals[i].p_payload, nals[i].i_payload);
                continue;
            }
            memcpy(p, nals[i].p_payload, nals[i].i_payload);
            p += nals[i].i_payload;
        }
        extra_data_size = (int)(p - extra_data);
    
    }
    return self;
}


- (void)encodeFrame:(uint8_t *)yBuffer uBuffer:(uint8_t *)uBuffer vBuffer:(uint8_t *)vBuffer outputBuffer:(uint8_t **)outputBuffer outputSize:(int *)outputSize width:(int)width height:(int)height {
    if (!encoder) {
        NSLog(@"Encoder is not initialized for encoding");
        *outputBuffer = NULL;
        *outputSize = -1;
        return;
    }
    
    // Initialize x264 picture and allocate memory for planes
    x264_picture_t pic_in;
    x264_picture_init(&pic_in);

    
    if (x264_picture_alloc(&pic_in, X264_CSP_I420, width, height) < 0) {
        NSLog(@"Failed to allocate memory for x264 picture");
        *outputBuffer = NULL;
        *outputSize = -1;
        return;
    }
    
    // Copy frame data to x264 picture
    int ySize = width * height;
    int uvSize = width * height / 4;
    
    if (yBuffer && uBuffer && vBuffer) {
        memcpy(pic_in.img.plane[0], yBuffer, ySize);
        memcpy(pic_in.img.plane[1], uBuffer, uvSize);
        memcpy(pic_in.img.plane[2], vBuffer, uvSize);
    }
    
    
    NSLog(@"Encoding frame...");
    
    int frame_size = x264_encoder_encode(encoder, &nals, &nnal, &pic_in, &pic_out);
    
    //    if (frame_size > 0) {
    //            int total_size = 0;
    //            bool hasIDR = false; // Assuming boolean for IDR frame detection
    //
    //            for (int i = 0; i < nnal; i++) {
    //                total_size += nals[i].i_payload;
    //                if (nals[i].i_type == NAL_SLICE_IDR) {
    //                    hasIDR = true;
    //                }
    //            }
    
    if (frame_size >= 0) {
        int total_size = 0;
        for (int i = 0; i < nnal; i++) {
            total_size += nals[i].i_payload;
        }
        
        //        *outputBuffer = (uint8_t *)malloc(total_size);
        //        if (!*outputBuffer) {
        //            NSLog(@"Failed to allocate memory for outputBuffer");
        //            *outputSize = -1;
        //            return;
        //        }
        
        if (*outputSize < total_size) {
            free(*outputBuffer);
            *outputBuffer = (uint8_t *)malloc(total_size);
            if (!*outputBuffer) {
                NSLog(@"Failed to allocate memory for outputBuffer");
                *outputSize = -1;
                return;
            }
        }
        
        uint8_t *p = *outputBuffer;
        
        for (int i = 0; i < nnal; i++) {
            memcpy(p, nals[i].p_payload, nals[i].i_payload);
            p += nals[i].i_payload;
        }
        
        *outputSize = total_size;
    } else {
        *outputBuffer = NULL;
        *outputSize = 0;
    }
    
    
    x264_picture_clean(&pic_in);
}



- (void)closeEncoder {
    if (encoder) {
        x264_encoder_close(encoder);
        encoder = NULL;
    }
    
    free(extra_data);
    free(sei);
    extra_data = NULL;
    sei = NULL;
}


- (int)delayedFrames {
    
    return x264_encoder_delayed_frames(encoder);
    
}

@end
