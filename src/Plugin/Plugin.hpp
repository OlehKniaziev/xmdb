#pragma once

#include <Core/ok.hpp>
#include <Core/Result.hpp>

#include "NativeLibrary.hpp"

namespace xmdb::plugin {
class PluginCapability {
public:
    PluginCapability(ok::StringView name, NativeSymbol symbol) : m_name{name}, m_symbol{symbol} {}

    ok::StringView name() const {
        return m_name;
    }

    NativeSymbol symbol() const {
        return m_symbol;
    }

private:
    ok::StringView m_name;
    NativeSymbol m_symbol;
};

/**
 * @brief A representation of a dynamic library with specific
 * exported functions and structure.
 */
class Plugin {
public:
    static Result<Plugin *, ok::String> load(ok::Allocator *allocator,
                                             const NativeLibrary *native_lib);

    void install(ok::StringView name);

    void unload();

    ok::StringView get_last_error();

    ok::Optional<PluginCapability> get_capability(ok::StringView name) const;

    template <typename T, typename... Args>
    T use_capability(PluginCapability capability, Args&&... args);

private:
    using LoadHook = int (*)(void *);
    using UnloadHook = void (*)(void *);
    using InstallHook = void (*)(void *,
                                 const char *,
                                 const char ***,
                                 void ***,
                                 int *);
    using GetLastErrorHook = const char *(*)(void *);

    using CapabilitiesList = ok::List<ok::Pair<ok::StringView, NativeSymbol>>;

    Plugin(const NativeLibrary *native_lib,
           void *plugin_state,
           UnloadHook unload_hook,
           InstallHook install_hook,
           GetLastErrorHook get_last_error_hook,
           CapabilitiesList capabilities) : m_native_lib{native_lib},
                                            m_plugin_state{plugin_state},
                                            m_unload_hook{unload_hook},
                                            m_install_hook{install_hook},
                                            m_get_last_error_hook{get_last_error_hook},
                                            m_capabilities{capabilities} {
    }

    const NativeLibrary *m_native_lib;
    void *m_plugin_state;
    ok::Optional<UnloadHook> m_unload_hook;
    InstallHook m_install_hook;
    ok::Optional<GetLastErrorHook> m_get_last_error_hook;
    CapabilitiesList m_capabilities;
    bool m_installed{};
};

template <typename T, typename... Args>
T Plugin::use_capability(PluginCapability capability, Args&&... args) {
    auto fn_ptr = capability.symbol().cast<T (*)(void *, Args...)>();
    T result = fn_ptr(m_plugin_state, args...);
    return result;
}
} // namespace xmdb::plugin
