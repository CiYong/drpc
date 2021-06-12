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

#include "channel.hpp"
#include "uuid/uuid.h"

namespace drpc {
namespace util {

std::string to_string(const char* fmt, ...) {
    std::string str(81, char());
    va_list ap;
    va_start(ap, fmt);
    auto n = vsnprintf(&str[0], str.size(), fmt, ap);
    va_end(ap);

    if (n >= (int)str.size()) {
        str.resize(n + 1);
        va_start(ap, fmt);
        vsnprintf(&str[0], str.size(), fmt, ap);
        va_end(ap);
    }
    str.resize(n);
    return str;
}

} // namespace util

namespace internal {

std::string unique_inproc_addr() {
    uuid_t uuid;
    char s[37];
    uuid_generate_random(uuid);
    uuid_unparse(uuid, s);
    return "inproc://" + std::string(s);
}

void init_socket(zmq::socket_t& socket, const Config& config, Handler* handler) {
    try {
        socket.set(zmq::sockopt::rcvhwm, config.recvhwm);
        socket.set(zmq::sockopt::rcvbuf, config.recvbuf);
        socket.set(zmq::sockopt::sndhwm, config.sendhwm);
        socket.set(zmq::sockopt::sndbuf, config.sendbuf);
        socket.set(zmq::sockopt::linger, config.linger);

        if (config.probe) {
            socket.set(zmq::sockopt::probe_router, 1);
        }

        if (config.bind) {
            if (config.socktype == zmq::socket_type::router) {
                socket.set(zmq::sockopt::router_mandatory, 1);
            }
            socket.bind(config.addr);
        } else {
            socket.connect(config.addr);

            if (config.socktype == zmq::socket_type::sub) {
                socket.set(zmq::sockopt::subscribe, "");
            }
        }
    } catch (zmq::error_t& e) {
        handler->on_error(e.num(), "ZMQ error on socket init: " + std::string(e.what()));
    }
}

void send(zmq::socket_t &socket, Message&& msg, Handler *handler) {
    auto remain = msg.size();

    try {
        for (auto& part : msg) {
            auto zmsg = zmq::message_t(part.data(), part.data() + part.size());
            auto flag = --remain > 0 ? zmq::send_flags::sndmore : zmq::send_flags::none;

            if (!socket.send(zmsg, flag)) {
                throw zmq::error_t(EAGAIN);
            }
        }
    } catch (zmq::error_t& e) {
        handler->on_error(e.num(), e.what());
    }
}

inline Message recv(zmq::socket_t& socket) {
    auto msg = Message();

    do {
        zmq::message_t part;

        if (!socket.recv(&part)) {
            return {};
        }
        msg.emplace_back(part.data<uint8_t>(), part.data<uint8_t>() + part.size());
    } while (socket.get(zmq::sockopt::rcvmore));

    return msg;
}

} // namespace internal

Sender::Sender(const internal::Config& config)
    : m_config(config),
      m_context(config.iothreads),
      m_socket(m_context, config.socktype),
      m_handler(new ErrorHandler) {
    internal::init_socket(m_socket, config, m_handler);
}

Sender::~Sender() {
    delete m_handler;
    m_handler = NULL;
}

void Sender::send(Message&& msg) {
    if (m_config.seq_check) {
        auto n = number();
        auto b = reinterpret_cast<uint8_t*>(&n);
        msg.emplace_back(b, b + sizeof(n));
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    internal::send(m_socket, std::move(msg), m_handler);
}

zmq::socket_t& Sender::socket() {
    return m_socket;
}

zmq::context_t& Sender::context() {
    return m_context;
}

uint64_t Sender::number() {
    return m_seqno += 1;
}

Receiver::Receiver(const internal::Config& config, ServerHandler* server)
    : m_config(config),
      m_context(config.iothreads),
      m_socket(m_context, config.socktype),
      m_handler(new Router(server)) {
    internal::init_socket(m_socket, config, m_handler);
    start();
}

Receiver::~Receiver() {
    if (m_thread.joinable()) {
        m_running = false;
        m_thread.join();
    }
}

zmq::socket_t& Receiver::socket() {
    return m_socket;
}

zmq::context_t& Receiver::context() {
    return m_context;
}

void Receiver::start() {
    m_running = true;
    m_thread = std::thread([this] {
        zmq::pollitem_t item;
        item.socket = static_cast<void*>(m_socket);
        item.events = ZMQ_POLLIN | ZMQ_POLLERR;

        auto last_recv = std::chrono::system_clock::now();

        while (m_running) {
            try {
                if (zmq::poll(&item, 1, 200) < 0) {
                    util::error(m_handler, -1, "ZMQ error on poll");
                    break;
                }
            } catch (zmq::error_t& e) {
                if (e.num() == EINTR) {
                    continue;
                }
                util::error(m_handler, e.num(), "ZMQ error on poll: %s", e.what());
                break;
            }

            if ( m_config.hb_timeout && std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() -last_recv).count() > m_config.hb_timeout) {
                util::error(m_handler, -1, "Heartbeat timed out");
                last_recv = std::chrono::system_clock::now();
            }

            if (item.revents & ZMQ_POLLIN) {
                try {
                    auto msg = internal::recv(m_socket);
                    if (msg.empty()) {
                        continue;
                    }

                    last_recv = std::chrono::system_clock::now();
                    if (m_config.seq_check) {
                        auto num = msg.back();
                        msg.pop_back();
                        seq_check(*reinterpret_cast<uint64_t*>(num.data()));
                    }

                    m_handler->dispatch("", std::move(msg));
                } catch (std::exception& e) {
                    util::error(m_handler, -1, "Uncaught exception on receiving: %s", e.what());
                    break;
                }
            }
        }
    });
}

void Receiver::seq_check(uint64_t n) {
    if (m_seqno && n != m_seqno) {
        util::error(m_handler, -1, "Gap detected: (Expected: %ld, Actual: %ld)", m_seqno, n);
    }
    m_seqno = n + 1;
}

Bidirectional::Bidirectional(const internal::Config& config, ServerHandler* server, bool checkconn)
    : m_config(config),
      m_context(config.iothreads),
      m_socket(m_context, config.socktype),
      m_tx(m_context, ZMQ_PUSH),
      m_rx(m_context, ZMQ_PULL),
      m_checkconn(checkconn),
      m_last_check(std::chrono::system_clock::now()),
      m_handler(new Router(server)) {
    if (config.seq_check) {
        std::cerr << "seq_check not supported in Bidirectional" << std::endl;
    }

    internal::init_socket(m_socket, m_config, m_handler);
    init_forward();
    start();
}

Bidirectional::~Bidirectional() {
    if (m_thread.joinable()) {
        m_running = false;
        m_thread.join();
    }
    delete m_handler;
    m_handler = NULL;
}

void Bidirectional::send(Message&& msg) {
    std::lock_guard<std::mutex> lock(m_inner_mutex);
    internal::send(m_tx, std::move(msg), m_handler);
}

zmq::socket_t& Bidirectional::socket() {
    return m_socket;
}

zmq::context_t& Bidirectional::context() {
    return m_context;
}

void Bidirectional::init_forward() {
    auto addr = internal::unique_inproc_addr();

    try {
        m_rx.set(zmq::sockopt::rcvhwm, m_config.recvhwm);
        m_rx.set(zmq::sockopt::rcvbuf, m_config.recvbuf);
        m_rx.bind(addr);

        m_rx.set(zmq::sockopt::sndhwm, m_config.sendhwm);
        m_rx.set(zmq::sockopt::sndbuf, m_config.sendbuf);
        m_rx.set(zmq::sockopt::linger, m_config.linger);
        m_tx.connect(addr);
    } catch (zmq::error_t& e) {
       util::error(m_handler, e.num(), "ZMQ error on start: %s", e.what());
    }
}

void Bidirectional::start() {
    m_running = true;
    m_thread = std::thread([this] {
        std::vector<zmq::pollitem_t> items(2);
        items[0].socket = static_cast<void*>(m_socket);
        items[0].events = ZMQ_POLLIN | ZMQ_POLLERR;
        items[1].socket = static_cast<void*>(m_rx);
        items[1].events = ZMQ_POLLIN | ZMQ_POLLERR;

        auto listen_sendout = [&] { items[0].events |= ZMQ_POLLOUT; };
        auto unlisten_sendout = [&] { items[0].events &= ~ZMQ_POLLOUT; };
        auto is_sendout = [&] { return m_socket.get(zmq::sockopt::events) & ZMQ_POLLOUT; };

        auto last_recv = std::chrono::system_clock::now();

        while (m_running) {
            try {
                if (zmq::poll(items, 200) < 0) {
                    m_handler->on_error( -1, "ZMQ error on poll");
                    break;
                }
            } catch (zmq::error_t& e) {
                util::error(m_handler, e.num(), "ZMQ error on poll: %s", e.what());
            }

            if ( m_config.hb_timeout && std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - last_recv).count() > m_config.hb_timeout) {
                m_handler->on_error(-1, "Heartbeat timed out");
                last_recv = std::chrono::system_clock::now();
            }

            if (items[0].revents & ZMQ_POLLIN) {
                try {
                    auto msg = internal::recv(m_socket);
                    if (msg.empty()) {
                        continue;
                    }
                    last_recv = std::chrono::system_clock::now();
                    auto id = to_id(msg[0]);
                    add_id(id);
                    if (msg.back().empty()) {
                        continue;
                    }

                    m_handler->dispatch(std::move(id), std::move(msg));
                } catch (zmq::error_t e) {
                    util::error(m_handler, e.num(), "ZMQ error on receiving: %s", e.what());
                    break;
                } catch (std::exception& e) {
                    util::error(m_handler, -1, "Uncaught exception on receiving: %s", e.what());
                    break;
                }
            }

            try {
                check_connection();
            } catch (zmq::error_t& e) {
                util::error(m_handler, e.num(), "ZMQ error on connection check: %s", e.what());
                break;
            } catch (std::exception& e) {
                util::error(m_handler, -1, "Uncaught exception in connection check: %s", e.what());
                break;
            }

            if (!is_sendout()) {
                listen_sendout();
                continue;
            } else {
                unlisten_sendout();
            }

            if (items[1].revents & ZMQ_POLLIN) {
                try {
                    forward_to_main();
                } catch (zmq::error_t& e) {
                    util::error(m_handler, e.num(), "ZMQ error on forwarding: %s", e.what());
                    break;
                } catch (std::exception& e) {
                    util::error(m_handler, -1, "Uncaught exception in forwarding: %s", e.what());
                    break;
                }
            }
        }
    });
}

void Bidirectional::forward_to_main() {
    auto id = zmq::message_t();

    while (true) {
        zmq::message_t msg;

        m_rx.recv(&msg);

        if (check_conn() && id.size() == 0) {
            id.copy(&msg);
        }

        auto more = m_rx.get(zmq::sockopt::rcvmore);
        auto flag = (more ? ZMQ_SNDMORE : 0);

        try {
            if (!m_socket.send(msg, flag)) {
                util::error(m_handler, EAGAIN, "Send failure");
            }
        } catch (zmq::error_t& e) {
            if (e.num() != EHOSTUNREACH) {
                throw;
            }

            remove_id(to_id(id));
            break;
        }

        if (!more) {
            break;
        }
    }
}

void Bidirectional::check_connection() {
    if (!check_conn()) {
        return;
    }

    auto now = std::chrono::system_clock::now();
    if (now - m_last_check < std::chrono::seconds(1)) {
        return;
    }
    m_last_check = now;

    auto remove_ids = std::vector<std::string>();

    for (auto& id : m_clients) {
        auto head = zmq::message_t(id.data(), id.size());
        auto body = zmq::message_t();

        try {
            m_socket.send(head, zmq::send_flags::sndmore);
            m_socket.send(body, zmq::send_flags::dontwait);
        } catch (zmq::error_t& e) {
            if (e.num() != EHOSTUNREACH) {
                throw;
            }

            remove_ids.emplace_back(id);
            continue;
        }
    }

    for (auto& id : remove_ids) {
        remove_id(id);
    }
}

void Bidirectional::add_id(const std::string& id) {
    if (!check_conn()) {
        return;
    }
    if (m_clients.emplace(id).second) {
        m_handler->on_connected(true, id);
    }
}

void Bidirectional::remove_id(const std::string& id) {
    if (!check_conn()) {
        return;
    }

    if (m_clients.erase(id) > 0) {
        m_handler->on_connected(false, id);
    }
}

std::string Bidirectional::to_id(const std::vector<uint8_t>& data) {
    if (!check_conn() || data.empty()) {
        return {};
    }
    return std::string(reinterpret_cast<const char*>(data.data()), data.size());
}

std::string Bidirectional::to_id(const zmq::message_t& data) {
    if (!check_conn() || data.size() == 0) {
        return {};
    }
    return std::string(data.data<char>(), data.size());
}

bool Bidirectional::check_conn() const {
    return m_checkconn && m_config.socktype == zmq::socket_type::router;
}


} // namespace drpc
