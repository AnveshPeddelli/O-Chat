#include "Server.h"

#include "Session.h"

#include <utility>

Server::Server(asio::io_context& io, std::uint16_t port)
    : acceptor_(io, tcp::endpoint(tcp::v4(), port)) {
    accept();
}

Server::~Server() {
    stop();
}

void Server::stop() {
    asio::error_code ignored;
    acceptor_.close(ignored);

    for (const auto& session : sessions_) {
        session->close();
    }
    sessions_.clear();
}

void Server::accept() {
    acceptor_.async_accept([this](const asio::error_code& error, tcp::socket socket) {
        if (!error) {
            auto session = std::make_shared<Session>(
                std::move(socket),
                [this](std::shared_ptr<Session> joined) { join(joined); },
                [this](std::shared_ptr<Session> left) { leave(left); },
                [this](const std::string& msg, std::shared_ptr<Session> sender) {
                    broadcast(msg, sender);
                }
            );
            session->start();
        }
        else {
            broadcast("* Accept failed: " + error.message());
        }

        accept();
    });
}

void Server::join(const std::shared_ptr<Session>& session) {
    sessions_.insert(session);
    broadcast("[system] " + session->username() + " joined the room");
}

void Server::leave(const std::shared_ptr<Session>& session) {
    const auto removed = sessions_.erase(session);
    if (removed > 0) {
        broadcast("[system] " + session->username() + " left the room");
    }
}

void Server::broadcast(const std::string& msg, const std::shared_ptr<Session>& sender) {
    for (const auto& session : sessions_) {
        session->send(msg);
    }
}
