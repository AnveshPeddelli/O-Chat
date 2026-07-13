#pragma once
#define _WIN32_WINNT 0x0A00
#include <asio.hpp>
#include <cstdint>
#include <memory>
#include <set>
#include <string>

using asio::ip::tcp;

class Session;

class Server {
    tcp::acceptor acceptor_;
    std::set<std::shared_ptr<Session>> sessions_;

public:
    Server(asio::io_context& io, std::uint16_t port);
    ~Server();
    void stop();

private:
    void accept();
    void join(const std::shared_ptr<Session>& session);
    void leave(const std::shared_ptr<Session>& session);
    void broadcast(const std::string& msg, const std::shared_ptr<Session>& sender = nullptr);
};
