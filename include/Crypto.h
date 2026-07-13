#pragma once

#include <array>
#include <string>

namespace Crypto {
    using Key = std::array<unsigned char, 32>;

    Key derive_key(const std::string& passphrase);
    std::string encrypt_line(const std::string& plain_text, const Key& key);
    std::string decrypt_line(const std::string& encrypted_line, const Key& key);
    void wipe(std::string& value);
}
