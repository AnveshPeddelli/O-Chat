#include <iostream>
#include <cstdint>
#include <conio.h>
#include <io.h>
#include <limits>
#include <memory>
#include <string>
#include <thread>
#include "ConsoleUi.h"
#include "Crypto.h"
#include "Server.h"
#include "Client.h"

namespace {
constexpr std::uint16_t kChatPort = 54000;

std::string find_lan_ip(asio::io_context& io) {
    asio::error_code error;
    tcp::resolver resolver(io);
    const auto results = resolver.resolve(asio::ip::host_name(), "0", error);

    if (error) {
        return "your LAN IPv4 address";
    }

    for (const auto& result : results) {
        const auto address = result.endpoint().address();
        if (address.is_v4() && !address.is_loopback()) {
            return address.to_string();
        }
    }

    return "your LAN IPv4 address";
}

void run_chat(Client& client) {
    ConsoleUi::print_status("You are in the room. Type /quit to leave.");
    ConsoleUi::print_prompt();

    while (true) {
        std::string msg = ConsoleUi::read_chat_line();
        if (msg == "/quit") {
            break;
        }

        if (!msg.empty()) {
            client.send_message(msg);
        }

        ConsoleUi::print_prompt();
    }
}

std::string read_passphrase() {
    std::cout << "room key  > ";

    if (!_isatty(_fileno(stdin))) {
        std::string passphrase;
        std::getline(std::cin, passphrase);
        return passphrase;
    }

    std::string passphrase;
    while (true) {
        const int ch = _getch();
        if (ch == '\r' || ch == '\n') {
            std::cout << '\n';
            break;
        }

        if (ch == '\b') {
            if (!passphrase.empty()) {
                passphrase.pop_back();
            }
            continue;
        }

        if (ch >= 32 && ch <= 126) {
            passphrase.push_back(static_cast<char>(ch));
        }
    }

    return passphrase;
}
}

int main() {
    try {
        asio::io_context server_io;
        asio::io_context client_io;
        std::unique_ptr<Server> server;
        std::thread server_thread;

        ConsoleUi::clear_screen();
        ConsoleUi::print_header("Office Console Chat", "Local network chat on port " + std::to_string(kChatPort));
        ConsoleUi::print_menu();

        int choice;
        std::cin >> choice;

        std::string ip = "127.0.0.1";
        if (choice == 1) {
            server = std::make_unique<Server>(server_io, kChatPort);
            server_thread = std::thread([&server_io] {
                server_io.run();
            });
            const auto lan_ip = find_lan_ip(client_io);
            ConsoleUi::print_status("Hosting room on port " + std::to_string(kChatPort));
            ConsoleUi::print_status("Same Wi-Fi/LAN users can join with: " + lan_ip);
            ConsoleUi::print_status("Internet users need your public IP plus router port forwarding for port " + std::to_string(kChatPort));
        }
        else {
            std::cout << "\nserver ip > ";
            std::cin >> ip;
        }

        std::string name;
        std::cout << "username  > ";
        std::cin >> name;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        auto passphrase = read_passphrase();
        if (passphrase.empty()) {
            ConsoleUi::print_error("Room key cannot be empty.");
            return 1;
        }

        auto room_key = Crypto::derive_key(passphrase);
        Crypto::wipe(passphrase);

        Client client(client_io, room_key, name);
        room_key.fill(0);
        client.connect(ip, kChatPort);
        client.send_username(name);
        client.start_receiving();
        ConsoleUi::print_status("Connected as " + name + " to " + ip + ":" + std::to_string(kChatPort));

        std::thread client_thread([&client_io] {
            client_io.run();
        });

        run_chat(client);

        client.close();
        client_io.stop();
        if (client_thread.joinable()) {
            client_thread.join();
        }

        if (server) {
            server->stop();
        }
        server_io.stop();
        if (server_thread.joinable()) {
            server_thread.join();
        }

        ConsoleUi::clear_screen();
    }
    catch (const std::exception& ex) {
        ConsoleUi::print_error(ex.what());
        return 1;
    }

    return 0;
}
