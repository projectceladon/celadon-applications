#include <jni.h>
#include "proto/appstatus.h"
#include "proto/notification.h"
#include "grpc/server.h"

extern JavaVM* gVm;
extern taf::gRPCServer* g_server_;

using namespace com::android::guest;

class JavaComponent {
public:
    JavaComponent(std::string name) { java_class_name = name; }
    ~JavaComponent();
    void init();
    void stop();
    void ProcessMsg(std::string& msg, uint64_t hndl);
private:
    jclass GetJClass();
    jobject GetSingletonInstance(jclass reqClass);

    std::string java_class_name;
};

class JavaObjectHelper {
public:
    JavaObjectHelper(jobject obj) { obj_ = obj; init(); }
    int GetIntField(const char* fldNm);
    const char* GetStringField(const char* fldNm);
private:
    void init();

    jobject obj_;
    jclass class_;
};

class ServiceAdapter {
public:
    ServiceAdapter(std::string& name) { comp_ = new JavaComponent(name); }
    virtual ~ServiceAdapter() {
        delete comp_;
	    comp_ = nullptr;
    }
    virtual void SendResponse(JavaObjectHelper* payload) = 0;
    inline void Register() {
        g_server_->RegisterService(GetService());
    }
    inline void init() { comp_->init(); }
    inline void stop() { comp_->stop(); }

protected:
    inline void ProcessRequest(std::string& payload) {
        comp_->ProcessMsg(payload, 0);
    }
    virtual taf::Service* GetService() = 0;
    JavaComponent* comp_;
};

class AppStatusAdapter : public ServiceAdapter {
public:
    AppStatusAdapter(std::string& name) : ServiceAdapter(name), svc_(nullptr) { }
    virtual ~AppStatusAdapter() {
	if (svc_ != nullptr) {
	    delete svc_;
	    svc_ = nullptr;
	}
    }
    void SendResponse(JavaObjectHelper* payload) override;

    class Service : public AppStatusImpl::Service {
    public:
        Service(ServiceAdapter* adapter) { adapter_ = adapter; }
	bool Observe(const AppStatusRequest*) override;

	ServiceAdapter* adapter_;
    };

protected:
    taf::Service* GetService() override;
private:
    Service* svc_;   
};

class NotificationAdapter : public ServiceAdapter {
public:
    NotificationAdapter(std::string& name) : ServiceAdapter(name), svc_(nullptr) { }
    virtual ~NotificationAdapter() {
	if (svc_ != nullptr) {
	    delete svc_;
	    svc_ = nullptr;
	}
    }
    void SendResponse(JavaObjectHelper* payload) override;

    class Service : public NotificationImpl::Service {
    public:
        Service(ServiceAdapter* adapter) { adapter_ = adapter; }
	bool Observe(const NotificationRequest* /*msg*/) override;

        ServiceAdapter* adapter_;
    };

protected:
    taf::Service* GetService() override;
private:
    Service* svc_;   
};

class AdapterFactory {
public:
    static void AddAdapter(std::string& name, ServiceAdapter* adapter);
    static ServiceAdapter* GetAdapter(std::string& name);
    static void RemoveAll();
private:
    static std::map<std::string, ServiceAdapter*> jadapter_map;
};

