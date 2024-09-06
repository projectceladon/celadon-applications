/*
 * Copyright (C) 2021 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <iostream>
#include <cstring>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/vm_sockets.h>
#include <arpa/inet.h>
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

#define DATA_SIZE_LENGTH 4
#define MAX_CHUNK_LENGTH 8192
#define MAX_DATA_LENGTH 512*1024

static const char *vsockClientImplPath = "com/intel/clipboardagent/VsockClientImpl";
static const char *vsockAddressPath = "com/intel/clipboardagent/VsockAddress";
static const char *javaConnException = "java/net/ConnectException";
static const char *javaIntrIOException = "java/io/InterruptedIOException";
static const char *sunConnResetException = "sun/net/ConnectionResetException";

int read_from_vsock(JNIEnv* env, int sockfd, uint8_t* bytes, uint32_t size) {
    int nread = (jint) recv(sockfd, bytes, size, 0);
    if (nread <= 0) {
        if (nread < 0 && errno != ENOTCONN) {
                env->ThrowNew(env->FindClass(javaConnException),
                    ("vsock read: Read failed with error no: " + std::to_string(errno)).c_str());
        } else {
                env->ThrowNew(env->FindClass(javaConnException),
                    ("vsock read: Connection is closed by peer."));
        }
        return nread;
    }
    return nread;
}

bool write_to_vsock(JNIEnv* env, int sockfd, uint8_t* bytes, uint32_t size) {
    int n = (int)send(sockfd, bytes, size, 0);
    if (n == JVM_IO_INTR) {
        env->ThrowNew(env->FindClass(javaIntrIOException), 0);
    } else if (n <= 0){
        if (errno == ECONNRESET) {
            env->ThrowNew(env->FindClass(sunConnResetException), "vsock write: Connection reset");
        } else {
            env->ThrowNew(env->FindClass(javaConnException), "vsock write: Write failed");
        }
        return false;
    } else if (n != size) {
        env->ThrowNew(env->FindClass(javaConnException), "vsock write: Failed to write complete msg");
        return false;
    }
    return true;
}

JNIEXPORT void JNICALL Java_com_intel_clipboardagent_VsockClientImpl_socketCreate
  (JNIEnv *env, jobject thisObject) {
    int sock = socket(AF_VSOCK, SOCK_STREAM, 0);

    jclass implement = env->FindClass(vsockClientImplPath);
    jfieldID fdField = env->GetFieldID(implement, "fd", "I");
    env->SetIntField(thisObject, fdField, sock);
}

JNIEXPORT void JNICALL Java_com_intel_clipboardagent_VsockClientImpl_connect
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
        if (errno == EALREADY || errno == EISCONN ) {
            env->ThrowNew(env->FindClass(javaConnException),
                ("Connect failed: " + std::to_string(errno)).c_str());
        }
    }
}

JNIEXPORT void JNICALL Java_com_intel_clipboardagent_VsockClientImpl_close
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

JNIEXPORT void JNICALL Java_com_intel_clipboardagent_VsockClientImpl_write
  (JNIEnv * env, jobject thisObject, jbyteArray b, jint offset, jint len) {
    jclass implement = env->FindClass(vsockClientImplPath);
    jfieldID fdField = env->GetFieldID(implement, "fd", "I");
    int s = (int)env->GetIntField(thisObject, fdField);

    if (s == -1) {
        env->ThrowNew(env->FindClass(javaConnException), "vsock write: Socket is already closed.");
        return;
    }


    // Send the actual data
    char buffer[MAX_CHUNK_LENGTH];
    while(len > 0) {
        int chunkLen = min(MAX_CHUNK_LENGTH, len);

        env->GetByteArrayRegion(b, offset, chunkLen, (jbyte *)buffer);
        if(!write_to_vsock(env, s, (uint8_t*)buffer, chunkLen)) {
            return;
        }
        len -= chunkLen;
        offset += chunkLen;
    }
    return;
}

JNIEXPORT jint JNICALL Java_com_intel_clipboardagent_VsockClientImpl_read
  (JNIEnv * env, jobject thisObject, jbyteArray b, jint off, jint len) {
    jclass implement = env->FindClass(vsockClientImplPath);
    jfieldID fdField = env->GetFieldID(implement, "fd", "I");
    int s = (int)env->GetIntField(thisObject, fdField);

    if (s == -1) {
        env->ThrowNew(env->FindClass(javaConnException), "vsock read: Socket is already closed");
        return -1;
    }
    uint8_t buffer[MAX_CHUNK_LENGTH];
    uint32_t remaining = len;
    while (remaining > 0) {
        int nread = 0;
        uint32_t chunkLen = min(remaining, MAX_CHUNK_LENGTH);
        if ((nread = read_from_vsock(env, s, buffer, chunkLen)) <= 0) {
            ALOGE("vsock read: Failed to read complete msg");
        }
        env->SetByteArrayRegion(b, off, nread, (jbyte *)buffer);
        remaining -= nread;
        off += nread;
    }

    return (jint)len;
}

JNIEXPORT void JNICALL Java_com_intel_clipboardagent_VsockClientImpl_writeInt
  (JNIEnv *env, jobject thisObject, jint length) {
    jclass implement = env->FindClass(vsockClientImplPath);
    jfieldID fdField = env->GetFieldID(implement, "fd", "I");
    int s = (int)env->GetIntField(thisObject, fdField);

    if (s == -1) {
        env->ThrowNew(env->FindClass(javaConnException), "vsock read: Socket is already closed");
        return;
    }

    {
        uint32_t size = length;
        size = htonl(size);
        uint8_t* buffer = (uint8_t*)&size;
        if (!write_to_vsock(env, s, buffer, DATA_SIZE_LENGTH)) {
            return;
        }
    }
}


JNIEXPORT jint JNICALL Java_com_intel_clipboardagent_VsockClientImpl_readInt
  (JNIEnv *env, jobject thisObject) {
    jclass implement = env->FindClass(vsockClientImplPath);
    jfieldID fdField = env->GetFieldID(implement, "fd", "I");
    int s = (int)env->GetIntField(thisObject, fdField);

    if (s == -1) {
        env->ThrowNew(env->FindClass(javaConnException), "vsock read: Socket is already closed");
        return -1;
    }

    uint32_t size = 0;
    {
        uint8_t buffer[DATA_SIZE_LENGTH + 1] = {0};
        if (read_from_vsock(env, s, buffer, DATA_SIZE_LENGTH) != DATA_SIZE_LENGTH) {
            ALOGE("vsock read: Failed to read data size.");
            return -1;
        }
        size = *(uint32_t*)buffer;
        size = ntohl(size);
    }
    return (jint)size;
}
