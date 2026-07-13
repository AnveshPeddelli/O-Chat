#include "Session.h"

#include "Crypto.h"

#include <algorithm>
#include <iostream>
#include <utility>

namespace {
std::string clean_line(std::string line) {
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }
    return line;
}
}

Session::Session(
    tcp::socket socket,
    std::function<void(std::shared_ptr<Session>)> on_join,
    std::function<void(std::shared_ptr<Session>)> on_leave,
    std::function<void(const std::string&, std::shared_ptr<Session>)> on_message
)
    : socket_(std::move(socket)),
      on_join_(std::move(on_join)),
      on_leave_(std::move(on_leave)),
      on_message_(std::move(on_message)) {
}

void Session::start() {
    read_username();
}

void Session::send(const std::string& msg) {
    auto self = shared_from_this();
    asio::post(socket_.get_executor(), [this, self, msg] {
        const bool writing = !outgoing_.empty();
        outgoing_.push_back(msg + "\n");
        if (!writing) {
            write_next();
        }
    });
}

void Session::close() {
    asio::error_code ignored;
    socket_.shutdown(tcp::socket::shutdown_both, ignored);
    socket_.close(ignored);
    clear_private_data();
}

const std::string& Session::username() const {
    return username_;
}

void Session::read_username() {
    auto self = shared_from_this();
    asio::async_read_until(socket_, buffer_, '\n', [this, self](const asio::error_code& error, std::size_t) {
        if (error) {
            disconnect();
            return;
        }

        std::istream input(&buffer_);
        std::getline(input, username_);
        username_ = clean_line(username_);
        username_.erase(std::remove(username_.begin(), username_.end(), '\n'), username_.end());

        if (username_.empty()) {
            username_ = "Anonymous";
        }

        on_join_(self);
        read_message();
    });
}

void Session::read_message() {
    auto self = shared_from_this();
    asio::async_read_until(socket_, buffer_, '\n', [this, self](const asio::error_code& error, std::size_t) {
        if (error) {
            disconnect();
            return;
        }

        std::string msg;
        std::istream input(&buffer_);
        std::getline(input, msg);
        msg = clean_line(msg);

        if (!msg.empty()) {
            on_message_(msg, self);
        }

        read_message();
    });
}

void Session::write_next() {
    auto self = shared_from_this();
    asio::async_write(socket_, asio::buffer(outgoing_.front()), [this, self](const asio::error_code& error, std::size_t) {
        if (error) {
            disconnect();
            return;
        }

        outgoing_.pop_front();
        if (!outgoing_.empty()) {
            write_next();
        }
    });
}

void Session::disconnect() {
    asio::error_code ignored;
    socket_.shutdown(tcp::socket::shutdown_both, ignored);
    socket_.close(ignored);
    on_leave_(shared_from_this());
    clear_private_data();
}

void Session::clear_private_data() {
    Crypto::wipe(username_);

    for (auto& message : outgoing_) {
        Crypto::wipe(message);
    }
    outgoing_.clear();

    if (buffer_.size() > 0) {
        buffer_.consume(buffer_.size());
    }
}
