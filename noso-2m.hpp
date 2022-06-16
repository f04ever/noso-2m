#ifndef __NOSO2M_HPP__
#define __NOSO2M_HPP__
#define NOSO_2M_VERSION_MAJOR 0
#define NOSO_2M_VERSION_MINOR 2
#define NOSO_2M_VERSION_PATCH 4
#define NOSO_2M_VERSION_CTEXT (\
             std::to_string(NOSO_2M_VERSION_MAJOR)\
        +"."+std::to_string(NOSO_2M_VERSION_MINOR)\
        +"."+std::to_string(NOSO_2M_VERSION_PATCH)\
    ).c_str()
#define NOSO_2M_VERSION_VTEXT (\
             std::string("version")\
        +" "+std::to_string(NOSO_2M_VERSION_MAJOR)\
        +"."+std::to_string(NOSO_2M_VERSION_MINOR)\
        +"."+std::to_string(NOSO_2M_VERSION_PATCH)\
    ).c_str()
#define DEFAULT_CONFIG_FILENAME "noso-2m.cfg"
#define DEFAULT_LOGGING_FILENAME "noso-2m.log"
#define DEFAULT_POOL_URL_LIST "f04ever;devnoso"
#define DEFAULT_MINER_ADDRESS "N3G1HhkpXvmLcsWFXySdAxX3GZpkMFS"
#define DEFAULT_MINER_ID 0
#define DEFAULT_THREADS_COUNT 2
#define DEFAULT_NODE_INET_TIMEOSEC 10
#define DEFAULT_POOL_INET_TIMEOSEC 60
#define DEFAULT_TIMESTAMP_DIFFERENCES 3
#define DEFAULT_CONSENSUS_NODES_COUNT 3
#define DEFAULT_INET_CIRCLE_SECONDS 0.1
#define DEFAULT_INET_BUFFER_SIZE 1024

#define NOSO_NUL_HASH "00000000000000000000000000000000"
#define NOSO_MAX_DIFF "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
#define NOSO_TIMESTAMP ( (long long)( std::time( 0 ) ) )
#define NOSO_BLOCK_AGE ( NOSO_TIMESTAMP % 600 )
#endif // __NOSO2M_HPP__
