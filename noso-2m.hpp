#ifndef __NOSO2M_HPP__
#define __NOSO2M_HPP__

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <tuple>
#include <string>

#include "config.hpp"

#define NOSO_NUL_HASH "00000000000000000000000000000000"
#define NOSO_MAX_DIFF "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
#define NOSO_TIMESTAMP ( (long long)( std::time( 0 ) ) )
#define NOSO_BLOCK_AGE ( NOSO_TIMESTAMP % 600 )
#define NOSO_BLOCK_AGE_INNER_MINING_PERIOD                      \
            (   ( 10 <= NOSO_BLOCK_AGE )                        \
                   && ( NOSO_BLOCK_AGE <= 585 ) )
#define NOSO_BLOCK_AGE_BEFORE_MINING_PERIOD                     \
            (   10 > NOSO_BLOCK_AGE     )
#define NOSO_BLOCK_AGE_BEHIND_MINING_PERIOD                     \
            (   NOSO_BLOCK_AGE > 585    )
#define NOSO_BLOCK_AGE_OUTER_MINING_PERIOD                      \
            (   ( NOSO_BLOCK_AGE_BEFORE_MINING_PERIOD )         \
                   || ( NOSO_BLOCK_AGE_BEHIND_MINING_PERIOD )   )

typedef std::tuple<std::string, std::string, std::string> pool_specs_t;

#endif // __NOSO2M_HPP__

