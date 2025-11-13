#pragma once

#include <string>

std::string SHA224( const char *Msg, uint64_t length );
std::string SHA256( const char *Msg, uint64_t length );

std::string SHA224( const std::string &msg );
std::string SHA256( const std::string &msg );