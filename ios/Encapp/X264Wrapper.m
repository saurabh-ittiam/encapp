
#import "X264Wrapper.h"
#include <stdio.h>
#include <stdint.h>
#include <assert.h>

@implementation X264Wrapper

- (instancetype)initWithGeneralConfig:(X264GeneralConfig)generalConfig preset:(NSString *)preset
{
    self = [super init];
    if (self) {
        // Initialize x264 parameters
        x264_param_t params;
        x264_param_default(&params);
        
    
        
        
        
        // Convert NSString to C-string for x264_param_default_preset
        const char *presetCStr = [preset UTF8String];
        
        if (x264_param_default_preset(&params, presetCStr, "zerolatency") < 0) {
            NSLog(@"Failed to set preset: %s", presetCStr);
            return nil;
        }
  
        
        // Mapping generalConfig to x264Params
        params.i_threads = generalConfig.i_threads;
        params.i_width = generalConfig.i_width;
        params.i_height = generalConfig.i_height;
        params.i_csp = X264_CSP_I420;
        params.i_bitdepth = generalConfig.i_bitdepth;
        
        
        
        int result = x264_param_apply_profile(&params, "high");

            if (result < 0) {
                NSLog(@"Failed to apply profile\n");
                return nil;
            }
            else{
                NSLog(@" profile set \n");
            }
        
        
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
        if (!extra_data) {
            NSLog(@"Failed to allocate memory for extra_data");
            x264_encoder_close(encoder);
            return nil;
        }
        
        // Copy NAL units to extra_data
        uint8_t *p = extra_data;
        for (int i = 0; i < nnal; i++) {
            if (nals[i].i_type == NAL_SEI) {
                sei_size = nals[i].i_payload;
                sei = (uint8_t *)malloc(sei_size);
                if (!sei) {
                    NSLog(@"Failed to allocate memory for SEI");
                    free(extra_data);
                    x264_encoder_close(encoder);
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
    
    
    if (frame_size >= 0) {
        int total_size = 0;
        for (int i = 0; i < nnal; i++) {
            total_size += nals[i].i_payload;
        }
        
        
        
        if (*outputSize < total_size) {
            free(*outputBuffer);
            *outputBuffer = (uint8_t *)malloc(total_size);
            if (!*outputBuffer) {
                NSLog(@"Failed to allocate memory for outputBuffer");
                *outputSize = -1;
                x264_picture_clean(&pic_in);
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
    
    if (extra_data) {
        free(extra_data);
        extra_data = NULL;
    }
    
    if (sei) {
        free(sei);
        sei = NULL;
    }
}


- (int)delayedFrames {
    
    if (!encoder) {
        NSLog(@"Encoder is not initialized");
        return -1;
    }
    return x264_encoder_delayed_frames(encoder);
    
}

@end
