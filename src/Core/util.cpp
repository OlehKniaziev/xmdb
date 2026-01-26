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
} // namespace xmdb
