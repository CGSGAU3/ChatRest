#pragma once

#include <optional>

#include <SQLiteCpp/SQLiteCpp.h>
#include <spdlog/spdlog.h>

struct User final
{
    int id;
    std::string login;
    std::string password;
    std::string firstName;
    std::string lastName;
    bool isOnline;
};

struct Message final
{
    int id;
    int userId;
    std::string messageText;
    std::string timestamp;
};

struct Token final
{
    int id;
    int userId;
    std::string token;
};

class Database final
{
private:
    SQLite::Database _db;
  
public:
    struct Error final
    {
        bool isError;
        std::string message;

        Error( bool errorFlag = false, const std::string &msg = "" ) : 
            isError(errorFlag), message(msg) {}

        operator bool( void ) const
        {
            return isError;
        }
    };

    Database( const std::string &name = "a.db" );

    auto addUser( const User &user ) -> Error;
    auto getAllUsers( void ) const -> std::vector<User>;
    auto getUserByLogin( const std::string &login ) const -> std::optional<User>;
    auto getUserById( const int id ) const -> std::optional<User>;
};