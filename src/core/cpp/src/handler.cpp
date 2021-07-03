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

#include "handler.hpp"
#include "drpc.hpp"
#include "channel.hpp"

namespace drpc {

ErrorHandler::ErrorHandler() {
}

ErrorHandler::~ErrorHandler() {
}

void ErrorHandler::on_error(int type, const std::string& client_id) {
}

void ErrorHandler::dispatch(Message&& msg) {
}

void ErrorHandler::on_connected(bool state, const std::string& client_id) {
}

Router::Router(drpc::ServerHandler* server)
    : m_server(server) {
}

Router::~Router() {
    if (m_server) {
        delete m_server; 
        m_server = NULL;
    }
}

void Router::on_error(int type, const std::string& client_id) {
}

void Router::dispatch(Message&& msg) {
    if (!m_server || !m_bichannel) {
        return;
    }

    if ( m_bichannel->need_reply() ) {
        process_client_message(std::move(msg));
    } else {
        process_server_message(std::move(msg));
    }
}

void Router::on_connected(bool state, const std::string& client_id) {
}

void Router::set_bichannel(Bidirectional* bichannel_) {
    m_bichannel = bichannel_;
}

void Router::process_server_message(Message&& msg) {
    auto result = m_server->process_message(msg);
}

void Router::process_client_message(Message&& msg) {
    auto client_id = msg.front();
    msg.pop_front();

    if (msg.front().empty()) {
        // req type blank frame.
        auto blank_frame = msg.front();
        msg.pop_front();
    }

    auto result = m_server->process_message(msg);

    result.emplace_front(client_id);

    m_bichannel->send(std::move(result));
}

} // namespace drpc

