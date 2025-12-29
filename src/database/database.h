#pragma once

#include <optional>

#include <SQLiteCpp/SQLiteCpp.h>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

struct User
{
    int id;
    std::string login;
    std::string password;
    std::string firstName;
    std::string lastName;
    bool isOnline;

    auto toJson( bool showPass = false ) const -> nlohmann::json
    {
        nlohmann::json res = {
            {"id", id},
            {"login", login},
            {"password", password},
            {"first_name", firstName},
            {"last_name", lastName},
            {"is_online", isOnline},
        };

        if (!showPass)
        {
            res.erase("password");
        }

        return res;
    }
};

struct Message
{
    int id;
    int userId;
    std::string messageText;
    std::string timestamp;
};

struct MessageJson : public Message
{
    User user;

    auto toJson( void ) const -> nlohmann::json
    {
        return nlohmann::json {
            {"id", id},
            {"user_id", userId},
            {"message_text", messageText},
            {"timestamp", timestamp},
            {"user", user.toJson()}
        };
    }
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
    auto _findToken( const std::string &token ) const -> std::optional<Token>;
  
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
    auto getOnlineUsers( void ) const -> std::vector<User>;
    auto getUserByLogin( const std::string &login ) const -> std::optional<User>;
    auto getUserById( const int id ) const -> std::optional<User>;
    auto getUserByToken( const std::string &token ) const -> std::optional<User>;

    auto sendMessage( const int userId, const std::string &text ) -> Error;
    auto getLastMessages( const int limit ) -> std::vector<MessageJson>;
    auto getMessagesAfter( const int afterId ) -> std::vector<MessageJson>;
    int getMessageCount( void );

    auto isTokenExists( const std::string &token ) -> bool;

    void clear( void );

    ~Database( void );
};