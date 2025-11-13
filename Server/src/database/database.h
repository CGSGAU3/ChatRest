#pragma once

#include <optional>

#include <SQLiteCpp/SQLiteCpp.h>
#include <spdlog/spdlog.h>

struct User
{
    int id;
    std::string login;
    std::string password;
    std::string firstName;
    std::string lastName;
    bool isOnline;
};

struct Message
{
    int id;
    int userId;
    std::string messageText;
    std::string timestamp;
};

struct Token
{
    int id;
    int userId;
    std::string token;
};

class Database final
{
private:
    SQLite::Database _db;

    struct Error;

    static auto generateToken( const int len = 32 ) -> std::string;
    auto _addToken( const Token &token ) -> Error;
    auto _findToken( const std::string &token ) -> std::optional<Token>;
  
public:
    struct Error
    {
        bool isError {};
        std::string message;
        int errorId {};

        Error( bool errorFlag = false, const std::string &msg = "", const int id = 0 ) : 
            isError(errorFlag), message(msg), errorId(id) {}

        operator bool( void ) const
        {
            return isError;
        }

        Error & operator =( const bool val )
        {
            isError = val;
            return *this;
        }
    };

    Database( const std::string &name = "a.db" );

    auto addUser( const User &user ) -> Error;
    auto loginUser( const std::string &login, const std::string &password ) -> std::pair<Token, Error>;
    auto logoutUser( const std::string &token ) -> Error;
    auto getAllUsers( void ) const -> std::vector<User>;
    auto getUserByLogin( const std::string &login ) const -> std::optional<User>;
    auto getUserById( const int id ) const -> std::optional<User>;

    ~Database( void );
};