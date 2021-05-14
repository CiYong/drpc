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


#include "drpc.hpp"
#include "channel.hpp"

namespace drpc {

void run(drpc::ServerHandler* server, const char* addr) {
    auto config = drpc::internal::Config::create(addr, zmq::socket_type::router, true);
    drpc::Bidirectional bi(config, server, true);

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

ClientHandler::ClientHandler(const std::string& address)
    : m_sender( new drpc::Sender(drpc::internal::Config::create(address, zmq::socket_type::dealer, false)) )  {

}

ClientHandler::~ClientHandler() {
    delete static_cast<drpc::Sender*>(m_sender);
    m_sender = NULL;
}

void ClientHandler::send(Message&& msg) {
    static_cast<drpc::Sender*>(m_sender)->send(std::move(msg));
}

Message ClientHandler::recv() {
    return {};
}

} // namespace drpc
