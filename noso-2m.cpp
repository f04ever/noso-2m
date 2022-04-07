#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <map>
#include <set>
#include <mutex>
#include <regex>
#include <tuple>
#include <array>
#include <vector>
#include <string>
#include <thread>
#include <random>
#include <chrono>
#include <numeric>
#include <iomanip>
#include <cassert>
#include <iostream>

#include <signal.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#endif

#include "md5-c.hpp"
#include "cxxopts.hpp"

#define NOSO_2M_VERSION_MAJOR 0
#define NOSO_2M_VERSION_MINOR 1
#define NOSO_2M_VERSION_PATCH 3

#define DEFAULT_MINER_ADDRESS "N3G1HhkpXvmLcsWFXySdAxX3GZpkMFS"
#define DEFAULT_MINER_ID 0
#define DEFAULT_THREADS_COUNT 2
#define DEFAULT_NODE_INET_TIMEOSEC 10
#define DEFAULT_POOL_INET_TIMEOSEC 30

#define CONSENSUS_NODES_COUNT 3
#define INET_CIRCLE_SECONDS 0.01
#define INET_BUFFER_SIZE 1024

#define NOSOHASH_COUNTER_MIN 100'000'000
#define NOSO_MAX_DIFF "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
#define NOSO_TIMESTAMP long( std::time( 0 ) )
#define NOSO_BLOCK_AGE ( NOSO_TIMESTAMP % 600 )

const std::vector<std::tuple<std::string, std::string>> g_default_nodes {
        { "45.146.252.103"  ,   "8080" },
        { "109.230.238.240" ,   "8080" },
        { "194.156.88.117"  ,   "8080" },
        { "23.94.21.83"     ,   "8080" },
        { "107.175.59.177"  ,   "8080" },
        { "107.172.193.176" ,   "8080" },
        { "107.175.194.151" ,   "8080" },
        { "192.3.173.184"   ,   "8080" },
    }; // seed nodes

const std::vector<std::tuple<std::string, std::string, std::string>> g_default_pools {
        { "devnoso", "45.146.252.103", "8082" },
    };

const char NOSOHASH_HASHEABLE_CHARS[] {
    "!\"#$%&')*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~" };
const std::size_t NOSOHASH_HASHEABLE_COUNT =  93 - 1; // std::strlen( NOSOHASH_HASHEABLE_COUNT );
inline std::string nosohash_prefix( int num ) {
    return std::string {
        NOSOHASH_HASHEABLE_CHARS[ num / NOSOHASH_HASHEABLE_COUNT ],
        NOSOHASH_HASHEABLE_CHARS[ num % NOSOHASH_HASHEABLE_COUNT ], };
}

inline int nosohash_char( int num ) {
    assert( 32 <= num && num <= 504 );
    while ( num > 126 ) num -= 95;
    assert( 32 <= num && num <= 126 );
    return num;
};

#ifndef NDEBUG
const std::array<char, 16> HEXCHAR_DOMAIN { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F' };
#endif
inline int hex_char2dec( char hexchar ) {
    assert( std::find( HEXCHAR_DOMAIN.begin(), HEXCHAR_DOMAIN.end(), hexchar ) != HEXCHAR_DOMAIN.end() );
    return  ( '0' <= hexchar && hexchar <= '9' ) ? hexchar - '0' :
            ( 'A' <= hexchar && hexchar <= 'F' ) ? hexchar - 'A' + 10 : 0;
}

#ifndef NDEBUG
const std::array<int, 16> HEXDEC_DOMAIN{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
#endif
inline char hex_dec2char( int hexdec ) {
    assert( std::find( HEXDEC_DOMAIN.begin(), HEXDEC_DOMAIN.end(), hexdec ) != HEXDEC_DOMAIN.end() );
    return  (  0 <= hexdec && hexdec <=  9 ) ? hexdec + '0' :
            ( 10 <= hexdec && hexdec <= 15 ) ? hexdec + 'A' - 10 : '\0';
}

class CNosoHasher {
private:
    char m_base[19]; // 18 = 9-chars-prefix + 9-chars-counter
    char m_hash[33];
    char m_diff[33];
    char m_stat[129][128]; // 1+128 rows x 128 columns
    // the 1st row is the input of hash function with len = 128 = 9-chars-prefix + 9 chars-counter + 30/31-chars-address + N-fill-chars rest
    MD5Context m_md5_ctx;
    constexpr static const char hexchars_table[] = "0123456789ABCDEF";
    constexpr static std::uint16_t nosohash_chars_table[505] {
    // for ( int i = 0; i < 505; ++i ) {    // as 4 * 126 = 504 maximum value
    //     int n = i >= 32 ? i : 0;
    //     while ( n > 126 ) n -= 95;       // as 127 - 95 =  32 minimum value
    //     // std::cout << std::setw( 3 ) << i << ", ";
    //     std::cout << std::setw( 3 ) << n << ", ";
    //     if ( i % 24 == 0 ) std::cout << std::endl;
    // }
  0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,
 49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,  72,
 73,  74,  75,  76,  77,  78,  79,  80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,  96,
 97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120,
121, 122, 123, 124, 125, 126,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,  49,
 50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,  72,  73,
 74,  75,  76,  77,  78,  79,  80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,  96,  97,
 98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121,
122, 123, 124, 125, 126,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,
 51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,
 75,  76,  77,  78,  79,  80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,  96,  97,  98,
 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122,
123, 124, 125, 126,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,
 52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,
 76,  77,  78,  79,  80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,  96,  97,  98,  99,
100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123,
124, 125, 126,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,  52,
 53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,
 77,  78,  79,  80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,  96,  97,  98,  99, 100,
101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124,
    };
    inline void _hash() {
        int row, col;
        for( row = 1; row < 129; row++ ) {
            for( col = 0; col < 127; col++ ) {
                // m_stat[row][col] = nosohash_char(        m_stat[row-1][col] + m_stat[row-1][col+1] );
                m_stat[row][col] = nosohash_chars_table[ m_stat[row-1][col] + m_stat[row-1][col+1] ];
            }
            // m_stat[row][127] = nosohash_char(        m_stat[row-1][127] + m_stat[row-1][0] );
            m_stat[row][127] = nosohash_chars_table[ m_stat[row-1][127] + m_stat[row-1][0] ];
        }
        int i;
        for( i = 0; i < 32; i++ )
            // m_hash[i] = hex_dec2char( nosohash_char(
            //                             m_stat[128][ ( i * 4 ) + 0 ] +
            //                             m_stat[128][ ( i * 4 ) + 1 ] +
            //                             m_stat[128][ ( i * 4 ) + 2 ] +
            //                             m_stat[128][ ( i * 4 ) + 3 ] ) % 16 );
            m_hash[i] = hex_dec2char( nosohash_chars_table[
                                        m_stat[128][ ( i * 4 ) + 0 ] +
                                        m_stat[128][ ( i * 4 ) + 1 ] +
                                        m_stat[128][ ( i * 4 ) + 2 ] +
                                        m_stat[128][ ( i * 4 ) + 3 ] ] % 16 );
        m_hash[32] = '\0';
        assert( std::strlen( m_hash ) == 32 );
    }
    inline void _md5() {
        assert( std::strlen( m_hash ) == 32 );
        md5Init( &m_md5_ctx );
        md5Update( &m_md5_ctx, (uint8_t *)m_hash, 32 );
        md5Finalize( &m_md5_ctx );
        // std::sprintf( m_hash,
        //             "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
        //             m_md5_ctx.digest[ 0], m_md5_ctx.digest[ 1],
        //             m_md5_ctx.digest[ 2], m_md5_ctx.digest[ 3],
        //             m_md5_ctx.digest[ 4], m_md5_ctx.digest[ 5],
        //             m_md5_ctx.digest[ 6], m_md5_ctx.digest[ 7],
        //             m_md5_ctx.digest[ 8], m_md5_ctx.digest[ 9],
        //             m_md5_ctx.digest[10], m_md5_ctx.digest[11],
        //             m_md5_ctx.digest[12], m_md5_ctx.digest[13],
        //             m_md5_ctx.digest[14], m_md5_ctx.digest[15] );
        m_hash[ 0] = hexchars_table[m_md5_ctx.digest[ 0] >>  4];
        m_hash[ 1] = hexchars_table[m_md5_ctx.digest[ 0] & 0xF];
        m_hash[ 2] = hexchars_table[m_md5_ctx.digest[ 1] >>  4];
        m_hash[ 3] = hexchars_table[m_md5_ctx.digest[ 1] & 0xF];
        m_hash[ 4] = hexchars_table[m_md5_ctx.digest[ 2] >>  4];
        m_hash[ 5] = hexchars_table[m_md5_ctx.digest[ 2] & 0xF];
        m_hash[ 6] = hexchars_table[m_md5_ctx.digest[ 3] >>  4];
        m_hash[ 7] = hexchars_table[m_md5_ctx.digest[ 3] & 0xF];
        m_hash[ 8] = hexchars_table[m_md5_ctx.digest[ 4] >>  4];
        m_hash[ 9] = hexchars_table[m_md5_ctx.digest[ 4] & 0xF];
        m_hash[10] = hexchars_table[m_md5_ctx.digest[ 5] >>  4];
        m_hash[11] = hexchars_table[m_md5_ctx.digest[ 5] & 0xF];
        m_hash[12] = hexchars_table[m_md5_ctx.digest[ 6] >>  4];
        m_hash[13] = hexchars_table[m_md5_ctx.digest[ 6] & 0xF];
        m_hash[14] = hexchars_table[m_md5_ctx.digest[ 7] >>  4];
        m_hash[15] = hexchars_table[m_md5_ctx.digest[ 7] & 0xF];
        m_hash[16] = hexchars_table[m_md5_ctx.digest[ 8] >>  4];
        m_hash[17] = hexchars_table[m_md5_ctx.digest[ 8] & 0xF];
        m_hash[18] = hexchars_table[m_md5_ctx.digest[ 9] >>  4];
        m_hash[19] = hexchars_table[m_md5_ctx.digest[ 9] & 0xF];
        m_hash[20] = hexchars_table[m_md5_ctx.digest[10] >>  4];
        m_hash[21] = hexchars_table[m_md5_ctx.digest[10] & 0xF];
        m_hash[22] = hexchars_table[m_md5_ctx.digest[11] >>  4];
        m_hash[23] = hexchars_table[m_md5_ctx.digest[11] & 0xF];
        m_hash[24] = hexchars_table[m_md5_ctx.digest[12] >>  4];
        m_hash[25] = hexchars_table[m_md5_ctx.digest[12] & 0xF];
        m_hash[26] = hexchars_table[m_md5_ctx.digest[13] >>  4];
        m_hash[27] = hexchars_table[m_md5_ctx.digest[13] & 0xF];
        m_hash[28] = hexchars_table[m_md5_ctx.digest[14] >>  4];
        m_hash[29] = hexchars_table[m_md5_ctx.digest[14] & 0xF];
        m_hash[30] = hexchars_table[m_md5_ctx.digest[15] >>  4];
        m_hash[31] = hexchars_table[m_md5_ctx.digest[15] & 0xF];
        m_hash[32] = '\0';
        assert( std::strlen( m_hash ) == 32 );
    }
public:
    CNosoHasher( const char prefix[10], const char address[32] ) {
        constexpr static const char NOSOHASH_FILLER_CHARS[] = "%)+/5;=CGIOSYaegk";
        constexpr static const int NOSOHASH_FILLER_COUNT = 17; // std::strlen( NOSOHASH_FILLER_CHARS );
        assert( std::strlen( prefix ) == 9
               && ( std::strlen( address ) == 30 || std::strlen( address ) == 31 ) );
        std::memcpy( m_base, prefix, 9 );
        std::sprintf( m_base + 9, "%09d", 0 ); // placehold for 9-digits-counter
        assert( std::strlen( m_base ) == 18 ); // 18 = 9 + 9 = 9-chars-prefix + 9-digits-counter
        int addr_len = std::strlen( address );
        std::memcpy( m_stat[0], m_base, 9 );
        std::memcpy( m_stat[0] + 9, m_base + 9, 9 );  // update the 9-digits-counter part as the same as it is updated in base
        std::memcpy( m_stat[0] + 9 + 9, address, addr_len );
        int len = 18 + addr_len; // 48/49 = 9 + 9 + 30/31 = 9-chars-prefix + 9-digits-counter + 30/31-chars-address
        int div = ( 128 - len ) / NOSOHASH_FILLER_COUNT;
        int mod = ( 128 - len ) % NOSOHASH_FILLER_COUNT;
        for ( int i = 0; i < div; i++ ) {
            std::memcpy( m_stat[0] + len, NOSOHASH_FILLER_CHARS, NOSOHASH_FILLER_COUNT );
            len += NOSOHASH_FILLER_COUNT;
        }
        std::memcpy( m_stat[0] + len, NOSOHASH_FILLER_CHARS, mod );
        assert( std::none_of( m_stat[0], m_stat[0] + 128, []( int c ){ return 33 > c || c > 126; } ) );
    }
    const char* GetBase( std::uint32_t counter ) {
        // TODO consider case counter > 999'999'999 => base len > 18. Currently
        // it does not happen, each single thread can hash/search under
        // 700'000'000 hashes each block
        std::sprintf( m_base + 9, "%09d", NOSOHASH_COUNTER_MIN + counter ); // update 9-digits-counter part
        assert( std::strlen( m_base ) == 18 ); // 18 = 9 + 9 = 9-chars-prefix + 9-digits-counter
        std::memcpy( m_stat[0] + 9, m_base + 9, 9 );  // update the 9-digits-counter part as it was updated in base
        assert( std::none_of( m_stat[0], m_stat[0] + 128, []( int c ){ return 33 > c || c > 126; } ) );
        return m_base;
    }
    const char* GetHash() {
        _hash();
        _md5();
        return m_hash;
    }
    const char* GetDiff( const char target[33] ) {
        assert( std::strlen( m_hash ) == 32 && std::strlen( target ) == 32 );
        for ( std::size_t i = 0; i < 32; i ++ )
            m_diff[i] = std::toupper( hex_dec2char( std::abs( hex_char2dec( m_hash[ i ] ) - hex_char2dec( target[ i ] ) ) ) );
        m_diff[32] = '\0';
        assert( std::strlen( m_diff ) == 32 );
        return m_diff;
    }
};
constexpr const char CNosoHasher::hexchars_table[];
constexpr std::uint16_t CNosoHasher::nosohash_chars_table[];

int inet_command( struct addrinfo *serv_info, uint32_t timeosec, char *buffer, size_t buffsize );

class CInet {
protected:
    struct addrinfo *m_serv_info;
public:
    const std::string m_host;
    const std::string m_port;
    const int m_timeosec;
    CInet( const std::string &host, const std::string &port, int timeosec )
        :   m_serv_info { NULL },
            m_host { host }, m_port { port }, m_timeosec( timeosec ) {
        this->InitService();
    }
    ~CInet() {
        this->CleanService();
    }
    void InitService() {
        if ( m_serv_info != NULL ) return;
        struct addrinfo hints, *serv_info;
        std::memset( &hints, 0, sizeof( hints ) );
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        int n = getaddrinfo( this->m_host.c_str(), this->m_port.c_str(), &hints, &serv_info );
        if ( n ) {
            std::fprintf( stderr, "getaddrinfo: %s\n", gai_strerror(n) );
            m_serv_info = NULL;
        }
        m_serv_info = serv_info;
    }
    void CleanService() {
        if ( m_serv_info == NULL ) return;
        freeaddrinfo( m_serv_info );
        m_serv_info = NULL;
    }
};

class CNodeInet : public CInet {
public:
    CNodeInet( const std::string &host, const std::string &port , int timeosec )
        :   CInet( host, port, timeosec ) {
    }
    int FetchNodestatus( char *buffer, std::size_t buffsize ) {
        std::strcpy( buffer, "NODESTATUS\n" );
        return inet_command( m_serv_info, m_timeosec, buffer, buffsize );
    }
    int SubmitSolution( std::uint32_t blck, const char base[19], const char address[32],
                       char *buffer, std::size_t buffsize ) {
        assert( std::strlen( base ) == 18
               && std::strlen( address ) == 30 || std::strlen( address ) == 31 );
        std::snprintf( buffer, buffsize, "BESTHASH 1 2 3 4 %s %s %d %ld\n", address, base, blck, NOSO_TIMESTAMP );
        return inet_command( m_serv_info, m_timeosec, buffer, buffsize );
    }
};

class CPoolInet : public CInet {
public:
    std::string m_name;
    CPoolInet( const std::string& name, const std::string &host, const std::string &port , int timeosec )
        :   CInet( host, port, timeosec ), m_name { name } {
    }
    int FetchSource( const char address[32], char *buffer, std::size_t buffsize ) {
        assert( std::strlen( address ) == 30 || std::strlen( address ) == 31 );
        std::snprintf( buffer, buffsize, "SOURCE %s\n", address );
        return inet_command( m_serv_info, m_timeosec, buffer, buffsize );
    }
    int SubmitShare( const char base[19], const char address[32], char *buffer, std::size_t buffsize ) {
        assert( std::strlen( base ) == 18
               && std::strlen( address ) == 30 || std::strlen( address ) == 31 );
        std::snprintf( buffer, buffsize, "SHARE %s %s\n", address, base );
        return inet_command( m_serv_info, m_timeosec, buffer, buffsize );
    }
};

struct CNodeStatus {
    // std::uint32_t peer;
    std::uint32_t blck_no;
    // std::uint32_t pending;
    // std::uint32_t delta;
    // std::string branch;
    // std::string version;
    // std::time_t utctime;
    // std::string mn_hash;
    // std::uint32_t mn_count;
    std::string lb_hash;
    std::string mn_diff;
    std::time_t lb_time;
    std::string lb_addr;
    CNodeStatus( const char *ns_line ) {
        assert( ns_line != nullptr && std::strlen( ns_line ) > 1 );
        auto next_status_token = []( size_t &p_pos, size_t &c_pos, const std::string &status ) {
            p_pos = c_pos;
            c_pos = status.find( ' ', c_pos + 1 );
        };
        auto extract_status_token = []( size_t p_pos, size_t c_pos, const std::string& status ) {
            return status.substr( p_pos + 1, c_pos == std::string::npos ? std::string::npos : ( c_pos - p_pos - 1 ) );
        };
        std::string status { ns_line };
        status.erase( status.length() - 2 ); // remove the carriage return and new line charaters
        size_t p_pos = -1, c_pos = -1;
        //NODESTATUS 1{Peers} 2{LastBlock} 3{Pendings} 4{Delta} 5{headers} 6{version} 7{UTCTime} 8{MNsHash} 9{MNscount}
        //           10{LastBlockHash} 11{BestHashDiff} 12{LastBlockTimeEnd} 13{LBMiner}
        // 0{NODESTATUS}
        next_status_token( p_pos, c_pos, status );
        // std::string nodestatus = extract_status_token( p_pos, c_pos, status );
        // 1{peer}
        next_status_token( p_pos, c_pos, status );
        // this->peer = std::stoul( extract_status_token( p_pos, c_pos, status ) );
        // 2{blck}
        next_status_token( p_pos, c_pos, status );
        this->blck_no = std::stoul( extract_status_token( p_pos, c_pos, status ) );
        // 3{pending}
        next_status_token( p_pos, c_pos, status );
        // this->pending = std::stoul( extract_status_token( p_pos, c_pos, status ) );
        // 4{delta}
        next_status_token( p_pos, c_pos, status );
        // this->delta = std::stoul( extract_status_token( p_pos, c_pos, status ) );
        // 5{header/branch}
        next_status_token( p_pos, c_pos, status );
        // this->branch = extract_status_token( p_pos, c_pos, status );
        // 6{version}
        next_status_token( p_pos, c_pos, status );
        // this->version = extract_status_token( p_pos, c_pos, status );
        // 7{utctime}
        next_status_token( p_pos, c_pos, status );
        // this->utctime = std::stoul( extract_status_token( p_pos, c_pos, status ) );
        // 8{mn_hash}
        next_status_token( p_pos, c_pos, status );
        // this->mn_hash = extract_status_token( p_pos, c_pos, status );
        // 9{mn_count}
        next_status_token( p_pos, c_pos, status );
        // this->mn_count = std::stoul( extract_status_token( p_pos, c_pos, status ) );
        // 10{lb_hash}
        next_status_token( p_pos, c_pos, status );
        this->lb_hash = extract_status_token( p_pos, c_pos, status );
        assert( this->lb_hash.length() == 32 );
        // 11{bh_diff/mn_diff}
        next_status_token( p_pos, c_pos, status );
        this->mn_diff = extract_status_token( p_pos, c_pos, status );
        assert( this->mn_diff.length() == 32 );
        // 12{lb_time}
        next_status_token( p_pos, c_pos, status );
        this->lb_time = std::stoul( extract_status_token( p_pos, c_pos, status ) );
        // 13{lb_addr}
        next_status_token( p_pos, c_pos, status );
        this->lb_addr = extract_status_token( p_pos, c_pos, status );
        assert( this->lb_addr.length() == 30 || this->lb_addr.length() == 31 );
    }
};

struct CPoolStatus {
    std::uint32_t blck_no;
    std::string lb_hash;
    std::string mn_diff;
    std::string prefix;
    std::string address;
    std::uint32_t balance;
    CPoolStatus( const char *ps_line ) {
        assert( ps_line != nullptr && std::strlen( ps_line ) > 1 );
        auto next_status_token = []( size_t &p_pos, size_t &c_pos, const std::string &status ) {
            p_pos = c_pos;
            c_pos = status.find(' ', c_pos + 1);
        };
        auto extract_status_token = []( size_t p_pos, size_t c_pos, const std::string& status ) {
            return status.substr( p_pos + 1, c_pos == std::string::npos ? std::string::npos : (c_pos - p_pos - 1) );
        };
        std::string status { ps_line };
        status.erase( status.length() - 2 ); // remove the carriage return and new line charaters
        size_t p_pos = -1, c_pos = -1;
        // {0}OK 1{MinerPrefix} 2{MinerAddress} 3{PoolMinDiff} 4{LBHash} 5{LBNumber} 6{MinerBalance}
        // 0{OK}
        next_status_token( p_pos, c_pos, status );
        // std::string status = extract_status_token( p_pos, c_pos, status );
        // 1{prefix}
        next_status_token( p_pos, c_pos, status );
        this->prefix = extract_status_token( p_pos, c_pos, status );
        assert( this->prefix.length() == 3 );
        // 2{address}
        next_status_token( p_pos, c_pos, status );
        this->address = extract_status_token( p_pos, c_pos, status );
        assert( this->address.length() == 30 || this->address.length() == 31 );
        // 3{mn_diff}
        next_status_token( p_pos, c_pos, status );
        this->mn_diff = extract_status_token( p_pos, c_pos, status );
        assert( this->mn_diff.length() == 32 );
        // 4{lb_hash}
        next_status_token( p_pos, c_pos, status );
        this->lb_hash = extract_status_token( p_pos, c_pos, status );
        assert(this->lb_hash.length() == 32 );
        // 5{blck_no}
        next_status_token( p_pos, c_pos, status );
        this->blck_no = std::stoul( extract_status_token( p_pos, c_pos, status ) );
        // 6{balance}
        next_status_token( p_pos, c_pos, status );
        this->balance = std::stoul( extract_status_token( p_pos, c_pos, status ) );
    }
};

struct CTarget {
    std::string prefix;
    std::string address;
    std::uint32_t blck_no;
    std::string lb_hash;
    std::string mn_diff;
    CTarget( std::uint32_t blck_no, const std::string &lb_hash, const std::string &mn_diff )
        :   blck_no { blck_no }, lb_hash { lb_hash }, mn_diff { mn_diff } {
        // prefix = "";
        // address = "";
    }
    virtual ~CTarget() = default;
};

struct CNodeTarget : public CTarget {
    std::time_t lb_time;
    std::string lb_addr;
    CNodeTarget( std::uint32_t blck_no, const std::string &lb_hash, const std::string &mn_diff,
              std::time_t lb_time, const std::string &lb_addr )
        :   CTarget( blck_no, lb_hash, mn_diff ), lb_time { lb_time }, lb_addr { lb_addr } {
    }
};

struct CPoolTarget : public CTarget {
    std::uint32_t balance;
    CPoolTarget( std::uint32_t blck_no, const std::string &lb_hash, const std::string &mn_diff,
              std::string &prefix, const std::string &address, std::uint32_t balance )
        : CTarget( blck_no, lb_hash, mn_diff ), balance { balance } {
        this->prefix = prefix;
        this->address = address;
    }
};

struct CSolution {
    std::uint32_t blck;
    std::string base;
    std::string hash;
    std::string diff;
    CSolution( std::uint32_t blck_, const char base_[19], const char hash_[33], const char diff_[33] )
        :   blck { blck_ }, base { base_ }, hash { hash_ }, diff { diff_ } {
        assert( std::strlen( base_ ) == 18 && std::strlen( hash_ ) == 32 && std::strlen( diff_ ) == 32 );
    }
};

struct CSolsSetCompare {
    bool operator()( const std::shared_ptr<CSolution>& lhs, const std::shared_ptr<CSolution>& rhs ) const {
        return  lhs->blck > rhs->blck ? true :
                lhs->blck < rhs->blck ? false :
                lhs->diff < rhs->diff ? true : false;
    }
};

class CMineThread {
protected:
    std::uint32_t m_miner_id;
    std::uint32_t m_thread_id;
    char m_address[32];
    char m_prefix[10];
    std::uint32_t m_blck_no { 0 };
    char m_lb_hash[33];
    char m_mn_diff[33];
    std::uint32_t m_computed_hashes_count { 0 };
public:
    CMineThread( std::uint32_t miner_id, std::uint32_t thread_id )
        : m_miner_id { miner_id }, m_thread_id { thread_id } {
    }
    void NewTarget( const std::shared_ptr<CTarget> &target ) {
        std::string thread_prefix = { target->prefix + nosohash_prefix( m_miner_id ) + nosohash_prefix( m_thread_id ) };
        thread_prefix.append( 9 - thread_prefix.size(), '!' );
        std::strcpy( m_prefix, thread_prefix.c_str() );
        std::strcpy( m_address, target->address.c_str() );
        std::strcpy( m_lb_hash, target->lb_hash.c_str() );
        std::strcpy( m_mn_diff, target->mn_diff.c_str() );
        m_computed_hashes_count = 0;
        m_blck_no = target->blck_no + 1;
    }
    void UpdateComputedHashesCount( std::uint32_t more ) {
        m_computed_hashes_count += more;
    }
    std::uint32_t GetComputedHashesCount() {
        return m_computed_hashes_count;
    }
    void Mine();
};

class CCommThread { // A singleton pattern class
private:
    mutable std::default_random_engine m_random_engine {
        std::default_random_engine { std::random_device {}() } };
    mutable std::mutex m_mutex_solutions;
    std::vector<std::shared_ptr<CNodeInet>> m_node_inets_good;
    std::vector<std::shared_ptr<CNodeInet>> m_node_inets_poor;
    std::vector<std::shared_ptr<CPoolInet>> m_pool_inets;
    std::uint32_t m_pool_inet_id;
    std::multiset<std::shared_ptr<CSolution>, CSolsSetCompare> m_solutions;
    std::uint32_t m_accepted_solutions_count { 0 };
    std::uint32_t m_rejected_solutions_count { 0 };
    std::uint32_t m_failured_solutions_count { 0 };
    char m_status_buffer[INET_BUFFER_SIZE];
    char m_submit_buffer[INET_BUFFER_SIZE];
    std::map<std::uint32_t, int> m_freq_blck_no;
    std::map<std::string  , int> m_freq_lb_hash;
    std::map<std::string  , int> m_freq_mn_diff;
    std::map<std::time_t  , int> m_freq_lb_time;
    std::map<std::string  , int> m_freq_lb_addr;
    CCommThread();
    std::vector<std::shared_ptr<CNodeStatus>> SyncSources( std::size_t min_nodes_count ) {
        if ( m_node_inets_good.size() < min_nodes_count ) {
            for ( auto ni : m_node_inets_poor ) {
                ni->InitService();
                m_node_inets_good.push_back( ni );
            }
            m_node_inets_poor.clear();
            std::cerr << "poor network reset: " << m_node_inets_good.size() << "/" << m_node_inets_poor.size() << std::endl;
        }
        std::shuffle( m_node_inets_good.begin(), m_node_inets_good.end(), m_random_engine );
        std::size_t nodes_count { 0 };
        std::vector<std::shared_ptr<CNodeStatus>> vec;
        for ( auto it = m_node_inets_good.begin(); it != m_node_inets_good.end(); ) {
            if ( (*it)->FetchNodestatus( m_status_buffer, INET_BUFFER_SIZE ) <= 0 ) {
                (*it)->CleanService();
                m_node_inets_poor.push_back( *it );
                it = m_node_inets_good.erase( it );
                std::cerr << "poor network found: " << m_node_inets_good.size() << "/" << m_node_inets_poor.size() << std::endl;
                continue;
            }
            ++it;
            try {
                vec.push_back( std::make_shared<CNodeStatus>( m_status_buffer ) );
                nodes_count ++;
                if ( nodes_count >= min_nodes_count ) break;
            }
            catch ( const std::exception &e ) {
                std::cout << e.what();
            }
        }
        return vec;
    }
    void _ResetMiningBlock();
    void _PrintBlockSummary( std::uint32_t blck_no, const std::chrono::duration<double>& elapsed_blck );
    void _ReportErrorSubmitting( int code, const std::shared_ptr<CSolution> &solution );
public:
    CCommThread( const CCommThread& ) = delete; // Copy prohibited
    CCommThread( CCommThread&& ) = delete; // Move prohibited
    void operator=( const CCommThread& ) = delete; // Assignment prohibited
    CCommThread& operator=( CCommThread&& ) = delete; // Move assignment prohibited
    static std::shared_ptr<CCommThread> GetInstance() {
        static std::shared_ptr<CCommThread> singleton { new CCommThread() };
        return singleton;
    }
    void AddSolution( const std::shared_ptr<CSolution>& solution ) {
        m_mutex_solutions.lock();
        m_solutions.insert( solution );
        m_mutex_solutions.unlock();
    }
    void ClearSolutions() {
        m_mutex_solutions.lock();
        m_solutions.clear();
        m_mutex_solutions.unlock();
    }
    const std::shared_ptr<CSolution> BestSolution() {
        std::shared_ptr<CSolution> best_solution { nullptr };
        m_mutex_solutions.lock();
        if ( m_solutions.begin() != m_solutions.end() ) {
            auto itor_best_solution = m_solutions.begin();
            best_solution = *itor_best_solution;
            m_solutions.clear();
        }
        m_mutex_solutions.unlock();
        return best_solution;
    }
    const std::shared_ptr<CSolution> GoodSolution() {
        std::shared_ptr<CSolution> good_solution { nullptr };
        m_mutex_solutions.lock();
        if ( m_solutions.begin() != m_solutions.end() ) {
            auto itor_good_solution = m_solutions.begin();
            good_solution = *itor_good_solution;
            m_solutions.erase( itor_good_solution );
        }
        m_mutex_solutions.unlock();
        return good_solution;
    }
    std::shared_ptr<CSolution> GetSolution();
    int PushSolution( std::uint32_t blck, const char base[19], const char address[32], char new_mn_diff[33] ) {
        assert( std::strlen( base ) == 18
               && ( std::strlen( address ) == 30 || std::strlen( address ) == 31 ) );
        if ( m_node_inets_good.size() < 1 ) {
            for ( auto ni : m_node_inets_poor ) {
                ni->InitService();
                m_node_inets_good.push_back( ni );
            }
            m_node_inets_poor.clear();
            std::cerr << "poor network reset: " << m_node_inets_good.size() << "/" << m_node_inets_poor.size() << std::endl;
        }
        std::shuffle( m_node_inets_good.begin(), m_node_inets_good.end(), m_random_engine );
        for ( auto it = m_node_inets_good.begin(); it != m_node_inets_good.end(); ) {
            if ( (*it)->SubmitSolution( blck, base, address, m_submit_buffer, INET_BUFFER_SIZE ) <= 0 ) {
                (*it)->CleanService();
                m_node_inets_poor.push_back( *it );
                it = m_node_inets_good.erase( it );
                std::cerr << "poor network found: " << m_node_inets_good.size() << "/" << m_node_inets_poor.size() << std::endl;
                continue;
            }
            ++it;
            assert( std::strlen( m_submit_buffer ) >= 40 + 2 ); // len=(70+2)~[True Diff(32) Hash(32)\r\n] OR len=(40+2)~[False Diff(32) Code#(1)\r\n]
            if ( strncmp( m_submit_buffer, "True", 4 ) == 0 ) {
                assert( std::strlen( m_submit_buffer ) == 70 + 2 ); // len=(70+2)~[True Diff(32) Hash(32)\r\n]
                std::strncpy( new_mn_diff, m_submit_buffer + 5, 32 );
                new_mn_diff[32] = '\0';
                assert( std::strlen( new_mn_diff ) == 32 );
                return 0;
            }
            else {
                assert( std::strncmp( m_submit_buffer, "False", 5 ) == 0
                       && std::strlen( m_submit_buffer ) == 40 + 2 // len=(40+2)~[False Diff(32) Code#(1)\r\n]
                       && '1' <= m_submit_buffer[39] && m_submit_buffer[39] <= '7' );
                std::strncpy( new_mn_diff, m_submit_buffer + 6, 32 );
                new_mn_diff[32] = '\0';
                assert( strlen( new_mn_diff ) == 32 );
                return m_submit_buffer[39] - '0';
            }
        }
        return -1;
    }
    std::shared_ptr<CNodeTarget> MakeConsensus() {
        std::vector<std::shared_ptr<CNodeStatus>> status_of_nodes = this->SyncSources( CONSENSUS_NODES_COUNT );
        if ( status_of_nodes.size() < CONSENSUS_NODES_COUNT ) return nullptr;
        const auto max_freq = []( const auto &freq ) {
            return std::max_element(
                std::begin( freq ), std::end( freq ),
                [] ( const auto &p1, const auto &p2 ) {
                    return p1.second < p2.second; } )->first;
        };
        m_freq_blck_no.clear();
        m_freq_lb_hash.clear();
        m_freq_mn_diff.clear();
        m_freq_lb_time.clear();
        m_freq_lb_addr.clear();
        for( auto ns : status_of_nodes ) {
            ++m_freq_blck_no [ns->blck_no];
            ++m_freq_lb_hash [ns->lb_hash];
            ++m_freq_mn_diff [ns->mn_diff];
            ++m_freq_lb_time [ns->lb_time];
            ++m_freq_lb_addr [ns->lb_addr];
        }
        return std::make_shared<CNodeTarget>(
            max_freq( m_freq_blck_no ),
            max_freq( m_freq_lb_hash ),
            max_freq( m_freq_mn_diff ),
            max_freq( m_freq_lb_time ),
            max_freq( m_freq_lb_addr ) );
    }
    std::shared_ptr<CPoolTarget> ReceivePoolData( const char address[32] ) {
        assert( std::strlen( address ) == 30 || std::strlen( address ) == 31 );
        if ( m_pool_inets[m_pool_inet_id]->FetchSource( address, m_status_buffer, INET_BUFFER_SIZE ) <= 0 )
            return nullptr;
        try {
            CPoolStatus ps( m_status_buffer );
            return std::make_shared<CPoolTarget>(
                ps.blck_no,
                ps.lb_hash,
                ps.mn_diff,
                ps.prefix,
                ps.address,
                ps.balance );
        }
        catch ( const std::exception &e ) {
            std::cout << e.what();
            return nullptr;
        }
    }
    std::shared_ptr<CTarget> GetTarget();
    int SendSolution( const char base[19], const char address[32] ) {
        assert( std::strlen( base ) == 18
               && ( std::strlen( address ) == 30 || std::strlen( address ) == 31 ) );
        for ( int i = 0; i < 5 ; ++i ) {
            if ( m_pool_inets[m_pool_inet_id]->SubmitShare( base, address, m_submit_buffer, INET_BUFFER_SIZE ) <= 0 ) {
                    std::cerr << "poor pool network! retrying " << i + 1 << std::endl;
                    continue;
            } else {
                assert( std::strlen( m_submit_buffer ) >= 4 + 2 ); //len=(4+2)~[True\r\n] OR len=(5+2)~[False\r\n]
                if ( std::strncmp( m_submit_buffer, "True", 4 ) == 0 ) {
                    assert( std::strlen( m_submit_buffer ) == 4 + 2 ); //len=(4+2)~[True"\r\n]"
                    return 0;
                }
                else {
                    assert( std::strncmp( m_submit_buffer, "False", 5 ) == 0
                           && std::strlen( m_submit_buffer ) == 5 + 2 ); //len=(5+2)~[False\r\n]
                    return 5;
                }
            }
        }
        return -1;
    }
    void SubmitSolution( const std::shared_ptr<CSolution> &solution, std::shared_ptr<CTarget> &target );
    void Communicate();
};

std::vector<std::tuple<std::string, std::string, std::string>> parse_pools_argv( const std::string& poolstr );
char g_miner_address[32] { DEFAULT_MINER_ADDRESS };
std::uint32_t g_miner_id { DEFAULT_MINER_ID };
std::uint32_t g_threads_count { DEFAULT_THREADS_COUNT };
std::uint32_t g_mined_block_count { 0 };
std::vector<std::thread> g_mine_threads;
std::vector<std::shared_ptr<CMineThread>> g_mine_objects;
std::vector<std::tuple<std::string, std::string, std::string>> g_mining_pools;
bool g_still_running { true };
bool g_solo_mining { false };

int main( int argc, char *argv[] ) {
    signal( SIGINT, []( int /* signum */ ) {
        if ( !g_still_running ) return;
        std::cout << "\nCtrl+C pressed! Wait for finishing all mining threads..." << std::endl;
        g_still_running = false; });
    #ifdef _WIN32
    WSADATA wsaData;
    if( WSAStartup( MAKEWORD( 2, 2 ), &wsaData ) != NO_ERROR ) {
        std::fprintf( stderr, "Error at WSAStartup\n" );
        std::exit( 1 );
    }
    #endif
    cxxopts::Options options( "noso-2m", "A miner for Nosocryptocurrency Protocol 2" );
    options.add_options()
        ( "a,address",  "An original noso wallet address",      cxxopts::value<std::string>()->default_value( DEFAULT_MINER_ADDRESS ) )
        ( "i,minerid",  "Miner ID - a number between 0-8100",   cxxopts::value<std::uint32_t>()->default_value( std::to_string( DEFAULT_MINER_ID ) ) )
        ( "t,threads",  "Number of threads use for mining",     cxxopts::value<std::uint32_t>()->default_value( std::to_string( DEFAULT_THREADS_COUNT ) ) )
        ( "p,pools",    "List of pools in pool mining mode",    cxxopts::value<std::string>()->default_value( "f04ever;devnoso;" ) )
        ( "s,solo",     "Mining mode solo? Default pool mining" )
        ( "v,version",  "Print current version" )
        ( "h,help",     "Print this usage" )
        ;
    auto result = options.parse( argc, argv );
    if ( result.count( "help" ) ) {
        std::cout << options.help() << std::endl;
        std::exit( 0 );
    }
    if ( result.count( "version" ) ) {
        std::cout << "version " << NOSO_2M_VERSION_MAJOR << "." << NOSO_2M_VERSION_MINOR << "." << NOSO_2M_VERSION_PATCH << std::endl;
        exit( 0 );
    }
    std::string miner_address = result["address"].as<std::string>();
    std::strcpy( g_miner_address, miner_address.c_str() );
    assert( std::strlen( g_miner_address ) == 30 || std::strlen( g_miner_address ) == 31 );
    if ( std::strlen( g_miner_address ) < 30 || std::strlen( g_miner_address ) > 31 ) std::exit(0);
    g_miner_id = result["minerid"].as<std::uint32_t>();
    assert( 0 <= g_miner_id && g_miner_id <= 8100 );
    if ( g_miner_id < 0 || g_miner_id > 8100 ) std::exit(0);
    g_threads_count = result["threads"].as<std::uint32_t>();
    g_mining_pools = parse_pools_argv( result["pools"].as<std::string>() );
    g_solo_mining = result.count( "solo" ) ? true : false;
    auto next_pool = g_mining_pools.cbegin();
    auto last_pool = g_mining_pools.cend();
    std::cout << "noso-2m - A miner for Nosocryptocurrency Protocol 2\n";
    std::cout << "by f04ever (c) 2022 @ https://github.com/f04ever/noso-2m\n";
    std::cout << "version " << NOSO_2M_VERSION_MAJOR << "." << NOSO_2M_VERSION_MINOR << "." << NOSO_2M_VERSION_PATCH << "\n";
    std::cout << "\n";
    std::cout << "- Wallet address: " << g_miner_address << std::endl;
    std::cout << "-       Miner ID: " << g_miner_id << std::endl;
    std::cout << "-  Threads count: " << g_threads_count << std::endl;
    std::cout << "-    Mining mode: " << ( g_solo_mining ? "solo" : "pool " + std::get<0>( *next_pool )
        + " \t(" + std::get<1>( *next_pool ) + ":" + std::get<2>( *next_pool ) + ")" ) << std::endl;
    if ( !g_solo_mining ) {
        for( next_pool = std::next( next_pool ); next_pool != last_pool; ++next_pool ) {
            std::cout << "                       " << ( std::get<0>( *next_pool )
                + " \t(" + std::get<1>( *next_pool ) + ":" + std::get<2>( *next_pool ) + ")" ) << std::endl;
        }
    }
    std::cout << "\n";
    std::cout << "Press Ctrl+C to stop" << std::endl;
    for ( std::uint32_t thread_id = 0; thread_id < g_threads_count - 1; ++thread_id )
        g_mine_objects.push_back( std::make_shared<CMineThread>( g_miner_id, thread_id ) );
    std::thread comm_thread( &CCommThread::Communicate, CCommThread::GetInstance() );
    for ( std::uint32_t thread_id = 0; thread_id < g_threads_count - 1; ++thread_id )
        g_mine_threads.emplace_back( &CMineThread::Mine, g_mine_objects[thread_id] );
    comm_thread.join();
    #ifdef _WIN32
    WSACleanup();
    #endif
    return 0;
}

#define COUT_NOSO_TIME std::cout << NOSO_TIMESTAMP << "(" << std::setfill('0') << std::setw(3) << NOSO_BLOCK_AGE << "))"

void CMineThread::Mine() {
    while ( g_still_running ) {
        while ( g_still_running && m_blck_no <= 0 ) {
            std::this_thread::sleep_for( std::chrono::milliseconds( static_cast<int>( 1000 * INET_CIRCLE_SECONDS ) ) );
        }
        if ( g_still_running && m_blck_no > 0 ) {
            assert( std::strlen( m_lb_hash ) == 32 );
            char best_diff[33];
            std::strcpy( best_diff, m_mn_diff );
            std::uint32_t noso_hash_counter { 0 };
            CNosoHasher noso_hasher( m_prefix, m_address );
            auto begin_mining { std::chrono::steady_clock::now() };
            while ( g_still_running && 1 <= NOSO_BLOCK_AGE && NOSO_BLOCK_AGE <= 585 ) {
                const char *base { noso_hasher.GetBase( noso_hash_counter++ ) };
                const char *hash { noso_hasher.GetHash() };
                const char *diff { noso_hasher.GetDiff( m_lb_hash ) };
                assert( std::strlen( base ) == 18 && std::strlen( hash ) == 32 && std::strlen( diff ) == 32 );
                if ( std::strcmp( diff, best_diff ) < 0 ) {
                    CCommThread::GetInstance()->AddSolution( std::make_shared<CSolution>( m_blck_no, base, hash, diff ) );
                    if ( g_solo_mining ) std::strcpy( best_diff, diff );
                }
            }
            std::chrono::duration<double> elapsed_mining { std::chrono::steady_clock::now() - begin_mining };
            this->UpdateComputedHashesCount( noso_hash_counter );
            m_blck_no = 0;
        } // END if ( g_still_running && m_blck_no > 0 ) {
    } // END while ( g_still_running ) {
}

CCommThread::CCommThread() {
    for( auto sn : g_default_nodes ) m_node_inets_good.push_back(
        std::make_shared<CNodeInet>( std::get<0>( sn ), std::get<1>( sn ), DEFAULT_NODE_INET_TIMEOSEC ) );
    for( auto dp : g_mining_pools ) m_pool_inets.push_back(
        std::make_shared<CPoolInet>( std::get<0>( dp ), std::get<1>( dp ), std::get<2>( dp ), DEFAULT_POOL_INET_TIMEOSEC ) );
    m_pool_inet_id = 0;
}

void CCommThread::_PrintBlockSummary( std::uint32_t blck_no, const std::chrono::duration<double>& elapsed_blck ) {
    std::uint32_t computed_hashes_count = std::accumulate(
            g_mine_objects.begin(), g_mine_objects.end(), 0,
            []( int a, const std::shared_ptr<CMineThread> &o ) {
                return a + o->GetComputedHashesCount(); } );
    std::cout << "SUMMARY BLOCK#" << blck_no << " : "
        << computed_hashes_count<< " hashes computed in "
        << std::fixed << std::setprecision(3)
        << elapsed_blck.count() / 60 << " minutes, hashrate approx. "
        << computed_hashes_count / elapsed_blck.count() / 1000 << " Kh/s\n\t"
        << " accepted " << m_accepted_solutions_count
        << " rejected " << m_rejected_solutions_count
        << " failured " << m_failured_solutions_count
        << " solution(s)" << std::endl;
    std::cout << "TOTAL MINED " << g_mined_block_count << " BLOCKS" << std::endl;
};

void CCommThread::_ResetMiningBlock() {
    m_accepted_solutions_count = 0;
    m_rejected_solutions_count = 0;
    m_failured_solutions_count = 0;
    this->ClearSolutions();
};

void CCommThread::_ReportErrorSubmitting( int code, const std::shared_ptr<CSolution> &solution ) {
    if      ( code == 1 ) {
        COUT_NOSO_TIME << "    ERROR"
            << ")blck[" << solution->blck
            << "]diff[" << solution->diff
            << "]hash[" << solution->hash
            << "]base[" << solution->base << "]Wrong block number " << solution->blck << " submitted!" << std::endl;
    } else if ( code == 2 ) {
        COUT_NOSO_TIME << "    ERROR"
            << ")blck[" << solution->blck
            << "]diff[" << solution->diff
            << "]hash[" << solution->hash
            << "]base[" << solution->base << "]Incorrect timestamp submitted!" << std::endl;
    } else if ( code == 3 ) {
        COUT_NOSO_TIME << "    ERROR"
            << ")blck[" << solution->blck
            << "]diff[" << solution->diff
            << "]hash[" << solution->hash
            << "]base[" << solution->base << "]Invalid address (" << g_miner_address << ")!" << std::endl;
    } else if ( code == 7 ) {
        COUT_NOSO_TIME << "    ERROR"
            << ")blck[" << solution->blck
            << "]diff[" << solution->diff
            << "]hash[" << solution->hash
            << "]base[" << solution->base << "]Wrong hash base submitted!" << std::endl;
    } else if ( code == 4 ) {
        COUT_NOSO_TIME << "    ERROR"
            << ")blck[" << solution->blck
            << "]diff[" << solution->diff
            << "]hash[" << solution->hash
            << "]base[" << solution->base << "]Duplicate hash submited!" << std::endl;
    }
}

std::shared_ptr<CTarget> CCommThread::GetTarget() {
    std::shared_ptr<CTarget> target = nullptr;
    if ( g_solo_mining ) {
        std::shared_ptr<CNodeTarget> nodedata = this->MakeConsensus();
        while ( g_still_running && nodedata == nullptr ) {
            COUT_NOSO_TIME << "WAITING CONSENSUS..." << std::endl;
            nodedata = this->MakeConsensus();
        }
        nodedata->prefix = ""; //TODO
        nodedata->address = g_miner_address;
        if ( nodedata->lb_addr == g_miner_address ) {
            g_mined_block_count++;
            COUT_NOSO_TIME << "YAY! YOU HAVE MINED BLOCK#" << nodedata->blck_no << std::endl;
        }
        return nodedata;
    } else {
        std::shared_ptr<CPoolTarget> pooldata = this->ReceivePoolData( g_miner_address );
        int trying_count = 1;
        while ( g_still_running && pooldata == nullptr ) {
            if ( trying_count > 5 ) {
                trying_count = 0;
                ++m_pool_inet_id;
                if ( m_pool_inet_id >= m_pool_inets.size() ) m_pool_inet_id = 0;
                if ( m_pool_inets.size() > 1 )
                    COUT_NOSO_TIME << "FAILOVER TO POOL "
                        << m_pool_inets[m_pool_inet_id]->m_name << " ("
                        << m_pool_inets[m_pool_inet_id]->m_host << ":"
                        << m_pool_inets[m_pool_inet_id]->m_port << ")" << std::endl;
            }
            COUT_NOSO_TIME << "WAITING POOL DATA... " << trying_count << std::endl;
            pooldata = this->ReceivePoolData( g_miner_address );
            ++trying_count;
        }
        COUT_NOSO_TIME << "MINING ON POOL "
            << m_pool_inets[m_pool_inet_id]->m_name << " ("
            << m_pool_inets[m_pool_inet_id]->m_host << ":"
            << m_pool_inets[m_pool_inet_id]->m_port << ")"
            << " YOUR BALANCE " << pooldata->balance << std::endl;
        return pooldata;
    }
}

std::shared_ptr<CSolution> CCommThread::GetSolution() {
    return g_solo_mining ? this->BestSolution() : this->GoodSolution();
}

void CCommThread::SubmitSolution( const std::shared_ptr<CSolution> &solution, std::shared_ptr<CTarget> &target ) {
    int code { 0 };
    if ( g_solo_mining ) {
        target->mn_diff = solution->diff;
        char new_mn_diff[33] { NOSO_MAX_DIFF };
        code = this->PushSolution( solution->blck, solution->base.c_str(), g_miner_address, new_mn_diff );
        if ( target->mn_diff > new_mn_diff ) target->mn_diff = new_mn_diff;
        if ( code == 6 ) {
            if ( solution->diff < new_mn_diff ) this->AddSolution( solution );
            COUT_NOSO_TIME << "WAITBLOCK"
                << ")blck[" << solution->blck
                << "]diff[" << solution->diff
                << "]hash[" << solution->hash
                << "]base[" << solution->base << "]Network building block!" << std::endl;
        }
        if ( code > 0 ) this->_ReportErrorSubmitting( code, solution ); // rest other error codes 1, 2, 3, 4, 7
    } else {
        code = this->SendSolution( solution->base.c_str(), g_miner_address );
    }
    if ( code == 0 ) {
        m_accepted_solutions_count ++;
        COUT_NOSO_TIME << " ACCEPTED"
            << ")blck[" << solution->blck
            << "]diff[" << solution->diff
            << "]hash[" << solution->hash
            << "]base[" << solution->base << "]" << std::endl;
    } else if ( code > 0) {
        m_rejected_solutions_count ++;
        COUT_NOSO_TIME << " REJECTED"
            << ")blck[" << solution->blck
            << "]diff[" << solution->diff
            << "]hash[" << solution->hash
            << "]base[" << solution->base << "]" << std::endl;
    } else {
        this->AddSolution( solution );
        m_failured_solutions_count ++;
        COUT_NOSO_TIME << " FAILURED"
            << ")blck[" << solution->blck
            << "]diff[" << solution->diff
            << "]hash[" << solution->hash
            << "]base[" << solution->base << "Will retry submitting!" << std::endl;
    }
}

void CCommThread::Communicate() {
    auto begin_blck = std::chrono::steady_clock::now();
    while ( g_still_running ) {
        if ( NOSO_BLOCK_AGE < 1 || 585 < NOSO_BLOCK_AGE ) {
            COUT_NOSO_TIME << "WAITBLOCK..." << std::flush;
            do {
                std::this_thread::sleep_for( std::chrono::milliseconds( static_cast<int>( 1000 * INET_CIRCLE_SECONDS ) ) );
            } while ( g_still_running && ( NOSO_BLOCK_AGE < 1 || 585 < NOSO_BLOCK_AGE ) );
            std::cout << std::endl;
            if ( !g_still_running ) break;
        }
        std::shared_ptr<CTarget> target = this->GetTarget();
        if ( !g_still_running ) break;
        std::cout << "-----------------------------------------------------------------------------------------------------------------" << std::endl;
        COUT_NOSO_TIME << "NEWTARGET"
            << ")blck[" << target->blck_no + 1
            << "]hash[" << target->lb_hash
            << "]diff[" << target->mn_diff
            << "]pref[" << target->prefix
            << "]addr[" << target->address
            << "]" << std::endl;
        begin_blck = std::chrono::steady_clock::now();
        for ( auto mo : g_mine_objects ) mo->NewTarget( target );
        while ( g_still_running && NOSO_BLOCK_AGE <= 585 ) {
            auto begin_submit = std::chrono::steady_clock::now();
            if ( NOSO_BLOCK_AGE >= 10 ) {
                std::shared_ptr<CSolution> solution = this->GetSolution();
                if ( solution != nullptr && solution->diff < target->mn_diff )
                    this->SubmitSolution( solution, target );
            }
            std::chrono::duration<double> elapsed_submit = std::chrono::steady_clock::now() - begin_submit;
            if ( elapsed_submit.count() < INET_CIRCLE_SECONDS ) {
                std::this_thread::sleep_for( std::chrono::milliseconds( static_cast<int>( 1000 * INET_CIRCLE_SECONDS ) ) );
            }
        } // END while ( g_still_running && NOSO_BLOCK_AGE <= 585 ) {
        std::chrono::duration<double> elapsed_blck = std::chrono::steady_clock::now() - begin_blck;
        while ( g_still_running && NOSO_BLOCK_AGE < 586 ) {
            std::this_thread::sleep_for( std::chrono::milliseconds( static_cast<int>( 1000 * INET_CIRCLE_SECONDS ) ) );
        }
        this->_PrintBlockSummary( target->blck_no, elapsed_blck );
        this->_ResetMiningBlock();
    } // END while ( g_still_running ) {
    for ( auto &thr : g_mine_threads ) thr.join();
}

std::vector<std::tuple<std::string, std::string, std::string>> parse_pools_argv( const std::string& poolstr ) {
    std::cout << poolstr << std::endl;
    const std::regex re_pool1 { ";" };
    const std::regex re_pool2 {
        "("
            "[a-zA-Z][a-zA-Z0-9\\-]*[a-zA-Z0-9]"
        ")"
        "(\\:"
            "("
                // "("
                    "(25[0-5]|2[0-4][0-9]|1[0-9]{2}|[1-9][0-9]|[0-9])\\."
                    "(25[0-5]|2[0-4][0-9]|1[0-9]{2}|[1-9][0-9]|[0-9])\\."
                    "(25[0-5]|2[0-4][0-9]|1[0-9]{2}|[1-9][0-9]|[0-9])\\."
                    "(25[0-5]|2[0-4][0-9]|1[0-9]{2}|[1-9][0-9]|[0-9])"
                // ")"
                // "|"
                // "("
                //     "(([a-zA-Z0-9]+[a-zA-Z0-9\\-]*[a-zA-Z0-9])\\.)*"
                //      "([a-zA-Z0-9]+[a-zA-Z0-9\\-]*[a-zA-Z0-9])"
                // ")"
            ")"
            "(\\:"
                "("
                    "6553[0-5]|655[0-2][0-9]|65[0-4][0-9]{2}|6[0-4][0-9]{3}|[1-5][0-9]{4}|[0-5]{0,5}|[0-9]{1,4}"
                ")$"
            ")?"
        ")?"
    };
    std::vector<std::tuple<std::string, std::string, std::string>> mining_pools;
    std::for_each(
            std::sregex_token_iterator { poolstr.begin(), poolstr.end(), re_pool1 , -1 },
            std::sregex_token_iterator {}, [&]( const auto &tok ) {
                std::string pls { tok.str() };
                std::for_each(
                        std::sregex_iterator { pls.begin(), pls.end(), re_pool2 },
                        std::sregex_iterator {}, [&]( const auto &sm0 ) {
                            std::string name { sm0[1].str() };
                            std::string host { sm0[3].str() };
                            std::string port { sm0[14].str() };
                            if ( host.length() <= 0 ) {
                                const auto pool = std::find_if(
                                        g_default_pools.begin(), g_default_pools.end(),
                                        [&name]( const auto& val ) { return std::get<0>( val ) == name ? true : false; } );
                                if ( pool != g_default_pools.end() ) {
                                    host = std::get<1>( *pool );
                                    port = std::get<2>( *pool );
                                }
                            }
                            if ( host.length() <= 0 ) return;
                            if ( port.length() <= 0 ) port = std::string { "8082" };
                            mining_pools.push_back( { name, host, port } );
                        }
                    );
            }
        );
    if  (mining_pools.size() <= 0 )
        for( const auto& dp : g_default_pools )
            mining_pools.push_back( { std::get<0>( dp ), std::get<1>( dp ), std::get<2>( dp ) } );
    return mining_pools;
}

int inet_socket( struct addrinfo *serv_info, int timeosec ) {
    struct addrinfo *psi = serv_info;
    struct timeval timeout {
        .tv_sec = timeosec,
        .tv_usec = 0
    };
    int sockfd, rc;
    fd_set rset, wset;
    for( ; psi != NULL; psi = psi->ai_next ) {
        if ( (sockfd = socket( psi->ai_family, psi->ai_socktype,
                               psi->ai_protocol ) ) == -1 ) {
            std::perror( "socket: error" );
            continue;
        }
        #ifdef _WIN32
        u_long iMode = 1;
        if ( ioctlsocket( sockfd, FIONBIO, &iMode ) != NO_ERROR ) {
            closesocket( sockfd );
            std::perror( "ioctlsocket/socket failed" );
            continue;
        }
        #else
        int flags = 0;
        if ( ( flags = fcntl( sockfd, F_GETFL, 0 ) ) < 0 ) {
            close( sockfd );
            std::perror( "fcntl/socket failed" );
            continue;
        }
        if ( fcntl( sockfd, F_SETFL, flags | O_NONBLOCK ) < 0 ) {
            close( sockfd );
            std::perror( "fcntl/socket failed" );
            continue;
        }
        #endif
        if ( ( rc = connect( sockfd, psi->ai_addr, psi->ai_addrlen ) ) >= 0 ) {
            return sockfd;
        }
        #ifdef _WIN32
        if ( rc != SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK ) {
            closesocket( sockfd );
            std::perror( "connect/socket failed" );
            continue;
        }
        #else
        if ( errno != EINPROGRESS ) {
            close( sockfd );
            std::perror( "connect/socket failed" );
            continue;
        }
        #endif
        FD_ZERO( &rset );
        FD_ZERO( &wset );
        FD_SET( sockfd, &rset );
        FD_SET( sockfd, &wset );
        int n = select( sockfd + 1, &rset, &wset, NULL, &timeout );
        if ( n == 0 ) {
            #ifdef _WIN32
            closesocket( sockfd );
            #else
            close( sockfd );
            #endif
            std::perror("select/socket timeout");
            continue;
        }
        if ( n == -1 ) {
            #ifdef _WIN32
            closesocket( sockfd );
            #else
            close( sockfd );
            #endif
            std::perror( "select/socket failed" );
            continue;
        }
        if ( FD_ISSET( sockfd, &rset ) || FD_ISSET( sockfd, &wset ) ) {
            int error = 0;
            socklen_t slen = sizeof( error );
            #ifdef _WIN32
            if ( getsockopt( sockfd, SOL_SOCKET, SO_ERROR, (char*)&error, &slen ) < 0 ) {
            #else
            if ( getsockopt( sockfd, SOL_SOCKET, SO_ERROR, (char*)&error, &slen ) < 0 ) {
            #endif
                #ifdef _WIN32
                closesocket( sockfd );
                #else
                close( sockfd );
                #endif
                std::perror( "getsockopt/socket: failed" );
                continue;
            }
            if ( error ) {
                #ifdef _WIN32
                closesocket( sockfd );
                #else
                close( sockfd );
                #endif
                std::perror( "getsockopt/socket failed" );
                continue;
            }
        }
        else {
            std::perror( "select/socket failed" );
            continue;
        }
        return sockfd;
    } // END for( ; psi != NULL; psi = psi->ai_next ) {
    return -1;
}

int inet_send( int sockfd, int timeosec, const char *message, size_t size ) {
    struct timeval timeout {
        .tv_sec = timeosec,
        .tv_usec = 0
    };
    fd_set fds;
    FD_ZERO( &fds );
    FD_SET( sockfd, &fds );
    int n = select( sockfd + 1, NULL, &fds, NULL, &timeout );
    if ( n == 0 ) {
        #ifdef _WIN32
        closesocket( sockfd );
        #else
        close( sockfd );
        #endif
        std::perror( "select/send timeout" );
        return -2; // timeout!
    }
    if ( n == -1 ) {
        #ifdef _WIN32
        closesocket( sockfd );
        #else
        close( sockfd );
        #endif
        std::perror( "select/send failed" );
        return -1; // error
    }
    int slen = send( sockfd, message, size, 0 );
    if ( slen < 1 ) {
        #ifdef _WIN32
        closesocket( sockfd );
        #else
        close( sockfd );
        #endif
        std::perror( "send failed" );
        return -1;
    }
    return slen;
}

int inet_recv( int sockfd, int timeosec, char *buffer, size_t buffsize ) {
    struct timeval timeout {
        .tv_sec = timeosec,
        .tv_usec = 0
    };
    fd_set fds;
    FD_ZERO( &fds );
    FD_SET( sockfd, &fds );
    int n = select( sockfd + 1, &fds, NULL, NULL, &timeout );
    if ( n == 0 ) {
        #ifdef _WIN32
        closesocket( sockfd );
        #else
        close( sockfd );
        #endif
        std::perror( "select/recv timeout" );
        return -2; // timeout!
    }
    if ( n == -1 ) {
        #ifdef _WIN32
        closesocket( sockfd );
        #else
        close( sockfd );
        #endif
        std::perror( "select/recv failed" );
        return -1; // error
    }
    int rlen = recv( sockfd, buffer, buffsize - 1, 0 );
    // if (rlen < 0) { //TODO rlen == 1
    if ( rlen < 1 ) {
        #ifdef _WIN32
        closesocket( sockfd );
        #else
        close( sockfd );
        #endif
        if ( rlen == 0 ) std::perror( "recv timeout" );
        else  std::perror( "recv failed" );
        return -1;
    }
    buffer[ rlen ] = '\0';
    return rlen;
}

int inet_command( struct addrinfo *serv_info, uint32_t timeosec, char *buffer, size_t buffsize ) {
    int sockfd = inet_socket( serv_info, timeosec );
    if (sockfd < 0) return -1;
    int slen = inet_send( sockfd, timeosec, buffer, std::strlen( buffer ) );
    if ( slen < 0 ) return slen;
    int rlen = inet_recv( sockfd, timeosec, buffer, buffsize );
    if ( rlen < 0 ) return rlen;
    #ifdef _WIN32
    closesocket( sockfd );
    #else
    close( sockfd );
    #endif
    return rlen;
}
