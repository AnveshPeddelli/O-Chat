#include "Crypto.h"

#include <iostream>

int main() {
    const auto key = Crypto::derive_key("shared-secret");
    const auto encrypted = Crypto::encrypt_line("[Host] hello from host", key);
    const auto decrypted = Crypto::decrypt_line(encrypted, key);

    if (decrypted != "[Host] hello from host") {
        std::cerr << "Crypto round trip failed\n";
        return 1;
    }

    std::cout << "Crypto round trip passed\n";
    return 0;
}
