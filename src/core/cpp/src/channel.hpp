/*
 *
 * Copyright 2020 drpc authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef __drpc_channel_hpp__
#define __drpc_channel_hpp__

#include <deque>
#include <functional>
#include <thread>
#include <unordered_set>
#include <vector>
#include <mutex>
#include <stdarg.h>
#include <chrono>
#include <iostream>
#include <set>

#include "zmq.hpp"
#include "drpc.hpp"
#include "router.hpp"

namespace drpc {

namespace util {

std::string to_string(const char* fmt, ...);

template <typename... Args>
void error(Handler* handler, int err, const char* fmt, Args&&... args) {
    handler->on_error(err, to_string(fmt, std::forward<Args>(args)...));
}

} // namespace util

namespace internal {

struct Config {
    int iothreads{1};
    zmq::socket_type socktype{zmq::socket_type::dealer};
    int sendhwm{0};
    int recvhwm{0};
    int sendbuf{0};
    int recvbuf{0};
    std::string addr{"tcp://localhost:8080"};
    bool bind{false};
    int linger{200};
    int blkrcv{1000};
    bool seq_check{false};
    bool probe{false};
    uint64_t hb_timeout{0};

    static Config create(const std::string& addr,
                                zmq::socket_type socktype,
                                bool bind) {
        auto cfg = Config{};
        cfg.addr = addr;
        cfg.socktype = socktype;
        cfg.bind = bind;
        return cfg;
    }
};

inline std::string unique_addr();
void init_socket(zmq::socket_t& socket, const Config& config, Handler* handler);
void send(zmq::socket_t& socket, Message&& msg, Handler* handler);
inline Message recv(zmq::socket_t& socket);

} // namespace internal

class Sender {
public:
    Sender(const internal::Config& config);
    ~Sender();
    void send(Message&& msg);
    zmq::socket_t& socket();
    zmq::context_t& context();

    Message recv();

private:
    uint64_t number();

    internal::Config m_config;
    std::mutex m_mutex;
    zmq::context_t m_context;
    zmq::socket_t m_socket;
    uint64_t m_seqno;
    Handler* m_handler;
};

class Receiver {
public:
    Receiver(const internal::Config& config, ServerHandler* server);
    ~Receiver();
    zmq::socket_t& socket();
    zmq::context_t& context();

private:
    void start();
    void seq_check(uint64_t n);

    internal::Config m_config;
    zmq::context_t m_context;
    zmq::socket_t m_socket;
    std::thread m_thread;
    bool m_running;
    uint64_t m_seqno;
    Handler* m_handler;
};

class Bidirectional {
public:
    Bidirectional(const internal::Config& config, ServerHandler* server, bool checkconn);
    ~Bidirectional();
    void send(Message&& msg);
    zmq::socket_t& socket();
    zmq::context_t& context();

    bool need_reply();

private:
    void init_forward();
    void start();
    void forward_to_main();
    void check_connection();
    void add_id(const std::string& id);

    void remove_id(const std::string& id);
    std::string to_id(const std::vector<uint8_t>& data);
    std::string to_id(const zmq::message_t& data);
    bool check_conn() const;

    internal::Config m_config;
    zmq::context_t m_context;
    zmq::socket_t m_socket;
    std::thread m_thread;
    bool m_running;
    std::mutex m_inner_mutex;
    zmq::socket_t m_tx;
    zmq::socket_t m_rx;
    bool m_checkconn;
    std::unordered_set<std::string> m_clients;
    std::chrono::system_clock::time_point m_last_check;
    Handler* m_handler;
};

class InternalSender {
public:
    InternalSender(const internal::Config& config, zmq::socket_t&& socket);
    ~InternalSender();
    void send(Message&& msg);
    zmq::socket_t& socket();

private:
    uint64_t number();

    internal::Config m_config;
    std::mutex m_mutex;
    zmq::socket_t m_socket;
    uint64_t m_seqno;
    Handler* m_handler;
};

class InternalReceiver {
public:
    InternalReceiver(const internal::Config& config, zmq::context_t* context);
    ~InternalReceiver();
    zmq::socket_t& socket();
    zmq::context_t& context();

private:
    void start();
    void seq_check(uint64_t n);

    internal::Config m_config;
    zmq::context_t* m_context;
    zmq::socket_t m_socket;
    std::thread m_thread;
    bool m_running;
    uint64_t m_seqno;
    Handler* m_handler;
};

struct ChannelPair {
    InternalSender* sender;
    InternalReceiver* receiver;
};

ChannelPair build_internal_channel();

} // namespace drpc

#endif // __drpc_channel_hpp__
