#pragma once

#include <SQL/type_check.hpp>

#include "ColumnLayout.hpp"
#include "image.hpp"
#include "video.hpp"

namespace xmdb
{
/**
 * @brief Abstract base class for database values used in query planning and
 * execution.
 */
class DBValue
{
public:
    /**
     * @brief The kind of database value.
     */
    enum class Kind
    {
        COLUMN, ///< Value from a table column.
        COMPARE, ///< Result of a comparison.
        CONSTANT, ///< A constant value.
        NONE, ///< Represents no value or null.
        CONCAT, ///< Result of a concatenation.
        IMAGE_DATA, ///< Raw image data.
        DELAYED, ///< A value that will be provided later.
        MEDIA, ///< Media source.
    };

    DBValue() = delete;

    /**
     * @brief Statically allocates a null value.
     * @return The null value.
     */
    static DBValue *null()
    {
        OK_TODO();
    }

    /**
     * @brief Gets the SQL type of the value.
     * @return The SQL type.
     */
    SQL::Type type() const
    {
        return m_type;
    }

    /**
     * @brief Gets the kind of the value.
     * @return The kind.
     */
    Kind kind() const
    {
        return m_kind;
    }

    /**
     * @brief Checks if this value is type-compatible with another value. NONE
     * values are compatible with any value.
     * @param other The other value to check against.
     * @return true if compatible, false otherwise.
     */
    bool is_compatible_with(const DBValue *other) const
    {
        if (m_kind == Kind::NONE || other->m_kind == Kind::NONE)
        {
            return true;
        }

        return m_type == other->m_type;
    }

protected:
    DBValue(SQL::Type type, Kind kind) : m_type{type}, m_kind{kind}
    {
    }

    SQL::Type m_type;
    Kind m_kind;
};

class DBTable;

/**
 * @brief A DBValue representing a column in a specific table.
 */
class ColumnDBValue : public DBValue
{
public:
    /**
     * @brief Constructs a new ColumnDBValue.
     * @param type The SQL type of the column.
     * @param layout The layout of the column.
     * @param table The table the column belongs to.
     */
    ColumnDBValue(SQL::Type type, ColumnLayout layout, DBTable *table) :
        DBValue{type, Kind::COLUMN}, m_layout{layout}, m_table{table}
    {
    }

    /**
     * @brief Gets the table this column value belongs to.
     * @return Pointer to the table.
     */
    DBTable *table()
    {
        return m_table;
    }

    /**
     * @brief Gets the layout of the column.
     * @return The column layout.
     */
    ColumnLayout layout()
    {
        return m_layout;
    }

private:
    ColumnLayout m_layout;
    DBTable *m_table;
};

/**
 * @brief A DBValue representing a constant (literal) value.
 */
class ConstDBValue : public DBValue
{
public:
    /**
     * @brief The kind of constant value.
     */
    enum class ConstKind
    {
        INT, ///< Integer constant.
        STRING, ///< String constant.
        BOOL, ///< Boolean constant.
    };

    /**
     * @brief Constructs a constant integer value.
     * @param i The integer value.
     */
    explicit ConstDBValue(S64 i) :
        ConstDBValue{SQL::TYPE_INT, ConstKind::INT, reinterpret_cast<void *>(i)}
    {
    }

    /**
     * @brief Constructs a constant string value.
     * @param s Pointer to the string value.
     */
    explicit ConstDBValue(ok::String *s) :
        ConstDBValue{SQL::TYPE_STRING, ConstKind::STRING,
                     static_cast<void *>(s)}
    {
    }

    /**
     * @brief Constructs a constant boolean value.
     * @param b The boolean value.
     */
    explicit ConstDBValue(bool b) :
        ConstDBValue{SQL::TYPE_BOOL, ConstKind::BOOL,
                     reinterpret_cast<void *>(static_cast<U64>(b))}
    {
    }

    /**
     * @brief Gets the constant kind.
     * @return The constant kind.
     */
    ConstKind kind() const
    {
        return m_const_kind;
    }

    /**
     * @brief Retrieves the value as an integer.
     * @return The integer value.
     */
    S64 as_int() const
    {
        return reinterpret_cast<S64>(m_data);
    }

    /**
     * @brief Retrieves the value as a boolean.
     * @return The boolean value.
     */
    bool as_bool() const
    {
        return static_cast<bool>(reinterpret_cast<U64>(m_data));
    }

    /**
     * @brief Retrieves the value as a string.
     * @return Pointer to the string value.
     */
    ok::String *as_string()
    {
        return static_cast<ok::String *>(m_data);
    }

private:
    explicit ConstDBValue(SQL::Type type, ConstKind kind, void *data) :
        DBValue{type, Kind::CONSTANT}, m_const_kind{kind}, m_data{data}
    {
    }

    ConstKind m_const_kind;
    void *m_data;
};

/**
 * @brief A DBValue representing a comparison between two other values.
 */
class CompareDBValue : public DBValue
{
public:
    /**
     * @brief Constructs a comparison value.
     * @param lhs The left-hand side value.
     * @param rhs The right-hand side value.
     */
    CompareDBValue(DBValue *lhs, DBValue *rhs) :
        DBValue{lhs->type(), Kind::COMPARE}, m_lhs{lhs}, m_rhs{rhs}
    {
        OK_ASSERT(lhs->is_compatible_with(rhs));
    }

    /**
     * @brief Gets the left-hand side of the comparison.
     * @return Pointer to the LHS value.
     */
    DBValue *lhs()
    {
        return m_lhs;
    }

    /**
     * @brief Gets the right-hand side of the comparison.
     * @return Pointer to the RHS value.
     */
    DBValue *rhs()
    {
        return m_rhs;
    }

private:
    DBValue *m_lhs;
    DBValue *m_rhs;
};

/**
 * @brief A DBValue representing "no value".
 */
class NoneDBValue : public DBValue
{
public:
    NoneDBValue() : DBValue{SQL::TYPE_NULL, Kind::NONE}
    {
    }
};

/**
 * @brief Base class for values that consist of a pair of other values.
 */
class PairDBValue : public DBValue
{
public:
    PairDBValue() = delete;

    /**
     * @brief Gets the left-hand side value.
     * @return Pointer to the LHS value.
     */
    DBValue *lhs()
    {
        return m_lhs;
    }

    /**
     * @brief Gets the right-hand side value.
     * @return Pointer to the RHS value.
     */
    DBValue *rhs()
    {
        return m_rhs;
    }

protected:
    PairDBValue(SQL::Type type, Kind kind, DBValue *lhs, DBValue *rhs) :
        DBValue{type, kind}, m_lhs{lhs}, m_rhs{rhs}
    {
    }

    DBValue *m_lhs;
    DBValue *m_rhs;
};

/**
 * @brief A DBValue representing the concatenation of two values.
 */
class ConcatDBValue : public PairDBValue
{
public:
    /**
     * @brief Constructs a concatenation value.
     * @param lhs The first value to concatenate.
     * @param rhs The second value to concatenate.
     */
    ConcatDBValue(DBValue *lhs, DBValue *rhs) :
        PairDBValue{lhs->type(), Kind::CONCAT, lhs, rhs}
    {
        OK_ASSERT(lhs->is_compatible_with(rhs));
    }
};

/**
 * @brief A DBValue representing raw image data.
 */
class ImageDataDBValue : public DBValue
{
public:
    /**
     * @brief Constructs an image data value.
     * @param width The width of the image.
     * @param height The height of the image.
     * @param data The raw pixel data.
     * @param format The pixel format.
     */
    ImageDataDBValue(U32 width, U32 height, ok::Slice<U8> data,
                     PixelFormat format) :
        DBValue{SQL::TYPE_IMAGE_CHUNK, Kind::IMAGE_DATA},
        m_width{width},
        m_height{height},
        m_data{data},
        m_format{format}
    {
    }

    /**
     * @brief Gets the width of the image.
     * @return The width.
     */
    U32 width()
    {
        return m_width;
    }

    /**
     * @brief Gets the height of the image.
     * @return The height.
     */
    U32 height()
    {
        return m_height;
    }

    /**
     * @brief Gets the raw pixel data.
     * @return A slice of the pixel data.
     */
    ok::Slice<U8> data()
    {
        return m_data;
    }

    /**
     * @brief Gets the pixel format.
     * @return The format.
     */
    PixelFormat format()
    {
        return m_format;
    }

private:
    U32 m_width;
    U32 m_height;
    ok::Slice<U8> m_data;
    PixelFormat m_format;
};

/**
 * @brief A DBValue whose underlying value is determined later during execution.
 */
class DelayedDBValue : public DBValue
{
public:
    /**
     * @brief Constructs a new DelayedDBValue.
     */
    explicit DelayedDBValue() : DBValue{{}, Kind::DELAYED}, m_value{}
    {
    }

    /**
     * @brief Sets the actual value.
     * @param value Pointer to the DBValue to set.
     */
    void set(DBValue *value)
    {
        m_value = value;
    }

    /**
     * @brief Gets the actual value if it has been set.
     * @return An optional containing the value.
     */
    ok::Optional<DBValue *> value()
    {
        return m_value;
    }

private:
    ok::Optional<DBValue *> m_value;
};

class MediaSourceDBValue : public DBValue
{
public:
    explicit MediaSourceDBValue(const MediaSource &source) :
        DBValue{SQL::TYPE_MEDIA, Kind::MEDIA}, m_source{source}
    {
    }

    MediaSource source() const
    {
        return m_source;
    }

private:
    MediaSource m_source;
};
} // namespace xmdb
