// Catch2 test runner entry point.
// Defines CATCH_CONFIG_RUNNER so we can call Catch::Session ourselves,
// but the amalgamated single-header approach is used here for simplicity.
#include "catch_amalgamated.hpp"

int main(int argc, char* argv[]) {
    return Catch::Session().run(argc, argv);
}
