#pragma once

/**
 * @brief Macro to create a distinct numeric type that wraps an underlying type.
 * This provides type safety by preventing accidental mixing of different
 * numeric contexts.
 */
#define XMDB_MAKE_DISTINCT_NUMERIC(type, underlying)                           \
    class type                                                                 \
    {                                                                          \
    public:                                                                    \
        type() : m_value{}                                                     \
        {                                                                      \
        }                                                                      \
        explicit type(underlying value) : m_value{value}                       \
        {                                                                      \
        }                                                                      \
                                                                               \
        type(const type &other) = default;                                     \
        type &operator=(const type &other) = default;                          \
                                                                               \
        underlying value() const                                               \
        {                                                                      \
            return m_value;                                                    \
        }                                                                      \
                                                                               \
        void set_value(underlying value)                                       \
        {                                                                      \
            m_value = value;                                                   \
        }                                                                      \
                                                                               \
        type operator+(const type &other) const                                \
        {                                                                      \
            return type(m_value + other.m_value);                              \
        }                                                                      \
                                                                               \
        type operator-(const type &other) const                                \
        {                                                                      \
            return type(m_value - other.m_value);                              \
        }                                                                      \
                                                                               \
        type operator*(const type &other) const                                \
        {                                                                      \
            return type(m_value * other.m_value);                              \
        }                                                                      \
                                                                               \
        type operator/(const type &other) const                                \
        {                                                                      \
            return type(m_value / other.m_value);                              \
        }                                                                      \
                                                                               \
        type operator&(const type &other) const                                \
        {                                                                      \
            return type(m_value & other.m_value);                              \
        }                                                                      \
                                                                               \
        type operator|(const type &other) const                                \
        {                                                                      \
            return type(m_value | other.m_value);                              \
        }                                                                      \
                                                                               \
        type operator^(const type &other) const                                \
        {                                                                      \
            return type(m_value ^ other.m_value);                              \
        }                                                                      \
                                                                               \
        type operator<<(int n) const                                           \
        {                                                                      \
            return type(m_value << n);                                         \
        }                                                                      \
                                                                               \
        type operator>>(int n) const                                           \
        {                                                                      \
            return type(m_value >> n);                                         \
        }                                                                      \
                                                                               \
        auto operator<=>(const type &other) const                              \
        {                                                                      \
            return m_value - other.m_value;                                    \
        }                                                                      \
                                                                               \
    private:                                                                   \
        underlying m_value;                                                    \
    };

/**
 * @brief Macro to declare a type-safe flags class.
 */
#define XMDB_DECLARE_FLAGS(name, underlying)                                   \
    class name                                                                 \
    {                                                                          \
    public:                                                                    \
        name() : m_flags{}                                                     \
        {                                                                      \
        }                                                                      \
                                                                               \
    TODO                                                                       \
                                                                               \
            private : underlying m_flags;                                      \
    };                                                                         \
                                                                               \
    enum : underlying
