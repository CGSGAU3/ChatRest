#include <chrono>
#include <format>

#include <spdlog/spdlog.h>

#include "server.h"

Server::Server( const std::string &host, const int port, const std::string &dbName ) :
    _db(dbName), _server(nullptr), _host(host), _port(port) {}

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

    _startedAt = _getCurrentTimestamp();

    if (!_server->listen(_host, _port)) {
        throw std::runtime_error("Server run error!");
    }
}

auto Server::_getCurrentTimestamp( void ) const -> std::string
{
    auto now = std::chrono::system_clock::now();
    auto localNow = std::chrono::current_zone()->to_local(now);
    auto localSeconds = std::chrono::floor<std::chrono::seconds>(localNow);

    return std::format(R"({:%Y-%m-%d %H:%M:%S})", localSeconds);
}

void Server::_handleAlive( const Request &req, Response &res ) const
{
    Json status = {
        {"status", "online"},
        {"started_at", _startedAt},
        {"timestamp", _getCurrentTimestamp()}
    };

    res.status = StatusCode::OK_200;
    res.set_content(status.dump(), "application/json");
}

void Server::_setupHandlers( void ) {
    _server->Get("/api/alive", [&]( const Request &req, Response &res ) {
        _handleAlive(req, res);
    });
}