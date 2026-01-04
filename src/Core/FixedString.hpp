#pragma once

namespace xmdb {
class FixedString {
public:
    static constexpr UZ SIZE = 256;

    ok::Slice<const U8> slice() const {
        return {reinterpret_cast<const U8 *>(m_buffer), SIZE};
    }

private:
    U8 m_buffer[SIZE];
};
}
