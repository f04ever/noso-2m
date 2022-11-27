#ifndef __NOSO2M_UTIL_HPP__
#define __NOSO2M_UTIL_HPP__

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <string>

std::string lpad( std::string const & s, std::size_t n, char c );
std::string& ltrim( std::string& s );
std::string& rtrim( std::string& s );
bool iequal( std::string const & s1, std::string const & s2 );
char hashrate_pretty_unit( std::uint64_t count );
double hashrate_pretty_value( std::uint64_t count );

#endif // __NOSO2M_UTIL_HPP__

