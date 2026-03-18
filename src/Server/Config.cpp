#include "Config.hpp"

#include <Core/util.hpp>
#include <ArgParser/ArgParser.hpp>

namespace xmdb::server {
using namespace ok::literals;

Config Config::parse(int argc, char **argv) {
    xmdb::argparser::ArgParser arg_parser{argc, argv};

    constexpr const char *default_protocol = "http";
    S64 default_port = 6969;

    const char *protocol_str{};
    S64 port_s64{};

    arg_parser.string("protocol", &protocol_str, "Network protocol to use", default_protocol);
    arg_parser.integer("port", &port_s64, "Port to start the server on", default_port);

    if (!arg_parser.parse()) {
        arg_parser.help();
        ok::StringView error_message = arg_parser.error_message();
        dief("Failed to parse command line arguments: " OK_SV_FMT, OK_SV_ARG(error_message));
    }

    Protocol protocol{};

    if (strcmp(protocol_str, "http") == 0) {
        protocol = Protocol::HTTP;
    } else {
        dief("Invalid protocol '%s'", protocol_str);
    }

    if (port_s64 < 0 || port_s64 > UINT16_MAX) {
        dief("Port value out of range");
    }

    return Config {
        .protocol = protocol,
        .port = (U16) port_s64,
    };
}
} // namespace xmbd::server
