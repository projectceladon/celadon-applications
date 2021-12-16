#include "VsockMsgDispatcher.h"
#include <errno.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/vm_sockets.h>
#include <arpa/inet.h>

namespace vsock {
#ifdef MSG_SERVER
static const bool gServer = true;
#else
static const bool gServer = false;
#endif

#define VMADDR_CID_HOST 2
#define VSOCK_PORT 77788
#define CHUNK_SIZE 8192

MsgDispatcher *MsgDispatcher::instance = nullptr;
MsgDispatcher::MsgDispatcher() {
    std::vector<Component*> empty_vector;
    fd_ = socket(AF_VSOCK, SOCK_STREAM, 0);
    if (fd_ <= 0) {
        LOGIT("FAILED TO CREATE SOCKET");
        ERR("Socket Create:");
    }
    nclients_ = 0;
    m_bStop = false;
    msg_type_map[MSG_TYPE_INVALID] = empty_vector;
}

bool MsgDispatcher::Start() {
    struct sockaddr_vm sock_addr;
    memset(&sock_addr, 0, sizeof(struct sockaddr_vm));
    sock_addr.svm_family = AF_VSOCK;
    sock_addr.svm_port = VSOCK_PORT;
    sock_addr.svm_cid = VMADDR_CID_HOST;

    if (gServer) {
        int ret;
        LOGIT("Binding as server\n");
        if ((ret = bind(fd_, (struct sockaddr*)&sock_addr, sizeof(sock_addr))) < 0) {
            ERR("Socket bind:");
            return ret;
        }

        //Bind succesful, so start accepting connections on a separate thread
        Channel* channel = new Channel(this, fd_, 0);
        channels_.emplace_back(channel);
        LOGIT("Start Listen thread\n");
        CHECK_PTHREAD_CALL(pthread_create, (&t_main_, nullptr, &Listen, channel), "Failed to create server listener thread");

    } else {
        bool bConnected = true;
        LOGIT("Connecting as client\n");
        bConnected = connect(fd_, (struct sockaddr *) &sock_addr, sizeof(struct sockaddr_vm));
        while(!bConnected) {
            LOGIT("Failed to connect to server. Waiting to try again...");
            sleep(1);
            ERR("Socket connect:");
            bConnected = connect(fd_, (struct sockaddr *) &sock_addr, sizeof(struct sockaddr_vm));
        }

        //Connected to server, start msg xchange
        Channel* channel = new Channel(this, fd_, 0);
        channels_.emplace_back(channel);
        LOGIT("Start Run thread\n");
        CHECK_PTHREAD_CALL(pthread_create, (&t_main_, nullptr, &Run, channel), "Failed to create leader thread");
    }

    //Create workers
    for (int i = 0; i < WORKER_POOL; ++i) {
        LOGIT("Start worker thread-%d\n", i);
        CHECK_PTHREAD_CALL(pthread_create, (&t_workers_[i], nullptr, &WorkerStart, this), "Failed to create worker thread");
    }

    return true;
}

void* MsgDispatcher::Listen(void* arg) {
    Channel* channel = reinterpret_cast<Channel*>(arg);
    MsgDispatcher* dispatcher = channel->dispatcher_;

    struct sockaddr_vm sock_addr;
    memset(&sock_addr, 0, sizeof(struct sockaddr_vm));
    sock_addr.svm_family = AF_VSOCK;
    sock_addr.svm_port = VSOCK_PORT;
    sock_addr.svm_cid = VMADDR_CID_HOST;
    size_t sock_addr_len = sizeof(struct sockaddr_vm);

    if (!gServer) {
        return (void*)-1;
    }

    int client_fd = -1;
    
    while (!dispatcher->m_bStop) {
        if (listen(channel->fd_, 2) < 0) {
            ERR("Socket listen:");
        }
        LOGIT("ACCEPTING..\n");
	//TODO: Make accept non-blocking
        if((client_fd = accept(channel->fd_, (struct sockaddr*)&sock_addr, (socklen_t*)&sock_addr_len)) < 0) {
            // if (errno == EAGAIN || errno == EWOULDBLOCK) {
            //     sleep(1);
            // } else {
                ERR("Socket accept:");
            // }
            return (void *)-1;
        }
        if (dispatcher->t_clients_.size() == MAX_CLIENT_CONNECTIONS) {
            close(client_fd);
            LOGIT("Max connections exceeded. Closing client connection.");
            continue;
        }
        // start msg xchange
        pthread_t thread;
        dispatcher->nclients_++;
        Channel* channel = new Channel(dispatcher, client_fd, dispatcher->nclients_);
        dispatcher->channels_.emplace_back(channel);
        LOGIT("Start Run thread\n");
        CHECK_PTHREAD_CALL(pthread_create, (&thread, nullptr, &Run, channel), "Failed to create client handler thread");
        dispatcher->t_clients_[dispatcher->nclients_] = thread;
    }
    LOGIT("Exiting Listen thread");
    pthread_exit(nullptr);
    return (void*)0;
}

void* MsgDispatcher::Run(void* arg) {
    Channel* channel = reinterpret_cast<Channel*>(arg);
    MsgDispatcher* dispatcher = channel->dispatcher_;
    LOGIT("In Run thread\n");
    if (gServer) {
        SendCapabilities(channel);
    }
    dispatcher->InitComponentsIfReq();
    MessageHeader hdr;
    //TODO: Make recv non-blocking
    do {
        int nread = (int) recv(channel->fd_, &hdr, sizeof(MessageHeader), 0);
	if (nread != sizeof(MessageHeader)) {
		LOGIT("Failed to recv header:%d", nread);
		ERR("Socket recv header:");
		break;
	}
        int remaining = ntohl(hdr._size);
	LOGIT("Reading payload - %d", remaining);
        // allocate a buffer to read the payload
        char* payload = new char[remaining + 1];
        payload[remaining] = 0;
        Message msg = {hdr, payload};
        while (nread > 0 && remaining != 0) {
            nread = (int) recv(channel->fd_, payload, (remaining > CHUNK_SIZE) ? CHUNK_SIZE : remaining, 0);
            payload += nread;
            remaining -= nread;
        }

        if (nread < 0) {
            LOGIT("Failed to recv... will reconnect...\n");
            ERR("Socket recv:");
            break;
        }
        LOGIT("Recvd payload - %s\n", payload);
        switch(hdr._type) {
            MSG_TYPE_CAPABILITIES:  SaveCapabilities(channel);
                                    if (!gServer) {
                                        SendCapabilities(channel);
                                    }
                                    break;
            default:    if (ValidateMsg(channel, hdr)) {
				LOGIT("Enqueue message");
                            Enqueue(channel, msg);
                        } else {
                            // Msg dropped
                        }
                        break;
        }

    } while (!dispatcher->m_bStop);
    
    // 0 is reserved for the main channel
    if (channel->id_ > 0) {
        //Mark the end of this thread
        dispatcher->t_clients_.erase(channel->id_);
        //Close the channel
        std::lock_guard<std::mutex> lock(dispatcher->q_lock_);
        for (auto it = dispatcher->channels_.begin(); it != dispatcher->channels_.end(); ++it) {
            if (*it == channel) {
                dispatcher->channels_.erase(it);
                delete channel;
                break;
            }
        }
    } else if (!dispatcher->m_bStop) {
        //We may want to reconnect
        close(channel->fd_);
        dispatcher->fd_ = socket(AF_VSOCK, SOCK_STREAM, 0);
        sleep(1);
        bool bConnected = true;
        struct sockaddr_vm sock_addr;
        memset(&sock_addr, 0, sizeof(struct sockaddr_vm));
        sock_addr.svm_family = AF_VSOCK;
        sock_addr.svm_port = VSOCK_PORT;
        sock_addr.svm_cid = VMADDR_CID_HOST;
        LOGIT("Reconnecting...\n");
        bConnected = connect(dispatcher->fd_, (struct sockaddr *) &sock_addr, sizeof(struct sockaddr_vm));
        while(!bConnected) {
            LOGIT("Failed to connect to server. Waiting to try again...");
            ERR("Socket connect:");
            sleep(1);
            bConnected = connect(dispatcher->fd_, (struct sockaddr *) &sock_addr, sizeof(struct sockaddr_vm));
        }
        channel->fd_ = dispatcher->fd_;
        return Run(arg);
    }
    LOGIT("Exiting Run thread");
    pthread_exit(nullptr);
    return (void*)0;
}

void* MsgDispatcher::WorkerStart(void* arg) {
    MsgDispatcher* dispatcher = reinterpret_cast<MsgDispatcher*>(arg);
    while (!dispatcher->m_bStop) {
        Message msg;
        Channel* channel = nullptr;
        bool bMsg = false;
        {
            std::lock_guard<std::mutex> lock(dispatcher->q_lock_);
            for (auto& chnl : dispatcher->channels_) {
                if (chnl->msg_queue.size() > 0) {
                    msg = chnl->msg_queue.front();
                    chnl->msg_queue.pop();
                    channel = chnl;
                    bMsg = true;
                }
            }
        }
        if (bMsg) {
            for (auto& component : dispatcher->GetComponentsForMsgType(msg.header._type)) {
                LOGIT("Worker Dispatch msg - %s\n", msg.payload);
                component->ProcessMsg(msg, reinterpret_cast<uint64_t>(channel));
            }
            delete msg.payload;
            msg.payload = nullptr;
            channel = nullptr;
        } else {
            sleep(1);
        }
    }
    LOGIT("Exiting WorkerStart thread");
    pthread_exit(nullptr);
    return (void*)0;
}

void MsgDispatcher::Enqueue(Channel* channel, Message& msg) {
    std::queue<Message>& mq = channel->msg_queue;
    // No need of locks, only a single thread does the queing
    while (mq.size() >= Q_SIZE) {
        // Sleep for the queue to free up
        sleep(1);
    }

    mq.push(msg);
}

void MsgDispatcher::Stop() {
    m_bStop = true;
    for (auto it = t_clients_.begin(); it != t_clients_.end(); ++it) {
        CHECK_PTHREAD_CALL(pthread_join, (it->second, nullptr), "Failed to join on client thread");
    }
    for (int i = 0; i < WORKER_POOL; ++i) {
        LOGIT("Stop worker thread-%d\n", i);
        CHECK_PTHREAD_CALL(pthread_join, (t_workers_[i], nullptr), "Failed to join on worker thread");
    }
    LOGIT("Stop main thread");
    CHECK_PTHREAD_CALL(pthread_join, (t_main_, nullptr), "Failed to join on the main thread");

    //Clear all the channels
    std::lock_guard<std::mutex> lock(q_lock_);
    for(auto& channel : channels_) {
        delete channel;
    }
    channels_.clear();
    for(auto& comp : components_) {
        delete comp;
    }
    components_.clear();

}

bool MsgDispatcher::SendMsg(uint64_t hndl, MSG_TYPE msg_type, const char* buffer, int size) {
    MessageHeader header;
    Channel* channel = reinterpret_cast<Channel*>(hndl);
    bool bValidChannel = false;
    for (auto it = channels_.begin(); it != channels_.end(); ++it) {
            if (*it == channel) {
              bValidChannel = true;
	            break;
	    }
    }
    if (!bValidChannel) {
	    LOGIT("Failed to send msg: Invalid Channel");
	    return false;
    }

    header._ver = MSG_VERSION;
    header._type = msg_type;
    header._id = 0;
    header._size = htonl(size);
    int nsent = (int)send(channel->fd_, &header, sizeof(MessageHeader), 0);
    if (nsent != sizeof(MessageHeader)) {
	    LOGIT("Failed to send header:%d", nsent);
	    ERR("Socket send hdr:");
	    return false;
    }
    while (nsent > 0 && size > 0) {
        nsent = (int)send(channel->fd_, buffer, (size > CHUNK_SIZE) ? CHUNK_SIZE : size, 0);
        size -= nsent;
        buffer += nsent;
    }
    if (nsent < 0) {
        ERR("Socket send:");
    }
    return (nsent >= 0);
}

std::vector<Component*>& MsgDispatcher::GetComponentsForMsgType(MSG_TYPE msg_type) {
    auto search = msg_type_map.find(msg_type);
    if (search != msg_type_map.end()) {
        return search->second;
    } else {
        return msg_type_map.find(MSG_TYPE_INVALID)->second;
    }
}

void MsgDispatcher::RegisterComponent(std::vector<MSG_TYPE>& msg_list, Component* component) {
    LOGIT("Registering component");
    components_.emplace_back(component);
    for (auto type : msg_list) {
        if (type == MSG_TYPE_INVALID) {
            continue;
        }
        auto search = msg_type_map.find(type);
        if (search != msg_type_map.end()) {
            search->second.emplace_back(component);
        } else {
            std::vector<Component*> comp_vector;
            comp_vector.emplace_back(component);
            msg_type_map[type] = comp_vector;
        }
    }
}

uint64_t MsgDispatcher::GetHandleForMsgType(MSG_TYPE msg_type) {
    return reinterpret_cast<uint64_t>(channels_.back());
}

void MsgDispatcher::InitComponentsIfReq() {
    LOGIT("Looping components");
    for (auto component : components_) {
        LOGIT("Initializing component");
        component->setDispatcher(this);
        component->Initialize();
    }
}

}
