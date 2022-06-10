#include <grpc++/grpc++.h>
#include <unistd.h>
#include "grpc/client.h"

#include "proto/appstatus.h"

using namespace ::com::android::guest;

bool AppStatusImpl::Stub::Observe_Response(const AppStatusResponse* msg) {
    if (msg != nullptr) {
        std::cout << "Received AppStatus message:" << msg->app_name() << std::endl;
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "/opt/cfc/mwc/bin/msg_agent localhost 3000 CRASHAPP %s", msg->app_name().c_str());
        std::cout << "Running cmd: " << cmd << std::endl;
        system(cmd);
    }
   return true;
}

