#pragma once
#define _WIN32_WINNT 0x0A00
#include <asio.hpp>
#include <cstdint>
#include <deque>
#include <string>

#include "Crypto.h"

using asio::ip::tcp;

class Client {
    tcp::socket socket_;
    asio::streambuf buffer_;
    std::deque<std::string> outgoing_;
    Crypto::Key room_key_{};
    std::string username_;

public:
    Client(asio::io_context& io, Crypto::Key room_key, std::string username);
    ~Client();

    void connect(const std::string& ip, std::uint16_t port);
    void send_username(const std::string& name);
    void start_receiving();
    void send_message(const std::string& msg);
    void close();

private:
    void send_line(const std::string& line);
    void write_next();
    void clear_private_data();
};
