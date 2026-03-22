#include "PluginManager.hpp"

namespace xmdb::plugin {
PluginManager::PluginManager(ok::Allocator *allocator) : m_allocator{allocator},
                                                         m_loaded_plugins{ok::Table<ok::StringView, Plugin *>::alloc(allocator)} {
}

Result<Plugin *, ok::String> PluginManager::get_or_load(ok::StringView path) {
    if (auto plugin_opt = m_loaded_plugins.get(path)) {
        return plugin_opt.get();
    }

    Result<NativeLibrary, ok::String> lib_res = NativeLibrary::load(m_allocator, path);
    if (!lib_res.ok()) {
        return ok::String::format(m_allocator,
                                  "Failed to load a native library from '" OK_SV_FMT "': %s",
                                  OK_SV_ARG(path),
                                  lib_res.error().cstr());
    }

    NativeLibrary &lib = lib_res.unwrap();

    Result<Plugin *, ok::String> plug_res = Plugin::load(m_allocator, &lib);
    if (!plug_res.ok()) {
        return ok::String::format(m_allocator,
                                  "Failed to load a plugin from a native library '" OK_SV_FMT "': %s",
                                  OK_SV_ARG(path),
                                  plug_res.error().cstr());
    }

    Plugin *plug = plug_res.unwrap();
    m_loaded_plugins.put(path, plug);
    return plug;
}

PluginManager::~PluginManager() {
    OK_TABLE_FOREACH(m_loaded_plugins, _, plug, {
        plug->unload();
    });

    m_loaded_plugins.dealloc();
}
} // namespace xmdb::plugin
