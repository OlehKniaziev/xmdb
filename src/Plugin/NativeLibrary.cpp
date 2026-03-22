#include "NativeLibrary.hpp"

#include "util.hpp"

#if OK_UNIX
#include <dlfcn.h>
#endif // OK_UNIX

namespace xmdb::plugin {
#if OK_UNIX
Result<NativeLibrary, ok::String> NativeLibrary::load(ok::Allocator *allocator,
                                                      ok::StringView path) {
    char *cstr_path = sv_to_cstr(ok::temp_allocator(), path);

    void *dl_handle = dlopen(cstr_path, RTLD_LAZY);
    if (dl_handle == nullptr) {
        const char *error = dlerror();
        return ok::String::format(allocator,
                                  "Failed to load a native library: %s",
                                  error);
    }

    return NativeLibrary{dl_handle, path};
}

ok::Optional<NativeSymbol> NativeLibrary::get_symbol(ok::StringView name) const {
    char *cstr_name = sv_to_cstr(ok::temp_allocator(), name);

    void *sym = dlsym(m_state, cstr_name);
    if (sym != nullptr) {
        return NativeSymbol{sym};
    } else {
        return ok::Optional<NativeSymbol>::empty();
    }
}
#endif // OK_UNIX
} // namespace xmdb::plugin
