#include <jni.h>
#include <android/log.h>
#include "x264.h"
/* Header for class MainActivity */

#ifndef JNIx264
#define JNIx264

class X264Encoder {
public:
    static X264Encoder& getInstance();

    static bool init(JNIEnv *env, jobject thisObj, jobject x264ConfigParamsObj, jobject x264ParamsObj, jobject x264CropRectObj,
              jobject x264NalObj, jobject x264AnalyseObj, jobject x264VuiObj, jobject x264RcObj);

    int encode(JNIEnv *env, jobject thisObj, jbyteArray yBuffer, jbyteArray uBuffer, jbyteArray vBuffer,
               jbyteArray outBuffer, jint width, jint height);

    void close(JNIEnv *env, jobject thisObj);

    x264_t *encoder;
    x264_nal_t *nal;
    int nnal;

private:
    X264Encoder();
    ~X264Encoder();

    X264Encoder(const X264Encoder&) = delete;
    X264Encoder& operator=(const X264Encoder&) = delete;

    static X264Encoder* x264encoder;

};

#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     SurfaceEncoder
 * Method:    JNIx264
 * Signature: (II)D
 */

#define LOG_TAG "x264-encoder"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

//JNIEXPORT jboolean Java_com_facebook_encapp_BufferX264Encoder_x264Init(JNIEnv *env, jobject thisObj, jobject x264ConfigParamsObj,
//                                                                       jobject x264ParamsObj, jobject x264CropRectObj, jobject x264NalObj, jobject x264AnalyseObj,
//                                                                       jobject x264VuiObj, jobject x264RcObj);
//JNIEXPORT jint Java_com_facebook_encapp_BufferX264Encoder_x264Encode(JNIEnv *env, jobject obj, jbyteArray yBuffer, jbyteArray uBuffer, jbyteArray vBuffer,
//                                                                     jbyteArray out_buffer, jint width, jint height);
//JNIEXPORT void Java_com_facebook_encapp_BufferX264Encoder_x264Close(JNIEnv *env, jobject thisObj);

//extern "C" {
    JNIEXPORT jboolean Java_com_facebook_encapp_BufferX264Encoder_x264Init(JNIEnv *env, jobject thisObj,
                                                                               jobject x264ConfigParamsObj, jobject x264ParamsObj,
                                                                               jobject x264CropRectObj, jobject x264NalObj,
                                                                               jobject x264AnalyseObj, jobject x264VuiObj,
                                                                               jobject x264RcObj);

    JNIEXPORT jint Java_com_facebook_encapp_BufferX264Encoder_x264Encode(JNIEnv *env, jobject thisObj, jbyteArray yBuffer,
                                                                             jbyteArray uBuffer, jbyteArray vBuffer,
                                                                             jbyteArray outBuffer, jint width, jint height);

    JNIEXPORT void Java_com_facebook_encapp_BufferX264Encoder_x264Close(JNIEnv *env, jobject thisObj);
//}

#ifdef __cplusplus
}
#endif
#endif