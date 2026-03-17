#pragma once

#include <SQL/type_check.hpp>

#include "ColumnLayout.hpp"
#include "FixedString.hpp"
#include "image.hpp"

namespace xmdb {
class Value {
public:
    enum class Type {
        INT,
        BOOL,
        STRING,
        IMAGE_CHUNK,
    };

    Value() = delete;

    static Value integer(S64 value) {
        return Value{Type::INT, reinterpret_cast<void *>(static_cast<U64>(value))};
    }

    static Value boolean(bool value) {
        return Value{Type::BOOL, reinterpret_cast<void *>(static_cast<U64>(value))};
    }

    static Value string(ok::Allocator *a, FixedString s) {
        FixedString *ptr = a->alloc<FixedString>();
        memcpy(ptr, reinterpret_cast<U8 *>(&s), sizeof(s));
        return Value{Type::STRING, reinterpret_cast<void *>(ptr)};
    }

    static Value image_chunk(ok::Allocator *a,
                             U64 x,
                             U64 y,
                             U64 width,
                             U64 height,
                             ok::Slice<U8> data,
                             PixelFormat format) {
        ImageChunk *chunk = a->alloc<ImageChunk>();
        chunk->x = x;
        chunk->y = y;
        chunk->width = width;
        chunk->height = height;
        chunk->data = data;
        chunk->pixel_format = format;
        return Value{Type::IMAGE_CHUNK, reinterpret_cast<void *>(chunk)};
    }

    static Value greater() {
        return integer(1);
    }

    static Value less() {
        return integer(-1);
    }

    static Value equal() {
        return integer(0);
    }

    S64 as_int() {
        OK_VERIFY(type() == Type::INT);
        return static_cast<S64>(reinterpret_cast<U64>(m_data));
    }

    bool as_bool() {
        OK_VERIFY(type() == Type::BOOL);
        return static_cast<bool>(reinterpret_cast<U64>(m_data));
    }

    FixedString as_string() {
        return *as_string_ptr();
    }

    const FixedString *as_string_ptr() const {
        OK_VERIFY(type() == Type::STRING);
        return reinterpret_cast<const FixedString *>(m_data);
    }

    ImageChunk *as_chunk() {
        OK_VERIFY(type() == Type::IMAGE_CHUNK);
        return reinterpret_cast<ImageChunk *>(m_data);
    }

    Type type() const {
        return m_type;
    }

    Value compare(Value);

private:
    Value(Type type, void *data) : m_type{type}, m_data{data} {}

    Type m_type;
    void *m_data;
};
} // namespace xmdb
