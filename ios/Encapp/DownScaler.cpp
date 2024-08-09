#include <cstdlib>
#include <iostream>
#include <string>
#include "DownScaler.hpp"

#define NUM_PLANES 3

int DownScaler(void* y_plane, void* u_plane, void* v_plane,
               void* out_y_plane, void* out_u_plane, void* out_v_plane,
               int inp_frame_width, int inp_frame_height,
               int inp_y_stride, int inp_uv_stride,
               int out_frame_width, int out_frame_height,
               int out_y_stride, int out_uv_stride,
               int inp_pixel_format, int out_pixel_format,
               const char* downscaleflag) {
    
    std::string downscale_filter_str(downscaleflag);
    std::cout << "downscale_filter_str: " << downscale_filter_str << std::endl;
    
    void* pv_scratch_buffer = nullptr;
    
    if (downscale_filter_str != "bicubic") {
        pv_scratch_buffer = malloc(CALC_SCRATCH_BUF_SIZE_DOWNSCALE(out_frame_width));
        std::cout << "Done pv_scratch_buffer assign" << std::endl;
    }
    
    // Print stride values
    std::cout << "Input Y stride: " << inp_y_stride << std::endl;
    std::cout << "Input UV stride: " << inp_uv_stride << std::endl;
    std::cout << "Output Y stride: " << out_y_stride << std::endl;
    std::cout << "Output UV stride: " << out_uv_stride << std::endl;
    
    // Handle input format and convert if necessary
    UWORD8* p_inp_cb_plane_local = nullptr;
    UWORD8* p_inp_cr_plane_local = nullptr;
    
    if (inp_pixel_format == 21) { // Assuming 21 is for NV12 or similar
        p_inp_cb_plane_local = static_cast<UWORD8*>(malloc((inp_frame_width * inp_frame_height) >> 2));
        p_inp_cr_plane_local = static_cast<UWORD8*>(malloc((inp_frame_width * inp_frame_height) >> 2));
        
        // Convert input UV semi-planar to planar
        WORD32 i4_i, i4_j;
        UWORD32 inp_str = inp_uv_stride;
        UWORD8* pu1_inp = static_cast<UWORD8*>(u_plane);
        UWORD8* pu1_chrm_u = p_inp_cb_plane_local;
        UWORD8* pu1_chrm_v = p_inp_cr_plane_local;
        
        for (i4_j = 0; i4_j < (inp_frame_height >> 1); i4_j++) {
            for (i4_i = 0; i4_i < (inp_frame_width >> 1); i4_i++) {
                *pu1_chrm_u++ = *pu1_inp++;
                *pu1_chrm_v++ = *pu1_inp++;
            }
            pu1_inp -= inp_frame_width;
            pu1_inp += inp_str;
        }
        
        u_plane = p_inp_cb_plane_local;
        v_plane = p_inp_cr_plane_local;
        //inp_y_stride = inp_frame_width;
        inp_uv_stride = inp_frame_width >> 1;
    }
    
    // Handle output format and allocate buffers if needed
    UWORD8* p_out_cb_plane_local = nullptr;
    UWORD8* p_out_cr_plane_local = nullptr;
    
    if (out_pixel_format == 21) { // Assuming 21 is for NV12 or similar
        p_out_cb_plane_local = static_cast<UWORD8*>(malloc((out_frame_width * out_frame_height) >> 2));
        p_out_cr_plane_local = static_cast<UWORD8*>(malloc((out_frame_width * out_frame_height) >> 2));
        
        // Backup UV interleaved pointer and stride
        UWORD8* out_u_plane_backup = static_cast<UWORD8*>(out_u_plane);
        UWORD32 out_uv_stride_backup = out_uv_stride;
        
        // Replace U and V buffer with local pointers
        out_u_plane = p_out_cb_plane_local;
        out_v_plane = p_out_cr_plane_local;
        out_uv_stride = out_frame_width >> 1;
        
        // Perform downscaling
        std::cout << "Filter function is called" << std::endl;
        if (downscale_filter_str == "bicubic") {
            std::cout << "Bicubic Filter function is called" << std::endl;
            bicubic_resize(static_cast<const unsigned char*>(y_plane),
                           static_cast<unsigned char*>(out_y_plane),
                           inp_frame_width, inp_frame_height,
                           inp_y_stride,
                           out_frame_width, out_frame_height,
                           out_y_stride);
            
            // Downscale UV planes
            bicubic_resize(static_cast<const unsigned char*>(u_plane),
                           static_cast<unsigned char*>(out_u_plane),
                           inp_frame_width / 2, inp_frame_height / 2,
                           inp_uv_stride,
                           out_frame_width / 2, out_frame_height / 2,
                           out_uv_stride);
            
            bicubic_resize(static_cast<const unsigned char*>(v_plane),
                           static_cast<unsigned char*>(out_v_plane),
                           inp_frame_width / 2, inp_frame_height / 2,
                           inp_uv_stride,
                           out_frame_width / 2, out_frame_height / 2,
                           out_uv_stride);
        } else {
            std::cout << "Lanczos Filter function is called" << std::endl;
            down_sample_8tap_filt(static_cast<unsigned char*>(y_plane),
                                  static_cast<unsigned char*>(out_y_plane),
                                  inp_frame_width, inp_frame_height,
                                  inp_y_stride,
                                  out_frame_width, out_frame_height,
                                  out_y_stride,
                                  pv_scratch_buffer);
            
            // Downscale UV planes
            down_sample_8tap_filt(static_cast<unsigned char*>(u_plane),
                                  static_cast<unsigned char*>(out_u_plane),
                                  inp_frame_width / 2, inp_frame_height / 2,
                                  inp_uv_stride,
                                  out_frame_width / 2, out_frame_height / 2,
                                  out_uv_stride,
                                  pv_scratch_buffer);
            
            down_sample_8tap_filt(static_cast<unsigned char*>(v_plane),
                                  static_cast<unsigned char*>(out_v_plane),
                                  inp_frame_width / 2, inp_frame_height / 2,
                                  inp_uv_stride,
                                  out_frame_width / 2, out_frame_height / 2,
                                  out_uv_stride,
                                  pv_scratch_buffer);
        }
        std::cout << "Done Filter function" << std::endl;
        
        // Convert output UV planar to semi-planar
        {
            WORD32 i4_i, i4_j;
            UWORD32 out_str = out_uv_stride_backup;
            UWORD8* pu1_out = out_u_plane_backup;
            UWORD8* pu1_chrm_u = static_cast<UWORD8*>(out_u_plane);
            UWORD8* pu1_chrm_v = static_cast<UWORD8*>(out_v_plane);
            
            for (i4_j = 0; i4_j < (out_frame_height >> 1); i4_j++) {
                for (i4_i = 0; i4_i < (out_frame_width >> 1); i4_i++) {
                    *pu1_out++ = *pu1_chrm_u++;
                    *pu1_out++ = *pu1_chrm_v++;
                }
                pu1_out -= out_frame_width;
                pu1_out += out_str;
            }
        }
        
        // Free output buffers
        free(p_out_cb_plane_local);
        free(p_out_cr_plane_local);
    }
    
    // Free local buffers if they were allocated
    if (p_inp_cb_plane_local) {
        free(p_inp_cb_plane_local);
    }
    if (p_inp_cr_plane_local) {
        free(p_inp_cr_plane_local);
    }
    if (pv_scratch_buffer) {
        free(pv_scratch_buffer);
    }
    
    std::cout << "Done buffer release" << std::endl;
    return 0;
}
