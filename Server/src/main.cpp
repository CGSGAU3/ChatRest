#include "server.h"

int main( int argc, char *argv[] )
{
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