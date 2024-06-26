package com.facebook.encapp;

import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
import android.media.MediaMuxer;
import android.os.Environment;
import android.util.Log;
import android.util.Size;

import androidx.annotation.NonNull;

import com.facebook.encapp.proto.Test;
import com.facebook.encapp.utils.FileReader;
import com.facebook.encapp.utils.SizeUtils;
import com.facebook.encapp.utils.Statistics;
import com.facebook.encapp.utils.TestDefinitionHelper;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.channels.FileChannel;
import java.util.Arrays;

/**
 * Created by jobl on 2018-02-27.
 */

class BufferX264Encoder extends Encoder {
    protected static final String TAG = "encapp.buffer_x264_encoder";

    public static class X264Nal {
        int i_ref_idc;  /* nal_priority_e */
        int i_type;     /* nal_unit_type_e */
        int b_long_startcode;
        int i_first_mb; /* If this NAL is a slice, the index of the first MB in the slice. */
        int i_last_mb;  /* If this NAL is a slice, the index of the last MB in the slice. */

        int     i_payload;
        byte[] p_payload;

        int i_padding;         /* Size of padding in bytes. */

        // Constructor
        public X264Nal(int i_ref_idc, int i_type, int b_long_startcode, int i_first_mb, int i_last_mb, int i_payload, byte[] p_payload, int i_padding) {
            this.i_ref_idc = i_ref_idc;
            this.i_type = i_type;
            this.b_long_startcode = b_long_startcode;
            this.i_first_mb = i_first_mb;
            this.i_last_mb = i_last_mb;
            this.i_payload = i_payload;
            this.p_payload = p_payload;
            this.i_padding = i_padding;
        }
    }

    public static class X264Zone {
        int i_start, i_end; /* range of frame numbers */
        int b_force_qp; /* whether to use qp vs bitrate factor */
        int i_qp;
        float f_bitrate_factor;
        X264Params param;

        public X264Zone(int i_start, int i_end, int b_force_qp, int i_qp, float f_bitrate_factor, X264Params param) {
            this.i_start = i_start;
            this.i_end = i_end;
            this.b_force_qp = b_force_qp;
            this.i_qp = i_qp;
            this.f_bitrate_factor = f_bitrate_factor;
            this.param = param;
        }
    }

    public static class X264Params {
        /* CPU flags */
        int         cpu;
        int         i_threads;           /* encode multiple frames in parallel */
        int         i_lookahead_threads; /* multiple threads for lookahead analysis */
        int         b_sliced_threads;  /* Whether to use slice-based threading. */
        int         b_deterministic; /* whether to allow non-deterministic optimizations when threaded */
        int         b_cpu_independent; /* force canonical behavior rather than cpu-dependent optimal algorithms */
        int         i_sync_lookahead; /* threaded lookahead buffer */

        /* Video Properties */
        int         i_width;
        int         i_height;
        int         i_csp;         /* CSP of encoded bitstream */
        int         i_bitdepth;
        int         i_level_idc;
        int         i_frame_total; /* number of frames to encode if known, else 0 */
        int         i_nal_hrd;

        static class vui {
            /* they will be reduced to be 0 < x <= 65535 and prime */
            int         i_sar_height;
            int         i_sar_width;

            int         i_overscan;    /* 0=undef, 1=no overscan, 2=overscan */

            /* see h264 annex E for the values of the following */
            int         i_vidformat;
            int         b_fullrange;
            int         i_colorprim;
            int         i_transfer;
            int         i_colmatrix;
            int         i_chroma_loc;    /* both top & bottom */

            public vui(int i_sar_height, int i_sar_width, int i_overscan, int i_vidformat, int b_fullrange, int i_colorprim, int i_transfer, int i_colmatrix, int i_chroma_loc) {
                this.i_sar_height = i_sar_height;
                this.i_sar_width = i_sar_width;
                this.i_overscan = i_overscan;
                this.i_vidformat = i_vidformat;
                this.b_fullrange = b_fullrange;
                this.i_colorprim = i_colorprim;
                this.i_transfer = i_transfer;
                this.i_colmatrix = i_colmatrix;
                this.i_chroma_loc = i_chroma_loc;
            }
        }

        /* Bitstream parameters */
        int         i_frame_reference;  /* Maximum number of reference frames */
        int         i_dpb_size;         /* Force a DPB size larger than that implied by B-frames and reference frames.
         * Useful in combination with interactive error resilience. */
        int         i_keyint_max;       /* Force an IDR keyframe at this interval */
        int         i_keyint_min;       /* Scenecuts closer together than this are coded as I, not IDR. */
        int         i_scenecut_threshold; /* how aggressively to insert extra I frames */
        int         b_intra_refresh;    /* Whether or not to use periodic intra refresh instead of IDR frames. */

        int         i_bframe;   /* how many b-frame between 2 references pictures */
        int         i_bframe_adaptive;
        int         i_bframe_bias;
        int         i_bframe_pyramid;   /* Keep some B-frames as references: 0=off, 1=strict hierarchical, 2=normal */
        int         b_open_gop;
        int         b_bluray_compat;
        int         i_avcintra_class;

        int         b_deblocking_filter;
        int         i_deblocking_filter_alphac0;    /* [-6, 6] -6 light filter, 6 strong */
        int         i_deblocking_filter_beta;       /* [-6, 6]  idem */

        int         b_cabac;
        int         i_cabac_init_idc;

        int         b_interlaced;

//        void        (*pf_log)( void *, int i_level, const String    psz, va_list );
//        void        *p_log_private;
        int         i_log_level;
        int         b_full_recon;   /* fully reconstruct frames, even when not necessary for encoding */


        /* Encoder analyser parameters */
        static class analyse
        {
            long intra;     /* intra partitions */
            long inter;     /* inter partitions */

            int          b_transform_8x8;
            int          i_weighted_pred; /* weighting for P-frames */
            int          b_weighted_bipred; /* implicit weighting for B-frames */
            int          i_direct_mv_pred; /* spatial vs temporal mv prediction */
            int          i_chroma_qp_offset;

            int          i_me_method; /* motion estimation algorithm to use (X264_ME_*) */
            int          i_me_range; /* integer pixel motion estimation search range (from predicted mv) */
            int          i_mv_range; /* maximum length of a mv (in pixels). -1 = auto, based on level */
            int          i_mv_range_thread; /* minimum space between threads. -1 = auto, based on number of threads. */
            int          i_subpel_refine; /* subpixel motion estimation quality */
            int          b_chroma_me; /* chroma ME for subpel and mode decision in P-frames */
            int          b_mixed_references; /* allow each mb partition to have its own reference number */
            int          i_trellis;  /* trellis RD quantization */
            int          b_fast_pskip; /* early SKIP detection on P-frames */
            int          b_dct_decimate; /* transform coefficient thresholding on P-frames */
            int          i_noise_reduction; /* adaptive pseudo-deadzone */
            float        f_psy_rd; /* Psy RD strength */
            float        f_psy_trellis; /* Psy trellis strength */
            int          b_psy; /* Toggle all psy optimizations */

            int          b_mb_info;            /* Use input mb_info data in x264_picture_t */
            int          b_mb_info_update; /* Update the values in mb_info according to the results of encoding. */

            int          b_psnr;    /* compute and print PSNR stats */
            int          b_ssim;    /* compute and print SSIM stats */

            public analyse(long intra, long inter, int b_transform_8x8, int i_weighted_pred, int b_weighted_bipred, int i_direct_mv_pred,
                           int i_chroma_qp_offset, int i_me_method, int i_me_range, int i_mv_range, int i_mv_range_thread,
                           int i_subpel_refine, int b_chroma_me, int b_mixed_references, int i_trellis, int b_fast_pskip,
                           int b_dct_decimate, int i_noise_reduction, float f_psy_rd, float f_psy_trellis, int b_psy,
                           int b_mb_info, int b_mb_info_update, int b_psnr, int b_ssim) {
                this.intra = intra;
                this.inter = inter;
                this.b_transform_8x8 = b_transform_8x8;
                this.i_weighted_pred = i_weighted_pred;
                this.b_weighted_bipred = b_weighted_bipred;
                this.i_direct_mv_pred = i_direct_mv_pred;
                this.i_chroma_qp_offset = i_chroma_qp_offset;
                this.i_me_method = i_me_method;
                this.i_me_range = i_me_range;
                this.i_mv_range = i_mv_range;
                this.i_mv_range_thread = i_mv_range_thread;
                this.i_subpel_refine = i_subpel_refine;
                this.b_chroma_me = b_chroma_me;
                this.b_mixed_references = b_mixed_references;
                this.i_trellis = i_trellis;
                this.b_fast_pskip = b_fast_pskip;
                this.b_dct_decimate = b_dct_decimate;
                this.i_noise_reduction = i_noise_reduction;
                this.f_psy_rd = f_psy_rd;
                this.f_psy_trellis = f_psy_trellis;
                this.b_psy = b_psy;
                this.b_mb_info = b_mb_info;
                this.b_mb_info_update = b_mb_info_update;
                this.b_psnr = b_psnr;
                this.b_ssim = b_ssim;
            }
        }

        /* Rate control parameters */
        static class rc
        {
            int         i_rc_method;    /* X264_RC_* */
            int         i_qp_constant;  /* 0=lossless */
            int         i_qp_min;       /* min allowed QP value */
            int         i_qp_max;       /* max allowed QP value */
            int         i_qp_step;      /* max QP step between frames */
            int         i_bitrate;
            float       f_rf_constant;  /* 1pass VBR, nominal QP */
            float       f_rf_constant_max;  /* In CRF mode, maximum CRF as caused by VBV */
            float       f_rate_tolerance;
            int         i_vbv_max_bitrate;
            int         i_vbv_buffer_size;
            float       f_vbv_buffer_init; /* <=1: fraction of buffer_size. >1: kbit */
            float       f_ip_factor;
            float       f_pb_factor;
            int         b_filler;
            int         i_aq_mode;      /* psy adaptive QP. (X264_AQ_*) */
            float       f_aq_strength;
            int         b_mb_tree;      /* Macroblock-tree ratecontrol. */
            int         i_lookahead;
            int         b_stat_write;   /* Enable stat writing in psz_stat_out */
            String    psz_stat_out;  /* output filename (in UTF-8) of the 2pass stats file */
            int         b_stat_read;    /* Read stat from psz_stat_in and use it */
            String    psz_stat_in;   /* input filename (in UTF-8) of the 2pass stats file */
            float       f_qcompress;    /* 0.0 => cbr, 1.0 => constant qp */
            float       f_qblur;        /* temporally blur quants */
            float       f_complexity_blur; /* temporally blur complexity */
            X264Zone zones;         /* ratecontrol overrides */
            int         i_zones;        /* number of zone_t's */
            String    psz_zones;     /* alternate method of specifying zones */

            public rc(int i_rc_method, int i_qp_constant, int i_qp_min, int i_qp_max, int i_qp_step, int i_bitrate,
                      float f_rf_constant, float f_rf_constant_max, float f_rate_tolerance, int i_vbv_max_bitrate,
                      int i_vbv_buffer_size, float f_vbv_buffer_init, float f_ip_factor, float f_pb_factor, int b_filler,
                      int i_aq_mode, float f_aq_strength, int b_mb_tree, int i_lookahead, int b_stat_write,
                      String psz_stat_out, int b_stat_read, String psz_stat_in, float f_qcompress, float f_qblur,
                      float f_complexity_blur, X264Zone zones, int i_zones, String psz_zones) {
                this.i_rc_method = i_rc_method;
                this.i_qp_constant = i_qp_constant;
                this.i_qp_min = i_qp_min;
                this.i_qp_max = i_qp_max;
                this.i_qp_step = i_qp_step;
                this.i_bitrate = i_bitrate;
                this.f_rf_constant = f_rf_constant;
                this.f_rf_constant_max = f_rf_constant_max;
                this.f_rate_tolerance = f_rate_tolerance;
                this.i_vbv_max_bitrate = i_vbv_max_bitrate;
                this.i_vbv_buffer_size = i_vbv_buffer_size;
                this.f_vbv_buffer_init = f_vbv_buffer_init;
                this.f_ip_factor = f_ip_factor;
                this.f_pb_factor = f_pb_factor;
                this.b_filler = b_filler;
                this.i_aq_mode = i_aq_mode;
                this.f_aq_strength = f_aq_strength;
                this.b_mb_tree = b_mb_tree;
                this.i_lookahead = i_lookahead;
                this.b_stat_write = b_stat_write;
                this.psz_stat_out = psz_stat_out;
                this.b_stat_read = b_stat_read;
                this.psz_stat_in = psz_stat_in;
                this.f_qcompress = f_qcompress;
                this.f_qblur = f_qblur;
                this.f_complexity_blur = f_complexity_blur;
                this.zones = zones;
                this.i_zones = i_zones;
                this.psz_zones = psz_zones;
            }
        }

        static class crop_rect
        {
            long i_left;
            long i_top;
            long i_right;
            long i_bottom;

            public crop_rect(long i_left, long i_top, long i_right, long i_bottom) {
                this.i_left = i_left;
                this.i_top = i_top;
                this.i_right = i_right;
                this.i_bottom = i_bottom;
            }
        }

        int i_frame_packing;

        /* Muxing parameters */
        int b_aud;                  /* generate access unit delimiters */
        int b_repeat_headers;       /* put SPS/PPS before each keyframe */
        int b_annexb;               /* if set, place start codes (4 bytes) before NAL units,
         * otherwise place size (4 bytes) before NAL units. */
        int i_sps_id;               /* SPS and PPS id number */
        int b_vfr_input;            /* VFR input.  If 1, use timebase and timestamps for ratecontrol purposes.
         * If 0, use fps only. */
        int b_pulldown;             /* use explicity set timebase for CFR */
        long i_fps_num;
        long i_fps_den;
        long i_timebase_num;    /* Timebase numerator */
        long i_timebase_den;    /* Timebase denominator */

        int b_tff;
        int b_pic_struct;
        int b_fake_interlaced;
        int b_stitchable;

        int b_opencl;            /* use OpenCL when available */
        int i_opencl_device;     /* specify count of GPU devices to skip, for CLI users */

        /* Slicing parameters */
        int i_slice_max_size;    /* Max size per slice in bytes; includes estimated NAL overhead. */
        int i_slice_max_mbs;     /* Max number of MBs per slice; overrides i_slice_count. */
        int i_slice_min_mbs;     /* Min number of MBs per slice */
        int i_slice_count;       /* Number of slices per frame: forces rectangular slices. */
        int i_slice_count_max;   /* Absolute cap on slices per frame; stops applying slice-max-size
         * and slice-max-mbs if this is reached. */

        interface ParamFreeFunction {
            void paramFree(Object param);
        }

//        interface NaluProcessFunction {
//            void naluProcess(X264T h, X264Nal nal, Object opaque);
//        }

        public X264Params(int cpu, int i_threads, int i_lookahead_threads, int b_sliced_threads, int b_deterministic, int b_cpu_independent, int i_sync_lookahead, int i_width, int i_height, int i_csp, int i_bitdepth, int i_level_idc, int i_frame_total, int i_nal_hrd, int i_frame_reference, int i_dpb_size, int i_keyint_max, int i_keyint_min, int i_scenecut_threshold, int b_intra_refresh, int i_bframe, int i_bframe_adaptive, int i_bframe_bias, int i_bframe_pyramid, int b_open_gop, int b_bluray_compat, int i_avcintra_class, int b_deblocking_filter, int i_deblocking_filter_alphac0, int i_deblocking_filter_beta, int b_cabac, int i_cabac_init_idc, int b_interlaced, int i_log_level, int b_full_recon, int i_frame_packing, int b_aud, int b_repeat_headers, int b_annexb, int i_sps_id, int b_vfr_input, int b_pulldown, long i_fps_num, long i_fps_den, long i_timebase_num, long i_timebase_den, int b_tff, int b_pic_struct, int b_fake_interlaced, int b_stitchable, int b_opencl, int i_opencl_device, int i_slice_max_size, int i_slice_max_mbs, int i_slice_min_mbs, int i_slice_count, int i_slice_count_max) {
            this.cpu = cpu;
            this.i_threads = i_threads;
            this.i_lookahead_threads = i_lookahead_threads;
            this.b_sliced_threads = b_sliced_threads;
            this.b_deterministic = b_deterministic;
            this.b_cpu_independent= b_cpu_independent;
            this.i_sync_lookahead = i_sync_lookahead;
            this.i_width = i_width;
            this.i_height = i_height;
            this.i_csp = i_csp;
            this.i_bitdepth = i_bitdepth;
            this.i_level_idc = i_level_idc;
            this.i_frame_total = i_frame_total;
            this.i_nal_hrd = i_nal_hrd;
            this.i_frame_reference = i_frame_reference;
            this.i_dpb_size = i_dpb_size;
            this.i_keyint_max = i_keyint_max;
            this.i_keyint_min = i_keyint_min;
            this.i_scenecut_threshold = i_scenecut_threshold;
            this.b_intra_refresh = b_intra_refresh;
            this.i_bframe = i_bframe;
            this.i_bframe_adaptive = i_bframe_adaptive;
            this.i_bframe_bias = i_bframe_bias;
            this.i_bframe_pyramid = i_bframe_pyramid;
            this.b_open_gop = b_open_gop;
            this.b_bluray_compat = b_bluray_compat;
            this.i_avcintra_class = i_avcintra_class;
            this.b_deblocking_filter = b_deblocking_filter;
            this.i_deblocking_filter_alphac0 = i_deblocking_filter_alphac0;
            this.i_deblocking_filter_beta = i_deblocking_filter_beta;
            this.b_cabac = b_cabac;
            this.i_cabac_init_idc = i_cabac_init_idc;
            this.b_interlaced = b_interlaced;
            this.i_log_level = i_log_level;
            this.b_full_recon = b_full_recon;
            this.i_frame_packing = i_frame_packing;
            this.b_aud = b_aud;
            this.b_repeat_headers = b_repeat_headers;
            this.b_annexb = b_annexb;
            this.i_sps_id = i_sps_id;
            this.b_vfr_input = b_vfr_input;
            this.b_pulldown = b_pulldown;
            this.i_fps_num = i_fps_num;
            this.i_fps_den = i_fps_den;
            this.i_timebase_num = i_timebase_num;
            this.i_timebase_den = i_timebase_den;
            this.b_tff = b_tff;
            this.b_pic_struct = b_pic_struct;
            this.b_fake_interlaced = b_fake_interlaced;
            this.b_stitchable = b_stitchable;
            this.b_opencl = b_opencl;
            this.i_opencl_device = i_opencl_device;
            this.i_slice_max_size = i_slice_max_size;
            this.i_slice_max_mbs = i_slice_max_mbs;
            this.i_slice_min_mbs = i_slice_min_mbs;
            this.i_slice_count = i_slice_count;
            this.i_slice_count_max = i_slice_count_max;
        }
    }

    public static class X264Image {
        int     i_csp;       /* Colorspace */
        int     i_plane;     /* Number of image planes */
        int[] iStride = new int[4]; /* Strides for each plane */
        String[] plane = new String[4]; /* Pointers to each plane */

        public X264Image(int i_csp, int i_plane, int[] iStride, String[] plane) {
            this.i_csp = i_csp;
            this.i_plane = i_plane;
            this.iStride = iStride;
            this.plane = plane;
        }
    }

    public static class X264ImageProperties {
        float[] quantOffsets;
        interface QuantOffsetsFreeFunction {
            void quantOffsetsFree(Object quantOffsets);
        }

        String mb_info;
        interface MbInfoFreeFunction {
            void mbInfoFree(Object mbInfo);
        }


        public static class Constants {
            public static final int X264_MBINFO_CONSTANT = 1;
        }

        double f_ssim;
        double f_psnr_avg;
        double[] fPsnr = new double[3];
        double f_crf_avg;

        public X264ImageProperties(float[] quantOffsets, String mb_info, double f_ssim, double f_psnr_avg, double[] fPsnr, double f_crf_avg) {
            this.quantOffsets = quantOffsets;
            this.mb_info = mb_info;
            this.f_ssim = f_ssim;
            this.f_psnr_avg = f_psnr_avg;
            this.fPsnr = fPsnr;
            this.f_crf_avg = f_crf_avg;
        }
    }

    public static class X264Hrd {
        double cpb_initial_arrival_time;
        double cpb_final_arrival_time;
        double cpb_removal_time;

        double dpb_output_time;

        public X264Hrd(double cpb_initial_arrival_time, double cpb_final_arrival_time, double cpb_removal_time, double dpb_output_time) {
            this.cpb_initial_arrival_time = cpb_initial_arrival_time;
            this.cpb_final_arrival_time = cpb_final_arrival_time;
            this.cpb_removal_time = cpb_removal_time;
            this.dpb_output_time = dpb_output_time;
        }
    }

    public static class  X264SEIPayload {
        int payload_size;
        int payload_type;
        String payload;

        public X264SEIPayload(int payload_size, int payload_type, String payload) {
            this.payload_size = payload_size;
            this.payload_type = payload_type;
            this.payload = payload;
        }
    }

    public static class  X264SEI {
        private final SeiFreeFunction seiFreeFunction;
        int num_payloads;
        X264SEIPayload payloads;
        /* In: optional callback to free each payload AND X264SEIPayload when used. */
        interface SeiFreeFunction {
            void seiFree();
        }
        public X264SEI(int num_payloads, X264SEIPayload payloads, SeiFreeFunction seiFreeFunction) {
            this.num_payloads = num_payloads;
            this.payloads = payloads;
            this.seiFreeFunction = seiFreeFunction;
        }
    }

    public static class x264Picture {
        int     i_type;
        int     i_qpplus1;
        int     i_pic_struct;
        int     b_keyframe;
        long i_pts;
        long i_dts;
        X264Params param;
        X264Image img;
        X264ImageProperties prop;
        X264Hrd hrd_timing;
        X264SEI extra_sei;
        Object opaque;

        public x264Picture(int i_type, int i_qpplus1, int i_pic_struct, int b_keyframe, long i_pts, long i_dts, X264Params param, X264Image img, X264ImageProperties prop, X264Hrd hrd_timing, X264SEI extra_sei, Object opaque) {
            this.i_type = i_type;
            this.i_qpplus1 = i_qpplus1;
            this.i_pic_struct = i_pic_struct;
            this.b_keyframe = b_keyframe;
            this.i_pts = i_pts;
            this.i_dts = i_dts;
            this.param = param;
            this.img = img;
            this.prop = prop;
            this.hrd_timing = hrd_timing;
            this.extra_sei = extra_sei;
            this.opaque = opaque;
        }
    }

    public static class X264ConfigParams {
        byte[]    sei;
        int       sei_size;
        String    preset;
        String    tune;
        String    profile;
        String    level;
        int       fastfirstpass;
        String    wpredp;
        String    x264opts;
        float crf;
        float crf_max;
        int cqp;
        int aq_mode;
        float aq_strength;
        String    psy_rd;
        int psy;
        int rc_lookahead;
        int weightp;
        int weightb;
        int ssim;
        int intra_refresh;
        int bluray_compat;
        int b_bias;
        int b_pyramid;
        int mixed_refs;
        int dct8x8;
        int fast_pskip;
        int aud;
        int mbtree;
        String    deblock;
        float cplxblur;
        String    partitions;
        int direct_pred;
        int slice_max_size;
        String    stats;
        int nal_hrd;
        int avcintra_class;
        int motion_est;
        int forced_idr;
        int coder;
        int a53_cc;
        int b_frame_strategy;
        int chroma_offset;
        int scenechange_threshold;
        int noise_reduction;
        int udu_sei;
        int nb_reordered_opaque, next_reordered_opaque;
        int roi_warned;

//        x264_t         *enc;

        public X264ConfigParams(byte[] sei, int sei_size, String preset, String tune, String profile, String level, int fastfirstpass, String wpredp, String x264opts, float crf, float crf_max, int cqp, int aq_mode, float aq_strength, String psy_rd, int psy, int rc_lookahead, int weightp, int weightb, int ssim, int intra_refresh, int bluray_compat, int b_bias, int b_pyramid, int mixed_refs, int dct8x8, int fast_pskip, int aud, int mbtree, String deblock, float cplxblur, String partitions, int direct_pred, int slice_max_size, String stats, int nal_hrd, int avcintra_class, int motion_est, int forced_idr, int coder, int a53_cc, int b_frame_strategy, int chroma_offset, int scenechange_threshold, int noise_reduction, int udu_sei, int nb_reordered_opaque, int next_reordered_opaque, int roi_warned) {
            this.sei = sei;
            this.sei_size = sei_size;
            this.preset = preset;
            this.tune = tune;
            this.profile = profile;
            this.level = level;
            this.fastfirstpass = fastfirstpass;
            this.wpredp = wpredp;
            this.x264opts = x264opts;
            this.crf = crf;
            this.crf_max = crf_max;
            this.cqp = cqp;
            this.aq_mode = aq_mode;
            this.aq_strength = aq_strength;
            this.psy_rd = psy_rd;
            this.psy = psy;
            this.rc_lookahead = rc_lookahead;
            this.weightp = weightp;
            this.weightb = weightb;
            this.ssim = ssim;
            this.intra_refresh = intra_refresh;
            this.bluray_compat = bluray_compat;
            this.b_bias = b_bias;
            this.b_pyramid = b_pyramid;
            this.mixed_refs = mixed_refs;
            this.dct8x8 = dct8x8;
            this.fast_pskip = fast_pskip;
            this.aud = aud;
            this.mbtree = mbtree;
            this.deblock = deblock;
            this.cplxblur = cplxblur;
            this.partitions = partitions;
            this.direct_pred = direct_pred;
            this.slice_max_size = slice_max_size;
            this.stats = stats;
            this.nal_hrd = nal_hrd;
            this.avcintra_class = avcintra_class;
            this.motion_est = motion_est;
            this.forced_idr = forced_idr;
            this.coder = coder;
            this.a53_cc = a53_cc;
            this.b_frame_strategy = b_frame_strategy;
            this.chroma_offset = chroma_offset;
            this.scenechange_threshold = scenechange_threshold;
            this.noise_reduction = noise_reduction;
            this.udu_sei = udu_sei;
            this.nb_reordered_opaque = nb_reordered_opaque;
            this.next_reordered_opaque = next_reordered_opaque;
            this.roi_warned = roi_warned;
        }
    }

    static{
        try {
            System.loadLibrary("x264");
        } catch (UnsatisfiedLinkError e) {
            Log.e(TAG, "Failed to load x264 library: " + e.getMessage());
        }
    }

    public static native int x264Init(X264ConfigParams x264ConfigParamsInstance, X264Params x264ParamsInstance, X264Params.crop_rect cropRectInstance, X264Nal x264NalInstance,  X264Params.analyse analyseInstance, X264Params.vui vuiInstance, X264Params.rc rcInstance, byte[] headerArray);
    public static native int x264Encode(byte[] yBuffer, byte[] uBuffer, byte[] vBuffer, byte[] outputBuffer, int width, int height);
    public static native void x264Close();

    public BufferX264Encoder(Test test) {
        super(test);
        mStats = new Statistics("raw encoder", mTest);
    }

    private long computePresentationTimeUs(int frameIndex) {
        return frameIndex * 1000000 / 30;
    }

    public static byte[] readYUVFromFile(String filePath, int size, int framePosition) throws IOException {
        byte[] inputBuffer = new byte[size];
        try (FileInputStream fis = new FileInputStream(filePath);
             FileChannel channel = fis.getChannel()) {
            channel.position(framePosition);
            ByteBuffer buffer = ByteBuffer.wrap(inputBuffer);
            int bytesRead = channel.read(buffer);
            if (bytesRead < size) {
                return null;
            }
        }
        return inputBuffer;
    }

    public static byte[][] extractYUVPlanes(byte[] yuvData, int width, int height) {
        int frameSize = width * height;
        int qFrameSize = frameSize / 4;

        byte[] yPlane = new byte[frameSize];
        byte[] uPlane = new byte[qFrameSize];
        byte[] vPlane = new byte[qFrameSize];

        System.arraycopy(yuvData, 0, yPlane, 0, frameSize);
        System.arraycopy(yuvData, frameSize, uPlane, 0, qFrameSize);
        System.arraycopy(yuvData, frameSize + qFrameSize, vPlane, 0, qFrameSize);

        return new byte[][]{yPlane, uPlane, vPlane};
    }

    private int findStartCodePrefix(byte[] frame) {
        // Look for the start code prefix (0x00 0x00 0x01 or 0x00 0x00 0x00 0x01)
        for (int i = 0; i < frame.length - 4; i++) {
            if (frame[i] == 0x00 && frame[i + 1] == 0x00 && frame[i + 2] == 0x01) {
                return i;
            }
            if (frame[i] == 0x00 && frame[i + 1] == 0x00 && frame[i + 2] == 0x00 && frame[i + 3] == 0x01) {
                return i;
            }
        }
        return -1;
    }
    private boolean checkIfKeyFrame(byte[] frame) {
        // H.264 NAL unit type for IDR frames (key frames) is 5
        final byte NAL_UNIT_TYPE_IDR = 5;

        // H.264 frames start with a start code prefix (0x00 0x00 0x01 or 0x00 0x00 0x00 0x01)
        // followed by the NAL unit header.

        // NAL unit header format (first byte after the start code prefix):
        // +---------------+
        // |0|1|2|3|4|5|6|7|
        // +-+-+-+-+-+-+-+-+
        // |F|NRI|  Type   |
        // +---------------+
        // F: forbidden_zero_bit (1 bit) - must be 0
        // NRI: nal_ref_idc (2 bits) - nal reference indicator
        // Type: nal_unit_type (5 bits)

        // Find the start code prefix (0x00 0x00 0x01 or 0x00 0x00 0x00 0x01)
        int startIndex = findStartCodePrefix(frame);
        if (startIndex == -1 || startIndex + 4 >= frame.length) {
            return false; // Invalid frame or no start code prefix found
        }

        // The NAL unit type is in the lower 5 bits of the byte following the start code prefix
        byte nalUnitHeader = frame[startIndex + 4];
        int nalUnitType = nalUnitHeader & 0x1F;

        return nalUnitType == NAL_UNIT_TYPE_IDR;
    }

    public byte[] concatenateBuffers(byte[] headerArray, byte[] outputBuffer) {
        byte[] concatenatedArray = new byte[headerArray.length + outputBuffer.length];
        ByteBuffer concatenatedBuffer = ByteBuffer.wrap(concatenatedArray);
        concatenatedBuffer.put(headerArray);
        concatenatedBuffer.put(outputBuffer);
        return concatenatedArray;
    }

    public String start() {
        Log.d(TAG, "** Raw buffer encoding - " + mTest.getCommon().getDescription() + " **");
        try {
            if (TestDefinitionHelper.checkBasicSettings(mTest)) {
                mTest = TestDefinitionHelper.updateBasicSettings(mTest);
            }
        } catch (RuntimeException e) {
            Log.e(TAG, "Error: " + e.getMessage());
        }
        if (mTest.hasRuntime())
            mRuntimeParams = mTest.getRuntime();
        if (mTest.getInput().hasRealtime())
            mRealtime = mTest.getInput().getRealtime();

        boolean useImage = false;

        mFrameRate = mTest.getConfigure().getFramerate();
        mWriteFile = !mTest.getConfigure().hasEncode() || mTest.getConfigure().getEncode();
        mSkipped = 0;
        mFramesAdded = 0;
        Size sourceResolution = SizeUtils.parseXString(mTest.getInput().getResolution());
        mRefFramesizeInBytes = (int) (sourceResolution.getWidth() * sourceResolution.getHeight() * 1.5);
        mYuvReader = new FileReader();
        int playoutframes = mTest.getInput().getPlayoutFrames();


        int i_ref_idc = 1;
        int i_type = 2;
        int b_long_startcode = 0;
        int i_first_mb = 10;
        int i_last_mb = 20;
        int i_payload = 100;
        byte[] p_payload = new byte[100];
        int i_padding = 5;

        int cpu = 34234;
        int i_threads = 1;
        int i_lookahead_threads = 1;
        int b_sliced_threads = 1;
        int b_deterministic = 0;
        int b_cpu_independent = 0;
        int i_sync_lookahead = -1;
        int i_width = 1920;
        int i_height = 1080;
        int i_csp = 2;
        int i_bitdepth = 8;
        int i_level_idc = 9;
        int i_frame_total = 15;
        int i_nal_hrd = 0;
        int i_bframe = 0;
        int i_bframe_adaptive = -1;
        int i_bframe_bias = -1;
        int i_bframe_pyramid = -1;
        int i_deblocking_filter_alphac0 = 0;
        int i_deblocking_filter_beta = 0;

        // vui
        int i_sar_height = 1;
        int i_sar_width = 1;
        int i_overscan = 0;
        int i_vidformat = 5;
        int b_fullrange = -1;
        int i_colorprism = 2;
        int i_transfer = 2;
        int i_colmatrix = -1;
        int i_chroma_loc = 0;

        int i_frame_reference = 1;
        int i_dpb_size = 1;
        int i_keyint_max = 250;
        int i_keyint_min = 0;
        int i_scenecut_threshold = 40;
        int b_intra_refresh = 0;
        int b_frame = 0;
        int b_frame_adaptive = 1;
        int b_frame_bias = 0;
        int b_frame_pyramid = 2;
        int b_open_gop = 0;
        int b_bluray_compat = 0;
        int i_avcintra_class = 0;
        int i_avcintra_flavour = 0;
        int b_deblocking_filter = 0;
        int b_deblocking_filter_alphac0 = 0;
        int b_deblocking_filter_beta = 0;
        int b_cabac = 1;
        int i_cabac_init_idc = 0;
        int b_interlaced = 0;
        int i_log_level = 0;
        int b_full_recon = 0;

        int intra = 3;
        int inter = 275;
        int b_transform_8x8 = 1;
        int i_weighted_pred = 2;
        int i_weighted_bipred = 1;
        int i_direct_mv_pred = 1;
        int i_chroma_qp_offset = 0;
        int i_me_method = 1;
        int i_me_range = 16;
        int i_mv_range = -1;
        int i_mv_range_thread = -1;
        int i_subpel_refine = 7;
        int b_chroma_me = 1;
        int b_mixed_references = 1;
        int i_trellis = 1;
        int b_fast_pskip = 1;
        int b_dct_decimate = 1;
        int i_noise_reduction = 0;
        int f_psy_rd = 1;
        int f_psy_trellis = 0;
        int b_psy = 1;
        int b_mb_info = 0;
        int b_mb_info_update = 0;
        int b_psnr = 0;
        int b_ssim = 0;

        int i_rc_method = 0;
        int i_qp_constant = 26;
        int i_qp_min = 0;
        int i_qp_max = 51;
        int i_qp_step = 4;
        int i_bitrate = 0;
        int f_rf_constant = 23;
        int f_rf_constant_max = 0;
        int f_rate_tolerance = 1;
        int i_vbv_max_bitrate = 0;
        int i_vbv_buffer_size = 0;
        float f_vbv_buffer_init = 0.89999F;
        float f_ip_factor = 1.399999F;
        float f_pb_factor = 1.299999F;
        int b_filler = 0;
        int i_aq_mode = 2;
        int f_aq_strength = 1;
        int b_mb_tree = 0;
        int i_lookahead = 40;
        int b_stat_write = 0;
        String psz_stat_out = "output_stats.txt";
        int b_stat_read = 0;
        String psz_stat_in = "input_stats.txt";
        float f_qcompress = 0.600000024F;
        float f_qblur = 0.5F;
        float f_complexity_blur = 20;
        int i_zones = 0;
        int i_frame_packing = -1;
        int b_weighted_bipred = -1;
        int i_colorprim = 2;

        //crop rect
        int i_left = 0;
        int i_top = 0;
        int i_right = 0;
        int i_bottom = 0;

        int b_aud = 0;
        int b_repeat_headers = 1;
        int b_annexb = 1;
        int i_sps_id = 0;

        int b_vfr_input = 1;
        int b_pulldown = 0;
        int i_fps_num = 25;
        int i_fps_den = 1;
        int i_timebase_num = 0;
        int i_timebase_den = 0;
        int b_tff = 1;
        int b_pic_struct = 0;
        int b_fake_interlaced = 0;
        int b_stitchable = 0;
        int b_opencl = 0;
        int i_opencl_device = 0;
        int i_slice_max_size = 0;
        int i_slice_max_mbs = 0;
        int i_slice_min_mbs = 0;
        int i_slice_count = 0;
        int i_slice_count_max = 0;

        byte[] sei = new byte[1000];
        int sei_size = 1000;
        String preset = "medium";
        String tune = "film";
        String profile = "main";
        String level = "4.1";
        int fastfirstpass = 1;
        String wpredp = "0";
        String x264opts = "keyint=60:min-keyint=30:scenecut=0";
        float crf = 23.0f;
        float crf_max = 25.0f;
        int cqp = 1;
        int aq_mode = 1;
        float aq_strength = 1.0f;
        String psy_rd = "1.0:0.15";
        int psy = 1;
        int rc_lookahead = 40;
        int weightp = 2;
        int weightb = 1;
        int ssim = 0;
        int intra_refresh = 0;
        int bluray_compat = 0;
        int b_bias = 0;
        int b_pyramid = 2;
        int mixed_refs = 1;
        int dct8x8 = 1;
        int fast_pskip = 1;
        int aud = 0;
        int mbtree = 1;
        String deblock = "1:1";
        float cplxblur = 20.0f;
        String partitions = "i4x4,i8x8,p8x8,b8x8";
        int direct_pred = 3;
        int slice_max_size = 0;
        String stats = "example.stats";
        int nal_hrd = 0;
        int avcintra_class = 0;
        int motion_est = 1;
        int forced_idr = 0;
        int coder = 1;
        int a53_cc = 0;
        int b_frame_strategy = 1;
        int chroma_offset = 0;
        int scenechange_threshold = 40;
        int noise_reduction = 0;
        int udu_sei = 0;
        int nb_reordered_opaque = 0;
        int next_reordered_opaque = 0;
        int roi_warned = 0;
        String psz_zones = "zones";

        X264ConfigParams x264ConfigParamsInstance = new X264ConfigParams(
                sei, sei_size, preset, tune, profile, level, fastfirstpass, wpredp, x264opts, crf, crf_max, cqp, aq_mode,
                aq_strength, psy_rd, psy, rc_lookahead, weightp, weightb, ssim, intra_refresh, bluray_compat, b_bias,
                b_pyramid, mixed_refs, dct8x8, fast_pskip, aud, mbtree, deblock, cplxblur, partitions, direct_pred,
                slice_max_size, stats, nal_hrd, avcintra_class, motion_est, forced_idr, coder, a53_cc, b_frame_strategy,
                chroma_offset, scenechange_threshold, noise_reduction, udu_sei, nb_reordered_opaque, next_reordered_opaque, roi_warned
        );

        X264Nal x264NalInstance = new X264Nal(i_ref_idc, i_type, b_long_startcode, i_first_mb, i_last_mb, i_payload, p_payload, i_padding);

        X264Params x264ParamsInstance = new X264Params(cpu, i_threads, i_lookahead_threads, b_sliced_threads, b_deterministic, b_cpu_independent, i_sync_lookahead, i_width, i_height, i_csp, i_bitdepth, i_level_idc,
                i_frame_total, i_nal_hrd, i_frame_reference, i_dpb_size, i_keyint_max, i_keyint_min, i_scenecut_threshold, b_intra_refresh, i_bframe, i_bframe_adaptive, i_bframe_bias, i_bframe_pyramid, b_open_gop, b_bluray_compat, i_avcintra_class, b_deblocking_filter, i_deblocking_filter_alphac0, i_deblocking_filter_beta, b_cabac, i_cabac_init_idc, b_interlaced, i_log_level, b_full_recon, i_frame_packing, b_aud, b_repeat_headers, b_annexb, i_sps_id, b_vfr_input, b_pulldown, i_fps_num, i_fps_den, i_timebase_num, i_timebase_den, b_tff, b_pic_struct, b_fake_interlaced, b_stitchable, b_opencl, i_opencl_device, i_slice_max_size, i_slice_max_mbs, i_slice_min_mbs, i_slice_count, i_slice_count_max);

        X264Params.crop_rect cropRectInstance = new X264Params.crop_rect(i_left,i_top,i_right,i_bottom);

        X264Params.analyse analyseInstance = new X264Params.analyse(intra, inter, b_transform_8x8, i_weighted_pred, b_weighted_bipred, i_direct_mv_pred, i_chroma_qp_offset, i_me_method, i_me_range, i_mv_range,
                i_mv_range_thread, i_subpel_refine, b_chroma_me, b_mixed_references, i_trellis, b_fast_pskip, b_dct_decimate, i_noise_reduction, f_psy_rd, f_psy_trellis, b_psy, b_mb_info, b_mb_info_update, b_psnr, b_ssim);

        X264Params.vui vuiInstance = new X264Params.vui(i_sar_height, i_sar_width, i_overscan, i_vidformat, b_fullrange, i_colorprim, i_transfer, i_colmatrix, i_chroma_loc);

        X264Zone zones = new X264Zone(0,0,0,0,0,x264ParamsInstance);

        X264Params.rc rcInstance = new X264Params.rc(
                i_rc_method, i_qp_constant, i_qp_min, i_qp_max, i_qp_step, i_bitrate,
                f_rf_constant, f_rf_constant_max, f_rate_tolerance, i_vbv_max_bitrate,
                i_vbv_buffer_size, f_vbv_buffer_init, f_ip_factor, f_pb_factor, b_filler,
                i_aq_mode, f_aq_strength, b_mb_tree, i_lookahead, b_stat_write,
                psz_stat_out, b_stat_read, psz_stat_in, f_qcompress, f_qblur,
                f_complexity_blur, zones, i_zones, psz_zones);

        if (!mYuvReader.openFile(checkFilePath(mTest.getInput().getFilepath()), mTest.getInput().getPixFmt())) {
            return "Could not open file";
        }

        MediaFormat mediaFormat;
        mediaFormat = TestDefinitionHelper.buildMediaFormat(mTest);
        logMediaFormat(mediaFormat);
        setConfigureParams(mTest, mediaFormat);

        float mReferenceFrameRate = mTest.getInput().getFramerate();
        mKeepInterval = mReferenceFrameRate / mFrameRate;
        mRefFrameTime = calculateFrameTimingUsec(mReferenceFrameRate);

        synchronized (this) {
            Log.d(TAG, "Wait for synchronized start");
            try {
                mInitDone = true;
                wait(WAIT_TIME_MS);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
        mStats.start();

        String outputStreamName = "x264_output.h264";
        String headerDump = "x264_header_dump.h264";
        File file = new File(Environment.getExternalStorageDirectory(), outputStreamName);
        File file2 = new File(Environment.getExternalStorageDirectory(), headerDump);

        // Ensure the parent directory exists
        File parentDir = file.getParentFile();
        if (parentDir != null && !parentDir.exists()) {
            parentDir.mkdirs();
        }

        try {
            FileOutputStream fileOutputStream = new FileOutputStream(file);
            FileOutputStream fileOutputStream2 = new FileOutputStream(file2);

            int currentFramePosition = 0;
            boolean input_done = false;
            boolean output_done = false;
            MediaMuxer muxer = null;
            MediaCodec.BufferInfo bufferInfo = null;

            int videoTrackIndex = -1;
            boolean muxerStarted = false;
            int frameSize = i_width * i_height * 3 / 2;
            byte[] outputBuffer = new byte[frameSize];
            //byte[][] headerArray = new byte[2][];
            int estimatedSize = 2048; // Adjust this size as needed
            byte[] headerArray = new byte[estimatedSize];
            int outputBufferSize;

            //int sizeOfHeader = 50;
            int sizeOfHeader = x264Init(x264ConfigParamsInstance, x264ParamsInstance, cropRectInstance, x264NalInstance,
                    analyseInstance, vuiInstance, rcInstance, headerArray);
            boolean flagHeaderSize = true;


            while (!input_done || !output_done) {
                try {
                    long timeoutUs = VIDEO_CODEC_WAIT_TIME_US;
                    int flags = 0;
                    String filePath = mTest.getInput().getFilepath();

                    if (mRealtime) {
                        sleepUntilNextFrame();
                    }

                    try {
                        byte[] yuvData = readYUVFromFile(filePath, frameSize, currentFramePosition);

                        if (yuvData == null) {
                            input_done = true;
                            output_done = true;
                            continue;
                        }

                        byte[][] planes = extractYUVPlanes(yuvData, i_width, i_height);

                        outputBufferSize = x264Encode(planes[0], planes[1], planes[2], outputBuffer, i_width, i_height);
                        //outputBufferSize = 4321;
                        if (outputBufferSize == 0) {
                            return "Failed to encode frame";
                        }

                        //int callMuxer = muxFrame(outputBuffer, i_width, i_height, headerArray);

                        mFramesAdded++;
                        currentFramePosition += frameSize;
                        //fileOutputStream.write(outputBuffer, 0, outputBufferSize);
                        Log.d(TAG, "Successfully written to " + outputStreamName);
                    } catch (IOException e) {
                        e.printStackTrace();
                        return e.getMessage();
                    }
                } catch (IllegalStateException ex) {
                    Log.e(TAG, "QueueInputBuffer: IllegalStateException error");
                    ex.printStackTrace();
                    return ex.getMessage();
                }

                try {
                    if (!muxerStarted) {
                        MediaFormat format = MediaFormat.createVideoFormat(MediaFormat.MIMETYPE_VIDEO_AVC, i_width, i_height);
//                            format.setInteger(MediaFormat.KEY_WIDTH, i_width);
//                            format.setInteger(MediaFormat.KEY_HEIGHT, i_height);
//                            format.setInteger(MediaFormat.KEY_BIT_RATE, 125000);
//                            format.setInteger(MediaFormat.KEY_FRAME_RATE, 30);
//                            format.setInteger(MediaFormat.KEY_COLOR_FORMAT, MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420Flexible);
//                            format.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, -1);
//                            format.setInteger(MediaFormat.KEY_BITRATE_MODE, 0);
                        format.setInteger(MediaFormat.KEY_WIDTH, i_width);
                        format.setInteger(MediaFormat.KEY_HEIGHT, i_height);
//                            format.setInteger(MediaFormat.KEY_M , 800000);
                        format.setInteger(MediaFormat.KEY_BIT_RATE, 800000);
                        format.setInteger(MediaFormat.KEY_FRAME_RATE, 50);
                            format.setInteger(MediaFormat.KEY_COLOR_FORMAT, MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420Flexible);
                            format.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, 1);
//                            format.setInteger(MediaFormat.KEY_BITRATE_MODE, MediaCodecInfo.EncoderCapabilities.BITRATE_MODE_CBR);


                        int spsStart = -1;
                        int spsEnd = -1;
                        int ppsStart = -1;
                        int ppsEnd = -1;

                        for (int i = 0; i < headerArray.length - 4; i++) {
                            // Check for the start code 0x00000001 or 0x000001
                            if ((headerArray[i] == 0x00 && headerArray[i+1] == 0x00 && headerArray[i+2] == 0x00 && headerArray[i+3] == 0x01) ||
                                    (headerArray[i] == 0x00 && headerArray[i+1] == 0x00 && headerArray[i+2] == 0x01)) {

                                int nalType = headerArray[i + (headerArray[i + 2] == 0x01 ? 3 : 4)] & 0x1F;
                                if (nalType == 7 && spsStart == -1) { // SPS NAL unit type is 7
                                    spsStart = i;
                                } else if (nalType == 8 && spsStart != -1 && spsEnd == -1) { // PPS NAL unit type is 8
                                    spsEnd = i;
                                    ppsStart = i;
                                }
                                else if(spsEnd != -1 && ppsStart != -1) {
                                    if(headerArray[i] == 0x00 && headerArray[i+1] == 0x00 && headerArray[i+2] == 0x01 && headerArray[i-1] != 0x00) {
                                        ppsEnd = i;
                                        break;
                                    }
                                }
                            }
                        }

                        byte[] spsBuffer = Arrays.copyOfRange(headerArray, spsStart, spsEnd);
                        byte[] ppsBuffer = Arrays.copyOfRange(headerArray, ppsStart, ppsEnd);

                        // Wrap them in ByteBuffers
                        //ByteBuffer spsBuffer = ByteBuffer.wrap(sps);
                        //ByteBuffer ppsBuffer = ByteBuffer.wrap(pps);
                        //if (headerArray[0] != null && headerArray[1] != null) {
                            ByteBuffer sps = ByteBuffer.wrap(spsBuffer);
                            ByteBuffer pps = ByteBuffer.wrap(ppsBuffer);
                            format.setByteBuffer("csd-0", sps);
                            format.setByteBuffer("csd-1", pps);
                        //}

                        bufferInfo = new MediaCodec.BufferInfo();
                        muxer = new MediaMuxer(Environment.getExternalStorageDirectory().getPath() + "/x264_output.mp4", MediaMuxer.OutputFormat.MUXER_OUTPUT_MPEG_4);
                        //bufferInfo.set(format, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);
                        videoTrackIndex = muxer.addTrack(format);
                        muxer.start();
                        muxerStarted = true;
                    }

                    //byte[] concatenatedResult = concatenateBuffers(headerArray, outputBuffer);
                    //ByteBuffer buffer = ByteBuffer.wrap(concatenatedResult);
                    ByteBuffer buffer = ByteBuffer.wrap(outputBuffer);
                    bufferInfo.offset = 0;
                    bufferInfo.size = flagHeaderSize ? (outputBufferSize /*+ sizeOfHeader*/) : outputBufferSize;

                    bufferInfo.presentationTimeUs = computePresentationTimeUsec(mFramesAdded, mRefFrameTime);

                    //boolean isKeyFrame = checkIfKeyFrame(outputBuffer);
                    //if (isKeyFrame) {
                        bufferInfo.flags = MediaCodec.BUFFER_FLAG_KEY_FRAME;
                    //}
                    //else
                    //    bufferInfo.flags = 0;
                    //FrameInfo frameInfo = mStats.stopEncodingFrame(bufferInfo.presentationTimeUs, bufferInfo.size,
                    //        (bufferInfo.flags & MediaCodec.BUFFER_FLAG_KEY_FRAME) != 0);

                    if(muxer != null) {
                        buffer.position(bufferInfo.offset);
                        buffer.limit(bufferInfo.offset + bufferInfo.size);

                        muxer.writeSampleData(videoTrackIndex, buffer, bufferInfo);
                        fileOutputStream2.write(headerArray, 0, sizeOfHeader);
                        fileOutputStream.write(buffer.array(), 0, outputBufferSize);
                    }
                    flagHeaderSize = false;
                } catch (MediaCodec.CodecException ex) {
                    Log.e(TAG, "dequeueOutputBuffer: MediaCodec.CodecException error");
                    ex.printStackTrace();
                    return "dequeueOutputBuffer: MediaCodec.CodecException error";
                }
            }
            mStats.stop();

            Log.d(TAG, "Close encoder and streams");
            x264Close();
            fileOutputStream.close();

            if (muxer != null) {
                try {
                    muxer.release();
                } catch (IllegalStateException ise) {
                    Log.e(TAG, "Illegal state exception when trying to release the muxer");
                }
            }
        } catch (Exception ex) {
            return ex.getMessage();
        }
        mYuvReader.closeFile();
        return "";
    }

    public void writeToBuffer(@NonNull MediaCodec codec, int index, boolean encoder) {
        // Not needed
    }

    public void readFromBuffer(@NonNull MediaCodec codec, int index, boolean encoder, MediaCodec.BufferInfo info) {
        // Not needed
    }

    public void stopAllActivity() {
        // Not needed
    }

    public void release() {
        // Not needed
    }
}
