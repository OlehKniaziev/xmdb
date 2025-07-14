#pragma once

#include <Core/ok.hpp>

namespace xmdb::server {
enum class Protocol {
    HTTP,
};

struct Config {
    static Config parse(ok::Slice<char *> args);

    Protocol protocol;
    U16 port;
};
} // namespace xmdb::server
