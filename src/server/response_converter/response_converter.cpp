#include "response_converter.h"

auto ResponseConverter::toJson( const ErrorSchema &error ) -> Json
{
    return Json {{"status", "error"}, {"error", error.error}, {"message", error.message}};
}