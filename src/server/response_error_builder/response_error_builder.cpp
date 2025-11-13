#include "response_error_builder.h"
#include "response_converter.h"

void ErrorResponseBuilder::badRequest( const std::string &message )
{
    _buildError("bad_request", message, ErrorCode::kBadRequest);
}

void ErrorResponseBuilder::unauthorized( const std::string &message )
{
    _buildError("not_found", message, ErrorCode::kUnauthorized);
}

void ErrorResponseBuilder::validationError( const std::string &message )
{
    _buildError("validation_error", message, ErrorCode::kValidationError);
}

void ErrorResponseBuilder::internal( const std::string &message )
{
    _buildError("internal_server_error", message, ErrorCode::kInternal);
}

void ErrorResponseBuilder::_buildError( const std::string &error, const std::string &message, ErrorCode code )
{
    ErrorSchema err;

    err.error = error;
    err.message = message;

    _resp.set_content(ResponseConverter::toJson(err).dump(), "application/json");
    _resp.status = static_cast<int>(code);
}