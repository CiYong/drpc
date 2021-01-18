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

#ifndef __DRPC_HPP__
#define __DRPC_HPP__

#include <string>
#include <vector>
#include <deque>

namespace drpc {

typedef unsigned char uint8_t;
using Part = std::vector<uint8_t>;
using Message = std::deque<Part>;

class ServerHandler {
public:
    virtual void process_message(Message& msg) {
        this->switch_interface(msg);
    }
    virtual void switch_interface(Message& msg) = 0;
};

class ClientHandler {
public:
    ClientHandler(const std::string& address);
    ~ClientHandler();

    void send(Message&& msg);
    Message recv();
private:
    void* m_sender;
};

void run(ServerHandler* server, const char* addr);

} // namespace drpc

#endif // __DRPC_HPP__
