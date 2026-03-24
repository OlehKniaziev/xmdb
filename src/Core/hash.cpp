#include "hash.hpp"

#include <cstring>

namespace xmdb {

static constexpr U32 K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
};

static constexpr U32 INIT[8] = {
    0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
    0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19,
};

static U32 rotr32(U32 x, U32 n) {
    return (x >> n) | (x << (32 - n));
}

static void process_chunk(U32 *h, const U8 *chunk) {
    U32 w[64];
    for (UZ word_idx = 0; word_idx < 16; ++word_idx) {
        UZ byte_idx = word_idx * 4;
        w[word_idx] = ((U32)chunk[byte_idx] << 24)
                    | ((U32)chunk[byte_idx + 1] << 16)
                    | ((U32)chunk[byte_idx + 2] << 8)
                    | ((U32)chunk[byte_idx + 3]);
    }
    for (UZ word_idx = 16; word_idx < 64; ++word_idx) {
        U32 s0 = rotr32(w[word_idx - 15], 7) ^ rotr32(w[word_idx - 15], 18) ^ (w[word_idx - 15] >> 3);
        U32 s1 = rotr32(w[word_idx - 2], 17) ^ rotr32(w[word_idx - 2], 19) ^ (w[word_idx - 2] >> 10);
        w[word_idx] = w[word_idx - 16] + s0 + w[word_idx - 7] + s1;
    }

    U32 a = h[0], b = h[1], c = h[2], d = h[3];
    U32 e = h[4], f = h[5], g = h[6], hh = h[7];

    for (UZ round_idx = 0; round_idx < 64; ++round_idx) {
        U32 S1    = rotr32(e, 6) ^ rotr32(e, 11) ^ rotr32(e, 25);
        U32 ch    = (e & f) ^ (~e & g);
        U32 temp1 = hh + S1 + ch + K[round_idx] + w[round_idx];
        U32 S0    = rotr32(a, 2) ^ rotr32(a, 13) ^ rotr32(a, 22);
        U32 maj   = (a & b) ^ (a & c) ^ (b & c);
        U32 temp2 = S0 + maj;

        hh = g;
        g  = f;
        f  = e;
        e  = d + temp1;
        d  = c;
        c  = b;
        b  = a;
        a  = temp1 + temp2;
    }

    h[0] += a; h[1] += b; h[2] += c; h[3] += d;
    h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
}

SHA256Digest sha256_digest(ok::StringView sv) {
    U32 h[8];
    memcpy(h, INIT, sizeof(h));

    UZ chunk_offset = 0;
    while (chunk_offset + 64 <= sv.count) {
        process_chunk(h, (const U8 *)(sv.data + chunk_offset));
        chunk_offset += 64;
    }

    U8 block[64] = {};
    UZ remaining = sv.count - chunk_offset;
    memcpy(block, sv.data + chunk_offset, remaining);
    block[remaining] = 0x80;

    if (remaining >= 56) {
        process_chunk(h, block);
        memset(block, 0, 64);
    }

    U64 bit_len = (U64)sv.count * 8;
    block[56] = (U8)(bit_len >> 56);
    block[57] = (U8)(bit_len >> 48);
    block[58] = (U8)(bit_len >> 40);
    block[59] = (U8)(bit_len >> 32);
    block[60] = (U8)(bit_len >> 24);
    block[61] = (U8)(bit_len >> 16);
    block[62] = (U8)(bit_len >> 8);
    block[63] = (U8)(bit_len);
    process_chunk(h, block);

    SHA256Digest digest{};
    for (UZ word_idx = 0; word_idx < 8; ++word_idx) {
        digest.bytes.items[word_idx * 4]     = (U8)(h[word_idx] >> 24);
        digest.bytes.items[word_idx * 4 + 1] = (U8)(h[word_idx] >> 16);
        digest.bytes.items[word_idx * 4 + 2] = (U8)(h[word_idx] >> 8);
        digest.bytes.items[word_idx * 4 + 3] = (U8)(h[word_idx]);
    }

    return digest;
}

} // namespace xmdb
