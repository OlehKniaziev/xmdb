#include <Core/ok.hpp>
#include <Core/DBPool.hpp>
#include <Core/DBConnection.hpp>
#include <Core/Core.hpp>
#include <Core/Logger.hpp>

#include <SQL/global_state.hpp>

#include "http.hpp"
#include "Config.hpp"

using namespace xmdb::server;

int main(int argc, char **argv) {
    xmdb::SQL::init_global_state();

    Config config = Config::parse(argc, argv);

    switch (config.protocol) {
    case Protocol::HTTP: {
        xmdb::log::info("Starting the server on port %u", config.port);
        run_http_server(config.port);
        break;
    }
    }

    return 0;
}
