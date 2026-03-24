#include <Core/hash.hpp>
#include <gtest/gtest.h>

using namespace xmdb;
using namespace ok::literals;

static SHA256Digest from_hex(const char *hex) {
    SHA256Digest d{};
    auto nibble = [](char c) -> U8 {
        if (c >= '0' && c <= '9') return (U8)(c - '0');
        return (U8)(c - 'a' + 10);
    };
    for (UZ byte_idx = 0; byte_idx < SHA256Digest::SIZE; ++byte_idx) {
        U8 hi = nibble(hex[byte_idx * 2]);
        U8 lo = nibble(hex[byte_idx * 2 + 1]);
        d.bytes.items[byte_idx] = (U8)((hi << 4) | lo);
    }
    return d;
}

TEST(SHA256Test, empty_string) {
    auto digest = sha256_digest(""_sv);
    auto expected = from_hex("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    EXPECT_EQ(digest, expected);
}

TEST(SHA256Test, abc) {
    auto digest = sha256_digest("abc"_sv);
    auto expected = from_hex("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    EXPECT_EQ(digest, expected);
}

// 55 bytes -- last byte fits in the same padding block as the length field
TEST(SHA256Test, message_55_bytes) {
    auto digest = sha256_digest("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"_sv);
    auto expected = from_hex("9f4390f8d30c2dd92ec9f095b65e2b9ae9b0a925a5258e241c9f1e910f734318");
    EXPECT_EQ(digest, expected);
}

// 56 bytes -- triggers the two-block padding path
TEST(SHA256Test, message_56_bytes) {
    auto digest = sha256_digest("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"_sv);
    auto expected = from_hex("b35439a4ac6f0948b6d6f9e3c6af0f5f590ce20f1bde7090ef7970686ec6738a");
    EXPECT_EQ(digest, expected);
}

// 64 bytes -- message fills exactly one block before padding
TEST(SHA256Test, message_64_bytes) {
    auto digest = sha256_digest("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"_sv);
    auto expected = from_hex("ffe054fe7ae0cb6dc65c3af9b61d5209f439851db43d0ba5997337df154668eb");
    EXPECT_EQ(digest, expected);
}

TEST(SHA256Test, same_input_produces_same_digest) {
    auto d1 = sha256_digest("hello world"_sv);
    auto d2 = sha256_digest("hello world"_sv);
    EXPECT_EQ(d1, d2);
}

TEST(SHA256Test, different_inputs_produce_different_digests) {
    auto d1 = sha256_digest("hello"_sv);
    auto d2 = sha256_digest("world"_sv);
    EXPECT_NE(d1, d2);
}
