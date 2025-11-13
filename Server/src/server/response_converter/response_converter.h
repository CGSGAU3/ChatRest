#pragma once

#include <nlohmann/json.hpp>

struct ErrorSchema
{
    std::string error;
    std::string message;
};

class ResponseConverter final {
private:

    using Json = nlohmann::json;

public:

    static auto toJson( const ErrorSchema &error ) -> Json;
};

