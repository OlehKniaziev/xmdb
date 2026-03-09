#include "util.hpp"
#include "ok.hpp"

namespace xmdb {
void dief(const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    printf("\n");

    exit(1);
}

static char quad_to_hex_table[] = {
    [0] = '0',
    [1] = '1',
    [2] = '2',
    [3] = '3',
    [4] = '4',
    [5] = '5',
    [6] = '6',
    [7] = '7',
    [8] = '8',
    [9] = '9',
    [10] = 'A',
    [11] = 'B',
    [12] = 'C',
    [13] = 'D',
    [14] = 'E',
    [15] = 'F',
};

ok::String to_hex_string(ok::Allocator *allocator, ok::Slice<U8> bytes) {
    ok::String s = ok::String::alloc(allocator, bytes.count * 2);
    for (UZ i = 0; i < bytes.count; ++i) {
        U8 b = bytes[i];
        char c1 = quad_to_hex_table[(b & 0xF0) >> 4];
        char c2 = quad_to_hex_table[ b & 0x0F      ];
        s.push(c1);
        s.push(c2);
    }
    return s;
}

ok::Optional<ok::Slice<U8>> from_hex_string(ok::Allocator *allocator, ok::StringView hex) {
    UZ buffer_count = hex.count / 2;
    U8 *buffer = allocator->alloc<U8>(buffer_count);

    for (SZ i = 0; i < (SZ) hex.count - 1; i += 2) {
        U8 c1 = tolower(hex[i]);
        U8 c2 = tolower(hex[i + 1]);
        U8 cs[] = {c1, c2};

        U8 b = 0;

        for (UZ ci = 0; ci < sizeof(cs) / sizeof(cs[0]); ++ci) {
            U8 c = cs[ci];
            U8 start = 0;

            if (c >= '0' && c <= '9') {
                start = '0';
            } else if (c >= 'a' && c <= 'f') {
                start = 'a' - 10;
            } else {
                return {};
            }

            b <<= 4;
            b |= c - start;
        }

        buffer[i / 2] = b;
    }

    return ok::Slice<U8>{buffer, buffer_count};
}
} // namespace xmdb
