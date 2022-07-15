#include <grpc++/grpc++.h>
#include <unistd.h>
#include "grpc/client.h"

#include "proto/appstatus.h"
#include "proto/notification.h"

using namespace ::com::android::guest;

bool NotificationImpl::Stub::Observe_Response(const NotificationResponse* msg) {
    if (msg != nullptr) {
        std::cout << "Received Notification message:" << msg->message() << std::endl;
        char cmd[512];
        //snprintf(cmd, sizeof(cmd), "notify-send -a %s '%s'", msg->package().c_str(), msg->message().c_str());
        snprintf(cmd, sizeof(cmd), "python3 /opt/cfc/host_agent/bin/notify.py -P %s -k '%s' -g '%s' -m '%s' -p %d", msg->package().c_str(), msg->key().c_str(), msg->group_key().c_str(), msg->message().c_str(), msg->priority());
        std::cout << "Running cmd: " << cmd << std::endl;
        system(cmd);
    }
   return true;
}

int main() {
    auto client = taf::Client::Create("vsock:3:50051", taf::DispatchKind::kgRPC);
    AppStatusImpl::Stub appStatusStub;
    NotificationImpl::Stub notificationStub;
    std::cout << "Registering Stub" << std::endl;
    client->Bind(&appStatusStub);
    client->Bind(&notificationStub);
    AppStatusRequest req;
    NotificationRequest notificationReq;
    std::cout << "Initiate AppStatus stream" << std::endl;
    appStatusStub.Observe(&req);
    std::cout << "Initiate Notification stream" << std::endl;
    notificationStub.Observe(&notificationReq);
    // Wait for both API streams to end
    auto appStatusStream = appStatusStub.GET_API_STREAM(Observe);
    if(appStatusStream != nullptr) {
        appStatusStream->WaitForFinish();
    }
    auto notificationStream = notificationStub.GET_API_STREAM(Observe);
    if (notificationStream != nullptr) {
        notificationStream->WaitForFinish();
    }
    std::cout << "Stream ended" << std::endl;
}
