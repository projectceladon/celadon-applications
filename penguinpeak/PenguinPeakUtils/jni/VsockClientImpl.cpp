#include <iostream>
#include <cstring>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/vm_sockets.h>
#include <unistd.h>
#include <errno.h>

#include <jni.h>
#include <VsockClientImpl.h>

#define JVM_IO_INTR (-2)
#ifndef bufferFER_LEN
#define bufferFER_LEN 65536
#endif
#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#define LOG_TAG "vsock"
#include <android/log.h>

#define ALOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#define ALOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define ALOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)


static const char *vsockClientImplPath = "com/intel/penguinpeakutils/VsockClientImpl";
static const char *vsockAddressPath = "com/intel/penguinpeakutils/VsockAddress";
static const char *javaConnException = "java/net/ConnectException";
static const char *javaIntrIOException = "java/io/InterruptedIOException";
static const char *sunConnResetException = "sun/net/ConnectionResetException";

JNIEXPORT void JNICALL Java_com_intel_penguinpeakutils_VsockClientImpl_socketCreate
  (JNIEnv *env, jobject thisObject) {
    int sock = socket(AF_VSOCK, SOCK_STREAM, 0);

    jclass implement = env->FindClass(vsockClientImplPath);
    jfieldID fdField = env->GetFieldID(implement, "fd", "I");
    env->SetIntField(thisObject, fdField, sock);
}

JNIEXPORT void JNICALL Java_com_intel_penguinpeakutils_VsockClientImpl_connect
  (JNIEnv *env, jobject thisObject, jobject addr) {
    jclass implement = env->FindClass(vsockClientImplPath);
    jfieldID fdField = env->GetFieldID(implement, "fd", "I");
    int sock = (int)env->GetIntField(thisObject, fdField);

    if (sock == -1) {
        env->ThrowNew(env->FindClass(javaConnException), "vsock: Socket is closed");
        return;
    }

    jclass vsockAddress = env->FindClass(vsockAddressPath);
    jfieldID cidField = env->GetFieldID(vsockAddress, "cid", "I");
    jfieldID portField = env->GetFieldID(vsockAddress, "port", "I");


    struct sockaddr_vm sock_addr;
    std::memset(&sock_addr, 0, sizeof(struct sockaddr_vm));
    sock_addr.svm_family = AF_VSOCK;
    sock_addr.svm_port = (int)env->GetIntField(addr, portField);
    sock_addr.svm_cid = (int)env->GetIntField(addr, cidField);
    int status = connect(sock, (struct sockaddr *) &sock_addr, sizeof(struct sockaddr_vm));
    if (status != 0) {
        if (errno == EALREADY) {
            env->ThrowNew(env->FindClass(javaConnException),
                ("Connect failed: " + std::to_string(errno)).c_str());
        }
    }
}

JNIEXPORT void JNICALL Java_com_intel_penguinpeakutils_VsockClientImpl_close
  (JNIEnv *env, jobject thisObject) {
    jclass implement = env->FindClass(vsockClientImplPath);
    jfieldID fdField = env->GetFieldID(implement, "fd", "I");
    int s = (int)env->GetIntField(thisObject, fdField);

    if (s == -1) {
        env->ThrowNew(env->FindClass(javaConnException), "vsock close: Socket is already closed.");
        return;
    }

    int status = close(s);

    env->SetIntField(thisObject, fdField, -1);
    if (status != 0) {
            env->ThrowNew(env->FindClass(javaConnException),
                ("Close failed: " + std::to_string(errno)).c_str());
    }
}

JNIEXPORT void JNICALL Java_com_intel_penguinpeakutils_VsockClientImpl_write
  (JNIEnv * env, jobject thisObject, jbyteArray b, jint offset, jint len) {
    jclass implement = env->FindClass(vsockClientImplPath);
    jfieldID fdField = env->GetFieldID(implement, "fd", "I");
    int s = (int)env->GetIntField(thisObject, fdField);

    if (s == -1) {
        env->ThrowNew(env->FindClass(javaConnException), "vsock write: Socket is already closed.");
        return;
    }

    char buffer[bufferFER_LEN];
    while(len > 0) {
        int chunk_offset = 0;
        int chunkLen = min(bufferFER_LEN, len);
        int chunkWorkingLen = chunkLen;

        env->GetByteArrayRegion(b, offset, chunkLen, (jbyte *)buffer);

        while(chunkWorkingLen > 0) {
            int n = (int)send(s, buffer + chunk_offset, len, 0);
            if (n > 0) {
                chunkWorkingLen -= n;
                chunk_offset += n;
                continue;
            }
            if (n == JVM_IO_INTR) {
                env->ThrowNew(env->FindClass(javaIntrIOException), 0);
            } else {
                if (errno == ECONNRESET) {
                    env->ThrowNew(env->FindClass(sunConnResetException), "vsock write: Connection reset");
                } else {
                    env->ThrowNew(env->FindClass(javaConnException), "vsock write: Write failed");
                }
            }
            return;
        }
        len -= chunkLen;
        offset += chunkLen;
    }
}

JNIEXPORT jint JNICALL Java_com_intel_penguinpeakutils_VsockClientImpl_read
  (JNIEnv * env, jobject thisObject, jbyteArray b, jint off, jint len) {
    jint nread;

    jclass implement = env->FindClass(vsockClientImplPath);
    jfieldID fdField = env->GetFieldID(implement, "fd", "I");
    int s = (int)env->GetIntField(thisObject, fdField);

    if (s == -1) {
        env->ThrowNew(env->FindClass(javaConnException), "vsock read: Socket is already closed");
        return -1;
    }

    char *bufP = (char *)malloc((size_t)len);

    nread = (jint) recv(s, bufP, len, 0);
    nread = nread < len - 1 ? nread : len - 1;
    /*
     * Append EOF here to since Java inputstream read blocks utils:
     * 1. Meet EOF
     * 2. Data count reaches the count intended to be read.
     */
    bufP[nread] = EOF;

    if (nread <= 0) {
        if (nread < 0) {
                env->ThrowNew(env->FindClass(javaConnException),
                    ("vsock read: Read failed with error no: " + std::to_string(errno)).c_str());
        } else {
                env->ThrowNew(env->FindClass(javaConnException),
                    ("vsock read: Connection is closed by peer."));
        }
    } else {
        env->SetByteArrayRegion(b, off, nread, (jbyte *)bufP);
    }

    free(bufP);
    return nread;
}
