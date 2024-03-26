#include "VsockMsgDispatcher.h"
#include "DispatchHelper.h"
#include <string.h>
#include <jni.h>

using namespace vsock;
std::map< std::string, std::vector<MSG_TYPE> > comp_msg_map {
    {"ClipboardComponent", {MSG_TYPE_CLIPBOARD}},
    {"AppstatusComponent", {MSG_TYPE_APPSTATUS}}
};
std::map< std::string, jclass > jclass_map;

static JavaVM* gVm = nullptr;
JNIEnv* getenv() {
            JNIEnv *env = nullptr;
            int getEnvStat = gVm->GetEnv((void **)&env, JNI_VERSION_1_6);
            if (getEnvStat == JNI_EDETACHED) {
                LOGIT("GetEnv: not attached");
                if (gVm->AttachCurrentThread(&env, NULL) != 0) {
                    LOGIT("Failed to attach"); 
                }
            } else if (getEnvStat == JNI_OK) {
              //
            } else if (getEnvStat == JNI_EVERSION) {
                LOGIT("GetEnv: version not supported");
            }
	    return env;	
}

class JavaComponent:public Component {
   public:
      std::string java_class_name;
      std::vector<MSG_TYPE> msg_list; 
      JavaComponent(std::string name) {
         std::map< std::string, std::vector<MSG_TYPE> >::iterator it;     
         java_class_name = name;
	 it = comp_msg_map.find(name);
	 if (it != comp_msg_map.end()) {
            msg_list = it->second;
         }	    
      }
      virtual ~JavaComponent(){
            JNIEnv* env = getenv();
	    jclass reqClass = GetJClass();
	    jobject singleInstance = GetSingletonInstance(reqClass);
            jmethodID reqMethod = env->GetMethodID(reqClass, "stop", "()V");
            env->CallVoidMethod(singleInstance, reqMethod); 

       }
      virtual void init() {
         LOGIT("init");
         JNIEnv* env = getenv();
	 jclass reqClass = GetJClass();
	 jobject singleInstance = GetSingletonInstance(reqClass);
         jmethodID reqMethod = env->GetMethodID(reqClass, "init", "()V");
         env->CallVoidMethod(singleInstance, reqMethod); 
    }

    virtual void ProcessMsg(Message& msg, uint64_t hndl) {
        LOGIT("Process msg - %s\n", msg.payload);
        JNIEnv *env = getenv();
	jclass reqClass = GetJClass();
	jobject singleInstance = GetSingletonInstance(reqClass);
        jmethodID reqMethod = env->GetMethodID(reqClass, "processMsg", "(Ljava/lang/String;J)V");
        jstring str = env->NewStringUTF(msg.payload);
        env->CallVoidMethod(singleInstance, reqMethod, str, static_cast<jlong>(hndl));
    }
   private:
       jclass GetJClass() {
           std::map< std::string, jclass >::iterator it;
           jclass reqClass = nullptr; 
           it = jclass_map.find(java_class_name.c_str());
           if (it != jclass_map.end()) {
               reqClass = it->second;
           } 
	   return reqClass;
       }

       jobject GetSingletonInstance(jclass reqClass) {
           JNIEnv *env = getenv();
	   std::string sig = "()Lcom/intel/clipboardagent/"+java_class_name+";";
           jmethodID instMethod = env->GetStaticMethodID(reqClass, "getInstance", sig.c_str());
           jobject singleInstance = env->CallStaticObjectMethod(reqClass, instMethod);
	   return singleInstance;
       }
};

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    JNIEnv *env;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        LOGIT("In OnLoad, failed to GetEnv"); 
        return JNI_ERR;
    }
    jclass tmp = nullptr;
    tmp = env->FindClass("com/intel/clipboardagent/ClipboardComponent");
    if (tmp!= nullptr) {
        jclass_map.insert({"ClipboardComponent", (jclass)env->NewGlobalRef(tmp)});
    }
    tmp = env->FindClass("com/intel/clipboardagent/AppstatusComponent");
    if (tmp!= nullptr) {
        jclass_map.insert({"AppstatusComponent", (jclass)env->NewGlobalRef(tmp)});
    }
    return JNI_VERSION_1_6;
}

 
JNIEXPORT void JNICALL Java_com_intel_clipboardagent_DispatchHelper_registerComponent(JNIEnv *env, jobject thisObject, jstring className) {
    MsgDispatcher* dispatcher = MsgDispatcher::getInstance();
    env->GetJavaVM(&gVm);
    std::string name = env->GetStringUTFChars(className, 0);
    JavaComponent* javaComponent = new JavaComponent(name);
    dispatcher->RegisterComponent(javaComponent->msg_list, javaComponent);
}


JNIEXPORT void JNICALL Java_com_intel_clipboardagent_DispatchHelper_sendMsg(JNIEnv *env, jobject thisObject, jstring className, jstring msg, jlong handle) {
    MsgDispatcher* dispatcher = MsgDispatcher::getInstance();
    std::string payload = env->GetStringUTFChars(msg, 0);  
    int size = env->GetStringUTFLength(msg);
    std::vector<MSG_TYPE> msg_list;
    std::map< std::string, std::vector<MSG_TYPE> >::iterator it;     
    std::string name = env->GetStringUTFChars(className, 0);
    it = comp_msg_map.find(name);
    if (it != comp_msg_map.end()) {
        msg_list = it->second;
    }
    if (handle == 0) {
        handle = dispatcher->GetHandleForMsgType(msg_list.front());
    }
    dispatcher->SendMsg(handle, msg_list.front(), payload.c_str(), size);
}

 
JNIEXPORT void JNICALL Java_com_intel_clipboardagent_DispatchHelper_start(JNIEnv *env, jobject thisObject) {
     MsgDispatcher* dispatcher = MsgDispatcher::getInstance();
     dispatcher->Start();
}

JNIEXPORT void JNICALL Java_com_intel_clipboardagent_DispatchHelper_stop(JNIEnv *env, jobject thisObject) {
     MsgDispatcher* dispatcher = MsgDispatcher::getInstance();
     dispatcher->Stop();
}
