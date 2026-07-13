#pragma once
#define _WIN32_WINNT 0x0A00
#include <asio.hpp>
#include <deque>
#include <memory>
#include <string>
#include <functional>

using asio::ip::tcp;

class Session : public std::enable_shared_from_this<Session> {
    tcp::socket socket_;
    asio::streambuf buffer_;
    std::string username_;
    std::deque<std::string> outgoing_;
    std::function<void(std::shared_ptr<Session>)> on_join_;
    std::function<void(std::shared_ptr<Session>)> on_leave_;
    std::function<void(const std::string&, std::shared_ptr<Session>)> on_message_;

public:
    Session(
        tcp::socket socket,
        std::function<void(std::shared_ptr<Session>)> on_join,
        std::function<void(std::shared_ptr<Session>)> on_leave,
        std::function<void(const std::string&, std::shared_ptr<Session>)> on_message
    );

    void start();
    void send(const std::string& msg);
    void close();
    const std::string& username() const;

private:
    void read_username();
    void read_message();
    void write_next();
    void disconnect();
    void clear_private_data();
};
