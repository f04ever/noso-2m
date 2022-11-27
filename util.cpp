#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <algorithm>

#include "md5-c.hpp"
#include "util.hpp"

std::string lpad( std::string const & s, std::size_t n, char c ) {
    std::string r { s };
    if ( n > r.length() ) r.append( n - r.length(), c );
    return r;
};

std::string& ltrim( std::string& s ) {
    s.erase( s.begin(), std::find_if( s.begin(), s.end(),
                                     []( unsigned char ch ) {
                                         return !std::isspace( ch ); } ) );
    return s;
}

std::string& rtrim( std::string& s ) {
    s.erase( std::find_if( s.rbegin(), s.rend(),
                          []( unsigned char ch ) {
                              return !std::isspace( ch ); } ).base(), s.end() );
    return s;
}

bool iequal( std::string const & s1, std::string const & s2 ) {
    return std::equal( s1.cbegin(), s1.cend(), s2.cbegin(), s2.cend(),
                      []( unsigned char a, unsigned char b ) {
                          return tolower( a ) == tolower( b ); } );
}

char hashrate_pretty_unit( std::uint64_t count ) {
    return
          ( count /             1'000.0 ) < 1'000 ? 'K' /* Kilo */
        : ( count /         1'000'000.0 ) < 1'000 ? 'M' /* Mega */
        : ( count /     1'000'000'000.0 ) < 1'000 ? 'G' /* Giga */
        : ( count / 1'000'000'000'000.0 ) < 1'000 ? 'T' /* Tera */
        :                                           'P' /* Peta */;
};

double hashrate_pretty_value( std::uint64_t count ) {
    return
          ( count /             1'000.0 ) < 1'000 ? ( count /                 1'000.0 ) /* Kilo */
        : ( count /         1'000'000.0 ) < 1'000 ? ( count /             1'000'000.0 ) /* Mega */
        : ( count /     1'000'000'000.0 ) < 1'000 ? ( count /         1'000'000'000.0 ) /* Giga */
        : ( count / 1'000'000'000'000.0 ) < 1'000 ? ( count /     1'000'000'000'000.0 ) /* Tera */
        :                                           ( count / 1'000'000'000'000'000.0 ) /* Peta */;
};

