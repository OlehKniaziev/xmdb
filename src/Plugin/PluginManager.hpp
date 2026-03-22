#pragma once

#include <Core/ok.hpp>

#include "Plugin.hpp"

namespace xmdb::plugin {
class PluginManager {
public:
    explicit PluginManager(ok::Allocator *allocator);

    /**
     * @brief Tries to load a plugin from a file.
     * @param path The file path of a plugin.
     * @return The plugin pointer if the plugin was already loaded,
     * or on a sucessfull load, an error string otherwise.
     */
    Result<Plugin *, ok::String> get_or_load(ok::StringView path);

    void unload(Plugin *plugin);

    ~PluginManager();

private:
    ok::Allocator *m_allocator;
    ok::Table<ok::StringView, Plugin *> m_loaded_plugins;
};
} // namespace xmdb::plugin
