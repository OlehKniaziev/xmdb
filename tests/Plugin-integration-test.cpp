// Self-contained plugin integration test. This file is compiled twice:
//   - As PluginIntegrationTestPlugin.so (XMDB_BUILDING_TEST_PLUGIN defined): exports
//     the xmdb_plugin_* hooks so it can be loaded as a plugin.
//   - As the Plugin-integration-test executable: GTest test executable that loads the
//     shared library and tests the Plugin workflow.

constexpr const char *SAY_HELLO_RESULT = "hello from test plugin";

#ifdef XMDB_BUILDING_TEST_PLUGIN

struct TestPluginState {
    bool loaded;
};

static const char *say_hello(void *state) {
    (void)state;
    return SAY_HELLO_RESULT;
}

static int add(void *state, int a, int b) {
    (void)state;
    return a + b;
}

static int was_loaded(void *state) {
    return reinterpret_cast<TestPluginState *>(state)->loaded;
}

static const char *g_cap_names[] = {"say_hello", "add", "was_loaded"};
static void *g_cap_syms[] = {reinterpret_cast<void *>(say_hello),
                             reinterpret_cast<void *>(add),
                             reinterpret_cast<void *>(was_loaded)};

extern "C" {

int xmdb_plugin_load(void **state_out) {
    *state_out = new TestPluginState{true};
    return 1;
}

void xmdb_plugin_unload(void *state) {
    delete static_cast<TestPluginState *>(state);
}

void xmdb_plugin_install(void *state,
                         const char *name,
                         const char ***cap_names_out,
                         void ***cap_syms_out,
                         int *cap_count_out) {
    (void)state;
    (void)name;
    *cap_names_out = g_cap_names;
    *cap_syms_out = g_cap_syms;
    *cap_count_out = sizeof(g_cap_syms) / sizeof(g_cap_syms[0]);
}

const char *xmdb_plugin_get_last_error(void *state) {
    (void)state;
    return nullptr;
}

} // extern "C"

#else // GTest runner

#include <gtest/gtest.h>

#include <Core/ok.hpp>

#include <Plugin/NativeLibrary.hpp>
#include <Plugin/Plugin.hpp>
#include <Plugin/PluginManager.hpp>

using namespace ok::literals;
using namespace xmdb;
using namespace xmdb::plugin;

static constexpr const char *PLUGIN_PATH = XMDB_TEST_PLUGIN_PATH;

class PluginIntegrationFixture : public ::testing::Test {
protected:
    ok::ArenaAllocator arena{};
    PluginManager manager{&arena};
};

TEST_F(PluginIntegrationFixture, load_plugin_succeeds) {
    Result<Plugin *, ok::String> result = manager.get_or_load(ok::StringView{PLUGIN_PATH});
    ASSERT_TRUE(result.ok()) << result.error().cstr();
    ASSERT_NE(result.unwrap(), nullptr);

    Plugin *plug = result.unwrap();

    plug->install("test-plugin"_sv);

    PluginCapability was_loaded_cap = plug->get_capability("was_loaded"_sv).get();
    int was_loaded = plug->use_capability<int>(was_loaded_cap);
    ASSERT_TRUE(was_loaded);
}

TEST_F(PluginIntegrationFixture, load_same_plugin_twice_returns_same_instance) {
    Result<Plugin *, ok::String> first = manager.get_or_load(ok::StringView{PLUGIN_PATH});
    ASSERT_TRUE(first.ok()) << first.error().cstr();

    Result<Plugin *, ok::String> second = manager.get_or_load(ok::StringView{PLUGIN_PATH});
    ASSERT_TRUE(second.ok()) << second.error().cstr();

    ASSERT_EQ(first.unwrap(), second.unwrap());
}

TEST_F(PluginIntegrationFixture, install_hook_registers_capabilities) {
    Result<Plugin *, ok::String> result = manager.get_or_load(ok::StringView{PLUGIN_PATH});
    ASSERT_TRUE(result.ok()) << result.error().cstr();

    Plugin *plugin = result.unwrap();
    plugin->install("test-plugin"_sv);

    ASSERT_TRUE(plugin->get_capability("say_hello"_sv).has_value());
    ASSERT_TRUE(plugin->get_capability("add"_sv).has_value());
}

TEST_F(PluginIntegrationFixture, missing_capability_returns_empty) {
    Result<Plugin *, ok::String> result = manager.get_or_load(ok::StringView{PLUGIN_PATH});
    ASSERT_TRUE(result.ok()) << result.error().cstr();

    Plugin *plugin = result.unwrap();
    plugin->install("test-plugin"_sv);

    ASSERT_FALSE(plugin->get_capability("nonexistent"_sv).has_value());
}

TEST_F(PluginIntegrationFixture, capabilities_are_callable) {
    Result<Plugin *, ok::String> result = manager.get_or_load(ok::StringView{PLUGIN_PATH});
    ASSERT_TRUE(result.ok()) << result.error().cstr();

    Plugin *plugin = result.unwrap();
    plugin->install("test-plugin"_sv);

    ok::Optional<PluginCapability> say_hello_cap = plugin->get_capability("say_hello"_sv);
    ASSERT_TRUE(say_hello_cap.has_value());
    const char *say_hello_res = plugin->use_capability<const char *>(say_hello_cap.get());
    ASSERT_STREQ(say_hello_res, SAY_HELLO_RESULT);

    ok::Optional<PluginCapability> add_cap = plugin->get_capability("add"_sv);
    ASSERT_TRUE(add_cap.has_value());
    int add_res = plugin->use_capability<int>(add_cap.get(), 3, 4);
    ASSERT_EQ(add_res, 7);
}

TEST_F(PluginIntegrationFixture, install_is_idempotent) {
    Result<Plugin *, ok::String> result = manager.get_or_load(ok::StringView{PLUGIN_PATH});
    ASSERT_TRUE(result.ok()) << result.error().cstr();

    Plugin *plugin = result.unwrap();
    plugin->install("test-plugin"_sv);
    plugin->install("test-plugin"_sv);

    ASSERT_TRUE(plugin->get_capability("say_hello"_sv).has_value());
}

#endif // XMDB_BUILDING_TEST_PLUGIN
