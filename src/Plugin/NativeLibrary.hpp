#pragma once

#include <Core/ok.hpp>
#include <Core/Result.hpp>

namespace xmdb::plugin {
class NativeSymbol {
public:
    NativeSymbol() = default;

    explicit NativeSymbol(void *ptr) : m_ptr{ptr} {}

    template <typename T>
    T cast() const {
        return reinterpret_cast<T>(m_ptr);
    }

private:
    void *m_ptr;
};

class NativeLibrary {
public:
    ok::Optional<NativeSymbol> get_symbol(ok::StringView name) const;

    static Result<NativeLibrary, ok::String> load(ok::Allocator *allocator,
                                                  ok::StringView path);

    void unload();

private:
    NativeLibrary(void *state, ok::StringView path) : m_state{state}, m_path{path} {}

    void *m_state;
    ok::StringView m_path;
};
} // namespace xmdb::plugin
