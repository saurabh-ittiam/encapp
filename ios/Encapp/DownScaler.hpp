#include "itt_scaler.h"  // Ensure this header file is correctly included and located

#ifndef ENCAPP_JNIDownScaler_HPP
#define ENCAPP_JNIDownScaler_HPP

#ifdef __cplusplus
extern "C" {
#endif

extern int DownScaler(void* y_plane, void* u_plane, void* v_plane,
                      void* out_y_plane, void* out_u_plane, void* out_v_plane,
                      int inp_frame_width, int inp_frame_height,
                      int inp_y_stride, int inp_uv_stride,
                      int out_frame_width, int out_frame_height,
                      int out_y_stride, int out_uv_stride,
                      int inp_pixel_format, int out_pixel_format,
                      const char* downscaleflag);

WORD32 down_sample_8tap_filt(UWORD8 *pu1_y_buf,
                             UWORD8 *pu1_dsampled_img,
                             UWORD32 u4_in_img_width,
                             UWORD32 u4_in_img_height,
                             UWORD32 u4_in_img_stride,
                             UWORD32 u4_out_image_width,
                             UWORD32 u4_out_image_height,
                             UWORD32 u4_out_image_stride,
                             void *pv_scratch_buffer);

#ifdef __cplusplus
}
#endif

#endif // ENCAPP_JNIDownScaler_HPP
