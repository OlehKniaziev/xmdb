#pragma once

namespace xmdb {
template <typename T>
struct RemoveRefT {
    using Type = T;
};

template <typename T>
struct RemoveRefT<T&> {
    using Type = T;
};

template <typename T>
struct RemoveRefT<T&&> {
    using Type = T;
};

template <typename T>
using RemoveRef = RemoveRefT<T>::Type;

template <typename T>
struct IsLValueRef {
    static constexpr bool Value = false;
};

template <typename T>
struct IsLValueRef<T&> {
    static constexpr bool Value = true;
};

template <typename T>
struct IsLValueRef<T&&> {
    static constexpr bool Value = false;
};
} // namespace xmdb
