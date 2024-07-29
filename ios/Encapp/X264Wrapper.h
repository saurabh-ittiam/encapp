//
//  X264Wrapper.h
//  Encapp
//
//  Created by Ittiam on 18/06/24.
//

#import <Foundation/Foundation.h>
#import "x264.h"



typedef struct {
    int i_threads;
    int i_width;
    int i_height;
    int  i_csp;
    int i_bitdepth;
} X264GeneralConfig;




@interface X264Wrapper : NSObject {
    x264_param_t params;
    int size_of_headers;
    x264_t *encoder;
    x264_nal_t *nals;
    int nnal;
    uint8_t *extra_data;
    int extra_data_size;
    uint8_t *sei;
    int sei_size;
    x264_picture_t pic_in;
    x264_picture_t pic_out;
}



- (instancetype)initWithGeneralConfig:(X264GeneralConfig)generalConfig preset:(NSString *)preset;

- (void)encodeFrame:(uint8_t *)yBuffer uBuffer:(uint8_t *)uBuffer vBuffer:(uint8_t *)vBuffer outputBuffer:(uint8_t **)outputBuffer outputSize:(int *)outputSize width:(int)width height:(int)height;

- (void)closeEncoder;
- (int)delayedFrames;

@end
