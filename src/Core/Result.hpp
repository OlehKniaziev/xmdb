#pragma once

namespace xmdb {
template <typename TOk, typename TError>
class Result {
public:
    Result(const TOk &ok) : m_ok{true}, m_u{.ok_val = ok} {}
    Result(const TError &error) : m_ok{false}, m_u{.error_val = error} {}

    TOk &unwrap();
    const TOk &unwrap() const;

    TOk &unwrap_unchecked();
    const TOk &unwrap_unchecked() const;

    TError &error();
    const TError &error() const;

    TError &error_unchecked();
    const TError &error_unchecked() const;

    template <typename TResult, typename TOkFn, typename TErrorFn>
    TResult match(TOkFn, TErrorFn);

    template <typename TResult, typename TOkFn, typename TErrorFn>
    TResult match(TOkFn, TErrorFn) const;

    bool ok() const {
        return m_ok;
    }

private:
    bool m_ok;
    union {
        TOk ok_val;
        TError error_val;
    } m_u;
};

template <typename TOk, typename TError>
TOk &Result<TOk, TError>::unwrap() {
    if (!ok()) {
        OK_PANIC("Called 'unwrap' on an error value");
    }

    return m_u.ok_val;
}

template <typename TOk, typename TError>
const TOk &Result<TOk, TError>::unwrap() const {
    if (!ok()) {
        OK_PANIC("Called 'unwrap' on an error value");
    }

    return m_u.ok_val;
}

template <typename TOk, typename TError>
TOk &Result<TOk, TError>::unwrap_unchecked() {
    OK_ASSERT(ok());
    return m_u.ok_val;
}

template <typename TOk, typename TError>
const TOk &Result<TOk, TError>::unwrap_unchecked() const {
    OK_ASSERT(ok());
    return m_u.ok_val;
}

template <typename TOk, typename TError>
TError &Result<TOk, TError>::error() {
    if (ok()) {
        OK_PANIC("Called 'error' on an ok value");
    }

    return m_u.error_val;
}

template <typename TOk, typename TError>
const TError &Result<TOk, TError>::error() const {
    if (ok()) {
        OK_PANIC("Called 'error' on an ok value");
    }

    return m_u.error_val;
}

template <typename TOk, typename TError>
TError &Result<TOk, TError>::error_unchecked() {
    OK_ASSERT(!ok());
    return m_u.error_val;
}

template <typename TOk, typename TError>
const TError &Result<TOk, TError>::error_unchecked() const {
    OK_ASSERT(!ok());
    return m_u.error_val;
}

template <typename TOk, typename TError>
template <typename TResult, typename TOkFn, typename TErrorFn>
TResult Result<TOk, TError>::match(TOkFn ok_fn, TErrorFn error_fn) {
    if (ok()) {
        return ok_fn(m_u.ok_val);
    } else {
        return error_fn(m_u.error_val);
    }
}

template <typename TOk, typename TError>
template <typename TResult, typename TOkFn, typename TErrorFn>
TResult Result<TOk, TError>::match(TOkFn ok_fn, TErrorFn error_fn) const {
    if (ok()) {
        return ok_fn(m_u.ok_val);
    } else {
        return error_fn(m_u.error_val);
    }
}
} // namespace xmdb
