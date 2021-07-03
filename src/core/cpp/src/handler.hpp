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

#ifndef __drpc_handler_hpp__
#define __drpc_handler_hpp__

#include <thread>
#include <mutex>
#include <deque>
#include "drpc.hpp"

namespace drpc {
class Bidirectional;

class Handler {
public:
    virtual void on_error(int type, const std::string& client_id) = 0;
    virtual void dispatch(Message&& msg) = 0;
    virtual void on_connected(bool state, const std::string& client_id) = 0;
};

class ErrorHandler : public Handler {
public:
    ErrorHandler();
    ~ErrorHandler();

    virtual void on_error(int type, const std::string& client_id);

    virtual void dispatch(Message&& msg);

    virtual void on_connected(bool state, const std::string& client_id);
};

class Router : public Handler {
public:
    Router(ServerHandler* server);

    ~Router();

    virtual void on_error(int type, const std::string& client_id);

    virtual void dispatch(Message&& msg);

    virtual void on_connected(bool state, const std::string& client_id);

    void set_bichannel(Bidirectional* bichannel_);


private:
    void process_server_message(Message&& msg);
    void process_client_message(Message&& msg);

    Bidirectional* m_bichannel;
    ServerHandler* m_server;
    std::set<std::string> m_clients;
    std::mutex m_mutex;
    std::thread m_thread;
    std::deque<Message> m_msg_queue;
};

} // namespace drpc

#endif // __drpc_handler_hpp__
