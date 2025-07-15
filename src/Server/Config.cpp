#include "Config.hpp"
#include <Core/util.hpp>

namespace xmdb::server {
constexpr const Protocol XMDB_DEFAULT_PROTOCOL = Protocol::HTTP;
constexpr const U16 XMDB_DEFAULT_PORT = 6969;

using namespace ok::literals;

Config Config::parse(ok::Slice<char *> args) {
    // char *program_name = args[0];
    args = args.slice(1);

    Protocol protocol = XMDB_DEFAULT_PROTOCOL;
    U16 port = XMDB_DEFAULT_PORT;

    for (UZ i = 0; i < args.count; ++i) {
        ok::StringView arg{args[i]};

        if (arg == "-protocol"_sv) {
            ++i;
            if (i >= args.count) {
                dief("No value provided for argument '-protocol'");
            }

            ok::StringView protocol_sv{args[i]};
            if (protocol_sv == "http"_sv) {
                protocol = Protocol::HTTP;
            } else {
                dief("Invalid protocol '" OK_SV_FMT "'", OK_SV_ARG(protocol_sv));
            }
        } else if (arg == "-port"_sv) {
            ++i;
            if (i >= args.count) {
                dief("No value provided for argument '-protocol'");
            }

            S64 port_s64;
            ok::StringView port_sv{args[i]};
            if (!ok::parse_int64(port_sv, &port_s64)) {
                dief("Invalid port value '" OK_SV_FMT "'", OK_SV_ARG(port_sv));
            }

            if (port_s64 < 0 || port_s64 > UINT16_MAX) {
                dief("Port value out of range");
            }

            port = (U16)port_s64;
        } else {
            dief("Unknown argument '" OK_SV_FMT "'", OK_SV_ARG(arg));
        }
    }

    return Config {
        .protocol = protocol,
        .port = port,
    };
}
} // namespace xmbd::server
