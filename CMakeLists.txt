cmake_minimum_required(VERSION 3.20)
project(O_Chat)

set(CMAKE_CXX_STANDARD 17)

add_executable(O_Chat
    src/main.cpp
    src/ConsoleUi.cpp
    src/Crypto.cpp
    src/Client.cpp
    src/Server.cpp
    src/Session.cpp
)

target_include_directories(O_Chat PRIVATE include)

find_package(asio CONFIG REQUIRED)

# 🔥 REQUIRED for standalone ASIO consistency
target_compile_definitions(O_Chat PRIVATE ASIO_STANDALONE)

target_link_libraries(O_Chat PRIVATE asio::asio bcrypt)

add_executable(CryptoSelfTest
    src/CryptoSelfTest.cpp
    src/Crypto.cpp
)

target_include_directories(CryptoSelfTest PRIVATE include)
target_compile_definitions(CryptoSelfTest PRIVATE ASIO_STANDALONE)
target_link_libraries(CryptoSelfTest PRIVATE bcrypt)
