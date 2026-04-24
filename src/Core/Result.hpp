#pragma once

#include "ok.hpp"

namespace xmdb
{
#define CHECK(res)                                                             \
    ({                                                                         \
        auto _x = (res);                                                       \
        if (!_x.ok()) return _x.error();                                       \
        _x.unwrap();                                                           \
    })

#define OK(res)                                                                \
    do {                                                                       \
        auto _x = (res);                                                       \
        if (!_x.ok()) return res.error();                                      \
    }                                                                          \
    while (false)

/**
 * @brief A template class representing either a success value or an error
 * value.
 * @tparam TOk The type of the success value.
 * @tparam TError The type of the error value.
 */
template <typename TOk, typename TError>
class [[nodiscard]] Result
{
public:
    /**
     * @brief Constructs a success Result.
     * @param ok The success value.
     */
    Result(const TOk &ok) : m_ok{true}, m_u{.ok_val = ok}
    {
    }

    /**
     * @brief Constructs an error Result.
     * @param error The error value.
     */
    Result(const TError &error) : m_ok{false}, m_u{.error_val = error}
    {
    }

    /**
     * @brief Unwraps the success value. Panics if the Result is an error.
     * @return Reference to the success value.
     */
    TOk &unwrap();

    /**
     * @brief Unwraps the success value (const version). Panics if the Result is
     * an error.
     * @return Const reference to the success value.
     */
    const TOk &unwrap() const;

    /**
     * @brief Unwraps the success value without checking if it's an error.
     * @return Reference to the success value.
     */
    TOk &unwrap_unchecked();

    /**
     * @brief Unwraps the success value without checking if it's an error (const
     * version).
     * @return Const reference to the success value.
     */
    const TOk &unwrap_unchecked() const;

    /**
     * @brief Retrieves the error value. Panics if the Result is success.
     * @return Reference to the error value.
     */
    TError &error();

    /**
     * @brief Retrieves the error value (const version). Panics if the Result is
     * success.
     * @return Const reference to the error value.
     */
    const TError &error() const;

    /**
     * @brief Retrieves the error value without checking if it's a success.
     * @return Reference to the error value.
     */
    TError &error_unchecked();

    /**
     * @brief Retrieves the error value without checking if it's a success
     * (const version).
     * @return Const reference to the error value.
     */
    const TError &error_unchecked() const;

    /**
     * @brief Matches the result using provided functions for success and error
     * cases.
     * @tparam TResult The return type of the matching functions.
     * @tparam TOkFn Type of the success function.
     * @tparam TErrorFn Type of the error function.
     * @param ok_fn Function to call on success.
     * @param error_fn Function to call on error.
     * @return The result of the called function.
     */
    template <typename TResult, typename TOkFn, typename TErrorFn>
    TResult match(TOkFn ok_fn, TErrorFn error_fn);

    /**
     * @brief Matches the result using provided functions (const version).
     */
    template <typename TResult, typename TOkFn, typename TErrorFn>
    TResult match(TOkFn ok_fn, TErrorFn error_fn) const;

    /**
     * @brief Checks if the Result is a success.
     * @return true if success, false if error.
     */
    bool ok() const
    {
        return m_ok;
    }

private:
    bool m_ok;
    union
    {
        TOk ok_val;
        TError error_val;
    } m_u;
};

template <typename TError>
class [[nodiscard]] Result<void, TError>
{
public:
    Result() : m_error{}
    {
    }

    Result(const TError &error) : m_error{error}
    {
    }

    void unwrap() = delete;
    void unwrap() const = delete;
    void unwrap_unchecked() = delete;
    void unwrap_unchecked() const = delete;

    TError &error()
    {
        if (ok())
        {
            OK_PANIC("Called 'error' on an ok value");
        }

        return m_error.get();
    }

    const TError &error() const
    {
        if (ok())
        {
            OK_PANIC("Called 'error' on an ok value");
        }

        return m_error.get();
    }

    TError &error_unchecked()
    {
        OK_ASSERT(!ok());
        return m_error.get_unchecked();
    }

    const TError &error_unchecked() const
    {
        OK_ASSERT(!ok());
        return m_error.get_unchecked();
    }

    template <typename TResult, typename TOkFn, typename TErrorFn>
    TResult match(TOkFn ok_fn, TErrorFn error_fn) = delete;

    template <typename TResult, typename TOkFn, typename TErrorFn>
    TResult match(TOkFn ok_fn, TErrorFn error_fn) const = delete;

    bool ok() const
    {
        return !m_error.has_value();
    }

private:
    ok::Optional<TError> m_error;
};

template <typename TOk, typename TError>
TOk &Result<TOk, TError>::unwrap()
{
    if (!ok())
    {
        OK_PANIC("Called 'unwrap' on an error value");
    }

    return m_u.ok_val;
}

template <typename TOk, typename TError>
const TOk &Result<TOk, TError>::unwrap() const
{
    if (!ok())
    {
        OK_PANIC("Called 'unwrap' on an error value");
    }

    return m_u.ok_val;
}

template <typename TOk, typename TError>
TOk &Result<TOk, TError>::unwrap_unchecked()
{
    OK_ASSERT(ok());
    return m_u.ok_val;
}

template <typename TOk, typename TError>
const TOk &Result<TOk, TError>::unwrap_unchecked() const
{
    OK_ASSERT(ok());
    return m_u.ok_val;
}

template <typename TOk, typename TError>
TError &Result<TOk, TError>::error()
{
    if (ok())
    {
        OK_PANIC("Called 'error' on an ok value");
    }

    return m_u.error_val;
}

template <typename TOk, typename TError>
const TError &Result<TOk, TError>::error() const
{
    if (ok())
    {
        OK_PANIC("Called 'error' on an ok value");
    }

    return m_u.error_val;
}

template <typename TOk, typename TError>
TError &Result<TOk, TError>::error_unchecked()
{
    OK_ASSERT(!ok());
    return m_u.error_val;
}

template <typename TOk, typename TError>
const TError &Result<TOk, TError>::error_unchecked() const
{
    OK_ASSERT(!ok());
    return m_u.error_val;
}

template <typename TOk, typename TError>
template <typename TResult, typename TOkFn, typename TErrorFn>
TResult Result<TOk, TError>::match(TOkFn ok_fn, TErrorFn error_fn)
{
    if (ok())
    {
        return ok_fn(m_u.ok_val);
    }
    else
    {
        return error_fn(m_u.error_val);
    }
}

template <typename TOk, typename TError>
template <typename TResult, typename TOkFn, typename TErrorFn>
TResult Result<TOk, TError>::match(TOkFn ok_fn, TErrorFn error_fn) const
{
    if (ok())
    {
        return ok_fn(m_u.ok_val);
    }
    else
    {
        return error_fn(m_u.error_val);
    }
}
} // namespace xmdb
