/*****************************************************************************/
/*                                                                           */
/*                           Ittiam Downscaler                               */
/*                     ITTIAM SYSTEMS PVT LTD, BANGALORE                     */
/*                             COPYRIGHT(C) 2011                             */
/*                                                                           */
/*  This program  is  proprietary to  Ittiam  Systems  Private  Limited  and */
/*  is protected under Indian  Copyright Law as an unpublished work. Its use */
/*  and  disclosure  is  limited by  the terms  and  conditions of a license */
/*  agreement. It may not be copied or otherwise  reproduced or disclosed to */
/*  persons outside the licensee's organization except in accordance with the*/
/*  terms  and  conditions   of  such  an  agreement.  All  copies  and      */
/*  reproductions shall be the property of Ittiam Systems Private Limited and*/
/*  must bear this notice in its entirety.                                   */
/*                                                                           */
/*****************************************************************************/
/*****************************************************************************/
/*                                                                           */
/*  File Name         : itt_scaler.h                                         */
/*                                                                           */
/*  Description       : This file contains definition and prototypes required*/
/*                      to get a scaled version of given image               */
/*                                                                           */
/*  List of Functions : down_sample_8tap_filt                                */
/*                                                                           */
/*  Issues / Problems : None                                                 */
/*                                                                           */
/*  Revision History  :                                                      */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes (Describe the changes made)  */
/*         15 11 2011   Ittiam          Draft                                */
/*                                                                           */
/*****************************************************************************/
#ifndef _ITT_SCALER_H_
#define _ITT_SCALER_H_

#include <math.h>

//#include <android/log.h>
#define LOG_TAG "MyApp_Native"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

/*****************************************************************************/
/* Constant Macros                                                           */
/*****************************************************************************/
/* Limits for different data types */
#define MAX_32 0x7fffffff

#define MIN_32 0x80000000

#define SC_Q_FORMAT 16

#define PI 3.141592

#define SC_RATIO_TH_2    524288
#define SC_RATIO_TH_16    65536


/*****************************************************************************/
/* Typedefs                                                                  */
/*****************************************************************************/

/*****************************************************************************/
/* Unsigned data types                                                       */
/*****************************************************************************/
typedef uint8_t                UWORD8;
typedef uint16_t            UWORD16;
typedef uint32_t            UWORD32;
typedef uint64_t            UWORD64;
typedef int64_t                WORD64;

/*****************************************************************************/
/* Signed data types                                                         */
/*****************************************************************************/
typedef int8_t                WORD8;
typedef int16_t                WORD16;
typedef int32_t                WORD32;

/*****************************************************************************/
/* Miscellaneous data types                                                  */
/*****************************************************************************/
//typedef int8_t                BOOL;
typedef char            *pSTRING;
typedef char            CHAR;
typedef float           FLOAT32;


/*****************************************************************************/
/* Functional Macros                                                         */
/*****************************************************************************/
#define CALC_SCRATCH_BUF_SIZE_DOWNSCALE(out_image_width)  (64*3 + \
    2*8*out_image_width*sizeof(UWORD16) + 400 )


/*****************************************************************************/
/* Function Declarations                                                     */
/*****************************************************************************/
/*WORD32 down_sample_8tap_filt(UWORD8 *pu1_y_buf,
                             UWORD8 *pu1_dsampled_img,
                             UWORD32 u4_in_img_width,
                             UWORD32 u4_in_img_height,
                             UWORD32 u4_in_img_stride,
                             UWORD32 u4_out_image_width,
                             UWORD32 u4_out_image_height,
                             UWORD32 u4_out_image_stride,
                             void *pv_scratch_buffer);*/
int bicubic_resize(const unsigned char* _src, unsigned char* _dst,
                   int iwidth, int iheight, int istride, int dwidth,
                   int dheight, int dstride);

#endif /* _ITT_SCALER_H_ */
