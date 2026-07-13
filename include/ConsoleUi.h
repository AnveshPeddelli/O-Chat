#pragma once

#include <mutex>
#include <string>

namespace ConsoleUi {
    std::mutex& output_mutex();

    void clear_screen();
    void print_header(const std::string& title, const std::string& subtitle);
    void print_menu();
    void print_status(const std::string& message);
    void print_error(const std::string& message);
    void print_message(const std::string& message);
    void print_prompt();
    std::string read_chat_line();
}
