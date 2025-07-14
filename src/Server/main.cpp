#include <Core/ok.hpp>
#include <Core/DBPool.hpp>
#include <Core/DBConnection.hpp>
#include <Core/Core.hpp>

#include <SQL/global_state.hpp>

#include "http.hpp"
#include "Config.hpp"

using namespace xmdb::server;

int main(int argc, char **argv) {
    xmdb::init_global_state();

    ok::Slice<char *> args = {argv, (UZ)argc};

    Config config = Config::parse(args);

    switch (config.protocol) {
    case Protocol::HTTP: {
        printf("Starting the server on port %u\n", config.port);
        run_http_server(config.port);
        break;
    }
    }

    return 0;
}
