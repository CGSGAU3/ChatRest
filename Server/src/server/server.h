#pragma once

#include <httplib.h>
#include <nlohmann/json.hpp>

#include "database/database.h"

class Server final
{
    using Request = httplib::Request;
    using Response = httplib::Response;
    using StatusCode = httplib::StatusCode;
    using Json = nlohmann::json;

public:

    explicit Server( const std::string &host, const int port, const std::string &dbName = "a.db" );
    void run( void );

private:

    std::unique_ptr<httplib::Server> _server;
    Database _db;

    std::string _host;
    int _port;
    std::string _startedAt;

    static auto getAuthorizationToken( const Request &req ) -> std::string;
    static void processErrors( Response &res, const Database::Error &err );

    void _handleAlive( const Request &req, Response &res ) const;
    void _handleRegister( const Request &req, Response &res );
    void _handleLogin( const Request &req, Response &res );
    void _handleLogout( const Request &req, Response &res );

    auto _getCurrentTimestamp( void ) const -> std::string;
    void _setupHandlers( void );

};