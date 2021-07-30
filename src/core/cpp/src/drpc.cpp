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
#include "dorm.hpp"

using namespace drpc;
DORM_MSGPACK(CallIdent, interface, uuid);

namespace drpc {

void run(drpc::ServerHandler* server, const char* addr) {
    auto config = drpc::internal::Config::create(addr, zmq::socket_type::router, true);
    drpc::Bidirectional bi(config, server, true);

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void ServerHandler::run_with_blockon(const char* address) {
    drpc::run(this, address);
}

void ServerHandler::run_with_nonblockon(const char* address) {
    std::thread thread([&](){
        drpc::run(this, address);
    });
    thread.detach();
}

void ServerHandler::terminate() {

}

void ServerHandler::broadcast(drpc::Message&& msg) {

}

ClientHandler::ClientHandler(const char* address)
    : m_sender( new drpc::Sender(drpc::internal::Config::create(address, zmq::socket_type::dealer, false)) )  {

}

ClientHandler::~ClientHandler() {
    delete static_cast<drpc::Sender*>(m_sender);
    m_sender = NULL;
}

bool ClientHandler::send(Message&& msg) {
    try {
        static_cast<drpc::Sender*>(m_sender)->send(std::move(msg));
    } catch (zmq::error_t& e) {
        return false;
    }

    return true;
}

Message ClientHandler::recv() {
    return static_cast<drpc::Sender*>(m_sender)->recv();
}

AsyncClientHandler::AsyncClientHandler(const char* address)
    : m_inner(nullptr)  {

    auto config = drpc::internal::Config::create(address, zmq::socket_type::dealer, false);
    auto inner = new drpc::Bidirectional(config, this, true);
    
    if (!inner) {
        return;
    }

    m_inner = inner;
}

AsyncClientHandler::~AsyncClientHandler() {
    //delete static_cast<drpc::Bidirectional*>(m_inner);
    //m_inner = NULL;
}

void AsyncClientHandler::send(Message&& msg) {
    static_cast<drpc::Bidirectional*>(m_inner)->send(std::move(msg));
}

Message AsyncClientHandler::recv() {
    return {};
}

Part packCallIdent(CallIdent&& ident_) {
    return drift::orm::msgpack_pack(ident_);
}

CallIdent unpackCallIdent(Part&& part_) {
    return drift::orm::msgpack_unpack<CallIdent>(part_);
}

} // namespace drpc
