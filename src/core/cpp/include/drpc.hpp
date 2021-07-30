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

#ifndef __drpc_hpp__
#define __drpc_hpp__

#include <cstdint>
#include <vector>
#include <deque>

/*  Version macros for compile-time API version detection                     */
#define DRPC_VERSION_MAJOR 0
#define DRPC_VERSION_MINOR 1
#define DRPC_VERSION_PATCH 0

#define DRPC_MAKE_VERSION(major, minor, patch)                                  \
    ((major) *10000 + (minor) *100 + (patch))
#define DRPC_VERSION                                                            \
    DRPC_MAKE_VERSION (DRPC_VERSION_MAJOR, DRPC_VERSION_MINOR, DRPC_VERSION_PATCH)

namespace drpc {
using Part = std::vector<uint8_t>;
using Message = std::deque<Part>;

struct CallIdent {
    int8_t interface;
    uint64_t uuid;
};

template<typename R>
class CallResult {
public:
    CallResult(CallIdent ident) : m_ident(ident) {}

    R get() {
        return {};
    }

private:
    CallIdent m_ident;
};

class ServerHandler {
public:
    virtual drpc::Message process_message(drpc::Message& msg) {
        return this->switch_interface(msg);
    }
    virtual drpc::Message switch_interface(drpc::Message& msg) = 0;

    virtual void on_connected() {}
    virtual void on_disconnected() {}

    void run_with_blockon(const char* address);
    void run_with_nonblockon(const char* address);

    void terminate();

    void broadcast(drpc::Message&& msg);
};

class ClientHandler {
public:
    ClientHandler(const char* address);
    ~ClientHandler();

    bool send(drpc::Message&& msg);
    Message recv();
private:
    void* m_sender;
};

class AsyncClientHandler : public ServerHandler {
public:
    AsyncClientHandler(const char* address);
    ~AsyncClientHandler();

    void send(drpc::Message&& msg);
    Message recv();
private:
    void* m_inner;
};

void run(ServerHandler* server, const char* addr);

Part packCallIdent(CallIdent&& ident_);
CallIdent unpackCallIdent(Part&& part_);

} // namespace drpc

#endif // __drpc_hpp__
