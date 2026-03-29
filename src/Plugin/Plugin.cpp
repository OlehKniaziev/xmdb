#include "Plugin.hpp"

#include <Core/new.hpp>

#include "util.hpp"

namespace xmdb::plugin {
using namespace ok::literals;

constexpr ok::StringView PLUG_LOAD_NAME = "xmdb_plugin_load"_sv;
constexpr ok::StringView PLUG_UNLOAD_NAME = "xmdb_plugin_unload"_sv;
constexpr ok::StringView PLUG_INSTALL_NAME = "xmdb_plugin_install"_sv;
constexpr ok::StringView PLUG_GET_LAST_ERROR_NAME = "xmdb_plugin_get_last_error"_sv;

Result<Plugin *, ok::String> Plugin::load(ok::Allocator *allocator,
                                          const NativeLibrary *native_lib) {
    ok::Optional<NativeSymbol> install_sym = native_lib->get_symbol(PLUG_INSTALL_NAME);
    if (!install_sym.has_value()) {
        return ok::String::format(allocator,
                                  "The native library does not export the 'install' hook '" OK_SV_FMT "'",
                                  OK_SV_ARG(PLUG_INSTALL_NAME));
    }

    InstallHook install_hook = install_sym.get().cast<InstallHook>();

    ok::Optional<NativeSymbol> get_last_error_sym = native_lib->get_symbol(PLUG_GET_LAST_ERROR_NAME);
    GetLastErrorHook get_last_error_hook = get_last_error_sym.or_else({}).cast<GetLastErrorHook>();

    ok::Optional<NativeSymbol> load_sym = native_lib->get_symbol(PLUG_LOAD_NAME);
    if (!load_sym) {
        return ok::String::format(allocator,
                                  "The native library does not export the 'load' hook '" OK_SV_FMT "'",
                                  OK_SV_ARG(PLUG_LOAD_NAME));
    }

    LoadHook load_hook = load_sym.get().cast<LoadHook>();

    void *plugin_state = nullptr;
    int ok = load_hook(&plugin_state);
    if (!ok) {
        const char *error = nullptr;
        if (get_last_error_hook != nullptr) error = get_last_error_hook(nullptr);

        if (error != nullptr) {
            return ok::String::format(allocator,
                                      "Failed to call the 'load' hook of the plugin: %s",
                                      error);
        }
    }

    ok::Optional<NativeSymbol> unload_sym = native_lib->get_symbol(PLUG_UNLOAD_NAME);
    UnloadHook unload_hook = unload_sym.or_else({}).cast<UnloadHook>();

    return new (allocator) Plugin{
        allocator,
        native_lib,
        plugin_state,
        unload_hook,
        install_hook,
        get_last_error_hook,
        CapabilitiesList::alloc(allocator),
    };
}

void Plugin::install(ok::StringView name) {
    if (m_installed) return;

    char *cstr_name = sv_to_cstr(ok::temp_allocator(), name);
    const char **caps_names = nullptr;
    void **caps_syms = nullptr;
    int caps_count = 0;

    m_install_hook(m_plugin_state,
                   cstr_name,
                   &caps_names,
                   &caps_syms,
                   &caps_count);

    for (int i = 0; i < caps_count; ++i) {
        ok::StringView cap_name{caps_names[i]};
        NativeSymbol cap_sym{caps_syms[i]};

        m_capabilities.push({cap_name, cap_sym});
    }

    m_installed = true;
}

void Plugin::unload() {
    if (m_unload_hook) {
        m_unload_hook.get()(m_plugin_state);
    }
}

ok::Optional<PluginCapability> Plugin::get_capability(ok::StringView name) const {
    for (UZ cap_idx = 0; cap_idx < m_capabilities.count; ++cap_idx) {
        const ok::Pair<ok::StringView, NativeSymbol> *cap = &m_capabilities[cap_idx];
        if (cap->a == name) return PluginCapability{cap->a, cap->b};
    }

    return ok::Optional<PluginCapability>::empty();
}

ok::Optional<ok::StringView> Plugin::get_last_error() const {
    if (!m_get_last_error_hook) return {};

    GetLastErrorHook gle_hook = m_get_last_error_hook.get();
    const char *error = gle_hook(m_plugin_state);

    if (error != nullptr) return ok::StringView{error};

    return {};
}
} // namespace xmdb::plugin
