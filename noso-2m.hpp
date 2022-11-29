#ifndef __NOSO2M_HPP__
#define __NOSO2M_HPP__

#include "config.hpp"

#define NOSO_NUL_HASH "00000000000000000000000000000000"
#define NOSO_MAX_DIFF "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
#define NOSO_TIMESTAMP ( (long long)( std::time( 0 ) ) )
#define NOSO_BLOCK_AGE ( NOSO_TIMESTAMP % 600 )
#define NOSO_BLOCK_IS_IN_MINING_AGE         \
            (                               \
                10 <= NOSO_BLOCK_AGE        \
                   && NOSO_BLOCK_AGE <= 585 \
            )

#endif // __NOSO2M_HPP__

