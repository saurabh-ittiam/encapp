/*****************************************************************************/
/*                                                                           */
/*                               Ittiam Downscaler                           */
/*                     ITTIAM SYSTEMS PVT LTD, BANGALORE                     */
/*                             COPYRIGHT(C) 2005                             */
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
/*  File Name         : itt_scaler.c                                         */
/*                                                                           */
/*  Description       : This file contains the functions to get a scaled     */
/*                      version of given image                               */
/*                                                                           */
/*  List of Functions : get_coeff,                                             */
/*                      get_num_pels,                                        */
/*                      down_sample_8tap_filt                                */
/*                                                                           */
/*  Issues / Problems : None                                                 */
/*                                                                           */
/*  Revision History  :                                                      */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes (Describe the changes made)  */
/*         15 11 2011   Ittiam          Draft                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/* File Includes                                                             */
/*****************************************************************************/
/*****************************************************************************/
/* Standard Include Files                                                    */
/*****************************************************************************/
//#include <stdio.h>
#include <string.h>
//#include <stdlib.h>
//#include <math.h>
//#include <stdint.h>
//#include <stddef.h>

/*****************************************************************************/
/* include file                                                              */
/*****************************************************************************/
#include "itt_scaler.h"

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  Function Name : convolve                                                 */
/*                                                                           */
/*  Description   :                                                          */
/*                                                                           */
/*  Inputs        :                                                          */
/*  Globals       :                                                          */
/*                                                                           */
/*  Processing    :                                                          */
/*                                                                           */
/*  Outputs       :                                                          */
/*                                                                           */
/*  Returns       :                                                          */
/*                                                                           */
/*  Issues        :                                                          */
/*                                                                           */
/*  Revision History:                                                        */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes (Describe the changes made)  */
/*         20 05 2007   Ittiam          Draft                                */
/*                                                                           */
/*****************************************************************************/

void get_lanc_filt(float cutoff_freq,float *filt_coeff)
{
    WORD8 x[7];
    WORD8 i,j;
    float Lf[7];
    float Lw[7];
    float coeff[7];
    float sum = 0;
    float temp;
    
    /* removed redundent i = 7 */
    for(i=0,j=-3;(i<7 && j<=3);i++,j++)
    {
        x[i] = j;
    }

    for(i=0;i<3;i++)
    {
        /* sinc(x) = sin(pi*x)/(pi*x) */
        Lf[i] = (sin((cutoff_freq * PI * x[i])))/(cutoff_freq * PI * x[i]);
        /* sinc(x/a) = sin(pi*x/a)/(pi*x/a) , where a = 3 */
        Lw[i] = (sin((cutoff_freq * PI * x[i])/3))/(cutoff_freq * PI * x[i]/3);
    }

    Lf[3] = 1;
    Lw[3] = 1;

    for (i = 4; i < 7; i++)
    {
        /* sinc(x) = sin(pi*x)/(pi*x) */
        Lf[i] = (sin((cutoff_freq * PI * x[i]))) / (cutoff_freq * PI * x[i]);
        /* sinc(x/a) = sin(pi*x/a)/(pi*x/a) , where a = 3 */
        Lw[i] = (sin((cutoff_freq * PI * x[i]) / 3)) / (cutoff_freq * PI * x[i] / 3);
    }

    for(i=0;i<7;i++)
    {
        /* sinc(x) * sinc(x/a) */
        coeff[i] = ((Lf[i]) * (Lw[i]));
        sum += coeff[i];
    }

    for(i=0;i<7;i++)
    {
        coeff[i] /= sum;
        temp = (coeff[i] * 128);
        filt_coeff[i] = temp;
    }

}

/*****************************************************************************/
/*                                                                           */
/*  Function Name : convolve                                                 */
/*                                                                           */
/*  Description   :                                                          */
/*                                                                           */
/*  Inputs        :                                                          */
/*  Globals       :                                                          */
/*                                                                           */
/*  Processing    :                                                          */
/*                                                                           */
/*  Outputs       :                                                          */
/*                                                                           */
/*  Returns       :                                                          */
/*                                                                           */
/*  Issues        :                                                          */
/*                                                                           */
/*  Revision History:                                                        */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes (Describe the changes made)  */
/*         20 05 2007   Ittiam          Draft                                */
/*                                                                           */
/*****************************************************************************/
void convolve(float *pos_tap, float *f, WORD8 *filt)
{
   WORD8 i,j;
   float filt_coeff[8];
   float filt1[8];
   float sum = 0;
   WORD16 sum_int = 0;
   for(i=0;i<8;i++)
   {
       filt_coeff[i] = 0;
       for(j=0;j<=i;j++)
           if((j<=1) &&((i-j)<7))
           filt_coeff[i] += (pos_tap[j] * f[i-j]);
   }

   for(i=0;i<8;i++)
   {
       if(filt_coeff[i]-(int)(filt_coeff[i])<0.5)
       filt[i] = filt_coeff[i];
       else
           filt[i] = (filt_coeff[i] + 1);

       sum += filt[i];
   }

   for(i=0;i<8;i++)
   {
          filt1[i] = ((float)filt[i])/sum;
          filt[i] = (filt1[i] * 128);
          sum_int += filt[i];
   }

   if(sum_int !=128)
   {
       if(sum_int < 128)
       {
           filt[2] = filt[2] + ((128 - sum_int)>>1);
           if((filt[7] == filt[0]) && (filt[6] == filt[1]))
               filt[5] = filt[5] + ((128 - sum_int)>>1);
           else
               filt[4] = filt[4] + ((128 - sum_int)>>1);
           if(sum_int & 1)
               filt[3] = filt[3] + (128 - sum_int) - (((128 - sum_int)>>1)<<1);
       }
       else
       {
           filt[2] = filt[2] - ((sum_int - 128)>>1);
           if((filt[7] == filt[0]) && (filt[6] == filt[1]))
               filt[5] = filt[5] - ((sum_int - 128)>>1);
           else
               filt[4] = filt[4] - ((sum_int - 128)>>1);
           if(sum_int & 1)
               filt[3] = filt[3] - ((sum_int - 128) - (((sum_int - 128)>>1)<<1));
       }
   }
}

/*****************************************************************************/
/*                                                                           */
/*  Function Name : get_coeff                                                */
/*                                                                           */
/*  Description   : Determine the filter tap for the given scaling ratio     */
/*                                                                           */
/*  Inputs        : i4_down_scale_factor - down scale factor in SC_Q_FORMAT  */
/*                                         format                            */
/*                  pi1_filter_bank -  fills in the filter coefficients for  */
/*                  the specified i4_down_scale_factor                       */
/*                                                                           */
/*  Globals       : g_dnscale_8tap_coeffs                                    */
/*                                                                           */
/*  Processing    : Determines the filter bank to be used and copies it to   */
/*                  the pi1_filter_bank buffer                               */
/*                                                                           */
/*  Outputs       :                                                          */
/*                                                                           */
/*  Returns       :                                                          */
/*                                                                           */
/*  Issues        :                                                          */
/*                                                                           */
/*  Revision History:                                                        */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes (Describe the changes made)  */
/*         15 11 2011   Ittiam          Draft                                */
/*                                                                           */
/*****************************************************************************/
WORD32 get_coeff(ptrdiff_t i4_downscale_factor,/* Q16 format */
                 WORD8 *pi1_filter_bank)
{
    float cutoff_freq;
    float filt_coeff[7];
    UWORD8 i,j,k;
    float pos_tap[2];
    WORD8 coeff[8];
    WORD8 coeff_high_cutoff[64] =
    {    9,    -18, 25,    96,    25,    -18, 9,    0,
         8,    -14, 20,    86,    33,    -12, 6,    1,
         7,    -11, 14,    78,    43,    -7,     2,    2,
         6,    -8,      9,    69,    52,    -2,    -1,    3,
         4,    -4,      4,    60,    60,    4,    -4,    4,
         3,    -1,     -2,    52,    69,    9,    -8,    6,
         2,    2,     -7,    43,    78,    14,    -11,7,
         1,    6,    -12,    33,    86,    20,    -14,8
    };

    cutoff_freq = (float)(1<< SC_Q_FORMAT)/i4_downscale_factor;

    if(cutoff_freq > 0.984375)
    {
        memcpy(pi1_filter_bank, coeff_high_cutoff, 64 * sizeof(WORD8));
    }
    else
    {
        get_lanc_filt(cutoff_freq,filt_coeff);

        for(i=0,j=8;(i<8 && j>0);i++,j--)
        {
            pos_tap[0] = (j/8.0);
            pos_tap[1] = (i/8.0);
            convolve(pos_tap,filt_coeff,coeff);
            for(k=0;k<8;k++)
                *pi1_filter_bank++ = coeff[k];
        }
    }
    return 0;
}

/*****************************************************************************/
/*                                                                           */
/*  Function Name : get the number of output pixels at the boundary          */
/*                                                                           */
/*  Description   : The routine deretermines the number of output pixels     */
/*                  which do not have enough neighbours in the given         */
/*                  dimension( Horizontal or vertical)                       */
/*                                                                           */
/*  Inputs        : i4_width : src image index where all neighbors are       */
/*                             avaliable                                     */
/*                  i4_offset: src image start offset in Q16 format          */
/*                  u4_inc   : src image step in Q16 format                  */
/*  Globals       :                                                          */
/*                                                                           */
/*  Processing    :                                                          */
/*                                                                           */
/*  Outputs       :                                                          */
/*                                                                           */
/*  Returns       :                                                          */
/*                                                                           */
/*  Issues        :                                                          */
/*                                                                           */
/*  Revision History:                                                        */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes (Describe the changes made)  */
/*         20 05 2007   Ittiam          Draft                                */
/*                                                                           */
/*****************************************************************************/
WORD32 get_num_pels(WORD32 i4_width, ptrdiff_t i4_offset, ptrdiff_t u4_inc)
{
    WORD32  i4_res;

    if(0 >= i4_width)
    {
        return(0);
    }
    if(0 == u4_inc)
    {
        return(0);
    }

    i4_res = 0;
    i4_width = i4_width << SC_Q_FORMAT;
    /*************************************************************************/
    /* this is required since if width = 1, then it should be 65535 and not  */
    /* 65536. i.e Shouldn't include the last pixel                           */
    /*************************************************************************/
    i4_width -= 1;
    if(i4_offset > i4_width)
    {
        return(0);
    }
    /* width is assumed to be larger than the fractional phase  offset */
    i4_width -= i4_offset;
    /* now need to calculate the quotient when i4_width is divided by i4_inc */
    i4_res = (i4_width / u4_inc) + 1;

    return(i4_res);
}

/*****************************************************************************/
/*                                                                           */
/*  Function Name : down_sample_8tap_filt                                    */
/*                                                                           */
/*  Description   : This function performs scaling by a factor ranging       */
/*                  from (2/16) to (15/16)                                   */
/*                                                                           */
/*  Inputs        : pu1_y_buf - Pointer to Luma data of input image.         */
/*                  pu1_dsampled_img - Pointer where output down scaled Luma */
/*                                      data has to be stored.               */
/*                  u4_in_img_width - input image width.                     */
/*                  u4_in_img_height - input image height.                   */
/*                  u4_in_img_stride - input image stride.                   */
/*                  u4_out_image_width - output image width.                 */
/*                  u4_out_image_height - output image height.               */
/*                     u4_out_image_stride - output image stride.                 */
/*                  pv_scratch_buffer - scratch buffer of size               */
/*                                         CALC_SCRATCH_BUF_SIZE_DOWNSCALE      */
/*                                                                           */
/*  Globals       : None                                                     */
/*                                                                           */
/*  Processing    : Performs horizontal filtering first followed by vertical */
/*                  filtering                                                */
/*                                                                           */
/*                                                                           */
/*  Outputs       : Down scaled luma data.                                   */
/*                                                                           */
/*  Returns       : returns offset of first sampled point in Q16 format in   */
/*                  source image.    */
/*                                                                           */
/*  Issues        : None                                                     */
/*                                                                           */
/*  Revision History:                                                        */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes (Describe the changes made)  */
/*         03 11 2009   Ittiam          Draft                                */
/*                                                                           */
/*****************************************************************************/

WORD32 down_sample_8tap_filt(UWORD8 *pu1_y_buf,
                             UWORD8 *pu1_dsampled_img,
                             UWORD32 u4_in_img_width,
                             UWORD32 u4_in_img_height,
                             UWORD32 u4_in_img_stride,
                             UWORD32 u4_out_image_width,
                             UWORD32 u4_out_image_height,
                             UWORD32 u4_out_image_stride,
                             void *pv_scratch_buffer)
{

    ptrdiff_t i4_horiz_step =
        (u4_in_img_width << SC_Q_FORMAT) / u4_out_image_width;
    ptrdiff_t i4_vert_step =
        (u4_in_img_height << SC_Q_FORMAT) / u4_out_image_height;
    ptrdiff_t i4_horz_init_offset = (i4_horiz_step - 65536) >> 1;
    ptrdiff_t i4_vert_init_offset = (i4_vert_step - 65536) >> 1;
    WORD8  *pi1_h_filt_grp, *pi1_v_filt_grp;
    UWORD32 u4_row, u4_col;
    WORD32 i4_start_boundary_h;
    WORD32 i4_end_boundary_h;
    UWORD8 *pu1_dest, *pu1_src;
    ptrdiff_t i4_horz_offset, i4_vert_offset;
    ptrdiff_t i4_offset_ret_value;
    WORD32 *temp_horz_filter_out;
    WORD32 i4_extra_rows=0,i4_total_shifts=0 ;
    WORD8  vert_filter_co_effs[8];
    void* temp_buf;
    WORD32 i4_stride_offset = u4_out_image_stride - u4_out_image_width;
    
    /* Note: assuming i4_horz_init_offset == i4_horz_init_offset hence      */
    /* returning 1 value                                                    */
    i4_offset_ret_value = i4_horz_init_offset;
    /************************************************************************/
    /* test parameters: filtering limits upper/lower                        */
    /************************************************************************/
    if((i4_horiz_step > SC_RATIO_TH_2) || (i4_horiz_step < SC_RATIO_TH_16) ||
       (i4_vert_step > SC_RATIO_TH_2) || (i4_vert_step < SC_RATIO_TH_16))
    {
        return -1;
    }

    /* Allocating memory for temperory horizontal filter output */
    temp_buf = (UWORD8 *)pv_scratch_buffer + 64*2;
    temp_horz_filter_out = temp_buf;
    
    /*Code for ratio 16/16 */
    if((u4_in_img_height == u4_out_image_height) &&
    (u4_in_img_width == u4_out_image_width))
    {
        UWORD32 i;
        UWORD8 *temp_y_buf = pu1_y_buf;
        
        for(i=0; i < u4_out_image_height; i++)
        {
            memcpy(pu1_dsampled_img, temp_y_buf ,u4_out_image_width);
            pu1_dsampled_img += u4_out_image_stride;
            temp_y_buf += u4_in_img_stride;
            
        }
        return 0;
    }

    /*************************************************************************/
    /* scratch buffer for filters copy the filter group based on scaling     */
    /* factor                                                                */
    /*************************************************************************/
    pi1_h_filt_grp = (WORD8 *)pv_scratch_buffer;
    pi1_v_filt_grp = (WORD8 *)((WORD8 *)pv_scratch_buffer + 64 * sizeof(WORD8));

    get_coeff(i4_horiz_step,  pi1_h_filt_grp);

    get_coeff(i4_vert_step,   pi1_v_filt_grp);

    /*************************************************************************/
    /* Determine the boundary where neighbors wont be available for filtering*/
    /*************************************************************************/
    i4_start_boundary_h = get_num_pels(4, i4_horz_init_offset, i4_horiz_step);
    i4_end_boundary_h   = get_num_pels(u4_in_img_width - 4,
                                        i4_horz_init_offset,
                                        i4_horiz_step);

    /*************************************************************************/
    /* For horizontal filtering at a pixel position                          */
    /* 1 current pixel position, 3 right pixels and 3 left pixels are needed */
    /*************************************************************************/

    i4_horz_init_offset = (i4_horz_init_offset - (3 << 16));
    i4_vert_init_offset = (i4_vert_init_offset - (3 << 16));

    pu1_dest = pu1_dsampled_img + 0 * u4_out_image_width;
    i4_vert_offset = i4_vert_init_offset;

    /* Initial 8 rows horizontal filter output calculations and storing */
    {
        WORD32 i4_row_cnt ;

        for(i4_row_cnt=0;i4_row_cnt<8;i4_row_cnt++)
        {
            ptrdiff_t i4_offset_out =  i4_row_cnt*u4_out_image_width ;
            ptrdiff_t i4_offset_in  =  i4_row_cnt*u4_in_img_stride;
            i4_horz_offset = i4_horz_init_offset;

            pu1_src =  pu1_y_buf +
                    ((i4_vert_offset >> 16) * u4_in_img_stride);
            pu1_src += i4_offset_in;

            if(pu1_src < pu1_y_buf)
                pu1_src    = pu1_y_buf;
            
            for(u4_col=0 ;(WORD32)u4_col < i4_start_boundary_h ; u4_col++)
            {
                UWORD8 *pu1_signal = &pu1_src[i4_horz_offset >> 16];
                UWORD8 u1_filter_id_h  = (i4_horz_offset >> 13) & 0x7;
                WORD8 *pi1_filter_h;
                UWORD8 *start_ptr = &pu1_src[0];
                WORD32 i4_val_r=0;
                ptrdiff_t horz_edge_pixels;
                WORD32 i;

                pi1_filter_h = pi1_h_filt_grp + (u1_filter_id_h << 3);

                horz_edge_pixels = (start_ptr - pu1_signal);
                
                for(i=0;(i<horz_edge_pixels) && (i<8); i++)
                    i4_val_r  += *start_ptr * pi1_filter_h[i];

                for(;i<8; i++)
                    i4_val_r  += *start_ptr++ * pi1_filter_h[i];

                    i4_val_r=i4_val_r >>1;
                temp_horz_filter_out[u4_col + i4_offset_out] = i4_val_r;
                /* update horz offset */
                i4_horz_offset += i4_horiz_step;
            }

            for(;(WORD32)u4_col < i4_end_boundary_h ; u4_col++)
            {
                UWORD8 *pu1_signal = &pu1_src[i4_horz_offset >> 16];
                UWORD8 u1_filter_id_h  = (i4_horz_offset >> 13) & 0x7;
                WORD8 *pi1_filter_h;

                WORD32 i4_val_r;

                pi1_filter_h = pi1_h_filt_grp + (u1_filter_id_h << 3);
                /*  horizontal filter  for 8 columns */
                i4_val_r   = pu1_signal[0] * pi1_filter_h[0];
                i4_val_r  += pu1_signal[1] * pi1_filter_h[1];
                i4_val_r  += pu1_signal[2] * pi1_filter_h[2];
                i4_val_r  += pu1_signal[3] * pi1_filter_h[3];
                i4_val_r  += pu1_signal[4] * pi1_filter_h[4];
                i4_val_r  += pu1_signal[5] * pi1_filter_h[5];
                i4_val_r  += pu1_signal[6] * pi1_filter_h[6];
                i4_val_r  += pu1_signal[7] * pi1_filter_h[7];
                i4_val_r=i4_val_r >>1;
                temp_horz_filter_out[u4_col + i4_offset_out] = i4_val_r;
                /* update horz offset */
                i4_horz_offset += i4_horiz_step;
            }
            /* Handling edge pixels for which 8 neighborhood is not available */
            for(; u4_col < u4_out_image_width; u4_col++)
            {
                UWORD8 *pu1_signal = &pu1_src[i4_horz_offset >> 16];
                UWORD8 u1_filter_id_h  = (i4_horz_offset >> 13) & 0x7;
                WORD8 *pi1_filter_h;
                UWORD8 *end_ptr =  &pu1_src[u4_in_img_width-1];
                ptrdiff_t i,horz_edge_pixels;
                WORD32 i4_val_r = 0;

                pi1_filter_h = pi1_h_filt_grp + (u1_filter_id_h << 3);
                /*  horizontal filter  for 8 columns */
                horz_edge_pixels = (end_ptr - pu1_signal);
                for(i=0;(i<horz_edge_pixels) && (i<8); i++)
                    i4_val_r  += *pu1_signal++ * pi1_filter_h[i];

                for(;i<8; i++)
                    i4_val_r  += *end_ptr * pi1_filter_h[i];
                i4_val_r=i4_val_r >>1;
                temp_horz_filter_out[u4_col + i4_offset_out] = i4_val_r;
                /* update horz offset */
                i4_horz_offset += i4_horiz_step;
            }
        }
    }

    /* Initial rows for which 8 neighborhood is not available vertically */
    i4_total_shifts = 0;

    /* Resizing Starts */
    for(u4_row=0; u4_row < u4_out_image_height ; u4_row++)
    {
        WORD32 i4_i;
        WORD8 *pi1_filter_v;
        UWORD8 u1_filter_id_v  = (i4_vert_offset >> 13) & 0x7;


        pi1_filter_v = pi1_v_filt_grp + (u1_filter_id_v << 3);
        pu1_src = pu1_y_buf +
                    ((i4_vert_offset >> 16) * u4_in_img_stride);
        i4_horz_offset = i4_horz_init_offset;

        /* Shifting of vertical co-effs */
        {
            /* Shifting the vert co-effs */
            WORD32 val = (8 - (i4_total_shifts&0x7))&0x7;
            vert_filter_co_effs[0] = pi1_filter_v[val];
            vert_filter_co_effs[1] = pi1_filter_v[(val+1)&0x7];
            vert_filter_co_effs[2] = pi1_filter_v[(val+2)&0x7];
            vert_filter_co_effs[3] = pi1_filter_v[(val+3)&0x7];
            vert_filter_co_effs[4] = pi1_filter_v[(val+4)&0x7];
            vert_filter_co_effs[5] = pi1_filter_v[(val+5)&0x7];
            vert_filter_co_effs[6] = pi1_filter_v[(val+6)&0x7];
            vert_filter_co_effs[7] = pi1_filter_v[(val+7)&0x7];
        }

        for(u4_col=0 ;u4_col < u4_out_image_width ; u4_col++)
        {
            WORD32 i4_vert_index ;
            WORD32 i4_out_pix;

            /* Doing vertical filtering and writing to output */
            i4_vert_index=u4_col;
            i4_out_pix  = temp_horz_filter_out[i4_vert_index]   * vert_filter_co_effs[0];
            i4_vert_index += u4_out_image_width;
            i4_out_pix += temp_horz_filter_out[i4_vert_index]   * vert_filter_co_effs[1];
            i4_vert_index += u4_out_image_width;
            i4_out_pix += temp_horz_filter_out[i4_vert_index]   * vert_filter_co_effs[2];
            i4_vert_index += u4_out_image_width;
            i4_out_pix += temp_horz_filter_out[i4_vert_index]   * vert_filter_co_effs[3];
            i4_vert_index += u4_out_image_width;
            i4_out_pix += temp_horz_filter_out[i4_vert_index]   * vert_filter_co_effs[4];
            i4_vert_index += u4_out_image_width;
            i4_out_pix += temp_horz_filter_out[i4_vert_index]   * vert_filter_co_effs[5];
            i4_vert_index += u4_out_image_width;
            i4_out_pix += temp_horz_filter_out[i4_vert_index]   * vert_filter_co_effs[6];
            i4_vert_index += u4_out_image_width;
            i4_out_pix += temp_horz_filter_out[i4_vert_index]   * vert_filter_co_effs[7];

            i4_out_pix = (i4_out_pix + (1 << 12)) >> 13;

            /* store the clipped( 0 to 255) output */
            *pu1_dest++ = (UWORD8)((i4_out_pix >> 8) ?
                    (UWORD8)((i4_out_pix >> 31) ^ 0xFFFFFFFF) : (UWORD8)i4_out_pix);

            /* update horz offset */
            i4_horz_offset += i4_horiz_step;
        }

        pu1_dest = pu1_dest + i4_stride_offset;

        /* Updating temp_horz_filter_out for next row */
        i4_extra_rows       = ((i4_vert_offset+i4_vert_step)>>16) - ((i4_vert_offset)>>16);

        for(i4_i=0;i4_i<i4_extra_rows ;i4_i++)
        {
            WORD32 i4_offset_out = 0;

            i4_offset_out  =  ((i4_total_shifts + i4_i)&0x7)*u4_out_image_width ;

            i4_horz_offset = i4_horz_init_offset;

            pu1_src = pu1_y_buf +
                        (((i4_vert_offset) >> 16) * u4_in_img_stride);

            /* Checking for height out of range */
            if((((i4_vert_offset) >> 16)+8+i4_i) > (WORD32)(u4_in_img_height-1))
            {
                pu1_src = pu1_y_buf + (u4_in_img_height-1) * u4_in_img_stride;
            }
            else
                pu1_src += (u4_in_img_stride<<3) + i4_i * u4_in_img_stride;


            for(u4_col=0 ;(WORD32)u4_col < i4_start_boundary_h ; u4_col++)
            {
                UWORD8 *pu1_signal = &pu1_src[i4_horz_offset >> 16];
                UWORD8 u1_filter_id_h  = (i4_horz_offset >> 13) & 0x7;
                WORD8 *pi1_filter_h;
                UWORD8 *start_ptr = &pu1_src[0];
                WORD32 i4_val_r=0;
                ptrdiff_t horz_edge_pixels;
                WORD32 i_loop_ctr;

                pi1_filter_h = pi1_h_filt_grp + (u1_filter_id_h << 3);

                horz_edge_pixels = (start_ptr - pu1_signal);

                for(i_loop_ctr=0;(i_loop_ctr<horz_edge_pixels) && (i_loop_ctr<8); i_loop_ctr++)
                    i4_val_r  += *start_ptr * pi1_filter_h[i_loop_ctr];

                for(;i_loop_ctr<8; i_loop_ctr++)
                    i4_val_r  += *start_ptr++ * pi1_filter_h[i_loop_ctr];
                i4_val_r=i4_val_r >>1;
                temp_horz_filter_out[u4_col + i4_offset_out] = i4_val_r;
                /* update horz offset */
                i4_horz_offset += i4_horiz_step;
            }
            for(;(WORD32)u4_col < i4_end_boundary_h ; u4_col++)
            {
                UWORD8 *pu1_signal= &pu1_src[i4_horz_offset >> 16];
                UWORD8 u1_filter_id_h  = (i4_horz_offset >> 13) & 0x7;
                WORD8 *pi1_filter_h;
                WORD32 i4_val_r;

                pi1_filter_h = pi1_h_filt_grp + (u1_filter_id_h << 3);
                /*  horizontal filter  for 8 columns */
                i4_val_r   = pu1_signal[0] * pi1_filter_h[0];
                i4_val_r  += pu1_signal[1] * pi1_filter_h[1];
                i4_val_r  += pu1_signal[2] * pi1_filter_h[2];
                i4_val_r  += pu1_signal[3] * pi1_filter_h[3];
                i4_val_r  += pu1_signal[4] * pi1_filter_h[4];
                i4_val_r  += pu1_signal[5] * pi1_filter_h[5];
                i4_val_r  += pu1_signal[6] * pi1_filter_h[6];
                i4_val_r  += pu1_signal[7] * pi1_filter_h[7];
                i4_val_r=i4_val_r >>1;
                temp_horz_filter_out[u4_col + i4_offset_out] = i4_val_r;
                /* update horz offset */
                i4_horz_offset += i4_horiz_step;
            }

            /* Handling edge pixels for which 8 neighborhood is not available */
            for(; u4_col < u4_out_image_width; u4_col++)
            {
                UWORD8 *pu1_signal = &pu1_src[i4_horz_offset >> 16];
                UWORD8 u1_filter_id_h  = (i4_horz_offset >> 13) & 0x7;
                WORD8 *pi1_filter_h;
                UWORD8 *end_ptr =  &pu1_src[u4_in_img_width-1];             // move out of the loop
                WORD32 i_counter,i4_val_r = 0,horz_edge_pixels;

                pi1_filter_h = pi1_h_filt_grp + (u1_filter_id_h << 3);
                /*  horizontal filter  for 8 columns */
                horz_edge_pixels = (end_ptr - pu1_signal);
                for(i_counter=0;i_counter<(horz_edge_pixels) && (i_counter<8); i_counter++)
                    i4_val_r  += *pu1_signal++ * pi1_filter_h[i_counter];

                for(;i_counter<8; i_counter++)
                    i4_val_r  += *end_ptr * pi1_filter_h[i_counter];
                i4_val_r=i4_val_r >>1;
                temp_horz_filter_out[u4_col + i4_offset_out] = i4_val_r;
                /* update horz offset */
                i4_horz_offset += i4_horiz_step;
            }
        }
        i4_vert_offset      += i4_vert_step;
        i4_total_shifts     += i4_extra_rows;
    }

    return i4_offset_ret_value;
}

