#include "server.h"

int main( int argc, char *argv[] )
{
    spdlog::set_level(spdlog::level::trace);

    try
    {
        Server server("localhost", 8080, "chat.db");

        server.run();
        return EXIT_SUCCESS;
    }
    catch ( const std::exception &e )
    {
        spdlog::critical(e.what());
        return EXIT_FAILURE;
    }
}