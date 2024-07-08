#include <jni.h>
#include <stdlib.h>
#include "x264.h"
#include "JNIx264.h"

#include <iostream>
#include <fstream>
#include <memory>
using namespace std;

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
    int size_of_headers;

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

    jfieldID presetFieldID = env->GetFieldID(x264ConfigParamsClass, "preset", "Ljava/lang/String;");
    jfieldID iThreadsFieldID = env->GetFieldID(x264ParamsClass, "i_threads", "I");
    jfieldID iWidthFieldID = env->GetFieldID(x264ParamsClass, "i_width", "I");
    jfieldID iHeightFieldID = env->GetFieldID(x264ParamsClass, "i_height", "I");
    jfieldID iCspFieldID = env->GetFieldID(x264ParamsClass, "i_csp", "I");
    jfieldID iBitdepthFieldID = env->GetFieldID(x264ParamsClass, "i_bitdepth", "I");

    jstring presetValueObj = (jstring)env->GetObjectField(x264ConfigParamsObj, presetFieldID);
    const char *presetValue = env->GetStringUTFChars(presetValueObj, NULL);
    jint iThreadsValue = env->GetIntField(x264ParamsObj, iThreadsFieldID);
    jint iWidthValue = env->GetIntField(x264ParamsObj, iWidthFieldID);
    jint iHeightValue = env->GetIntField(x264ParamsObj, iHeightFieldID);
    jint iCspValue = env->GetIntField(x264ParamsObj, iCspFieldID);
    jint iBitdepthValue = env->GetIntField(x264ParamsObj, iBitdepthFieldID);

    if (x264_param_default_preset(&x264Params, presetValue, "zerolatency") < 0) {
        LOGI("Failed to set preset: %s", presetValue);
    }

    // Mapping to x264 structure members
    x264Params.i_threads = iThreadsValue;
    x264Params.i_width = iWidthValue;
    x264Params.i_height = iHeightValue;
    x264Params.i_csp = iCspValue;
    x264Params.i_bitdepth = iBitdepthValue;

    x264encoder->encoder = x264_encoder_open(&x264Params);
    x264_t *encoder = x264encoder->encoder;
    if(!encoder)
    {
        LOGI("Failed encoder_open");
        return -1;
    }

    x264_encoder_parameters(encoder, &x264Params);
    size_of_headers = x264_encoder_headers(encoder, &(x264encoder->nal), &(x264encoder->nnal));

    x264_nal_t *nal = x264encoder->nal;
    int nnal = x264encoder->nnal;

    int offset = 0;
    if (headerArray != nullptr) {
        jsize headerArraySize = env->GetArrayLength(headerArray);
        jbyte* headerArrayBuffer = env->GetByteArrayElements(headerArray, NULL);

        for (int i = 0; i < nnal; i++) {
            if (nal[i].i_type == NAL_SPS || nal[i].i_type == NAL_PPS || nal[i].i_type == NAL_SEI ||
                nal[i].i_type == NAL_AUD || nal[i].i_type == NAL_FILLER) {
                // Check if there is enough space in the header array
                if (offset + nal[i].i_payload > headerArraySize) {
                    env->ReleaseByteArrayElements(headerArray, headerArrayBuffer, 0);
                    return -1;
                }

                memcpy(headerArrayBuffer + offset, nal[i].p_payload, nal[i].i_payload);
                offset += nal[i].i_payload;
            }
        }
        env->ReleaseByteArrayElements(headerArray, headerArrayBuffer, 0);
    }
    env->ReleaseStringUTFChars(presetValueObj, presetValue);
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

    x264_picture_init(&pic_in);
    pic_in.img.i_csp = 2;
    pic_in.img.i_plane = 3;

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
        // TODO: Added total_size = 2 for debugging purpose
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
