#pragma once

#include <SQL/type_check.hpp>

#include "ColumnLayout.hpp"
#include "FixedString.hpp"
#include "image.hpp"
#include "new.hpp"
#include "video.hpp"

namespace xmdb {
/**
 * @brief Represents a concrete value of a specific SQL type.
 */
class Value {
public:
    /**
     * @brief The type of the value.
     */
    enum class Type {
        INT, ///< Integer value.
        BOOL, ///< Boolean value.
        STRING, ///< Fixed string value.
        IMAGE_CHUNK, ///< Image chunk value.
        BIG_STRING, ///< String of size exceeding the maximum size of a fixed
                    ///< string.
        MEDIA_SOURCE, ///< Media stream.
    };

    Value() = delete;

    /**
     * @brief Creates an integer value.
     * @param value The integer.
     * @return The resulting Value.
     */
    static Value integer(S64 value) {
        return Value{Type::INT,
                     reinterpret_cast<void *>(static_cast<U64>(value))};
    }

    /**
     * @brief Creates a boolean value.
     * @param value The boolean.
     * @return The resulting Value.
     */
    static Value boolean(bool value) {
        return Value{Type::BOOL,
                     reinterpret_cast<void *>(static_cast<U64>(value))};
    }

    /**
     * @brief Creates a string value.
     * @param a The allocator to use for storing the string.
     * @param s The fixed string.
     * @return The resulting Value.
     */
    static Value string(ok::Allocator *a, FixedString s) {
        FixedString *ptr = a->alloc<FixedString>();
        memcpy(ptr, reinterpret_cast<U8 *>(&s), sizeof(s));
        return Value{Type::STRING, reinterpret_cast<void *>(ptr)};
    }

    /**
     * @brief Creates an image chunk value.
     * @param a The allocator to use.
     * @param x X-coordinate.
     * @param y Y-coordinate.
     * @param width Width.
     * @param height Height.
     * @param data Raw pixel data.
     * @param format Pixel format.
     * @return The resulting Value.
     */
    static Value image_chunk(ok::Allocator *a, U64 x, U64 y, U64 width,
                             U64 height, ok::Slice<U8> data,
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

    /**
     * @brief Creates a "big string" value.
     * @param allocator The allocator to use.
     * @param data String data.
     * @return The resulting Value.
     */
    static Value big_string(ok::Allocator *allocator, ok::StringView data) {
        ok::String s_value = data.to_string(allocator);
        ok::String *s = allocator->alloc<ok::String>();
        *s = s_value;
        return Value{Type::BIG_STRING, reinterpret_cast<void *>(s)};
    }

    static Value media_source(MediaSource *source) {
        return Value{Type::MEDIA_SOURCE, reinterpret_cast<void *>(source)};
    }

    /**
     * @brief Creates a "greater than" comparison result (integer 1).
     * @return The resulting Value.
     */
    static Value greater() {
        return integer(1);
    }

    /**
     * @brief Creates a "less than" comparison result (integer -1).
     * @return The resulting Value.
     */
    static Value less() {
        return integer(-1);
    }

    /**
     * @brief Creates an "equal" comparison result (integer 0).
     * @return The resulting Value.
     */
    static Value equal() {
        return integer(0);
    }

    /**
     * @brief Retrieves the value as an integer.
     * @return The integer value.
     */
    S64 as_int() {
        OK_VERIFY(type() == Type::INT);
        return static_cast<S64>(reinterpret_cast<U64>(m_data));
    }

    /**
     * @brief Retrieves the value as a boolean.
     * @return The boolean value.
     */
    bool as_bool() {
        OK_VERIFY(type() == Type::BOOL);
        return static_cast<bool>(reinterpret_cast<U64>(m_data));
    }

    /**
     * @brief Retrieves the value as a FixedString.
     * @return The fixed string.
     */
    FixedString as_string() {
        return *as_string_ptr();
    }

    /**
     * @brief Retrieves the value as a pointer to FixedString.
     * @return Const pointer to the fixed string.
     */
    const FixedString *as_string_ptr() const {
        OK_VERIFY(type() == Type::STRING);
        return reinterpret_cast<const FixedString *>(m_data);
    }

    /**
     * @brief Retrieves the value as an ImageChunk pointer.
     * @return Pointer to the image chunk.
     */
    ImageChunk *as_chunk() {
        OK_VERIFY(type() == Type::IMAGE_CHUNK);
        return reinterpret_cast<ImageChunk *>(m_data);
    }

    /**
     * @brief Retrieves the value as a pointer to a String.
     * @return Pointer to the string.
     */
    ok::String *as_big_string() {
        OK_VERIFY(type() == Type::BIG_STRING);
        return reinterpret_cast<ok::String *>(m_data);
    }

    MediaSource *as_media_source() {
        OK_VERIFY(type() == Type::MEDIA_SOURCE);
        return reinterpret_cast<MediaSource *>(m_data);
    }

    /**
     * @brief Gets the type of the value.
     * @return The type.
     */
    Type type() const {
        return m_type;
    }

    /**
     * @brief Compares this value with another value.
     * @param other The other value to compare with.
     * @return A Value representing the comparison result.
     */
    Value compare(Value other);

private:
    Value(Type type, void *data) : m_type{type}, m_data{data} {
    }

    Type m_type;
    void *m_data;
};
} // namespace xmdb
