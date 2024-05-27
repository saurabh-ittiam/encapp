#include <jni.h>
#include <stdlib.h>
#include "x264.h"
#include "JNIx264.h"

// JNI_FILE_DUMP
#include <iostream>
#include <fstream>
using namespace std;
// JNI_FILE_DUMP

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FILE_NAME 1024
#define FILTER_NAME 32

string jni_x264logfile;

x264_t *encoder;
x264_picture_t pic_in, pic_out;

extern "C" JNIEXPORT jint JNICALL
Java_com_facebook_encapp_BufferX264Encoder_Create(JNIEnv *env, jobject thisObj, jint width, jint height)
{
    x264_param_t param;
    x264_param_default(&param); // Initialize with default values

    param.i_threads = 1;
    LOGI("param.i_threads = %d", param.i_threads);

    param.i_width = width;
    LOGI("param.i_width = %d", param.i_width);

    param.i_height = height;
    LOGI("param.i_height = %d", param.i_height);

    param.i_fps_num = 25;
    LOGI("param.i_fps_num = %d", param.i_fps_num);

    param.i_fps_den = 1;
    LOGI("param.i_fps_den = %d", param.i_fps_den);

    param.i_keyint_max = 25;
    LOGI("param.i_keyint_max = %d", param.i_keyint_max);

    param.b_intra_refresh = 1;
    LOGI("param.b_intra_refresh = %d", param.b_intra_refresh);

    param.rc.i_rc_method = X264_RC_CRF;
    LOGI("param.rc.i_rc_method = %d", param.rc.i_rc_method);

    param.rc.f_rf_constant = 25;
    LOGI("param.rc.f_rf_constant = %f", param.rc.f_rf_constant);

    param.rc.f_rf_constant_max = 35;
    LOGI("param.rc.f_rf_constant_max = %f", param.rc.f_rf_constant_max);

    param.i_sps_id = 7;
    LOGI("param.i_sps_id = %d", param.i_sps_id);

    param.b_repeat_headers = 1;
    LOGI("param.b_repeat_headers = %d", param.b_repeat_headers);

    param.b_annexb = 1;

    encoder = x264_encoder_open(&param);
    x264_picture_alloc(&pic_in, X264_CSP_I420, width, height);

    // Return a success message
    return 1;
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_com_facebook_encapp_BufferX264Encoder_EncodeFrame(JNIEnv *env, jobject obj, jbyteArray frameData)
{
    jbyte *inputFrame = env->GetByteArrayElements(frameData, NULL);
    int frameSize = pic_in.img.i_stride[0] * pic_in.param->i_height;
    memcpy(pic_in.img.plane[0], inputFrame, frameSize);  // Y plane
    memcpy(pic_in.img.plane[1], inputFrame + frameSize, frameSize / 4);  // U plane
    memcpy(pic_in.img.plane[2], inputFrame + frameSize * 5 / 4, frameSize / 4);  // V plane

    x264_nal_t *nals;
    int i_nals;
    int frame_size = x264_encoder_encode(encoder, &nals, &i_nals, &pic_in, &pic_out);
    env->ReleaseByteArrayElements(frameData, inputFrame, 0);

    if (frame_size > 0) {
        jbyteArray output = env->NewByteArray(frame_size);
        env->SetByteArrayRegion(output, 0, frame_size, (jbyte*)nals[0].p_payload);
        return output;
    } else {
        return NULL;
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_facebook_encapp_BufferX264Encoder_Close(JNIEnv *env, jobject obj)
{
    x264_picture_clean(&pic_in);
    x264_encoder_close(encoder);
}
