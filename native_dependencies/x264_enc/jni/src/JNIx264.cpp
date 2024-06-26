#include <jni.h>
#include <stdlib.h>
#include "x264.h"
#include "JNIx264.h"

// JNI_FILE_DUMP
#include <iostream>
#include <fstream>
#include <memory>
using namespace std;
// JNI_FILE_DUMP

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

X264Encoder* X264Encoder::x264encoder;

X264Encoder& X264Encoder::getInstance() {
    return *x264encoder;
}

X264Encoder::X264Encoder() : encoder(nullptr), nal(nullptr), nnal(0) {}

X264Encoder::~X264Encoder() {
    close();
}

int X264Encoder::init(JNIEnv *env, jobject thisObj, jobject x264ConfigParamsObj,
                       jobject x264ParamsObj, jobject x264CropRectObj, jobject x264NalObj, jobject x264AnalyseObj,
                       jobject x264VuiObj, jobject x264RcObj, jbyteArray headerArray)
{
    assert(x264encoder == NULL);
    x264encoder = new X264Encoder();
    x264_param_t x264Params;
    uint8_t *p, *extra_data, *sei;
    int sei_size, size_of_headers, i, extra_data_size;

    if (!x264encoder) {
        return false;
    }

    jclass x264ConfigParamsClass = env->GetObjectClass(x264ConfigParamsObj);
    jclass x264ParamsClass = env->GetObjectClass(x264ParamsObj);
    jclass x264CropRectClass = env->GetObjectClass(x264CropRectObj);
    jclass x264NalClass = env->GetObjectClass(x264NalObj);
    jclass x264AnalyseClass = env->GetObjectClass(x264AnalyseObj);
    jclass x264VuiClass = env->GetObjectClass(x264VuiObj);
    jclass x264RcClass = env->GetObjectClass(x264RcObj);

    jfieldID seiFieldID = env->GetFieldID(x264ConfigParamsClass, "sei", "[B");
    jfieldID seiSizeFieldID = env->GetFieldID(x264ConfigParamsClass, "sei_size", "I");
    jfieldID presetFieldID = env->GetFieldID(x264ConfigParamsClass, "preset", "Ljava/lang/String;");
    jfieldID tuneFieldID = env->GetFieldID(x264ConfigParamsClass, "tune", "Ljava/lang/String;");
    jfieldID profileFieldID = env->GetFieldID(x264ConfigParamsClass, "profile", "Ljava/lang/String;");
    jfieldID levelFieldID = env->GetFieldID(x264ConfigParamsClass, "level", "Ljava/lang/String;");
    jfieldID fastfirstpassFieldID = env->GetFieldID(x264ConfigParamsClass, "fastfirstpass", "I");
    jfieldID wpredpFieldID = env->GetFieldID(x264ConfigParamsClass, "wpredp", "Ljava/lang/String;");
    jfieldID x264optsFieldID = env->GetFieldID(x264ConfigParamsClass, "x264opts", "Ljava/lang/String;");
    jfieldID crfFieldID = env->GetFieldID(x264ConfigParamsClass, "crf", "F");
    jfieldID crfMaxFieldID = env->GetFieldID(x264ConfigParamsClass, "crf_max", "F");
    jfieldID cqpFieldID = env->GetFieldID(x264ConfigParamsClass, "cqp", "I");
    jfieldID aqModeFieldID = env->GetFieldID(x264ConfigParamsClass, "aq_mode", "I");
    jfieldID aqStrengthFieldID = env->GetFieldID(x264ConfigParamsClass, "aq_strength", "F");
    jfieldID psyRdFieldID = env->GetFieldID(x264ConfigParamsClass, "psy_rd", "Ljava/lang/String;");
    jfieldID psyFieldID = env->GetFieldID(x264ConfigParamsClass, "psy", "I");
    jfieldID rcLookaheadFieldID = env->GetFieldID(x264ConfigParamsClass, "rc_lookahead", "I");
    jfieldID weightpFieldID = env->GetFieldID(x264ConfigParamsClass, "weightp", "I");
    jfieldID weightbFieldID = env->GetFieldID(x264ConfigParamsClass, "weightb", "I");
    jfieldID ssimFieldID = env->GetFieldID(x264ConfigParamsClass, "ssim", "I");
    jfieldID intraRefreshFieldID = env->GetFieldID(x264ConfigParamsClass, "intra_refresh", "I");
    jfieldID blurayCompatFieldID = env->GetFieldID(x264ConfigParamsClass, "bluray_compat", "I");
    jfieldID bBiasFieldID = env->GetFieldID(x264ConfigParamsClass, "b_bias", "I");
    jfieldID bPyramidFieldID = env->GetFieldID(x264ConfigParamsClass, "b_pyramid", "I");
    jfieldID mixedRefsFieldID = env->GetFieldID(x264ConfigParamsClass, "mixed_refs", "I");
    jfieldID dct8x8FieldID = env->GetFieldID(x264ConfigParamsClass, "dct8x8", "I");
    jfieldID fastPskipFieldID = env->GetFieldID(x264ConfigParamsClass, "fast_pskip", "I");
    jfieldID audFieldID = env->GetFieldID(x264ConfigParamsClass, "aud", "I");
    jfieldID mbtreeFieldID = env->GetFieldID(x264ConfigParamsClass, "mbtree", "I");
    jfieldID deblockFieldID = env->GetFieldID(x264ConfigParamsClass, "deblock", "Ljava/lang/String;");
    jfieldID cplxblurFieldID = env->GetFieldID(x264ConfigParamsClass, "cplxblur", "F");
    jfieldID partitionsFieldID = env->GetFieldID(x264ConfigParamsClass, "partitions", "Ljava/lang/String;");
    jfieldID directPredFieldID = env->GetFieldID(x264ConfigParamsClass, "direct_pred", "I");
    jfieldID sliceMaxSizeFieldID = env->GetFieldID(x264ConfigParamsClass, "slice_max_size", "I");
    jfieldID statsFieldID = env->GetFieldID(x264ConfigParamsClass, "stats", "Ljava/lang/String;");
    jfieldID nalHrdFieldID = env->GetFieldID(x264ConfigParamsClass, "nal_hrd", "I");
    jfieldID avcintraClassFieldID = env->GetFieldID(x264ConfigParamsClass, "avcintra_class", "I");
    jfieldID motionEstFieldID = env->GetFieldID(x264ConfigParamsClass, "motion_est", "I");
    jfieldID forcedIdrFieldID = env->GetFieldID(x264ConfigParamsClass, "forced_idr", "I");
    jfieldID coderFieldID = env->GetFieldID(x264ConfigParamsClass, "coder", "I");
    jfieldID a53CcFieldID = env->GetFieldID(x264ConfigParamsClass, "a53_cc", "I");
    jfieldID bFrameStrategyFieldID = env->GetFieldID(x264ConfigParamsClass, "b_frame_strategy", "I");
    jfieldID chromaOffsetFieldID = env->GetFieldID(x264ConfigParamsClass, "chroma_offset", "I");
    jfieldID scenechangeThresholdFieldID = env->GetFieldID(x264ConfigParamsClass, "scenechange_threshold", "I");
    jfieldID noiseReductionFieldID = env->GetFieldID(x264ConfigParamsClass, "noise_reduction", "I");
    jfieldID uduSeiFieldID = env->GetFieldID(x264ConfigParamsClass, "udu_sei", "I");
    jfieldID nbReorderedOpaqueFieldID = env->GetFieldID(x264ConfigParamsClass, "nb_reordered_opaque", "I");
    jfieldID nextReorderedOpaqueFieldID = env->GetFieldID(x264ConfigParamsClass, "next_reordered_opaque", "I");
    jfieldID roiWarnedFieldID = env->GetFieldID(x264ConfigParamsClass, "roi_warned", "I");

    jfieldID cpuFieldID = env->GetFieldID(x264ParamsClass, "cpu", "I");
    jfieldID iThreadsFieldID = env->GetFieldID(x264ParamsClass, "i_threads", "I");
    jfieldID iLookaheadThreadsFieldID = env->GetFieldID(x264ParamsClass, "i_lookahead_threads", "I");
    jfieldID bSlicedThreadsFieldID = env->GetFieldID(x264ParamsClass, "b_sliced_threads", "I");
    jfieldID bDeterministicFieldID = env->GetFieldID(x264ParamsClass, "b_deterministic", "I");
    jfieldID bCpuIndependentFieldID = env->GetFieldID(x264ParamsClass, "b_cpu_independent", "I");
    jfieldID iSyncLookaheadFieldID = env->GetFieldID(x264ParamsClass, "i_sync_lookahead", "I");
    jfieldID iWidthFieldID = env->GetFieldID(x264ParamsClass, "i_width", "I");
    jfieldID iHeightFieldID = env->GetFieldID(x264ParamsClass, "i_height", "I");
    jfieldID iCspFieldID = env->GetFieldID(x264ParamsClass, "i_csp", "I");
    jfieldID iBitdepthFieldID = env->GetFieldID(x264ParamsClass, "i_bitdepth", "I");
    jfieldID iLevelIdcFieldID = env->GetFieldID(x264ParamsClass, "i_level_idc", "I");
    jfieldID iFrameTotalFieldID = env->GetFieldID(x264ParamsClass, "i_frame_total", "I");
    jfieldID iNalHrdFieldID = env->GetFieldID(x264ParamsClass, "i_nal_hrd", "I");
    jfieldID iFrameReferenceFieldID = env->GetFieldID(x264ParamsClass, "i_frame_reference", "I");
    jfieldID iDpbSizeFieldID = env->GetFieldID(x264ParamsClass, "i_dpb_size", "I");
    jfieldID iKeyintMaxFieldID = env->GetFieldID(x264ParamsClass, "i_keyint_max", "I");
    jfieldID iKeyintMinFieldID = env->GetFieldID(x264ParamsClass, "i_keyint_min", "I");
    jfieldID iScenecutThresholdFieldID = env->GetFieldID(x264ParamsClass, "i_scenecut_threshold", "I");
    jfieldID bIntraRefreshFieldID = env->GetFieldID(x264ParamsClass, "b_intra_refresh", "I");
    jfieldID iBframeFieldID = env->GetFieldID(x264ParamsClass, "i_bframe", "I");
    jfieldID iBframeAdaptiveFieldID = env->GetFieldID(x264ParamsClass, "i_bframe_adaptive", "I");
    jfieldID iBframeBiasFieldID = env->GetFieldID(x264ParamsClass, "i_bframe_bias", "I");
    jfieldID iBframePyramidFieldID = env->GetFieldID(x264ParamsClass, "i_bframe_pyramid", "I");
    jfieldID bOpenGopFieldID = env->GetFieldID(x264ParamsClass, "b_open_gop", "I");
    jfieldID bBlurayCompatFieldID = env->GetFieldID(x264ParamsClass, "b_bluray_compat", "I");
    jfieldID iAvcintraClassFieldID = env->GetFieldID(x264ParamsClass, "i_avcintra_class", "I");
    jfieldID bDeblockingFilterFieldID = env->GetFieldID(x264ParamsClass, "b_deblocking_filter", "I");
    jfieldID iDeblockingFilterAlphac0FieldID = env->GetFieldID(x264ParamsClass, "i_deblocking_filter_alphac0", "I");
    jfieldID iDeblockingFilterBetaFieldID = env->GetFieldID(x264ParamsClass, "i_deblocking_filter_beta", "I");
    jfieldID bCabacFieldID = env->GetFieldID(x264ParamsClass, "b_cabac", "I");
    jfieldID iCabacInitIdcFieldID = env->GetFieldID(x264ParamsClass, "i_cabac_init_idc", "I");
    jfieldID bInterlacedFieldID = env->GetFieldID(x264ParamsClass, "b_interlaced", "I");
    jfieldID iLogLevelFieldID = env->GetFieldID(x264ParamsClass, "i_log_level", "I");
    jfieldID bFullReconFieldID = env->GetFieldID(x264ParamsClass, "b_full_recon", "I");
    jfieldID iFramePackingFieldID = env->GetFieldID(x264ParamsClass, "i_frame_packing", "I");
    jfieldID bAudFieldID = env->GetFieldID(x264ParamsClass, "b_aud", "I");
    jfieldID bRepeatHeadersFieldID = env->GetFieldID(x264ParamsClass, "b_repeat_headers", "I");
    jfieldID bAnnexbFieldID = env->GetFieldID(x264ParamsClass, "b_annexb", "I");
    jfieldID iSpsIdFieldID = env->GetFieldID(x264ParamsClass, "i_sps_id", "I");
    jfieldID bVfrInputFieldID = env->GetFieldID(x264ParamsClass, "b_vfr_input", "I");
    jfieldID bPulldownFieldID = env->GetFieldID(x264ParamsClass, "b_pulldown", "I");
    jfieldID iFpsNumFieldID = env->GetFieldID(x264ParamsClass, "i_fps_num", "J");
    jfieldID iFpsDenFieldID = env->GetFieldID(x264ParamsClass, "i_fps_den", "J");
    jfieldID iTimebaseNumFieldID = env->GetFieldID(x264ParamsClass, "i_timebase_num", "J");
    jfieldID iTimebaseDenFieldID = env->GetFieldID(x264ParamsClass, "i_timebase_den", "J");
    jfieldID bTffFieldID = env->GetFieldID(x264ParamsClass, "b_tff", "I");
    jfieldID bPicStructFieldID = env->GetFieldID(x264ParamsClass, "b_pic_struct", "I");
    jfieldID bFakeInterlacedFieldID = env->GetFieldID(x264ParamsClass, "b_fake_interlaced", "I");
    jfieldID bStitchableFieldID = env->GetFieldID(x264ParamsClass, "b_stitchable", "I");
    jfieldID bOpenclFieldID = env->GetFieldID(x264ParamsClass, "b_opencl", "I");
    jfieldID iOpenclDeviceFieldID = env->GetFieldID(x264ParamsClass, "i_opencl_device", "I");
    jfieldID iSliceMaxSizeFieldID = env->GetFieldID(x264ParamsClass, "i_slice_max_size", "I");
    jfieldID iSliceMaxMbsFieldID = env->GetFieldID(x264ParamsClass, "i_slice_max_mbs", "I");
    jfieldID iSliceMinMbsFieldID = env->GetFieldID(x264ParamsClass, "i_slice_min_mbs", "I");
    jfieldID iSliceCountFieldID = env->GetFieldID(x264ParamsClass, "i_slice_count", "I");
    jfieldID iSliceCountMaxFieldID = env->GetFieldID(x264ParamsClass, "i_slice_count_max", "I");

    jfieldID iLeftFieldID = env->GetFieldID(x264CropRectClass, "i_left", "J");
    jfieldID iTopFieldID = env->GetFieldID(x264CropRectClass, "i_top", "J");
    jfieldID iRightFieldID = env->GetFieldID(x264CropRectClass, "i_right", "J");
    jfieldID iBottomFieldID = env->GetFieldID(x264CropRectClass, "i_bottom", "J");

    jfieldID iRefIdcFieldID = env->GetFieldID(x264NalClass, "i_ref_idc", "I");
    jfieldID iTypeFieldID = env->GetFieldID(x264NalClass, "i_type", "I");
    jfieldID bLongStartcodeFieldID = env->GetFieldID(x264NalClass, "b_long_startcode", "I");
    jfieldID iFirstMbFieldID = env->GetFieldID(x264NalClass, "i_first_mb", "I");
    jfieldID iLastMbFieldID = env->GetFieldID(x264NalClass, "i_last_mb", "I");
    jfieldID iPayloadFieldID = env->GetFieldID(x264NalClass, "i_payload", "I");
    jfieldID pPayloadFieldID = env->GetFieldID(x264NalClass, "p_payload", "[B");
    jfieldID iPaddingFieldID = env->GetFieldID(x264NalClass, "i_padding", "I");

    jfieldID iSarHeightFieldID = env->GetFieldID(x264VuiClass, "i_sar_height", "I");
    jfieldID iSarWidthFieldID  = env->GetFieldID(x264VuiClass, "i_sar_width", "I");
    jfieldID iOverscanFieldID  = env->GetFieldID(x264VuiClass, "i_overscan", "I");
    jfieldID iVidformatFieldID = env->GetFieldID(x264VuiClass, "i_vidformat", "I");
    jfieldID bFullrangeFieldID = env->GetFieldID(x264VuiClass, "b_fullrange", "I");
    jfieldID iColorprimFieldID = env->GetFieldID(x264VuiClass, "i_colorprim", "I");
    jfieldID iTransferFieldID  = env->GetFieldID(x264VuiClass, "i_transfer", "I");
    jfieldID iColmatrixFieldID = env->GetFieldID(x264VuiClass, "i_colmatrix", "I");
    jfieldID iChromaLocFieldID = env->GetFieldID(x264VuiClass, "i_chroma_loc", "I");

    jfieldID intraFieldID = env->GetFieldID(x264AnalyseClass, "intra", "J");
    jfieldID interFieldID = env->GetFieldID(x264AnalyseClass, "inter", "J");
    jfieldID bTransform8x8FieldID = env->GetFieldID(x264AnalyseClass, "b_transform_8x8", "I");
    jfieldID iWeightedPredFieldID = env->GetFieldID(x264AnalyseClass, "i_weighted_pred", "I");
    jfieldID bWeightedBipredFieldID = env->GetFieldID(x264AnalyseClass, "b_weighted_bipred", "I");
    jfieldID iDirectMvPredFieldID = env->GetFieldID(x264AnalyseClass, "i_direct_mv_pred", "I");
    jfieldID iChromaQpOffsetFieldID = env->GetFieldID(x264AnalyseClass, "i_chroma_qp_offset", "I");
    jfieldID iMeMethodFieldID = env->GetFieldID(x264AnalyseClass, "i_me_method", "I");
    jfieldID iMeRangeFieldID = env->GetFieldID(x264AnalyseClass, "i_me_range", "I");
    jfieldID iMvRangeFieldID = env->GetFieldID(x264AnalyseClass, "i_mv_range", "I");
    jfieldID iMvRangeThreadFieldID = env->GetFieldID(x264AnalyseClass, "i_mv_range_thread", "I");
    jfieldID iSubpelRefineFieldID = env->GetFieldID(x264AnalyseClass, "i_subpel_refine", "I");
    jfieldID bChromaMeFieldID = env->GetFieldID(x264AnalyseClass, "b_chroma_me", "I");
    jfieldID bMixedReferencesFieldID = env->GetFieldID(x264AnalyseClass, "b_mixed_references", "I");
    jfieldID iTrellisFieldID = env->GetFieldID(x264AnalyseClass, "i_trellis", "I");
    jfieldID bFastPskipFieldID = env->GetFieldID(x264AnalyseClass, "b_fast_pskip", "I");
    jfieldID bDctDecimateFieldID = env->GetFieldID(x264AnalyseClass, "b_dct_decimate", "I");
    jfieldID iNoiseReductionFieldID = env->GetFieldID(x264AnalyseClass, "i_noise_reduction", "I");
    jfieldID fPsyRdFieldID = env->GetFieldID(x264AnalyseClass, "f_psy_rd", "F");
    jfieldID fPsyTrellisFieldID = env->GetFieldID(x264AnalyseClass, "f_psy_trellis", "F");
    jfieldID bPsyFieldID = env->GetFieldID(x264AnalyseClass, "b_psy", "I");
    jfieldID bMbInfoFieldID = env->GetFieldID(x264AnalyseClass, "b_mb_info", "I");
    jfieldID bMbInfoUpdateFieldID = env->GetFieldID(x264AnalyseClass, "b_mb_info_update", "I");
    jfieldID bPsnrFieldID = env->GetFieldID(x264AnalyseClass, "b_psnr", "I");
    jfieldID bSsimFieldID = env->GetFieldID(x264AnalyseClass, "b_ssim", "I");

    jfieldID iRcMethodFieldID = env->GetFieldID(x264RcClass, "i_rc_method", "I");
    jfieldID iQpConstantFieldID = env->GetFieldID(x264RcClass, "i_qp_constant", "I");
    jfieldID iQpMinFieldID = env->GetFieldID(x264RcClass, "i_qp_min", "I");
    jfieldID iQpMaxFieldID = env->GetFieldID(x264RcClass, "i_qp_max", "I");
    jfieldID iQpStepFieldID = env->GetFieldID(x264RcClass, "i_qp_step", "I");
    jfieldID iBitrateFieldID = env->GetFieldID(x264RcClass, "i_bitrate", "I");
    jfieldID fRfConstantFieldID = env->GetFieldID(x264RcClass, "f_rf_constant", "F");
    jfieldID fRfConstantMaxFieldID = env->GetFieldID(x264RcClass, "f_rf_constant_max", "F");
    jfieldID fRateToleranceFieldID = env->GetFieldID(x264RcClass, "f_rate_tolerance", "F");
    jfieldID iVbvMaxBitrateFieldID = env->GetFieldID(x264RcClass, "i_vbv_max_bitrate", "I");
    jfieldID iVbvBufferSizeFieldID = env->GetFieldID(x264RcClass, "i_vbv_buffer_size", "I");
    jfieldID fVbvBufferInitFieldID = env->GetFieldID(x264RcClass, "f_vbv_buffer_init", "F");
    jfieldID fIpFactorFieldID = env->GetFieldID(x264RcClass, "f_ip_factor", "F");
    jfieldID fPbFactorFieldID = env->GetFieldID(x264RcClass, "f_pb_factor", "F");
    jfieldID bFillerFieldID = env->GetFieldID(x264RcClass, "b_filler", "I");
    jfieldID iAqModeFieldID = env->GetFieldID(x264RcClass, "i_aq_mode", "I");
    jfieldID fAqStrengthFieldID = env->GetFieldID(x264RcClass, "f_aq_strength", "F");
    jfieldID bMbTreeFieldID = env->GetFieldID(x264RcClass, "b_mb_tree", "I");
    jfieldID iLookaheadFieldID = env->GetFieldID(x264RcClass, "i_lookahead", "I");
    jfieldID bStatWriteFieldID = env->GetFieldID(x264RcClass, "b_stat_write", "I");
    jfieldID pszStatOutFieldID = env->GetFieldID(x264RcClass, "psz_stat_out", "Ljava/lang/String;");
    jfieldID bStatReadFieldID = env->GetFieldID(x264RcClass, "b_stat_read", "I");
    jfieldID pszStatInFieldID = env->GetFieldID(x264RcClass, "psz_stat_in", "Ljava/lang/String;");
    jfieldID fQcompressFieldID = env->GetFieldID(x264RcClass, "f_qcompress", "F");
    jfieldID fQblurFieldID = env->GetFieldID(x264RcClass, "f_qblur", "F");
    jfieldID fComplexityBlurFieldID = env->GetFieldID(x264RcClass, "f_complexity_blur", "F");


    jbyteArray seiValueObj = (jbyteArray)env->GetObjectField(x264ConfigParamsObj, seiFieldID);
    jint seiSizeValue = env->GetIntField(x264ConfigParamsObj, seiSizeFieldID);
    jstring presetValueObj = (jstring)env->GetObjectField(x264ConfigParamsObj, presetFieldID);
    const char *presetValue = env->GetStringUTFChars(presetValueObj, NULL);
    jstring tuneValueObj = (jstring)env->GetObjectField(x264ConfigParamsObj, tuneFieldID);
    const char *tuneValue = env->GetStringUTFChars(tuneValueObj, NULL);
    jstring profileValueObj = (jstring)env->GetObjectField(x264ConfigParamsObj, profileFieldID);
    const char *profileValue = env->GetStringUTFChars(profileValueObj, NULL);
    jstring levelValueObj = (jstring)env->GetObjectField(x264ConfigParamsObj, levelFieldID);
    const char *levelValue = env->GetStringUTFChars(levelValueObj, NULL);
    jint fastfirstpassValue = env->GetIntField(x264ConfigParamsObj, fastfirstpassFieldID);
    jstring wpredpValueObj = (jstring)env->GetObjectField(x264ConfigParamsObj, wpredpFieldID);
    const char *wpredpValue = env->GetStringUTFChars(wpredpValueObj, NULL);
    jstring x264optsValueObj = (jstring)env->GetObjectField(x264ConfigParamsObj, x264optsFieldID);
    const char *x264optsValue = env->GetStringUTFChars(x264optsValueObj, NULL);
    jfloat crfValue = env->GetFloatField(x264ConfigParamsObj, crfFieldID);
    jfloat crfMaxValue = env->GetFloatField(x264ConfigParamsObj, crfMaxFieldID);
    jint cqpValue = env->GetIntField(x264ConfigParamsObj, cqpFieldID);
    jint aqModeValue = env->GetIntField(x264ConfigParamsObj, aqModeFieldID);
    jfloat aqStrengthValue = env->GetFloatField(x264ConfigParamsObj, aqStrengthFieldID);
    jstring psyRdValueObj = (jstring)env->GetObjectField(x264ConfigParamsObj, psyRdFieldID);
    const char *psyRdValue = env->GetStringUTFChars(psyRdValueObj, NULL);
    jint psyValue = env->GetIntField(x264ConfigParamsObj, psyFieldID);
    jint rcLookaheadValue = env->GetIntField(x264ConfigParamsObj, rcLookaheadFieldID);
    jint weightpValue = env->GetIntField(x264ConfigParamsObj, weightpFieldID);
    jint weightbValue = env->GetIntField(x264ConfigParamsObj, weightbFieldID);
    jint ssimValue = env->GetIntField(x264ConfigParamsObj, ssimFieldID);
    jint intraRefreshValue = env->GetIntField(x264ConfigParamsObj, intraRefreshFieldID);
    jint blurayCompatValue = env->GetIntField(x264ConfigParamsObj, blurayCompatFieldID);
    jint bBiasValue = env->GetIntField(x264ConfigParamsObj, bBiasFieldID);
    jint bPyramidValue = env->GetIntField(x264ConfigParamsObj, bPyramidFieldID);
    jint mixedRefsValue = env->GetIntField(x264ConfigParamsObj, mixedRefsFieldID);
    jint dct8x8Value = env->GetIntField(x264ConfigParamsObj, dct8x8FieldID);
    jint fastPskipValue = env->GetIntField(x264ConfigParamsObj, fastPskipFieldID);
    jint audValue = env->GetIntField(x264ConfigParamsObj, audFieldID);
    jint mbtreeValue = env->GetIntField(x264ConfigParamsObj, mbtreeFieldID);
    jstring deblockValueObj = (jstring)env->GetObjectField(x264ConfigParamsObj, deblockFieldID);
    const char *deblockValue = env->GetStringUTFChars(deblockValueObj, NULL);
    jfloat cplxblurValue = env->GetFloatField(x264ConfigParamsObj, cplxblurFieldID);
    jstring partitionsValueObj = (jstring)env->GetObjectField(x264ConfigParamsObj, partitionsFieldID);
    const char *partitionsValue = env->GetStringUTFChars(partitionsValueObj, NULL);
    jint directPredValue = env->GetIntField(x264ConfigParamsObj, directPredFieldID);
    jint sliceMaxSizeValue = env->GetIntField(x264ConfigParamsObj, sliceMaxSizeFieldID);
    jstring statsValueObj = (jstring)env->GetObjectField(x264ConfigParamsObj, statsFieldID);
    const char *statsValue = env->GetStringUTFChars(statsValueObj, NULL);
    jint nalHrdValue = env->GetIntField(x264ConfigParamsObj, nalHrdFieldID);
    jint avcintraClassValue = env->GetIntField(x264ConfigParamsObj, avcintraClassFieldID);
    jint motionEstValue = env->GetIntField(x264ConfigParamsObj, motionEstFieldID);
    jint forcedIdrValue = env->GetIntField(x264ConfigParamsObj, forcedIdrFieldID);
    jint coderValue = env->GetIntField(x264ConfigParamsObj, coderFieldID);
    jint a53CcValue = env->GetIntField(x264ConfigParamsObj, a53CcFieldID);
    jint bFrameStrategyValue = env->GetIntField(x264ConfigParamsObj, bFrameStrategyFieldID);
    jint chromaOffsetValue = env->GetIntField(x264ConfigParamsObj, chromaOffsetFieldID);
    jint scenechangeThresholdValue = env->GetIntField(x264ConfigParamsObj, scenechangeThresholdFieldID);
    jint noiseReductionValue = env->GetIntField(x264ConfigParamsObj, noiseReductionFieldID);
    jint uduSeiValue = env->GetIntField(x264ConfigParamsObj, uduSeiFieldID);
    jint nbReorderedOpaqueValue = env->GetIntField(x264ConfigParamsObj, nbReorderedOpaqueFieldID);
    jint nextReorderedOpaqueValue = env->GetIntField(x264ConfigParamsObj, nextReorderedOpaqueFieldID);
    jint roiWarnedValue = env->GetIntField(x264ConfigParamsObj, roiWarnedFieldID);

    jint cpuValue = env->GetIntField(x264ParamsObj, cpuFieldID);
    jint iThreadsValue = env->GetIntField(x264ParamsObj, iThreadsFieldID);
    jint iLookaheadThreadsValue = env->GetIntField(x264ParamsObj, iLookaheadThreadsFieldID);
    jint bSlicedThreadsValue = env->GetIntField(x264ParamsObj, bSlicedThreadsFieldID);
    jint bDeterministicValue = env->GetIntField(x264ParamsObj, bDeterministicFieldID);
    jint bCpuIndependentValue = env->GetIntField(x264ParamsObj, bCpuIndependentFieldID);
    jint iSyncLookaheadValue = env->GetIntField(x264ParamsObj, iSyncLookaheadFieldID);
    jint iWidthValue = env->GetIntField(x264ParamsObj, iWidthFieldID);
    jint iHeightValue = env->GetIntField(x264ParamsObj, iHeightFieldID);
    jint iCspValue = env->GetIntField(x264ParamsObj, iCspFieldID);
    jint iBitdepthValue = env->GetIntField(x264ParamsObj, iBitdepthFieldID);
    jint iLevelIdcValue = env->GetIntField(x264ParamsObj, iLevelIdcFieldID);
    jint iFrameTotalValue = env->GetIntField(x264ParamsObj, iFrameTotalFieldID);
    jint iNalHrdValue = env->GetIntField(x264ParamsObj, iNalHrdFieldID);
    jint iFrameReferenceValue = env->GetIntField(x264ParamsObj, iFrameReferenceFieldID);
    jint iDpbSizeValue = env->GetIntField(x264ParamsObj, iDpbSizeFieldID);
    jint iKeyintMaxValue = env->GetIntField(x264ParamsObj, iKeyintMaxFieldID);
    jint iKeyintMinValue = env->GetIntField(x264ParamsObj, iKeyintMinFieldID);
    jint iScenecutThresholdValue = env->GetIntField(x264ParamsObj, iScenecutThresholdFieldID);
    jint bIntraRefreshValue = env->GetIntField(x264ParamsObj, bIntraRefreshFieldID);
    jint iBframeValue = env->GetIntField(x264ParamsObj, iBframeFieldID);
    jint iBframeAdaptiveValue = env->GetIntField(x264ParamsObj, iBframeAdaptiveFieldID);
    jint iBframeBiasValue = env->GetIntField(x264ParamsObj, iBframeBiasFieldID);
    jint iBframePyramidValue = env->GetIntField(x264ParamsObj, iBframePyramidFieldID);
    jint bOpenGopValue = env->GetIntField(x264ParamsObj, bOpenGopFieldID);
    jint bBlurayCompatValue = env->GetIntField(x264ParamsObj, bBlurayCompatFieldID);
    jint iAvcintraClassValue = env->GetIntField(x264ParamsObj, iAvcintraClassFieldID);
    jint bDeblockingFilterValue = env->GetIntField(x264ParamsObj, bDeblockingFilterFieldID);
    jint iDeblockingFilterAlphac0Value = env->GetIntField(x264ParamsObj, iDeblockingFilterAlphac0FieldID);
    jint iDeblockingFilterBetaValue = env->GetIntField(x264ParamsObj, iDeblockingFilterBetaFieldID);
    jint bCabacValue = env->GetIntField(x264ParamsObj, bCabacFieldID);
    jint iCabacInitIdcValue = env->GetIntField(x264ParamsObj, iCabacInitIdcFieldID);
    jint bInterlacedValue = env->GetIntField(x264ParamsObj, bInterlacedFieldID);
    jint iLogLevelValue = env->GetIntField(x264ParamsObj, iLogLevelFieldID);
    jint bFullReconValue = env->GetIntField(x264ParamsObj, bFullReconFieldID);
    jint iFramePackingValue = env->GetIntField(x264ParamsObj, iFramePackingFieldID);
    jint bAudValue = env->GetIntField(x264ParamsObj, bAudFieldID);
    jint bRepeatHeadersValue = env->GetIntField(x264ParamsObj, bRepeatHeadersFieldID);
    jint bAnnexbValue = env->GetIntField(x264ParamsObj, bAnnexbFieldID);
    jint iSpsIdValue = env->GetIntField(x264ParamsObj, iSpsIdFieldID);
    jint bVfrInputValue = env->GetIntField(x264ParamsObj, bVfrInputFieldID);
    jint bPulldownValue = env->GetIntField(x264ParamsObj, bPulldownFieldID);
    jlong iFpsNumValue = env->GetLongField(x264ParamsObj, iFpsNumFieldID);
    jlong iFpsDenValue = env->GetLongField(x264ParamsObj, iFpsDenFieldID);
    jlong iTimebaseNumValue = env->GetLongField(x264ParamsObj, iTimebaseNumFieldID);
    jlong iTimebaseDenValue = env->GetLongField(x264ParamsObj, iTimebaseDenFieldID);
    jint bTffValue = env->GetIntField(x264ParamsObj, bTffFieldID);
    jint bPicStructValue = env->GetIntField(x264ParamsObj, bPicStructFieldID);
    jint bFakeInterlacedValue = env->GetIntField(x264ParamsObj, bFakeInterlacedFieldID);
    jint bStitchableValue = env->GetIntField(x264ParamsObj, bStitchableFieldID);
    jint bOpenclValue = env->GetIntField(x264ParamsObj, bOpenclFieldID);
    jint iOpenclDeviceValue = env->GetIntField(x264ParamsObj, iOpenclDeviceFieldID);
    jint iSliceMaxSizeValue = env->GetIntField(x264ParamsObj, iSliceMaxSizeFieldID);
    jint iSliceMaxMbsValue = env->GetIntField(x264ParamsObj, iSliceMaxMbsFieldID);
    jint iSliceMinMbsValue = env->GetIntField(x264ParamsObj, iSliceMinMbsFieldID);
    jint iSliceCountValue = env->GetIntField(x264ParamsObj, iSliceCountFieldID);
    jint iSliceCountMaxValue = env->GetIntField(x264ParamsObj, iSliceCountMaxFieldID);

    jlong iLeftValue = env->GetLongField(x264CropRectObj, iLeftFieldID);
    jlong iTopValue = env->GetLongField(x264CropRectObj, iTopFieldID);
    jlong iRightValue = env->GetLongField(x264CropRectObj, iRightFieldID);
    jlong iBottomValue = env->GetLongField(x264CropRectObj, iBottomFieldID);

    jint iRefIdcValue = env->GetIntField(x264NalObj, iRefIdcFieldID);
    jint iTypeValue = env->GetIntField(x264NalObj, iTypeFieldID);
    jint bLongStartcodeValue = env->GetIntField(x264NalObj, bLongStartcodeFieldID);
    jint iFirstMbValue = env->GetIntField(x264NalObj, iFirstMbFieldID);
    jint iLastMbValue = env->GetIntField(x264NalObj, iLastMbFieldID);
    jint iPayloadValue = env->GetIntField(x264NalObj, iPayloadFieldID);
    jbyteArray pPayloadValue = (jbyteArray)env->GetObjectField(x264NalObj, pPayloadFieldID);
    jint iPaddingValue = env->GetIntField(x264NalObj, iPaddingFieldID);

    jint iSarHeightValue = env->GetIntField(x264VuiObj, iSarHeightFieldID);
    jint iSarWidthValue = env->GetIntField(x264VuiObj, iSarWidthFieldID);
    jint iOverscanValue = env->GetIntField(x264VuiObj, iOverscanFieldID);
    jint iVidformatValue = env->GetIntField(x264VuiObj, iVidformatFieldID);
    jint bFullrangeValue = env->GetIntField(x264VuiObj, bFullrangeFieldID);
    jint iColorprimValue = env->GetIntField(x264VuiObj, iColorprimFieldID);
    jint iTransferValue = env->GetIntField(x264VuiObj, iTransferFieldID);
    jint iColmatrixValue = env->GetIntField(x264VuiObj, iColmatrixFieldID);
    jint iChromaLocValue = env->GetIntField(x264VuiObj, iChromaLocFieldID);

    jlong intraValue = env->GetLongField(x264AnalyseObj, intraFieldID);
    jlong interValue = env->GetLongField(x264AnalyseObj, interFieldID);
    jint bTransform8x8Value = env->GetIntField(x264AnalyseObj, bTransform8x8FieldID);
    jint iWeightedPredValue = env->GetIntField(x264AnalyseObj, iWeightedPredFieldID);
    jint bWeightedBipredValue = env->GetIntField(x264AnalyseObj, bWeightedBipredFieldID);
    jint iDirectMvPredValue = env->GetIntField(x264AnalyseObj, iDirectMvPredFieldID);
    jint iChromaQpOffsetValue = env->GetIntField(x264AnalyseObj, iChromaQpOffsetFieldID);
    jint iMeMethodValue = env->GetIntField(x264AnalyseObj, iMeMethodFieldID);
    jint iMeRangeValue = env->GetIntField(x264AnalyseObj, iMeRangeFieldID);
    jint iMvRangeValue = env->GetIntField(x264AnalyseObj, iMvRangeFieldID);
    jint iMvRangeThreadValue = env->GetIntField(x264AnalyseObj, iMvRangeThreadFieldID);
    jint iSubpelRefineValue = env->GetIntField(x264AnalyseObj, iSubpelRefineFieldID);
    jint bChromaMeValue = env->GetIntField(x264AnalyseObj, bChromaMeFieldID);
    jint bMixedReferencesValue = env->GetIntField(x264AnalyseObj, bMixedReferencesFieldID);
    jint iTrellisValue = env->GetIntField(x264AnalyseObj, iTrellisFieldID);
    jint bFastPskipValue = env->GetIntField(x264AnalyseObj, bFastPskipFieldID);
    jint bDctDecimateValue = env->GetIntField(x264AnalyseObj, bDctDecimateFieldID);
    jint iNoiseReductionValue = env->GetIntField(x264AnalyseObj, iNoiseReductionFieldID);
    jfloat fPsyRdValue = env->GetFloatField(x264AnalyseObj, fPsyRdFieldID);
    jfloat fPsyTrellisValue = env->GetFloatField(x264AnalyseObj, fPsyTrellisFieldID);
    jint bPsyValue = env->GetIntField(x264AnalyseObj, bPsyFieldID);
    jint bMbInfoValue = env->GetIntField(x264AnalyseObj, bMbInfoFieldID);
    jint bMbInfoUpdateValue = env->GetIntField(x264AnalyseObj, bMbInfoUpdateFieldID);
    jint bPsnrValue = env->GetIntField(x264AnalyseObj, bPsnrFieldID);
    jint bSsimValue = env->GetIntField(x264AnalyseObj, bSsimFieldID);

    jint iRcMethodValue = env->GetIntField(x264RcObj, iRcMethodFieldID);
    jint iQpConstantValue = env->GetIntField(x264RcObj, iQpConstantFieldID);
    jint iQpMinValue = env->GetIntField(x264RcObj, iQpMinFieldID);
    jint iQpMaxValue = env->GetIntField(x264RcObj, iQpMaxFieldID);
    jint iQpStepValue = env->GetIntField(x264RcObj, iQpStepFieldID);
    jint iBitrateValue = env->GetIntField(x264RcObj, iBitrateFieldID);
    jfloat fRfConstantValue = env->GetFloatField(x264RcObj, fRfConstantFieldID);
    jfloat fRfConstantMaxValue = env->GetFloatField(x264RcObj, fRfConstantMaxFieldID);
    jfloat fRateToleranceValue = env->GetFloatField(x264RcObj, fRateToleranceFieldID);
    jint iVbvMaxBitrateValue = env->GetIntField(x264RcObj, iVbvMaxBitrateFieldID);
    jint iVbvBufferSizeValue = env->GetIntField(x264RcObj, iVbvBufferSizeFieldID);
    jfloat fVbvBufferInitValue = env->GetFloatField(x264RcObj, fVbvBufferInitFieldID);
    jfloat fIpFactorValue = env->GetFloatField(x264RcObj, fIpFactorFieldID);
    jfloat fPbFactorValue = env->GetFloatField(x264RcObj, fPbFactorFieldID);
    jint bFillerValue = env->GetIntField(x264RcObj, bFillerFieldID);
    jint iAqModeValue = env->GetIntField(x264RcObj, iAqModeFieldID);
    jfloat fAqStrengthValue = env->GetFloatField(x264RcObj, fAqStrengthFieldID);
    jint bMbTreeValue = env->GetIntField(x264RcObj, bMbTreeFieldID);
    jint iLookaheadValue = env->GetIntField(x264RcObj, iLookaheadFieldID);
    jint bStatWriteValue = env->GetIntField(x264RcObj, bStatWriteFieldID);
    jstring pszStatOutValue = (jstring)env->GetObjectField(x264RcObj, pszStatOutFieldID);
    jint bStatReadValue = env->GetIntField(x264RcObj, bStatReadFieldID);
    jstring pszStatInValue = (jstring)env->GetObjectField(x264RcObj, pszStatInFieldID);
    jfloat fQcompressValue = env->GetFloatField(x264RcObj, fQcompressFieldID);
    jfloat fQblurValue = env->GetFloatField(x264RcObj, fQblurFieldID);
    jfloat fComplexityBlurValue = env->GetFloatField(x264RcObj, fComplexityBlurFieldID);
    //jobject zonesValue = env->GetObjectField(x264RcObj, zonesFieldID);
    //jint iZonesValue = env->GetIntField(x264RcObj, iZonesFieldID);
    //jstring pszZonesValue = (jstring)env->GetObjectField(x264RcObj, pszZonesFieldID);
    //const char *pszZonesChars = env->GetStringUTFChars(pszZonesValue, NULL);

    //x264_param_default(&x264Params);
    if (x264_param_default_preset(&x264Params, presetValue, "zerolatency") < 0) {
        LOGI("Failed to set preset: %s", presetValue);
    } else {
        LOGI("Preset set to: %s", presetValue);
    }
    // Mapping to x264 structure members
    x264Params.cpu = cpuValue;
    x264Params.i_threads = iThreadsValue;
    x264Params.i_lookahead_threads = iLookaheadThreadsValue;
    x264Params.b_sliced_threads = bSlicedThreadsValue;
    x264Params.b_deterministic = bDeterministicValue;
    x264Params.b_cpu_independent = bCpuIndependentValue;
    x264Params.i_sync_lookahead = iSyncLookaheadValue;
    x264Params.i_width = iWidthValue;
    x264Params.i_height = iHeightValue;

    x264Params.i_csp = iCspValue;
    x264Params.i_bitdepth = iBitdepthValue;
    x264Params.i_level_idc = iLevelIdcValue;
    x264Params.i_frame_total = iFrameTotalValue;
    x264Params.i_nal_hrd = iNalHrdValue;
    x264Params.i_frame_reference = iFrameReferenceValue;
    x264Params.i_dpb_size = iDpbSizeValue;
    x264Params.i_keyint_max = iKeyintMaxValue;
    x264Params.i_keyint_min = iKeyintMinValue;
    x264Params.i_scenecut_threshold = iScenecutThresholdValue;
    x264Params.b_intra_refresh = bIntraRefreshValue;
    x264Params.i_bframe = iBframeValue;
    x264Params.i_bframe_adaptive = iBframeAdaptiveValue;
    x264Params.i_bframe_bias = iBframeBiasValue;
    x264Params.i_bframe_pyramid = iBframePyramidValue;
    x264Params.b_open_gop = bOpenGopValue;
    x264Params.b_bluray_compat = bBlurayCompatValue;
    x264Params.i_avcintra_class = iAvcintraClassValue;
    x264Params.b_deblocking_filter = bDeblockingFilterValue;
    x264Params.i_deblocking_filter_alphac0 = iDeblockingFilterAlphac0Value;
    x264Params.i_deblocking_filter_beta = iDeblockingFilterBetaValue;
    x264Params.b_cabac = bCabacValue;
    x264Params.i_cabac_init_idc = iCabacInitIdcValue;
    x264Params.b_interlaced = bInterlacedValue;
    x264Params.i_log_level = iLogLevelValue;
    x264Params.b_full_recon = bFullReconValue;

    x264Params.vui.i_sar_height = iSarHeightValue;
    x264Params.vui.i_sar_width = iSarWidthValue;
    x264Params.vui.i_overscan = iOverscanValue;
    x264Params.vui.i_vidformat = iVidformatValue;
    x264Params.vui.b_fullrange = bFullrangeValue;
    x264Params.vui.i_colorprim = iColorprimValue;
    x264Params.vui.i_transfer = iTransferValue;
    x264Params.vui.i_colmatrix = iColmatrixValue;
    x264Params.vui.i_chroma_loc = iChromaLocValue;

    x264Params.analyse.intra = intraValue;
    x264Params.analyse.inter = interValue;
    x264Params.analyse.b_transform_8x8 = bTransform8x8Value;
    x264Params.analyse.i_weighted_pred = iWeightedPredValue;
    x264Params.analyse.b_weighted_bipred = bWeightedBipredValue;
    x264Params.analyse.i_direct_mv_pred = iDirectMvPredValue;
    x264Params.analyse.i_chroma_qp_offset = iChromaQpOffsetValue;
    x264Params.analyse.i_me_method = iMeMethodValue;
    x264Params.analyse.i_me_range = iMeRangeValue;
    x264Params.analyse.i_mv_range = iMvRangeValue;
    x264Params.analyse.i_mv_range_thread = iMvRangeThreadValue;
    x264Params.analyse.i_subpel_refine = iSubpelRefineValue;
    x264Params.analyse.b_chroma_me = bChromaMeValue;
    x264Params.analyse.b_mixed_references = bMixedReferencesValue;
    x264Params.analyse.i_trellis = iTrellisValue;
    x264Params.analyse.b_fast_pskip = bFastPskipValue;
    x264Params.analyse.b_dct_decimate = bDctDecimateValue;
    x264Params.analyse.i_noise_reduction = iNoiseReductionValue;
    x264Params.analyse.f_psy_rd = fPsyRdValue;
    x264Params.analyse.f_psy_trellis = fPsyTrellisValue;
    x264Params.analyse.b_psy = bPsyValue;
    x264Params.analyse.b_mb_info = bMbInfoValue;
    x264Params.analyse.b_mb_info_update = bMbInfoUpdateValue;
    x264Params.analyse.i_luma_deadzone[0] = 0;
    x264Params.analyse.i_luma_deadzone[1] = 0;
    x264Params.analyse.b_psnr = bPsnrValue;
    x264Params.analyse.b_ssim = bSsimValue;

//    x264Params.rc.i_rc_method = iRcMethodValue;
//    x264Params.rc.i_qp_constant = iQpConstantValue;
//    x264Params.rc.i_qp_min = iQpMinValue;
//    x264Params.rc.i_qp_max = iQpMaxValue;
//    x264Params.rc.i_qp_step = iQpStepValue;
//    x264Params.rc.i_bitrate = iBitrateValue;
//    x264Params.rc.f_rf_constant = fRfConstantValue;
//    x264Params.rc.f_rf_constant_max = fRfConstantMaxValue;
//    x264Params.rc.f_rate_tolerance = fRateToleranceValue;
//    x264Params.rc.i_vbv_max_bitrate = iVbvMaxBitrateValue;
//    x264Params.rc.i_vbv_buffer_size = iVbvBufferSizeValue;
//    x264Params.rc.f_vbv_buffer_init = fVbvBufferInitValue;
//    x264Params.rc.f_ip_factor = fIpFactorValue;
//    x264Params.rc.f_pb_factor = fPbFactorValue;
//    x264Params.rc.b_filler = bFillerValue;
//    //x264Params.rc.i_aq_mode = iAqModeValue;
//    //x264Params.rc.f_aq_strength = fAqStrengthValue;
//    x264Params.rc.b_mb_tree = bMbTreeValue;
//    x264Params.rc.i_lookahead = iLookaheadValue;
//    x264Params.rc.b_stat_write = bStatWriteValue;
//    x264Params.rc.b_stat_read = bStatReadValue;
//    x264Params.rc.f_qcompress = fQcompressValue;
//    x264Params.rc.f_qblur = fQblurValue;
//    x264Params.rc.f_complexity_blur = fComplexityBlurValue;

    x264Params.crop_rect.i_left = iLeftValue;
    x264Params.crop_rect.i_top = iTopValue;
    x264Params.crop_rect.i_right = iRightValue;
    x264Params.crop_rect.i_bottom = iBottomValue;

    x264encoder->encoder = x264_encoder_open(&x264Params);
    x264_t *encoder = x264encoder->encoder;
    if(!encoder)
    {
        LOGI("Failed encoder_open");
        return -1;
    }
    LOGI("Passed encoder_open");

    x264_encoder_parameters(encoder, &x264Params);

    size_of_headers = x264_encoder_headers(encoder, &(x264encoder->nal), &(x264encoder->nnal));
    LOGI("size_of_headers: %d", size_of_headers);

    x264_nal_t *nal = x264encoder->nal;
    int nnal = x264encoder->nnal;
    // Fill headerArray with SPS and PPS if not already set

    int offset = 0;
    if (headerArray != nullptr) {
        jsize headerArraySize = env->GetArrayLength(headerArray);
        jbyte* headerArrayBuffer = env->GetByteArrayElements(headerArray, NULL);

        for (int i = 0; i < nnal; i++) {
            if (nal[i].i_type == NAL_SPS || nal[i].i_type == NAL_PPS || nal[i].i_type == NAL_SEI ||
                    nal[i].i_type == NAL_AUD || nal[i].i_type == NAL_FILLER) {
                //jbyteArray header = env->NewByteArray(nal[i].i_payload);
                //env->SetByteArrayRegion(header, 0, nal[i].i_payload, reinterpret_cast<jbyte*>(nal[i].p_payload));
                //env->SetObjectArrayElement(headerArrayBuffer, i, header);

                if (nal[i].i_type == NAL_SEI)
                {
                    LOGI("In SEI");
                    sei_size = nal[i].i_payload;
                    LOGI("sei_size: %d", sei_size);
                    sei      = (uint8_t *)malloc(sei_size);
                    memcpy(sei, nal[i].p_payload, nal[i].i_payload);
                }

                // Check if there is enough space in the header array
                if (offset + nal[i].i_payload > headerArraySize) {
                    // Handle the error, e.g., by returning an error code
                    env->ReleaseByteArrayElements(headerArray, headerArrayBuffer, 0);
                    return -1;
                }

                // Copy the payload into the header array
                memcpy(headerArrayBuffer + offset, nal[i].p_payload, nal[i].i_payload);
                offset += nal[i].i_payload;
            }
        }
        env->ReleaseByteArrayElements(headerArray, headerArrayBuffer, 0);
    }

    /*
    LOGI("Passed x264_encoder_headers. Size size_of_headers=%d", size_of_headers);
    extra_data = p = (uint8_t *)malloc(size_of_headers + 64);

    for (i = 0; i < nnal; i++) {
        // Don't put the SEI in extradata.
        if (nal[i].i_type == NAL_SEI) {
            sei_size = nal[i].i_payload;
            sei      = (uint8_t *)malloc(sei_size);
            if (!sei)
                return 0;
            memcpy(sei, nal[i].p_payload, nal[i].i_payload);
            continue;
        }
        memcpy(p, nal[i].p_payload, nal[i].i_payload);
        p += nal[i].i_payload;
    }
    extra_data_size = p - extra_data;
*/
    env->ReleaseStringUTFChars(presetValueObj, presetValue);
    env->ReleaseStringUTFChars(tuneValueObj, tuneValue);
    env->ReleaseStringUTFChars(profileValueObj, profileValue);
    env->ReleaseStringUTFChars(levelValueObj, levelValue);
    env->ReleaseStringUTFChars(wpredpValueObj, wpredpValue);
    env->ReleaseStringUTFChars(x264optsValueObj, x264optsValue);
    env->ReleaseStringUTFChars(psyRdValueObj, psyRdValue);
    env->ReleaseStringUTFChars(deblockValueObj, deblockValue);
    env->ReleaseStringUTFChars(partitionsValueObj, partitionsValue);
    env->ReleaseStringUTFChars(statsValueObj, statsValue);

    return size_of_headers;
}


int X264Encoder::encode(JNIEnv *env, jobject obj, jbyteArray yBuffer, jbyteArray uBuffer, jbyteArray vBuffer,
                        jbyteArray out_buffer, jint width, jint height)
{
    if (!encoder) {
        LOGI("Encoder is not initialized for encoding");
        return -1;
    }
    x264_picture_t pic_in = {0};
    x264_picture_t pic_out = {0};

    //x264_t *encoder = x264encoder->encoder;
    //x264_nal_t *nal = x264encoder->nal;
    //int nnal = x264encoder->nnal;

    x264_picture_init(&pic_in);
    pic_in.img.i_csp = 2;
    pic_in.img.i_plane = 3;

    jsize yBuffer_size = env->GetArrayLength(yBuffer);
    jsize uBuffer_size = env->GetArrayLength(uBuffer);
    jsize vBuffer_size = env->GetArrayLength(vBuffer);
    jsize out_buffer_size = env->GetArrayLength(out_buffer);

    jbyte* yInp_YuvBuffer = env->GetByteArrayElements(yBuffer, NULL);
    jbyte* uInp_YuvBuffer = env->GetByteArrayElements(uBuffer, NULL);
    jbyte* vInp_YuvBuffer = env->GetByteArrayElements(vBuffer, NULL);
    jbyte* out_YuvBuffer = env->GetByteArrayElements(out_buffer, NULL);

    jint ySize = width * height;
    jint uvSize = width * height / 4;

    pic_in.img.plane[0] = new uint8_t[ySize];
    pic_in.img.plane[1] = new uint8_t[uvSize];
    pic_in.img.plane[2] = new uint8_t[uvSize];

    memcpy(pic_in.img.plane[0], yInp_YuvBuffer, ySize);
    memcpy(pic_in.img.plane[1], uInp_YuvBuffer, uvSize);
    memcpy(pic_in.img.plane[2], vInp_YuvBuffer, uvSize);

    pic_in.img.i_stride[0] = width;
    pic_in.img.i_stride[1] = width / 2;
    pic_in.img.i_stride[2] = width / 2;

    int frame_size = x264_encoder_encode(encoder, &nal, &nnal, &pic_in, &pic_out);

    if (frame_size >= 0) {
        int total_size = 2;
        for (int i = 0; i < nnal; i++) {
            total_size += nal[i].i_payload;
        }

        jsize out_buffer_size = env->GetArrayLength(out_buffer);
        if (out_buffer_size < total_size) {
            out_buffer = env->NewByteArray(total_size);
        }

        jbyte *out_buffer_data = env->GetByteArrayElements(out_buffer, NULL);

        int offset = 2;
        out_buffer_data[0] = 0;
        out_buffer_data[1] = 0;
        for (int i = 0; i < nnal; i++) {
            if (nal[i].i_type == NAL_SPS || nal[i].i_type == NAL_PPS || nal[i].i_type == NAL_SEI ||
            nal[i].i_type == NAL_AUD || nal[i].i_type == NAL_FILLER) {
                continue;
            }
            memcpy(out_buffer_data + offset, nal[i].p_payload, nal[i].i_payload);
            offset += nal[i].i_payload;
        }
        return total_size;
    }

    return frame_size;
}

void X264Encoder::close()
{
    //x264_t *encoder = x264encoder->encoder;
    if (encoder) {
        x264_encoder_close(encoder);
        encoder = nullptr;
    }
}

extern "C" {

JNIEXPORT jint JNICALL Java_com_facebook_encapp_BufferX264Encoder_x264Init(JNIEnv *env, jobject thisObj,
                                                                               jobject x264ConfigParamsObj, jobject x264ParamsObj, 
                                                                               jobject x264CropRectObj, jobject x264NalObj, 
                                                                               jobject x264AnalyseObj, jobject x264VuiObj, 
                                                                               jobject x264RcObj, jbyteArray headerArray) {
    return X264Encoder::init(env, thisObj, x264ConfigParamsObj, x264ParamsObj, x264CropRectObj,
                                           x264NalObj, x264AnalyseObj, x264VuiObj, x264RcObj, headerArray);
}

JNIEXPORT jint JNICALL Java_com_facebook_encapp_BufferX264Encoder_x264Encode(JNIEnv *env, jobject thisObj, jbyteArray yBuffer, 
                                                                             jbyteArray uBuffer, jbyteArray vBuffer, 
                                                                             jbyteArray outBuffer, jint width, jint height) {
    return X264Encoder::getInstance().encode(env, thisObj, yBuffer, uBuffer, vBuffer, outBuffer, width, height);
}

JNIEXPORT void JNICALL Java_com_facebook_encapp_BufferX264Encoder_x264Close(JNIEnv *env, jobject thisObj) {
    X264Encoder::getInstance().close();
}

}
