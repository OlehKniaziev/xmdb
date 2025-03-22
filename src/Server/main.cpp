#include <Core/ok.hpp>
#include <Core/BTree.hpp>
#include <SQL/global_state.hpp>

int main() {
    xmdb::init_global_state();

    ok::println("Hello, world");

    return 0;
}
