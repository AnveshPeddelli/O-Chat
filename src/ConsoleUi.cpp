#include "ConsoleUi.h"

#define NOMINMAX
#include <windows.h>

#include <conio.h>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <io.h>
#include <string>

namespace ConsoleUi {
namespace {
constexpr const char* kDivider = "------------------------------------------------------------";
constexpr const char* kPrompt = "You > ";
constexpr const char* kInputClear = "\r                                                                                \r";

enum class Color {
    Default = 0,
    Green = 92,
    Cyan = 96,
    Red = 91,
    Magenta = 95,
    Yellow = 93,
    White = 97
};

void enable_virtual_terminal_colors() {
    const HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    if (output == INVALID_HANDLE_VALUE) {
        return;
    }

    DWORD mode = 0;
    if (!GetConsoleMode(output, &mode)) {
        return;
    }

    SetConsoleMode(output, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}

void set_color(Color color) {
    std::cout << "\x1b[" << static_cast<int>(color) << "m";
}

Color color_for_user(const std::string& name) {
    const Color colors[] = {
        Color::Cyan,
        Color::Green,
        Color::Yellow,
        Color::Magenta,
        Color::Red,
        Color::White
    };
    const auto hash = std::hash<std::string>{}(name);
    return colors[hash % (sizeof(colors) / sizeof(colors[0]))];
}

void print_colored_chat_message(const std::string& message) {
    if (message.rfind("[system]", 0) == 0) {
        set_color(Color::Yellow);
        std::cout << message;
        set_color(Color::Default);
        return;
    }

    if (!message.empty() && message[0] == '[') {
        const auto end = message.find(']');
        if (end != std::string::npos && end > 1) {
            const auto name = message.substr(1, end - 1);
            set_color(color_for_user(name));
            std::cout << message.substr(0, end + 1);
            set_color(Color::Default);
            std::cout << message.substr(end + 1);
            return;
        }
    }

    std::cout << message;
}
}

std::mutex& output_mutex() {
    static std::mutex mutex;
    return mutex;
}

void clear_screen() {
    enable_virtual_terminal_colors();
    std::system("cls");
}

void print_header(const std::string& title, const std::string& subtitle) {
    std::lock_guard<std::mutex> lock(output_mutex());
    std::cout << kDivider << '\n'
              << "  " << title << '\n'
              << "  " << subtitle << '\n'
              << kDivider << "\n\n";
}

void print_menu() {
    std::lock_guard<std::mutex> lock(output_mutex());
    std::cout << "Choose a mode\n"
              << "  1) Host a room\n"
              << "  2) Join a room\n\n"
              << "choice > ";
    std::cout.flush();
}

void print_status(const std::string& message) {
    std::lock_guard<std::mutex> lock(output_mutex());
    std::cout << "\n[status] " << message << '\n';
}

void print_error(const std::string& message) {
    std::lock_guard<std::mutex> lock(output_mutex());
    std::cerr << "\n[error] " << message << '\n';
}

void print_message(const std::string& message) {
    std::lock_guard<std::mutex> lock(output_mutex());
    std::cout << kInputClear << "| ";
    print_colored_chat_message(message);
    std::cout << "\n| " << kDivider << '\n' << kPrompt;
    std::cout.flush();
}

void print_prompt() {
    std::lock_guard<std::mutex> lock(output_mutex());
    std::cout << "| " << kDivider << '\n';
    std::cout << kPrompt;
    std::cout.flush();
}

std::string read_chat_line() {
    if (!_isatty(_fileno(stdin))) {
        std::string line;
        std::getline(std::cin, line);
        return line;
    }

    std::string line;

    while (true) {
        const int ch = _getch();

        if (ch == '\r' || ch == '\n') {
            std::lock_guard<std::mutex> lock(output_mutex());
            std::cout << kInputClear;
            std::cout.flush();
            return line;
        }

        if (ch == '\b') {
            if (!line.empty()) {
                line.pop_back();
                std::lock_guard<std::mutex> lock(output_mutex());
                std::cout << "\b \b";
                std::cout.flush();
            }
            continue;
        }

        if (ch >= 32 && ch <= 126) {
            line.push_back(static_cast<char>(ch));
            std::lock_guard<std::mutex> lock(output_mutex());
            std::cout << static_cast<char>(ch);
            std::cout.flush();
        }
    }
}
}
