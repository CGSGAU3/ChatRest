#include <random>

#include <sqlite3.h>

#include "database.h"
#include "sha256.h"

Database::Database( const std::string &name ) : 
    _db(name, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE)
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

        _db.exec("UPDATE users SET is_online = false");

        _db.exec(R"(
                CREATE TABLE IF NOT EXISTS messages (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                user_id INTEGER NOT NULL,
                message_text TEXT NOT NULL,
                timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
                FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
            ))");

        _db.exec(R"(DROP TABLE IF EXISTS auth_tokens)");

        _db.exec(R"(
                CREATE TABLE IF NOT EXISTS auth_tokens (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                user_id INTEGER NOT NULL,
                token TEXT UNIQUE NOT NULL,
                FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
            ))");

        spdlog::trace("Database and tables are created or opened successfully!");
    } 
    catch ( const std::exception &e )
    {
        spdlog::error(std::string("Create SQLite error: ") + e.what());
    }
}

auto Database::generateToken( const int len ) -> std::string
{
    const std::string characters = 
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<> distribution(0, characters.size() - 1);
    
    std::string token;

    for (int i = 0; i < len; i++) 
    {
        token += characters[distribution(generator)];
    }

    return token;
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
        err = true;
        if (e.getErrorCode() == SQLITE_CONSTRAINT_UNIQUE)
        {
            err.message = "Login '" + user.login + "' has already taken!";
        }
        else
        {
            err.message = std::string("Error: ") + e.what();
        }
    }
    catch ( const std::exception &e )
    {
        err = true;
        err.message = e.what();
    }

    return err;
}

auto Database::_findToken( const std::string &token ) const -> std::optional<Token>
{
    try
    {
        SQLite::Statement query(_db, R"(
            SELECT * FROM auth_tokens WHERE token = ?
        )");

        query.bind(1, token);

        if (query.executeStep())
        {
            Token tok;

            tok.id = query.getColumn("id");
            tok.userId = query.getColumn("user_id");
            tok.token = query.getColumn("token").getString();

            return tok;
        }
    }
    catch ( const std::exception &e )
    {
        spdlog::error(std::string("Error while find token: ") + e.what());
    }

    return std::nullopt;
}

auto Database::isTokenExists( const std::string &token ) -> bool
{
    return static_cast<bool>(_findToken(SHA256(token)));
}

auto Database::_addToken( const Token &token ) -> Error
{
    Error err;

    try
    {
        SQLite::Statement query(_db, R"(
            INSERT INTO auth_tokens (user_id, token) VALUES (?, ?)
        )");

        query.bind(1, token.userId);
        query.bind(2, SHA256(token.token));

        query.exec();
    }
    catch ( const std::exception &e )
    {
        err = true;
        err.message = e.what();
    }

    return err;
}

auto Database::loginUser( const std::string &login, const std::string &password ) -> std::pair<Token, Error>
{
    auto user = getUserByLogin(login);
    Error err;

    // Find user
    if (!user)
    {
        err = true;
        err.message = "Such user does not exist!";
        err.errorId = 400;
        return {{}, err};
    }

    // Check password
    if (SHA256(password) != user.value().password)
    {
        err = true;
        err.message = "Incorrect password!";
        err.errorId = 401;
        return {{}, err};
    }

    // Add auth token to database
    Token token;

    token.userId = user.value().id;

    do
    {
        token.token = generateToken(64);
    } while (_findToken(SHA256(token.token)));

    err = _addToken(token);

    if (err)
    {
        err.errorId = 400;
        return {{}, err};
    }

    // Set user online to 1
    try
    {
        SQLite::Statement query(_db, R"(
            UPDATE users set is_online = true WHERE id = ?
        )");

        query.bind(1, user.value().id);

        query.exec();
    }
    catch ( const std::exception &e )
    {
        err = true;
        err.message = e.what();
        err.errorId = 500;
        return {{}, err};
    }

    spdlog::info("User with login " + user.value().login + " has just signed in!");
    return {token, err};
}

auto Database::logoutUser( const std::string &token ) -> Error
{
    auto tokOpt = _findToken(SHA256(token));
    Error err;

    if (!tokOpt)
    {
        err = true;
        err.message = "Unknown auth token!";
        err.errorId = 401;
        return err;
    }

    Token tok = tokOpt.value();

    try
    {
        // Set user online to 0
        SQLite::Statement query(_db, R"(
            UPDATE users set is_online = false WHERE id = ?
        )");

        query.bind(1, tok.userId);
        query.exec();

        // Remove token from base
        query = SQLite::Statement(_db, R"(
            DELETE FROM auth_tokens WHERE token = ?
        )");

        query.bind(1, tok.token);
    }
    catch ( const std::exception &e )
    {
        err = true;
        err.message = e.what();
        err.errorId = 500;
        return err;
    }

    spdlog::info("User with id " + std::to_string(tok.userId) + " has just signed out!");
    return err;
}

auto Database::getUserByToken( const std::string &token ) const -> std::optional<User>
{
    auto tokOpt = _findToken(SHA256(token));
    Error err;

    if (!tokOpt)
    {
        spdlog::warn("Token " + token + " does not exist!");
        return std::nullopt;
    }

    Token tok = tokOpt.value();
    return getUserById(tok.userId);
}

auto Database::getAllUsers( void ) const -> std::vector<User>
{
    std::vector<User> users;

    try
    {
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
    }
    catch ( const std::exception &e )
    {
        spdlog::error(std::string("Error while get all users: ") + e.what());
    }

    return users;
}

auto Database::getOnlineUsers( void ) const -> std::vector<User>
{
    std::vector<User> users;

    try
    {
        SQLite::Statement query(_db, "SELECT * FROM users WHERE is_online = true");
            
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
    }
    catch ( const std::exception &e )
    {
        spdlog::error(std::string("Error while get all users: ") + e.what());
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
    } catch ( const std::exception &e )
    {
        spdlog::error(std::string("Error finding user by login: ") + e.what());
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
    } catch ( const std::exception &e )
    {
        spdlog::error(std::string("Error finding user by id: ") + e.what());
    }
    
    return std::nullopt; 
}

auto Database::sendMessage( const int userId, const std::string &text ) -> Error
{
    Error err;

    try
    {
        SQLite::Statement query(_db, R"(
            INSERT INTO messages (user_id, message_text) VALUES (?, ?)
        )");

        query.bind(1, userId);
        query.bind(2, text);

        query.exec();
    }
    catch ( const std::exception &e )
    {
        err = true;
        err.errorId = 500;
        err.message = e.what();
        spdlog::error(e.what());
    }

    return err;
}

auto Database::getLastMessages( const int limit ) -> std::vector<MessageJson>
{
    std::vector<MessageJson> messages;
    
    try
    {
        SQLite::Statement query(_db, R"(
            SELECT m.*, u.login as login, u.is_online as is_online,
            u.first_name as first_name, u.last_name as last_name
            FROM messages m 
            LEFT JOIN users u ON m.user_id = u.id 
            ORDER BY m.timestamp ASC 
            LIMIT ?
        )");
        query.bind(1, limit);
        
        while (query.executeStep())
        {
            MessageJson msg;

            msg.id = query.getColumn("id").getInt();
            msg.userId = query.getColumn("user_id").getInt();
            msg.messageText = query.getColumn("message_text").getString();

            msg.user.id = msg.userId;
            msg.user.login = query.getColumn("login").getString();
            msg.user.firstName = query.getColumn("first_name").getString();
            msg.user.lastName = query.getColumn("last_name").getString();
            msg.user.isOnline = (int)query.getColumn("is_online");

            msg.timestamp = query.getColumn("timestamp").getString();
            
            messages.push_back(msg);
        }
        
    }
    catch ( const std::exception &e )
    {
        spdlog::error(std::string("Error getting messages: ") + e.what());
        return {};
    }
    
    return messages;
}

int Database::getMessageCount( void )
{
    try
    {
        SQLite::Statement query(_db, "SELECT COUNT(*) FROM messages");

        if (query.executeStep())
        {
            return query.getColumn(0).getInt();
        }
    }
    catch ( const std::exception &e )
    {
        spdlog::error(e.what());
    }
    
    return -1;
}

Database::~Database( void )
{
    try
    {
        _db.exec(R"(DROP TABLE IF EXISTS auth_tokens CASCADE)");
        spdlog::trace("All the authorization tokens were reset!");
    }
    catch ( const std::exception &e )
    {
        spdlog::error(e.what());
    }
}