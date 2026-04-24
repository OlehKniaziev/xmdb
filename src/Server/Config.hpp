#pragma once

#include <Core/ok.hpp>

namespace xmdb::server
{
enum class Protocol
{
    HTTP,
};

struct Config
{
    static Config parse(int argc, char **argv);

    Protocol protocol;
    U16 port;
};
} // namespace xmdb::server
