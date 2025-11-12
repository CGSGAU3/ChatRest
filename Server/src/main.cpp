#include "server.h"
#include "database.h"

int main( int argc, char *argv[] )
{
    try
    {
        SQLite::Database db("test_tt.db", SQLite::OPEN_CREATE | SQLite::OPEN_READWRITE);

        db.exec(R"(
                CREATE TABLE IF NOT EXISTS times (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                data TEXT NOT NULL,
                timestamp DATETIME DEFAULT CURRENT_TIMESTAMP))");

        db.exec(R"(INSERT INTO times (data) VALUES ("Yo bro nice hat!"))");

        SQLite::Statement query(db, "SELECT * FROM times");
        
        while (query.executeStep())
        {
            std::cout << query.getColumn("id") << std::endl;
            std::cout << query.getColumn("timestamp").getString() << std::endl;
            std::cout << query.getColumn("data") << std::endl;
        }

        return EXIT_SUCCESS;
    }
    catch ( const std::exception &e )
    {
        spdlog::error(e.what());
        return EXIT_FAILURE;
    }
}