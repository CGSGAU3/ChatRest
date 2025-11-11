#include "server.h"
#include "database.h"

int main( int argc, char *argv[] )
{
    try
    {
        Database db("test.db");

        return EXIT_SUCCESS;
    }
    catch ( const std::exception &e )
    {
        spdlog::error(e.what());
        return EXIT_FAILURE;
    }
}