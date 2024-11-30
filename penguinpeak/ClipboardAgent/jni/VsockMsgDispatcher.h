#ifndef VSOCK_MSG_DISPATCHER_H
#define VSOCK_MSG_DISPATCHER_H

#include <iostream>
#include <cstring>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <queue>
#include <map>
#include <vector>
#include <mutex>

#define NODEBUG 0 //set this to 0 to turn on logging

#ifdef __ANDROID__
#define LOG_TAG "vsock"
#include <android-base/logging.h>
#include <android/log.h>

#define ALOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#define ALOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define ALOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#ifdef NODEBUG
#define LOGIT(...)
#else
#define LOGIT(...) ALOGV(__VA_ARGS__)
#endif
#else
#define CHECK_PTHREAD_CALL(call, args, what)                           \
  do {                                                                 \
    int rc = call args;                                                \
    if (rc != 0) {                                                     \
      errno = rc;                                                      \
      std::cout << #call << " failed for " << (what);                       \
      exit(-1);                                                         \
    }                                                                  \
  } while (false)
#ifdef NODEBUG
#define LOGIT(...)
#else
#define LOGIT(...) printf(__VA_ARGS__);
#endif
#endif

#define ERR(msg) perror(msg);

#define MSG_VERSION 1
#define MAX_CLIENT_CONNECTIONS 4
#define WORKER_POOL 4
#define Q_SIZE 32

namespace vsock {

enum MSG_TYPE : char {
    MSG_TYPE_CAPABILITY,
    MSG_TYPE_CLIPBOARD,
    MSG_TYPE_APPSTATUS,
    MSG_TYPE_INVALID,
};


struct MessageHeader {
    char _ver;
    MSG_TYPE _type;
    int _id;
    int _size;
};

struct Message {
    MessageHeader header;
    char*         payload;
};

class MsgDispatcher;
class Component {
    public:
        Component () { m_bInitDone = false; }
        MsgDispatcher* GetDispatcher() { return dispatcher_; }
        
        virtual ~Component() {}
        virtual void ProcessMsg(Message& msg, uint64_t hndl) {}
        virtual void init() {}
    private:
        void setDispatcher(MsgDispatcher* dispatcher) { dispatcher_ = dispatcher; }
        void Initialize() { 
            if (!m_bInitDone) {
                init();
            }
            m_bInitDone = true;
        }
        bool m_bInitDone;
        MsgDispatcher* dispatcher_;
    friend class MsgDispatcher;
};


class MsgDispatcher {
    public:
        MsgDispatcher();
        bool Start();
        void Stop();
        void RegisterComponent(std::vector<MSG_TYPE>& msg_list, Component* component);
        bool SendMsg(uint64_t hndl, MSG_TYPE msg_type, const char* buffer, int size);
        uint64_t GetHandleForMsgType(MSG_TYPE msg_type);
	static MsgDispatcher *instance;
        static MsgDispatcher *getInstance() {
	    if (instance == nullptr) {
	        instance = new MsgDispatcher();
            }
            return instance;
	}    
    private:

        class Channel {
            public:
                Channel(MsgDispatcher* dispatcher, int fd, int id) {
                    dispatcher_ = dispatcher;
                    fd_ = fd;
                    id_ = id;
                }

                ~Channel() {
                    close(fd_);
                    while(!msg_queue.empty()) {
                        auto msg = msg_queue.front();
                        delete msg.payload;
                        msg.payload = nullptr;
                        msg_queue.pop();
                    }
                }
            private:
                MsgDispatcher* dispatcher_;
                int fd_;
                int id_;
                std::queue<Message> msg_queue;
            friend class MsgDispatcher;
        };

        static void Enqueue(Channel* channel, Message& msg);

        static void* Listen(void* arg);
        static void* Run(void* arg);
        static void* WorkerStart(void* arg);

        static void SaveCapabilities(Channel* ) {}
        static void SendCapabilities(Channel* ) {}

        static bool ValidateMsg(Channel* channel, MessageHeader hdr) { return true; }
        void InitComponentsIfReq();
        std::vector<Component*>& GetComponentsForMsgType(MSG_TYPE msg_type);

        //Dont close, managed by a channel
        int fd_;
        bool m_bStop;
        pthread_t t_main_;
        pthread_t t_workers_[WORKER_POOL];
        std::map<int, pthread_t> t_clients_;
        int nclients_;
        std::vector<Channel*> channels_;
        std::vector<Component*> components_;
        std::map<MSG_TYPE, std::vector<Component*>> msg_type_map;

        std::mutex client_lock_;
        std::mutex q_lock_;
};
}


#endif //VSOCK_MSG_DISPATCHER_H
