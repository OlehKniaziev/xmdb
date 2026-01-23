#include "FixedString.hpp"

namespace xmdb {
FixedString create_fixed_string(ok::StringView sv) {
    OK_VERIFY(sv.count <= FixedString::DATA_SIZE);

    FixedString fs = {};
    memcpy(fs.items, sv.data, sv.count);
    fs.count = (U8) sv.count;
    return fs;
}
} // namespace xmdb
