#include "Crypto.h"

#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>

#include <algorithm>
#include <stdexcept>
#include <vector>

namespace {
constexpr ULONG kNonceSize = 12;
constexpr ULONG kTagSize = 16;
constexpr ULONGLONG kPbkdf2Iterations = 5000;
constexpr const char* kSalt = "O_Chat private room v1";
constexpr const char* kBase64Chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void check_status(NTSTATUS status, const char* operation) {
    if (status < 0) {
        throw std::runtime_error(std::string(operation) + " failed");
    }
}

std::string base64_encode(const std::vector<unsigned char>& bytes) {
    std::string encoded;
    int value = 0;
    int bits = -6;

    for (const auto byte : bytes) {
        value = (value << 8) + byte;
        bits += 8;
        while (bits >= 0) {
            encoded.push_back(kBase64Chars[(value >> bits) & 0x3F]);
            bits -= 6;
        }
    }

    if (bits > -6) {
        encoded.push_back(kBase64Chars[((value << 8) >> (bits + 8)) & 0x3F]);
    }

    while (encoded.size() % 4 != 0) {
        encoded.push_back('=');
    }

    return encoded;
}

std::vector<unsigned char> base64_decode(const std::string& encoded) {
    std::array<int, 256> table{};
    table.fill(-1);
    for (int i = 0; i < 64; ++i) {
        table[static_cast<unsigned char>(kBase64Chars[i])] = i;
    }

    std::vector<unsigned char> decoded;
    int value = 0;
    int bits = -8;

    for (const auto character : encoded) {
        if (character == '=') {
            break;
        }

        const int mapped = table[static_cast<unsigned char>(character)];
        if (mapped == -1) {
            throw std::runtime_error("Encrypted message was not valid base64");
        }

        value = (value << 6) + mapped;
        bits += 6;
        if (bits >= 0) {
            decoded.push_back(static_cast<unsigned char>((value >> bits) & 0xFF));
            bits -= 8;
        }
    }

    return decoded;
}

struct AlgorithmHandle {
    BCRYPT_ALG_HANDLE value = nullptr;

    ~AlgorithmHandle() {
        if (value) {
            BCryptCloseAlgorithmProvider(value, 0);
        }
    }
};

struct KeyHandle {
    BCRYPT_KEY_HANDLE value = nullptr;

    ~KeyHandle() {
        if (value) {
            BCryptDestroyKey(value);
        }
    }
};

struct AesGcmKey {
    AlgorithmHandle algorithm;
    KeyHandle key_handle;

    explicit AesGcmKey(const Crypto::Key& key) {
        check_status(BCryptOpenAlgorithmProvider(&algorithm.value, BCRYPT_AES_ALGORITHM, nullptr, 0), "Open AES");
        check_status(
            BCryptSetProperty(
                algorithm.value,
                BCRYPT_CHAINING_MODE,
                reinterpret_cast<PUCHAR>(const_cast<wchar_t*>(BCRYPT_CHAIN_MODE_GCM)),
                static_cast<ULONG>((wcslen(BCRYPT_CHAIN_MODE_GCM) + 1) * sizeof(wchar_t)),
                0
            ),
            "Set AES-GCM mode"
        );

        check_status(
            BCryptGenerateSymmetricKey(
                algorithm.value,
                &key_handle.value,
                nullptr,
                0,
                const_cast<PUCHAR>(key.data()),
                static_cast<ULONG>(key.size()),
                0
            ),
            "Create AES key"
        );
    }
};
}

namespace Crypto {
Key derive_key(const std::string& passphrase) {
    AlgorithmHandle algorithm;
    check_status(
        BCryptOpenAlgorithmProvider(&algorithm.value, BCRYPT_SHA256_ALGORITHM, nullptr, BCRYPT_ALG_HANDLE_HMAC_FLAG),
        "Open PBKDF2"
    );

    Key key{};
    check_status(
        BCryptDeriveKeyPBKDF2(
            algorithm.value,
            reinterpret_cast<PUCHAR>(const_cast<char*>(passphrase.data())),
            static_cast<ULONG>(passphrase.size()),
            reinterpret_cast<PUCHAR>(const_cast<char*>(kSalt)),
            static_cast<ULONG>(std::char_traits<char>::length(kSalt)),
            kPbkdf2Iterations,
            key.data(),
            static_cast<ULONG>(key.size()),
            0
        ),
        "Derive room key"
    );

    return key;
}

std::string encrypt_line(const std::string& plain_text, const Key& key) {
    AesGcmKey aes_key(key);

    std::array<unsigned char, kNonceSize> nonce{};
    check_status(
        BCryptGenRandom(nullptr, nonce.data(), static_cast<ULONG>(nonce.size()), BCRYPT_USE_SYSTEM_PREFERRED_RNG),
        "Generate nonce"
    );

    std::vector<unsigned char> cipher_text(plain_text.size());
    std::array<unsigned char, kTagSize> tag{};

    BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO auth_info;
    BCRYPT_INIT_AUTH_MODE_INFO(auth_info);
    auth_info.pbNonce = nonce.data();
    auth_info.cbNonce = static_cast<ULONG>(nonce.size());
    auth_info.pbTag = tag.data();
    auth_info.cbTag = static_cast<ULONG>(tag.size());

    ULONG bytes_done = 0;
    check_status(
        BCryptEncrypt(
            aes_key.key_handle.value,
            reinterpret_cast<PUCHAR>(const_cast<char*>(plain_text.data())),
            static_cast<ULONG>(plain_text.size()),
            &auth_info,
            nullptr,
            0,
            cipher_text.data(),
            static_cast<ULONG>(cipher_text.size()),
            &bytes_done,
            0
        ),
        "Encrypt message"
    );
    cipher_text.resize(bytes_done);

    std::vector<unsigned char> packed;
    packed.reserve(nonce.size() + tag.size() + cipher_text.size());
    packed.insert(packed.end(), nonce.begin(), nonce.end());
    packed.insert(packed.end(), tag.begin(), tag.end());
    packed.insert(packed.end(), cipher_text.begin(), cipher_text.end());

    SecureZeroMemory(cipher_text.data(), cipher_text.size());
    return "ENC " + base64_encode(packed);
}

std::string decrypt_line(const std::string& encrypted_line, const Key& key) {
    if (encrypted_line.rfind("ENC ", 0) != 0) {
        return encrypted_line;
    }

    auto packed = base64_decode(encrypted_line.substr(4));
    if (packed.size() < kNonceSize + kTagSize) {
        throw std::runtime_error("Encrypted message was too short");
    }

    AesGcmKey aes_key(key);

    const auto* nonce = packed.data();
    const auto* tag = packed.data() + kNonceSize;
    const auto* cipher_text = packed.data() + kNonceSize + kTagSize;
    const auto cipher_size = static_cast<ULONG>(packed.size() - kNonceSize - kTagSize);

    std::vector<unsigned char> plain_text(cipher_size);

    BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO auth_info;
    BCRYPT_INIT_AUTH_MODE_INFO(auth_info);
    auth_info.pbNonce = const_cast<PUCHAR>(nonce);
    auth_info.cbNonce = kNonceSize;
    auth_info.pbTag = const_cast<PUCHAR>(tag);
    auth_info.cbTag = kTagSize;

    ULONG bytes_done = 0;
    check_status(
        BCryptDecrypt(
            aes_key.key_handle.value,
            const_cast<PUCHAR>(cipher_text),
            cipher_size,
            &auth_info,
            nullptr,
            0,
            plain_text.data(),
            static_cast<ULONG>(plain_text.size()),
            &bytes_done,
            0
        ),
        "Decrypt message"
    );

    std::string result(plain_text.begin(), plain_text.begin() + bytes_done);
    SecureZeroMemory(plain_text.data(), plain_text.size());
    SecureZeroMemory(packed.data(), packed.size());
    return result;
}

void wipe(std::string& value) {
    if (!value.empty()) {
        SecureZeroMemory(&value[0], value.size());
        value.clear();
    }
}
}
