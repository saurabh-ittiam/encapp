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


JNIEXPORT jboolean
Java_com_facebook_encapp_BufferX264Encoder_x264Init(JNIEnv *env, jobject thisObj, jint width, jint height);
JNIEXPORT jint
Java_com_facebook_encapp_BufferX264Encoder_x264Encode(JNIEnv *env, jobject thisObj, jbyteArray inputData, jbyteArray outputData, jint size);
JNIEXPORT void
Java_com_facebook_encapp_BufferX264Encoder_x264Close(JNIEnv *env, jobject thisObj);

#ifdef __cplusplus
}
#endif
#endif