#include "adapter.h"

JavaVM* gVm = nullptr;
taf::gRPCServer* g_server_ = nullptr;
std::map<std::string, ServiceAdapter*> AdapterFactory::jadapter_map;

static std::map< std::string, jclass > jclass_map;

JNIEnv* getenv() {
    JNIEnv *env = nullptr;
    int getEnvStat = gVm->GetEnv((void **)&env, JNI_VERSION_1_6);
    if (getEnvStat == JNI_EDETACHED) {
        if (gVm->AttachCurrentThread(&env, NULL) != 0) {
            LOG_ERROR("GetEnv: not attached. Failed to attach");
        }
    } else if (getEnvStat == JNI_OK) {
        //
    } else if (getEnvStat == JNI_EVERSION) {
        LOG_ERROR("GetEnv: version not supported");
    }
    return env;
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    JNIEnv *env;
    std::string compName;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        LOG_ERROR("In OnLoad, failed to GetEnv");
        return JNI_ERR;
    }
    jclass tmp = nullptr;
    tmp = env->FindClass("com/intel/clipboardagent/ClipboardComponent");
    if (tmp!= nullptr) {
        jclass_map.insert({"ClipboardComponent", (jclass)env->NewGlobalRef(tmp)});
    }
    tmp = env->FindClass("com/intel/clipboardagent/AppstatusComponent");
    if (tmp!= nullptr) {
        compName = "AppstatusComponent";
        jclass_map.insert({compName , (jclass)env->NewGlobalRef(tmp)});
	    AdapterFactory::AddAdapter(compName, new AppStatusAdapter(compName));
    }
    tmp = env->FindClass("com/intel/clipboardagent/NotificationComponent");
    if (tmp!= nullptr) {
        compName = "NotificationComponent";
        jclass_map.insert({compName , (jclass)env->NewGlobalRef(tmp)});
	    AdapterFactory::AddAdapter(compName, new NotificationAdapter(compName));
    }
    // Setup gRPC Server
    g_server_ = new taf::gRPCServer("vsock:-1:50051");
    //g_server_ = new taf::gRPCServer("127.0.0.1:8787");
    LOG_INFO("Created gRPC Server\n");
    return JNI_VERSION_1_6;
}

void AdapterFactory::AddAdapter(std::string& name, ServiceAdapter* adapter) {
    if (GetAdapter(name) == nullptr) {
        jadapter_map.insert({name, adapter});
    }
}

ServiceAdapter* AdapterFactory::GetAdapter(std::string& name) {
    std::map< std::string, ServiceAdapter*>::iterator it;
    it = jadapter_map.find(name);
    return (it != jadapter_map.end()) ? it->second: nullptr;
}

void AdapterFactory::RemoveAll() {
    for (auto adapter : jadapter_map) {
        delete adapter.second;
    }
    jadapter_map.clear();
}

JavaComponent::~JavaComponent(){
    JNIEnv* env = getenv();
    jclass reqClass = GetJClass();
    jobject singleInstance = GetSingletonInstance(reqClass);
    jmethodID reqMethod = env->GetMethodID(reqClass, "stop", "()V");
    env->CallVoidMethod(singleInstance, reqMethod); 
}

void JavaComponent::init() {
    JNIEnv* env = getenv();
    jclass reqClass = GetJClass();
    jobject singleInstance = GetSingletonInstance(reqClass);
    jmethodID reqMethod = env->GetMethodID(reqClass, "init", "()V");
    env->CallVoidMethod(singleInstance, reqMethod); 
}

void JavaComponent::stop() {
    JNIEnv* env = getenv();
    jclass reqClass = GetJClass();
    jobject singleInstance = GetSingletonInstance(reqClass);
    jmethodID reqMethod = env->GetMethodID(reqClass, "stop", "()V");
    env->CallVoidMethod(singleInstance, reqMethod);
}

void JavaComponent::ProcessMsg(std::string& msg, uint64_t hndl) {
    LOG_INFO("Process msg - %s\n", msg.c_str());
    JNIEnv *env = getenv();
    jclass reqClass = GetJClass();
    jobject singleInstance = GetSingletonInstance(reqClass);
    jmethodID reqMethod = env->GetMethodID(reqClass, "processMsg", "(Ljava/lang/String;J)V");
    jstring str = env->NewStringUTF(msg.c_str());
    env->CallVoidMethod(singleInstance, reqMethod, str, static_cast<jlong>(hndl));
}

jclass JavaComponent::GetJClass() {
    std::map< std::string, jclass >::iterator it;
    jclass reqClass = nullptr; 
    it = jclass_map.find(java_class_name.c_str());
    if (it != jclass_map.end()) {
        reqClass = it->second;
    } 
    return reqClass;
}

jobject JavaComponent::GetSingletonInstance(jclass reqClass) {
    JNIEnv *env = getenv();
    std::string sig = "()Lcom/intel/clipboardagent/"+java_class_name+";";
    jmethodID instMethod = env->GetStaticMethodID(reqClass, "getInstance", sig.c_str());
    jobject singleInstance = env->CallStaticObjectMethod(reqClass, instMethod);
    return singleInstance;
}

void JavaObjectHelper::init() {
    JNIEnv *env = getenv();
    class_ = env->GetObjectClass(obj_);
}

int JavaObjectHelper::GetIntField(const char* fldNm) {
    JNIEnv *env = getenv();
    jfieldID fld = env->GetFieldID(class_, fldNm, "I");
    return env->GetIntField(obj_, fld);
}

const char* JavaObjectHelper::GetStringField(const char* fldNm) {
    JNIEnv *env = getenv();
    jfieldID fld = env->GetFieldID(class_, fldNm, "Ljava/lang/String;");
    jstring str = (jstring) env->GetObjectField(obj_, fld);
    return env->GetStringUTFChars(str, 0);
}

void AppStatusAdapter::SendResponse(JavaObjectHelper* jobjHelper) {
    AppStatusResponse resp;
    resp.set_app_name(jobjHelper->GetStringField("app_name"));
    if (svc_ != nullptr && !svc_->Observe_Response(&resp)) {
        stop();
        auto stream = svc_->GET_API_STREAM(Observe);
        if (stream != nullptr) {
            // Something wrong with the stream, end the call
            stream->WritesDone();
            stream->Finish();
        }
    }
}

taf::Service* AppStatusAdapter::GetService() {
    if (svc_ == nullptr) {
        svc_ = new AppStatusAdapter::Service(this);
    }
    return svc_;
}

bool AppStatusAdapter::Service::Observe(const AppStatusRequest* /*msg*/) {
    LOG_INFO("Initializing AppStatus");
    adapter_->init();
    return true;
}

void NotificationAdapter::SendResponse(JavaObjectHelper* jobjHelper) {
    NotificationResponse resp;
    resp.set_package(jobjHelper->GetStringField("packageName"));
    resp.set_key(jobjHelper->GetStringField("key"));
    resp.set_group_key(jobjHelper->GetStringField("groupKey"));
    resp.set_message(jobjHelper->GetStringField("message"));
    resp.set_priority(jobjHelper->GetIntField("priority"));
    if (svc_ != nullptr && !svc_->Observe_Response(&resp)) {
        stop();
        auto stream = svc_->GET_API_STREAM(Observe);
        if (stream != nullptr) {
            // Something wrong with the stream, end the call
            stream->WritesDone();
            stream->Finish();
        }
    }
}

taf::Service* NotificationAdapter::GetService() {
    if (svc_ == nullptr) {
        svc_ = new NotificationAdapter::Service(this);
    }
    return svc_;
}

bool NotificationAdapter::Service::Observe(const NotificationRequest* /*msg*/) {
    LOG_INFO("Initializing Notification sync");
    adapter_->init();
    return true;
}

