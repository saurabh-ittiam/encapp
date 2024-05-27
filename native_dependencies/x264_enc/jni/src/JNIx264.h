#include <jni.h>
#include <android/log.h>
#include "x264.h"
/* Header for class MainActivity */

#ifndef JNIx264
#define JNIx264
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


JNIEXPORT jint
Java_com_facebook_encapp_BufferX264Encoder_Create(JNIEnv *env, jobject thisObj, jint width, jint height);
JNIEXPORT jbyteArray
Java_com_facebook_encapp_BufferX264Encoder_EncodeFrame(JNIEnv *env, jobject thisObj, jbyteArray frameData);
JNIEXPORT void
Java_com_facebook_encapp_BufferX264Encoder_Close(JNIEnv *env, jobject thisObj);

#ifdef __cplusplus
}
#endif
#endif