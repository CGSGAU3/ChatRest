#include "httplib.h"

enum struct ErrorCode
{
    kBadRequest = 400,
    kUnauthorized = 401,
    kValidationError = 422,
    kInternal = 500,
};

class ErrorResponseBuilder
{
private:

    httplib::Response &_resp;

    void _buildError( const std::string& error, const std::string& message, ErrorCode code );

public:

    explicit ErrorResponseBuilder( httplib::Response &res ) : _resp(res) {}

    void badRequest( const std::string &message );
    void unauthorized( const std::string &message );
    void validationError( const std::string &message );
    void internal( const std::string &message );
};
