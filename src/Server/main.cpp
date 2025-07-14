#include <Core/ok.hpp>
#include <Core/DBPool.hpp>
#include <Core/DBConnection.hpp>
#include <Core/Core.hpp>

#include <SQL/global_state.hpp>

#include "http.hpp"

constexpr U16 XMDB_SERVER_PORT = 6969;

int main() {
    xmdb::init_global_state();

    printf("Starting the server on port %u\n", XMDB_SERVER_PORT);
    xmdb::server::run_http_server(XMDB_SERVER_PORT);

    return 0;
}
