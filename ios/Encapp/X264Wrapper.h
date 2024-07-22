//
//  X264Wrapper.h
//  Encapp
//
//  Created by Ittiam on 18/06/24.
//

#import <Foundation/Foundation.h>
#import "x264.h"

// Forward declaration of structs
typedef struct {
    int cpu;
    int i_threads;
    int i_lookahead_threads;
    int b_sliced_threads;
    int b_deterministic;
    int b_cpu_independent;
    int i_sync_lookahead;
    int i_width;
    int i_height;
    int i_csp;
    int i_bitdepth;
    int i_level_idc;
    int i_frame_total;
    int i_nal_hrd;
    int i_frame_reference;
    int i_dpb_size;
    int i_keyint_max;
    int i_keyint_min;
    int i_scenecut_threshold;
    int b_intra_refresh;
    int i_bframe;
    int i_bframe_adaptive;
    int i_bframe_bias;
    int i_bframe_pyramid;
    int b_open_gop;
    int b_bluray_compat;
    int i_avcintra_class;
    int b_deblocking_filter;
    int i_deblocking_filter_alphac0;
    int i_deblocking_filter_beta;
    int b_cabac;
    int i_cabac_init_idc;
    int b_interlaced;
    int i_log_level;
    int b_full_recon;
} X264GeneralConfig;

typedef struct {
    int i_sar_height;
    int i_sar_width;
    int i_overscan;
    int i_vidformat;
    int b_fullrange;
    int i_colorprim;
    int i_transfer;
    int i_colmatrix;
    int i_chroma_loc;
} X264VUIConfig;

typedef struct {
    int intra;
    int inter;
    int b_transform_8x8;
    int i_weighted_pred;
    int b_weighted_bipred;
    int i_direct_mv_pred;
    int i_chroma_qp_offset;
    int i_me_method;
    int i_me_range;
    int i_mv_range;
    int i_mv_range_thread;
    int i_subpel_refine;
    int b_chroma_me;
    int b_mixed_references;
    int i_trellis;
    int b_fast_pskip;
    int b_dct_decimate;
    int i_noise_reduction;
    float f_psy_rd;
    float f_psy_trellis;
    int b_psy;
    int b_mb_info;
    int b_mb_info_update;
    int b_psnr;
    int b_ssim;
} X264AnalysisConfig;

typedef struct {
    int i_left;
    int i_top;
    int i_right;
    int i_bottom;
} X264CropRect;

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

- (instancetype)initWithGeneralConfig:(X264GeneralConfig)generalConfig
                            vuiConfig:(X264VUIConfig)vuiConfig
                       analysisConfig:(X264AnalysisConfig)analysisConfig
                             cropRect:(X264CropRect)cropRect;

- (void)encodeFrame:(uint8_t *)yBuffer uBuffer:(uint8_t *)uBuffer vBuffer:(uint8_t *)vBuffer outputBuffer:(uint8_t **)outputBuffer outputSize:(int *)outputSize width:(int)width height:(int)height;

- (void)closeEncoder;
- (int)delayedFrames;

@end
