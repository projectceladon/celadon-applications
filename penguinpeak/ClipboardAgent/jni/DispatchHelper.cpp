#include "DispatchHelper.h"
#include <string.h>
#include <jni.h>
#include "adapter.h"

#undef LOG_TAG
#define LOG_TAG "DispatchHelper"

JNIEXPORT void JNICALL Java_com_intel_clipboardagent_DispatchHelper_registerComponent(JNIEnv *env, jobject thisObject, jstring className) {
    env->GetJavaVM(&gVm);
    std::string name = env->GetStringUTFChars(className, 0);
    LOG_INFO("Attempting to register Service %s\n", name.c_str());
    ServiceAdapter* adapter = AdapterFactory::GetAdapter(name);
    if (adapter != nullptr) {
        adapter->Register();
    } else {
        LOG_ERROR("Service adapter not found for %s\n", name.c_str());
    }
}

JNIEXPORT void JNICALL Java_com_intel_clipboardagent_DispatchHelper_sendMsg(JNIEnv *env, jobject thisObject, jstring className, jobject msg, jlong handle) {
    JavaObjectHelper jobjHelper(msg);
    std::string name = env->GetStringUTFChars(className, 0);
    ServiceAdapter* adapter = AdapterFactory::GetAdapter(name);
    if (adapter == nullptr) {
        LOG_ERROR("Service adapter not found for %s\n", name.c_str());
	return;
    }
    adapter->SendResponse(&jobjHelper);
}

JNIEXPORT void JNICALL Java_com_intel_clipboardagent_DispatchHelper_start(JNIEnv *env, jobject thisObject) {
     if (!g_server_->Start()) {
         LOG_ERROR("FATAL! Failed to start server");
     }
}

JNIEXPORT void JNICALL Java_com_intel_clipboardagent_DispatchHelper_stop(JNIEnv *env, jobject thisObject) {
     g_server_->Stop();
     delete g_server_;
     g_server_ = nullptr;
     AdapterFactory::RemoveAll();
}

