/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class com_intel_penguinpeakutils_VsockClientImpl */

#ifndef _Included_com_intel_penguinpeakutils_VsockClientImpl
#define _Included_com_intel_penguinpeakutils_VsockClientImpl
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     com_intel_penguinpeakutils_VsockClientImpl
 * Method:    socketCreate
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_intel_penguinpeakutils_VsockClientImpl_socketCreate
  (JNIEnv *, jobject);

/*
 * Class:     com_intel_penguinpeakutils_VsockClientImpl
 * Method:    connect
 * Signature: (Lcom/intel/penguinpeakutils/VsockAddress;)V
 */
JNIEXPORT void JNICALL Java_com_intel_penguinpeakutils_VsockClientImpl_connect
  (JNIEnv *, jobject, jobject);

/*
 * Class:     com_intel_penguinpeakutils_VsockClientImpl
 * Method:    close
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_intel_penguinpeakutils_VsockClientImpl_close
  (JNIEnv *, jobject);

/*
 * Class:     com_intel_penguinpeakutils_VsockClientImpl
 * Method:    write
 * Signature: ([BII)V
 */
JNIEXPORT void JNICALL Java_com_intel_penguinpeakutils_VsockClientImpl_write
  (JNIEnv *, jobject, jbyteArray, jint, jint);

/*
 * Class:     com_intel_penguinpeakutils_VsockClientImpl
 * Method:    read
 * Signature: ([BII)I
 */
JNIEXPORT jint JNICALL Java_com_intel_penguinpeakutils_VsockClientImpl_read
  (JNIEnv *, jobject, jbyteArray, jint, jint);

#ifdef __cplusplus
}
#endif
#endif
