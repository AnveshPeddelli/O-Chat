#include "Client.h"

#include "ConsoleUi.h"

#include <iostream>
#include <utility>

namespace {
std::string ensure_newline(std::string line) {
    if (line.empty() || line.back() != '\n') {
        line += '\n';
    }
    return line;
}

std::string label_own_message(std::string message, const std::string& username) {
    const std::string own_prefix = "[" + username + "]";
    if (message.rfind(own_prefix, 0) == 0) {
        message.replace(0, own_prefix.size(), "[You]");
    }
    return message;
}
}

Client::Client(asio::io_context& io, Crypto::Key room_key, std::string username)
    : socket_(io),
      room_key_(room_key),
      username_(std::move(username)) {
}

Client::~Client() {
    close();
}

void Client::connect(const std::string& ip, std::uint16_t port) {
    tcp::resolver resolver(socket_.get_executor());
    const auto endpoints = resolver.resolve(ip, std::to_string(port));
    asio::connect(socket_, endpoints);
}

void Client::send_username(const std::string& name) {
    asio::write(socket_, asio::buffer(ensure_newline(name)));
}

void Client::start_receiving() {
    asio::async_read_until(socket_, buffer_, '\n', [this](const asio::error_code& error, std::size_t) {
        if (error) {
            if (error != asio::error::operation_aborted && error != asio::error::eof) {
                ConsoleUi::print_error("Disconnected from server: " + error.message());
            }
            return;
        }

        std::string msg;
        std::istream input(&buffer_);
        std::getline(input, msg);

        if (!msg.empty() && msg.back() == '\r') {
            msg.pop_back();
        }

        try {
            ConsoleUi::print_message(label_own_message(Crypto::decrypt_line(msg, room_key_), username_));
        }
        catch (const std::exception&) {
            ConsoleUi::print_error("Could not decrypt a message. Check that everyone used the same room passphrase.");
        }

        start_receiving();
    });
}

void Client::send_message(const std::string& msg) {
    try {
        send_line(Crypto::encrypt_line("[" + username_ + "] " + msg, room_key_));
    }
    catch (const std::exception& ex) {
        ConsoleUi::print_error(std::string("Could not encrypt message: ") + ex.what());
    }
}

void Client::send_line(const std::string& line) {
    asio::post(socket_.get_executor(), [this, message = ensure_newline(line)] {
        const bool writing = !outgoing_.empty();
        outgoing_.push_back(message);
        if (!writing) {
            write_next();
        }
    });
}

void Client::close() {
    asio::error_code ignored;
    socket_.shutdown(tcp::socket::shutdown_both, ignored);
    socket_.close(ignored);
    clear_private_data();
}

void Client::clear_private_data() {
    for (auto& message : outgoing_) {
        Crypto::wipe(message);
    }
    outgoing_.clear();

    if (buffer_.size() > 0) {
        buffer_.consume(buffer_.size());
    }

    Crypto::wipe(username_);
    room_key_.fill(0);
}

void Client::write_next() {
    asio::async_write(socket_, asio::buffer(outgoing_.front()), [this](const asio::error_code& error, std::size_t) {
        if (error) {
            ConsoleUi::print_error("Send failed: " + error.message());
            return;
        }

        outgoing_.pop_front();
        if (!outgoing_.empty()) {
            write_next();
        }
    });
}
