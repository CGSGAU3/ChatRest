#include <chrono>
#include <format>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iterator>
#include <regex>

#include <spdlog/spdlog.h>

#include "server.h"
#include "response_converter.h"
#include "response_error_builder.h"

Server::Server( const std::string &host, const int port, const std::string &dbName ) :
    _db(dbName), _server(nullptr), _host(host), _port(port) {}

auto Server::readFile( const std::string &filename ) -> std::string
{
    std::filesystem::path fullPath = __FILE__;

    fullPath = fullPath.parent_path().parent_path();
    fullPath += "/public/" + filename;

    std::ifstream file(fullPath);

    if (!file)
    {
        return ""; 
    }
    std::string content(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );

    return content;
}

void Server::run( void )
{
    if (_server)
    {
        spdlog::warn("Server is already existed!");
        return;
    }

    _server = std::make_unique<httplib::Server>();

    spdlog::info("Running server on " + _host + ":" + std::to_string(_port) + "...");

    httplib::Headers corsHeaders = {
        {"Access-Control-Allow-Origin", "*"},
        {"Access-Control-Allow-Methods", "*"},
        {"Access-Control-Allow-Headers", "*"}};

    _server->set_default_headers(corsHeaders);

    _setupHandlers();
    _setupStaticHandlers();

    _startedAt = getCurrentTimestamp();

    if (!_server->listen(_host, _port)) {
        throw std::runtime_error("Server run error!");
    }
}

auto Server::getCurrentTimestamp( void ) -> std::string
{
    auto now = std::chrono::system_clock::now();
    auto localNow = std::chrono::current_zone()->to_local(now);
    auto localSeconds = std::chrono::floor<std::chrono::seconds>(localNow);

    return std::format(R"({:%Y-%m-%d %H:%M:%S})", localSeconds);
}

void Server::_handleAlive( const Request &req, Response &res )
{
    Json status = {
        {"status", "online"},
        {"started_at", _startedAt},
        {"timestamp", getCurrentTimestamp()}
    };

    res.status = StatusCode::OK_200;
    res.set_content(status.dump(), "application/json");
}

void Server::_handleRegister( const Request &req, Response &res )
{
    try
    {
        auto inputUser = Json::parse(req.body);
        User user {};

        user.login = inputUser["login"];
        user.password = inputUser["password"];
        user.firstName = inputUser["first_name"];
        user.lastName = inputUser["last_name"];

        std::regex loginReg("^[a-zA-Z0-9_]{3,20}$");

        if (!std::regex_match(user.login, loginReg) || user.firstName.length() < 2)
        {
            ErrorResponseBuilder(res).badRequest("Login, first and last name should be normal!");
            spdlog::warn("Incorrect register arguments!");
            return;
        }

        auto err = _db.addUser(user);

        if (err)
        {
            ErrorResponseBuilder(res).badRequest(err.message);
            spdlog::warn(err.message);
            return;
        }

        Json status = {
            {"status", "success"}
        };

        res.status = StatusCode::OK_200;
        res.set_content(status.dump(), "application/json");
    }
    catch ( const std::exception &e )
    {
        spdlog::warn(std::string(__FILE__) + ":" + std::to_string(__LINE__) + ": " + e.what());
        ErrorResponseBuilder(res).badRequest("Error while parsing user!");
    }
}

void Server::processErrors( Response &res, const Database::Error &err )
{
    if (!err)
    {
        return;
    }

    switch (static_cast<StatusCode>(err.errorId))
    {
    case StatusCode::BadRequest_400:
        ErrorResponseBuilder(res).badRequest(err.message);
        break;
    case StatusCode::Unauthorized_401:
        ErrorResponseBuilder(res).unauthorized(err.message);
        break;
    case StatusCode::InternalServerError_500:
    default:
        ErrorResponseBuilder(res).internal(err.message);
        break;
    }
    spdlog::warn(err.message);
}

void Server::_handleLogin( const Request &req, Response &res )
{
    try
    {
        auto input = Json::parse(req.body);
        std::string login = input["login"];
        std::string password = input["password"];

        auto [token, err] = _db.loginUser(login, password);

        if (err)
        {
            processErrors(res, err);
            return;
        }

        Json payload = {
            {"status", "success"},
            {"auth_token", token.token}
        };

        res.status = StatusCode::OK_200;
        res.set_content(payload.dump(), "application/json");
    }
    catch ( const std::exception &e )
    {
        spdlog::warn(std::string(__FILE__) + ":" + std::to_string(__LINE__) + ": " + e.what());
        ErrorResponseBuilder(res).badRequest("Error while parsing user!");
    }
}

void Server::_handleLogout( const Request &req, Response &res )
{
    const std::string token = getAuthorizationToken(req);
    auto err = _db.logoutUser(token);

    if (err)
    {
        processErrors(res, err);
        return;
    }

    Json status = {
        {"status", "success"}
    };

    res.status = StatusCode::OK_200;
    res.set_content(status.dump(), "application/json");
}

void Server::_handleMe( const Request &req, Response &res )
{
    const std::string token = getAuthorizationToken(req);
    auto userOpt = _db.getUserByToken(token);

    if (!userOpt)
    {
        ErrorResponseBuilder(res).unauthorized("Unknown token!");
        spdlog::warn("Unknown token '" + token + "'!");
        return;
    }

    Json user = {
        {"login", userOpt.value().login},
        {"first_name", userOpt.value().firstName},
        {"last_name", userOpt.value().lastName},
    };

    res.status = StatusCode::OK_200;
    res.set_content(user.dump(), "application/json");
}

void Server::_handleOnline( const Request &req, Response &res )
{
    const std::string token = getAuthorizationToken(req);

    if (!_db.isTokenExists(token))
    {
        ErrorResponseBuilder(res).unauthorized("Unknown token!");
        spdlog::warn("Unknown token '" + token + "'!");
        return;
    }

    auto users = _db.getOnlineUsers();
    Json usersJsons = Json::array();

    std::transform(users.begin(), users.end(), std::back_inserter(usersJsons), 
                   []( const User &user ) {return user.toJson();});

    Json usersOnline = {
        {"total_online", users.size()},
        {"online_users", usersJsons}
    };

    res.status = StatusCode::OK_200;
    res.set_content(usersOnline.dump(), "applciation/json");
}

void Server::_handleUsersCount( const Request &req, Response &res )
{
    const std::string token = getAuthorizationToken(req);

    if (!_db.isTokenExists(token))
    {
        ErrorResponseBuilder(res).unauthorized("Unknown token!");
        spdlog::warn("Unknown token '" + token + "'!");
        return;
    }

    int count = _db.getAllUsers().size();

    Json countResp = {
        {"status", "success"},
        {"count", count}
    };

    res.status = StatusCode::OK_200;
    res.set_content(countResp.dump(), "application/json");
}

auto Server::getAuthorizationToken( const Request &req ) -> std::string
{
    return req.get_header_value("Authorization-Token");
}

void Server::_handleMessagesPost( const Request &req, Response &res )
{
    const std::string token = getAuthorizationToken(req);
    auto userOpt = _db.getUserByToken(token);

    if (!userOpt)
    {
        ErrorResponseBuilder(res).unauthorized("Unknown token!");
        spdlog::warn("Unknown token '" + token + "'!");
        return;
    }

    try
    {
        Json body = Json::parse(req.body);
        std::string text = body["message_text"];
        auto err = _db.sendMessage(userOpt.value().id, text);

        if (err)
        {
            processErrors(res, err);
            return;
        }

        res.status = StatusCode::OK_200;
    }
    catch ( const std::exception &e )
    {
        spdlog::warn(std::string(__FILE__) + std::to_string(__LINE__) + e.what());
        ErrorResponseBuilder(res).badRequest("Error while send message!");
    }
}

void Server::_handleMessagesGet( const Request &req, Response &res )
{
    const std::string token = getAuthorizationToken(req);
    auto userOpt = _db.getUserByToken(token);

    if (!userOpt)
    {
        ErrorResponseBuilder(res).unauthorized("Unknown token!");
        spdlog::warn("Unknown token '" + token + "'!");
        return;
    }

    try
    {
        if (!req.has_param("limit"))
        {
            ErrorResponseBuilder(res).badRequest("Limit is needed!");
            return;
        }

        int limit = std::stoi(req.get_param_value("limit"));
        auto messages = _db.getLastMessages(limit);
        Json msgArray = Json::array();

        std::transform(messages.begin(), messages.end(), std::back_inserter(msgArray), 
                   []( const MessageJson &msg ) {return msg.toJson();});
        
        Json payload = {
            {"total_count", msgArray.size()},
            {"messages", msgArray}
        };

        res.status = StatusCode::OK_200;
        res.set_content(payload.dump(), "application/json");
    }
    catch ( const std::exception &e )
    {
        spdlog::warn(std::string(__FILE__) + std::to_string(__LINE__) + e.what());
        ErrorResponseBuilder(res).badRequest("Error while send message!");
    }
}

void Server::_handleMessagesCount( const Request &req, Response &res )
{
    const std::string token = getAuthorizationToken(req);

    if (!_db.isTokenExists(token))
    {
        ErrorResponseBuilder(res).unauthorized("Unknown token!");
        spdlog::warn("Unknown token '" + token + "'!");
        return;
    }

    int count = _db.getMessageCount();

    if (count == -1)
    {
        ErrorResponseBuilder(res).internal("Something was wrong, check logs...");
        return;
    }

    Json countResp = {
        {"status", "success"},
        {"count", count}
    };

    res.status = StatusCode::OK_200;
    res.set_content(countResp.dump(), "application/json");
}

void Server::_setupHandlers( void )
{
    // System endpoints
    _server->Get("/api/alive", [&]( const Request &req, Response &res ) {
        _handleAlive(req, res);
    });

    _server->Post("/api/check_token", [&]( const Request &req, Response &res ) {
        try
        {
            std::string token = Json::parse(req.body)["token"];
            Json result = {
                {"check_status", _db.isTokenExists(token)} 
            };

            res.status = StatusCode::OK_200;
            res.set_content(result.dump(), "application/json");
        }
        catch ( const std::exception &e )
        {
            ErrorResponseBuilder(res).badRequest(e.what());
        }
    });

    // Authentication endpoints
    _server->Post("/api/auth/register", [&]( const Request &req, Response &res ) {
        _handleRegister(req, res);
    });

    _server->Post("/api/auth/login", [&]( const Request &req, Response &res ) {
        _handleLogin(req, res);
    });

    _server->Post("/api/auth/logout", [&]( const Request &req, Response &res ) {
        _handleLogout(req, res);
    });

    // Users endpoints
    _server->Get("/api/users/me", [&]( const Request &req, Response &res ) {
        _handleMe(req, res);
    });

    _server->Get("/api/users/online", [&]( const Request &req, Response &res ) {
        _handleOnline(req, res);
    });

    _server->Get("/api/users/count", [&]( const Request &req, Response &res ) {
        _handleUsersCount(req, res);
    });

    // Messages endpoints
    _server->Post("/api/messages", [&]( const Request &req, Response &res ) {
        _handleMessagesPost(req, res);
    });

    _server->Get("/api/messages", [&]( const Request &req, Response &res ) {
        _handleMessagesGet(req, res);
    });

    _server->Get("/api/messages/count", [&]( const Request &req, Response &res ) {
        _handleMessagesCount(req, res);
    });
}

void Server::_setupStaticHandlers( void )
{
    _server->Get("/", [&]( const Request& req, Response& res ) {
        res.set_content(readFile("index.html"), "text/html");
    });

    _server->Get("/css/style.css", [&]( const Request& req, Response& res ) {
        res.set_content(readFile("css/style.css"), "text/css");
    });

    _server->Get("/js/utils.js", [&]( const Request& req, Response& res ) {
        res.set_content(readFile("/js/utils.js"), "application/javascript");
    });

    _server->Get("/js/auth.js", [&]( const Request& req, Response& res ) {
        res.set_content(readFile("/js/auth.js"), "application/javascript");
    });

    _server->Get("/register", [&]( const Request& req, Response& res ) {
        res.set_content(readFile("register.html"), "text/html");
    });

    _server->Get("/chat", [&]( const Request& req, Response& res ) {
        res.set_content(readFile("chat.html"), "text/html");
    });

    _server->Get("/css/chat.css", [&]( const Request& req, Response& res ) {
        res.set_content(readFile("css/chat.css"), "text/css");
    });

    _server->Get("/js/chat.js", [&]( const Request& req, Response& res ) {
        res.set_content(readFile("js/chat.js"), "application/javascript");
    });
}