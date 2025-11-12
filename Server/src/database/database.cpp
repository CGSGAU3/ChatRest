#include <sqlite3.h>

#include "database.h"
#include "sha256.h"

Database::Database( const std::string &name ) : 
    _db(name, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE), opened(true)
{
    try
    {
        _db.exec(R"(
                CREATE TABLE IF NOT EXISTS users (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                login TEXT UNIQUE NOT NULL,
                password TEXT NOT NULL,
                first_name TEXT NOT NULL,
                last_name TEXT NOT NULL,
                is_online BOOLEAN
            ))");

        _db.exec(R"(
                CREATE TABLE IF NOT EXISTS messages (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                user_id INTEGER NOT NULL,
                message_text TEXT NOT NULL,
                timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
                FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
            ))");

        spdlog::trace("Database and tables are created or opened successfully!");
    } 
    catch ( const std::exception &e )
    {
        spdlog::error(std::string("Create SQLite error: ") + e.what());
        opened = false;
    }
}

auto Database::addUser( const User &user ) -> Error
{
    Error err;

    try
    {
        SQLite::Statement query(_db, R"(
            INSERT INTO users (login, password, first_name, last_name, is_online) VALUES (?, ?, ?, ?, ?)
        )");

        query.bind(1, user.login);
        query.bind(2, SHA256(user.password));
        query.bind(3, user.firstName);
        query.bind(4, user.lastName);
        query.bind(5, false);

        query.exec();
        spdlog::info(std::string("User with login '") + user.login + "' has just registered!");
    }
    catch ( const SQLite::Exception &e )
    {
        err.isError = true;
        if (e.getErrorCode() == SQLITE_CONSTRAINT_UNIQUE)
        {
            err.message = "Error: login '" + user.login + "' has already taken!";
        }
        else
        {
            err.message = std::string("Error: ") + e.what();
        }

        spdlog::warn(err.message);
    }
    catch ( const std::exception &e )
    {
        err.isError = true;
        err.message = e.what();

        spdlog::warn(e.what());
    }

    return err;
}

auto Database::getAllUsers( void ) const -> std::vector<User>
{
    std::vector<User> users;
    SQLite::Statement query(_db, "SELECT * FROM users");
        
    while (query.executeStep())
    {
        User user;

        user.id = query.getColumn("id");
        user.login = query.getColumn("login").getString();
        user.password = query.getColumn("password").getString();
        user.firstName = query.getColumn("first_name").getString();
        user.lastName = query.getColumn("last_name").getString();
        user.isOnline = (int)query.getColumn("is_online");

        users.emplace_back(user);
    }

    return users;
}

auto Database::getUserByLogin( const std::string &login ) const -> std::optional<User>
{
    try
    {
        SQLite::Statement query(_db, "SELECT * FROM users WHERE login = ?");

        query.bind(1, login);
        if (query.executeStep())
        {
            User user;

            user.id = query.getColumn("id");
            user.login = query.getColumn("login").getString();
            user.password = query.getColumn("password").getString();
            user.firstName = query.getColumn("first_name").getString();
            user.lastName = query.getColumn("last_name").getString();
            user.isOnline = (int)query.getColumn("is_online");
            
            return user;
        }        
    } catch (const std::exception& e) {
        spdlog::warn(std::string("Error finding user by login: ") + e.what());
    }
    
    return std::nullopt;
}

auto Database::getUserById( const int id ) const -> std::optional<User>
{
    try
    {
        SQLite::Statement query(_db, "SELECT * FROM users WHERE id = ?");

        query.bind(1, id);
        if (query.executeStep())
        {
            User user;

            user.id = query.getColumn("id");
            user.login = query.getColumn("login").getString();
            user.password = query.getColumn("password").getString();
            user.firstName = query.getColumn("first_name").getString();
            user.lastName = query.getColumn("last_name").getString();
            user.isOnline = (int)query.getColumn("is_online");
            
            return user;
        }        
    } catch (const std::exception& e) {
        spdlog::warn(std::string("Error finding user by login: ") + e.what());
    }
    
    return std::nullopt; 
}