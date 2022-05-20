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
#include <fstream>
#include <iomanip>
#include <cassert>
#include <iostream>
#include <condition_variable>
#include <signal.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#endif // _WIN32
#include "md5-c.hpp"
#include "cxxopts.hpp"

#define NOSO_2M_VERSION_MAJOR 0
#define NOSO_2M_VERSION_MINOR 2
#define NOSO_2M_VERSION_PATCH 3

#define DEFAULT_CONFIG_FILENAME "noso-2m.cfg"
#define DEFAULT_LOGGING_FILENAME "noso-2m.log"
#define DEFAULT_POOL_URL_LIST "f04ever;devnoso"
#define DEFAULT_MINER_ADDRESS "N3G1HhkpXvmLcsWFXySdAxX3GZpkMFS"
#define DEFAULT_MINER_ID 0
#define DEFAULT_THREADS_COUNT 2
#define DEFAULT_NODE_INET_TIMEOSEC 10
#define DEFAULT_POOL_INET_TIMEOSEC 60
#define DEFAULT_TIMESTAMP_DIFFERENCES 3

#define CONSENSUS_NODES_COUNT 3
#define INET_CIRCLE_SECONDS 0.1
#define INET_BUFFER_SIZE 1024

#define NOSO_NUL_HASH "00000000000000000000000000000000"
#define NOSO_MAX_DIFF "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
#define NOSO_TIMESTAMP ( (long long)( std::time( 0 ) ) )
#define NOSO_BLOCK_AGE ( NOSO_TIMESTAMP % 600 )

static const std::vector<std::tuple<std::string, std::string>> g_default_nodes {
        { "45.146.252.103"  ,   "8080" },
        { "109.230.238.240" ,   "8080" },
        { "194.156.88.117"  ,   "8080" },
        { "23.94.21.83"     ,   "8080" },
        { "107.175.59.177"  ,   "8080" },
        { "107.172.193.176" ,   "8080" },
        { "107.175.194.151" ,   "8080" },
        { "192.3.73.184"    ,   "8080" },
        { "107.175.24.151"  ,   "8080" },
        { "107.174.137.27"  ,   "8080" },
    }; // seed nodes

static const std::vector<std::tuple<std::string, std::string, std::string>> g_default_pools {
        { "f04ever", "209.126.80.203", "8082" },
        { "devnoso", "45.146.252.103", "8082" },
    };

constexpr static const char NOSOHASH_HASHEABLE_CHARS[] {
    "!\"#$%&')*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~" };
constexpr static const std::size_t NOSOHASH_HASHEABLE_COUNT =  92;
inline std::string nosohash_prefix( int num ) {
    return std::string {
        NOSOHASH_HASHEABLE_CHARS[ num / NOSOHASH_HASHEABLE_COUNT ],
        NOSOHASH_HASHEABLE_CHARS[ num % NOSOHASH_HASHEABLE_COUNT ], };
}

class CNosoHasher {
private:
    char m_base[19];
    char m_hash[33];
    char m_diff[33];
    char m_stat[129][128];
    MD5Context m_md5_ctx;
    constexpr static const char hex_dec2char_table[] {
'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F' };
    constexpr static const char hex_char2dec_table[] {
 0,
 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
 0,   0,   0,   0,   0,   0,   0,   0,   1,   2,
 3,   4,   5,   6,   7,   8,   9,   0,   0,   0,
 0,   0,   0,   0,  10,  11,  12,  13,  14,  15, };
    constexpr static std::uint16_t nosohash_chars_table[505] {
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
        int row, col, row1;
        for( row = 1; row < 129; row++ ) {
            row1 = row-1;
            for( col = 0; col < 127; col++ )
                m_stat[row][col] = nosohash_chars_table[ m_stat[row1][col] + m_stat[row1][col+1] ];
            m_stat[row][127] = nosohash_chars_table[ m_stat[row1][127] + m_stat[row1][0] ];
        }
        m_hash[ 0] = hex_dec2char_table[ nosohash_chars_table[ m_stat[128][  0] + m_stat[128][  1] + m_stat[128][  2] + m_stat[128][  3] ] % 16 ];
        m_hash[ 1] = hex_dec2char_table[ nosohash_chars_table[ m_stat[128][  4] + m_stat[128][  5] + m_stat[128][  6] + m_stat[128][  7] ] % 16 ];
        m_hash[ 2] = hex_dec2char_table[ nosohash_chars_table[ m_stat[128][  8] + m_stat[128][  9] + m_stat[128][ 10] + m_stat[128][ 11] ] % 16 ];
        m_hash[ 3] = hex_dec2char_table[ nosohash_chars_table[ m_stat[128][ 12] + m_stat[128][ 13] + m_stat[128][ 14] + m_stat[128][ 15] ] % 16 ];
        m_hash[ 4] = hex_dec2char_table[ nosohash_chars_table[ m_stat[128][ 16] + m_stat[128][ 17] + m_stat[128][ 18] + m_stat[128][ 19] ] % 16 ];
        m_hash[ 5] = hex_dec2char_table[ nosohash_chars_table[ m_stat[128][ 20] + m_stat[128][ 21] + m_stat[128][ 22] + m_stat[128][ 23] ] % 16 ];
        m_hash[ 6] = hex_dec2char_table[ nosohash_chars_table[ m_stat[128][ 24] + m_stat[128][ 25] + m_stat[128][ 26] + m_stat[128][ 27] ] % 16 ];
        m_hash[ 7] = hex_dec2char_table[ nosohash_chars_table[ m_stat[128][ 28] + m_stat[128][ 29] + m_stat[128][ 30] + m_stat[128][ 31] ] % 16 ];
        m_hash[ 8] = hex_dec2char_table[ nosohash_chars_table[ m_stat[128][ 32] + m_stat[128][ 33] + m_stat[128][ 34] + m_stat[128][ 35] ] % 16 ];
        m_hash[ 9] = hex_dec2char_table[ nosohash_chars_table[ m_stat[128][ 36] + m_stat[128][ 37] + m_stat[128][ 38] + m_stat[128][ 39] ] % 16 ];
        m_hash[10] = hex_dec2char_table[ nosohash_chars_table[ m_stat[128][ 40] + m_stat[128][ 41] + m_stat[128][ 42] + m_stat[128][ 43] ] % 16 ];
        m_hash[11] = hex_dec2char_table[ nosohash_chars_table[ m_stat[128][ 44] + m_stat[128][ 45] + m_stat[128][ 46] + m_stat[128][ 47] ] % 16 ];
        m_hash[12] = hex_dec2char_table[ nosohash_chars_table[ m_stat[128][ 48] + m_stat[128][ 49] + m_stat[128][ 50] + m_stat[128][ 51] ] % 16 ];
        m_hash[13] = hex_dec2char_table[ nosohash_chars_table[ m_stat[128][ 52] + m_stat[128][ 53] + m_stat[128][ 54] + m_stat[128][ 55] ] % 16 ];
        m_hash[14] = hex_dec2char_table[ nosohash_chars_table[ m_stat[128][ 56] + m_stat[128][ 57] + m_stat[128][ 58] + m_stat[128][ 59] ] % 16 ];
        m_hash[15] = hex_dec2char_table[ nosohash_chars_table[ m_stat[128][ 60] + m_stat[128][ 61] + m_stat[128][ 62] + m_stat[128][ 63] ] % 16 ];
        m_hash[16] = hex_dec2char_table[ nosohash_chars_table[ m_stat[128][ 64] + m_stat[128][ 65] + m_stat[128][ 66] + m_stat[128][ 67] ] % 16 ];
        m_hash[17] = hex_dec2char_table[ nosohash_chars_table[ m_stat[128][ 68] + m_stat[128][ 69] + m_stat[128][ 70] + m_stat[128][ 71] ] % 16 ];
        m_hash[18] = hex_dec2char_table[ nosohash_chars_table[ m_stat[128][ 72] + m_stat[128][ 73] + m_stat[128][ 74] + m_stat[128][ 75] ] % 16 ];
        m_hash[19] = hex_dec2char_table[ nosohash_chars_table[ m_stat[128][ 76] + m_stat[128][ 77] + m_stat[128][ 78] + m_stat[128][ 79] ] % 16 ];
        m_hash[20] = hex_dec2char_table[ nosohash_chars_table[ m_stat[128][ 80] + m_stat[128][ 81] + m_stat[128][ 82] + m_stat[128][ 83] ] % 16 ];
        m_hash[21] = hex_dec2char_table[ nosohash_chars_table[ m_stat[128][ 84] + m_stat[128][ 85] + m_stat[128][ 86] + m_stat[128][ 87] ] % 16 ];
        m_hash[22] = hex_dec2char_table[ nosohash_chars_table[ m_stat[128][ 88] + m_stat[128][ 89] + m_stat[128][ 90] + m_stat[128][ 91] ] % 16 ];
        m_hash[23] = hex_dec2char_table[ nosohash_chars_table[ m_stat[128][ 92] + m_stat[128][ 93] + m_stat[128][ 94] + m_stat[128][ 95] ] % 16 ];
        m_hash[24] = hex_dec2char_table[ nosohash_chars_table[ m_stat[128][ 96] + m_stat[128][ 97] + m_stat[128][ 98] + m_stat[128][ 99] ] % 16 ];
        m_hash[25] = hex_dec2char_table[ nosohash_chars_table[ m_stat[128][100] + m_stat[128][101] + m_stat[128][102] + m_stat[128][103] ] % 16 ];
        m_hash[26] = hex_dec2char_table[ nosohash_chars_table[ m_stat[128][104] + m_stat[128][105] + m_stat[128][106] + m_stat[128][107] ] % 16 ];
        m_hash[27] = hex_dec2char_table[ nosohash_chars_table[ m_stat[128][108] + m_stat[128][109] + m_stat[128][110] + m_stat[128][111] ] % 16 ];
        m_hash[28] = hex_dec2char_table[ nosohash_chars_table[ m_stat[128][112] + m_stat[128][113] + m_stat[128][114] + m_stat[128][115] ] % 16 ];
        m_hash[29] = hex_dec2char_table[ nosohash_chars_table[ m_stat[128][116] + m_stat[128][117] + m_stat[128][118] + m_stat[128][119] ] % 16 ];
        m_hash[30] = hex_dec2char_table[ nosohash_chars_table[ m_stat[128][120] + m_stat[128][121] + m_stat[128][122] + m_stat[128][123] ] % 16 ];
        m_hash[31] = hex_dec2char_table[ nosohash_chars_table[ m_stat[128][124] + m_stat[128][125] + m_stat[128][126] + m_stat[128][127] ] % 16 ];
        m_hash[32] = '\0';
    }
    inline void _md5() {
        assert( std::strlen( m_hash ) == 32 );
        md5Init( &m_md5_ctx );
        md5Update( &m_md5_ctx, (uint8_t *)m_hash, 32 );
        md5Finalize( &m_md5_ctx );
        m_hash[ 0] = hex_dec2char_table[m_md5_ctx.digest[ 0] >>  4];
        m_hash[ 1] = hex_dec2char_table[m_md5_ctx.digest[ 0] & 0xF];
        m_hash[ 2] = hex_dec2char_table[m_md5_ctx.digest[ 1] >>  4];
        m_hash[ 3] = hex_dec2char_table[m_md5_ctx.digest[ 1] & 0xF];
        m_hash[ 4] = hex_dec2char_table[m_md5_ctx.digest[ 2] >>  4];
        m_hash[ 5] = hex_dec2char_table[m_md5_ctx.digest[ 2] & 0xF];
        m_hash[ 6] = hex_dec2char_table[m_md5_ctx.digest[ 3] >>  4];
        m_hash[ 7] = hex_dec2char_table[m_md5_ctx.digest[ 3] & 0xF];
        m_hash[ 8] = hex_dec2char_table[m_md5_ctx.digest[ 4] >>  4];
        m_hash[ 9] = hex_dec2char_table[m_md5_ctx.digest[ 4] & 0xF];
        m_hash[10] = hex_dec2char_table[m_md5_ctx.digest[ 5] >>  4];
        m_hash[11] = hex_dec2char_table[m_md5_ctx.digest[ 5] & 0xF];
        m_hash[12] = hex_dec2char_table[m_md5_ctx.digest[ 6] >>  4];
        m_hash[13] = hex_dec2char_table[m_md5_ctx.digest[ 6] & 0xF];
        m_hash[14] = hex_dec2char_table[m_md5_ctx.digest[ 7] >>  4];
        m_hash[15] = hex_dec2char_table[m_md5_ctx.digest[ 7] & 0xF];
        m_hash[16] = hex_dec2char_table[m_md5_ctx.digest[ 8] >>  4];
        m_hash[17] = hex_dec2char_table[m_md5_ctx.digest[ 8] & 0xF];
        m_hash[18] = hex_dec2char_table[m_md5_ctx.digest[ 9] >>  4];
        m_hash[19] = hex_dec2char_table[m_md5_ctx.digest[ 9] & 0xF];
        m_hash[20] = hex_dec2char_table[m_md5_ctx.digest[10] >>  4];
        m_hash[21] = hex_dec2char_table[m_md5_ctx.digest[10] & 0xF];
        m_hash[22] = hex_dec2char_table[m_md5_ctx.digest[11] >>  4];
        m_hash[23] = hex_dec2char_table[m_md5_ctx.digest[11] & 0xF];
        m_hash[24] = hex_dec2char_table[m_md5_ctx.digest[12] >>  4];
        m_hash[25] = hex_dec2char_table[m_md5_ctx.digest[12] & 0xF];
        m_hash[26] = hex_dec2char_table[m_md5_ctx.digest[13] >>  4];
        m_hash[27] = hex_dec2char_table[m_md5_ctx.digest[13] & 0xF];
        m_hash[28] = hex_dec2char_table[m_md5_ctx.digest[14] >>  4];
        m_hash[29] = hex_dec2char_table[m_md5_ctx.digest[14] & 0xF];
        m_hash[30] = hex_dec2char_table[m_md5_ctx.digest[15] >>  4];
        m_hash[31] = hex_dec2char_table[m_md5_ctx.digest[15] & 0xF];
        m_hash[32] = '\0';
    }
public:
    CNosoHasher( const char prefix[10], const char address[32] ) {
        constexpr static const char NOSOHASH_FILLER_CHARS[] = "%)+/5;=CGIOSYaegk";
        constexpr static const int NOSOHASH_FILLER_COUNT = 17;
        assert( std::strlen( prefix ) == 9
               && ( std::strlen( address ) == 30 || std::strlen( address ) == 31 ) );
        std::memcpy( m_base, prefix, 9 );
        std::sprintf( m_base + 9, "%09d", 0 );
        assert( std::strlen( m_base ) == 18 );
        int addr_len = 30 + (address[30] ? 1 : 0);
        std::memcpy( m_stat[0], m_base, 9 );
        std::memcpy( m_stat[0] + 9, m_base + 9, 9 );
        std::memcpy( m_stat[0] + 9 + 9, address, addr_len );
        int len = 18 + addr_len;
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
        std::sprintf( m_base + 9, "%09d", counter );
        assert( std::strlen( m_base ) == 18 );
        std::memcpy( m_stat[0] + 9, m_base + 9, 9 );
        assert( std::none_of( m_stat[0], m_stat[0] + 128, []( int c ){ return 33 > c || c > 126; } ) );
        return m_base;
    }
    const char* GetHash() {
        this->_hash();
        this->_md5();
        return m_hash;
    }
    const char* GetDiff( const char target[33] ) {
        assert( std::strlen( m_hash ) == 32 && std::strlen( target ) == 32 );
        m_diff[ 0] = hex_dec2char_table[ std::abs( hex_char2dec_table[ (int)m_hash[ 0] ] - hex_char2dec_table[ (int)target[ 0] ] ) ];
        m_diff[ 1] = hex_dec2char_table[ std::abs( hex_char2dec_table[ (int)m_hash[ 1] ] - hex_char2dec_table[ (int)target[ 1] ] ) ];
        m_diff[ 2] = hex_dec2char_table[ std::abs( hex_char2dec_table[ (int)m_hash[ 2] ] - hex_char2dec_table[ (int)target[ 2] ] ) ];
        m_diff[ 3] = hex_dec2char_table[ std::abs( hex_char2dec_table[ (int)m_hash[ 3] ] - hex_char2dec_table[ (int)target[ 3] ] ) ];
        m_diff[ 4] = hex_dec2char_table[ std::abs( hex_char2dec_table[ (int)m_hash[ 4] ] - hex_char2dec_table[ (int)target[ 4] ] ) ];
        m_diff[ 5] = hex_dec2char_table[ std::abs( hex_char2dec_table[ (int)m_hash[ 5] ] - hex_char2dec_table[ (int)target[ 5] ] ) ];
        m_diff[ 6] = hex_dec2char_table[ std::abs( hex_char2dec_table[ (int)m_hash[ 6] ] - hex_char2dec_table[ (int)target[ 6] ] ) ];
        m_diff[ 7] = hex_dec2char_table[ std::abs( hex_char2dec_table[ (int)m_hash[ 7] ] - hex_char2dec_table[ (int)target[ 7] ] ) ];
        m_diff[ 8] = hex_dec2char_table[ std::abs( hex_char2dec_table[ (int)m_hash[ 8] ] - hex_char2dec_table[ (int)target[ 8] ] ) ];
        m_diff[ 9] = hex_dec2char_table[ std::abs( hex_char2dec_table[ (int)m_hash[ 9] ] - hex_char2dec_table[ (int)target[ 9] ] ) ];
        m_diff[10] = hex_dec2char_table[ std::abs( hex_char2dec_table[ (int)m_hash[10] ] - hex_char2dec_table[ (int)target[10] ] ) ];
        m_diff[11] = hex_dec2char_table[ std::abs( hex_char2dec_table[ (int)m_hash[11] ] - hex_char2dec_table[ (int)target[11] ] ) ];
        m_diff[12] = hex_dec2char_table[ std::abs( hex_char2dec_table[ (int)m_hash[12] ] - hex_char2dec_table[ (int)target[12] ] ) ];
        m_diff[13] = hex_dec2char_table[ std::abs( hex_char2dec_table[ (int)m_hash[13] ] - hex_char2dec_table[ (int)target[13] ] ) ];
        m_diff[14] = hex_dec2char_table[ std::abs( hex_char2dec_table[ (int)m_hash[14] ] - hex_char2dec_table[ (int)target[14] ] ) ];
        m_diff[15] = hex_dec2char_table[ std::abs( hex_char2dec_table[ (int)m_hash[15] ] - hex_char2dec_table[ (int)target[15] ] ) ];
        m_diff[16] = hex_dec2char_table[ std::abs( hex_char2dec_table[ (int)m_hash[16] ] - hex_char2dec_table[ (int)target[16] ] ) ];
        m_diff[17] = hex_dec2char_table[ std::abs( hex_char2dec_table[ (int)m_hash[17] ] - hex_char2dec_table[ (int)target[17] ] ) ];
        m_diff[18] = hex_dec2char_table[ std::abs( hex_char2dec_table[ (int)m_hash[18] ] - hex_char2dec_table[ (int)target[18] ] ) ];
        m_diff[19] = hex_dec2char_table[ std::abs( hex_char2dec_table[ (int)m_hash[19] ] - hex_char2dec_table[ (int)target[19] ] ) ];
        m_diff[20] = hex_dec2char_table[ std::abs( hex_char2dec_table[ (int)m_hash[20] ] - hex_char2dec_table[ (int)target[20] ] ) ];
        m_diff[21] = hex_dec2char_table[ std::abs( hex_char2dec_table[ (int)m_hash[21] ] - hex_char2dec_table[ (int)target[21] ] ) ];
        m_diff[22] = hex_dec2char_table[ std::abs( hex_char2dec_table[ (int)m_hash[22] ] - hex_char2dec_table[ (int)target[22] ] ) ];
        m_diff[23] = hex_dec2char_table[ std::abs( hex_char2dec_table[ (int)m_hash[23] ] - hex_char2dec_table[ (int)target[23] ] ) ];
        m_diff[24] = hex_dec2char_table[ std::abs( hex_char2dec_table[ (int)m_hash[24] ] - hex_char2dec_table[ (int)target[24] ] ) ];
        m_diff[25] = hex_dec2char_table[ std::abs( hex_char2dec_table[ (int)m_hash[25] ] - hex_char2dec_table[ (int)target[25] ] ) ];
        m_diff[26] = hex_dec2char_table[ std::abs( hex_char2dec_table[ (int)m_hash[26] ] - hex_char2dec_table[ (int)target[26] ] ) ];
        m_diff[27] = hex_dec2char_table[ std::abs( hex_char2dec_table[ (int)m_hash[27] ] - hex_char2dec_table[ (int)target[27] ] ) ];
        m_diff[28] = hex_dec2char_table[ std::abs( hex_char2dec_table[ (int)m_hash[28] ] - hex_char2dec_table[ (int)target[28] ] ) ];
        m_diff[29] = hex_dec2char_table[ std::abs( hex_char2dec_table[ (int)m_hash[29] ] - hex_char2dec_table[ (int)target[29] ] ) ];
        m_diff[30] = hex_dec2char_table[ std::abs( hex_char2dec_table[ (int)m_hash[30] ] - hex_char2dec_table[ (int)target[30] ] ) ];
        m_diff[31] = hex_dec2char_table[ std::abs( hex_char2dec_table[ (int)m_hash[31] ] - hex_char2dec_table[ (int)target[31] ] ) ];
        m_diff[32] = '\0';
        return m_diff;
    }
};
constexpr const char CNosoHasher::hex_dec2char_table[];
constexpr const char CNosoHasher::hex_char2dec_table[];
constexpr const std::uint16_t CNosoHasher::nosohash_chars_table[];

class CInet {
public:
    const std::string m_host;
    const std::string m_port;
    const int m_timeosec;
    CInet( const std::string &host, const std::string &port, int timeosec )
        :   m_host { host }, m_port { port }, m_timeosec( timeosec ) {
    }
private:
    struct addrinfo * InitService();
    static void CleanService( struct addrinfo * serv_info );
public:
    int ExecCommand( char *buffer, std::size_t buffsize );
};

class CNodeInet : public CInet {
public:
    CNodeInet( const std::string &host, const std::string &port , int timeosec )
        :   CInet( host, port, timeosec ) {
    }
    int RequestTimestamp( char *buffer, std::size_t buffsize ) {
        assert( buffer && buffsize > 0 );
        std::strcpy( buffer, "NSLTIME\n" );
        return this->ExecCommand( buffer, buffsize );
    }
    int RequestSource( char *buffer, std::size_t buffsize ) {
        assert( buffer && buffsize > 0 );
        std::strcpy( buffer, "NODESTATUS\n" );
        return this->ExecCommand( buffer, buffsize );
    }
    int SubmitSolution( std::uint32_t blck, const char base[19], const char address[32],
                        char *buffer, std::size_t buffsize ) {
        assert( buffer && buffsize > 0 );
        assert( std::strlen( base ) == 18
               && ( std::strlen( address ) == 30 || std::strlen( address ) == 31 ) );
        std::snprintf( buffer, buffsize, "BESTHASH 1 2 3 4 %s %s %d %lld\n", address, base, blck, NOSO_TIMESTAMP );
        return this->ExecCommand( buffer, buffsize );
    }
};

class CPoolInet : public CInet {
public:
    std::string m_name;
    CPoolInet( const std::string& name, const std::string &host, const std::string &port , int timeosec )
        :   CInet( host, port, timeosec ), m_name { name } {
    }
    int RequestSource( const char address[32], char *buffer, std::size_t buffsize ) {
        assert( buffer && buffsize > 0 );
        assert( std::strlen( address ) == 30 || std::strlen( address ) == 31 );
        // SOURCE {address} {MinerName}
        std::snprintf( buffer, buffsize, "SOURCE %s noso-2m-v%d.%d.%d\n", address,
                      NOSO_2M_VERSION_MAJOR, NOSO_2M_VERSION_MINOR, NOSO_2M_VERSION_PATCH );
        return this->ExecCommand( buffer, buffsize );
    }
    int SubmitSolution( std::uint32_t blck_no, const char base[19], const char address[32],
                        char *buffer, std::size_t buffsize ) {
        assert( buffer && buffsize > 0 );
        assert( std::strlen( base ) == 18
               && std::strlen( address ) == 30 || std::strlen( address ) == 31 );
        // SHARE {Address} {Hash} {MinerName}
        std::snprintf( buffer, buffsize, "SHARE %s %s noso-2m-v%d.%d.%d %d\n", address, base,
                      NOSO_2M_VERSION_MAJOR, NOSO_2M_VERSION_MINOR, NOSO_2M_VERSION_PATCH, blck_no );
        return this->ExecCommand( buffer, buffsize );
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
    // std::uint32_t check_count;
    // std::uint64_t lb_pows;
    // std::string lb_diff;
    CNodeStatus( const char *ns_line ) {
        assert( ns_line != nullptr && std::strlen( ns_line ) > 0 );
        auto next_status_token = []( char sep, size_t &p_pos, size_t &c_pos, const std::string &status ) {
            p_pos = c_pos;
            c_pos = status.find( sep, c_pos + 1 );
        };
        auto extract_status_token = []( size_t p_pos, size_t c_pos, const std::string& status ) {
            return status.substr( p_pos + 1, c_pos == std::string::npos ? std::string::npos : ( c_pos - p_pos - 1 ) );
        };
        std::string status { ns_line };
        status.erase( status.length() - 2 ); // remove the carriage return and new line charaters
        size_t p_pos = -1, c_pos = -1;
        //NODESTATUS 1{Peers} 2{LastBlock} 3{Pendings} 4{Delta} 5{headers} 6{version} 7{UTCTime} 8{MNsHash} 9{MNscount}
        //           10{LasBlockHash} 11{BestHashDiff} 12{LastBlockTimeEnd} 13{LBMiner} 14{ChecksCount} 15{LastBlockPoW}
        //           16{LastBlockDiff}
        // 0{NODESTATUS}
        next_status_token( ' ', p_pos, c_pos, status );
        // std::string nodestatus = extract_status_token( p_pos, c_pos, status );
        // 1{peer}
        next_status_token( ' ', p_pos, c_pos, status );
        // this->peer = std::stoul( extract_status_token( p_pos, c_pos, status ) );
        // 2{blck}
        next_status_token( ' ', p_pos, c_pos, status );
        this->blck_no = std::stoul( extract_status_token( p_pos, c_pos, status ) );
        // 3{pending}
        next_status_token( ' ', p_pos, c_pos, status );
        // this->pending = std::stoul( extract_status_token( p_pos, c_pos, status ) );
        // 4{delta}
        next_status_token( ' ', p_pos, c_pos, status );
        // this->delta = std::stoul( extract_status_token( p_pos, c_pos, status ) );
        // 5{header/branch}
        next_status_token( ' ', p_pos, c_pos, status );
        // this->branch = extract_status_token( p_pos, c_pos, status );
        // 6{version}
        next_status_token( ' ', p_pos, c_pos, status );
        // this->version = extract_status_token( p_pos, c_pos, status );
        // 7{utctime}
        next_status_token( ' ', p_pos, c_pos, status );
        // this->utctime = std::stoul( extract_status_token( p_pos, c_pos, status ) );
        // 8{mn_hash}
        next_status_token( ' ', p_pos, c_pos, status );
        // this->mn_hash = extract_status_token( p_pos, c_pos, status );
        // 9{mn_count}
        next_status_token( ' ', p_pos, c_pos, status );
        // this->mn_count = std::stoul( extract_status_token( p_pos, c_pos, status ) );
        // 10{lb_hash}
        next_status_token( ' ', p_pos, c_pos, status );
        this->lb_hash = extract_status_token( p_pos, c_pos, status );
        if ( this->lb_hash.length() != 32 ) throw std::out_of_range( "Wrong receiving lb_hash" );
        // 11{bh_diff/mn_diff}
        next_status_token( ' ', p_pos, c_pos, status );
        this->mn_diff = extract_status_token( p_pos, c_pos, status );
        if ( this->mn_diff.length() != 32 ) throw std::out_of_range( "Wrong receiving mn_diff" );
        // 12{lb_time}
        next_status_token( ' ', p_pos, c_pos, status );
        this->lb_time = std::stoul( extract_status_token( p_pos, c_pos, status ) );
        // 13{lb_addr}
        next_status_token( ' ', p_pos, c_pos, status );
        this->lb_addr = extract_status_token( p_pos, c_pos, status );
        if ( this->lb_addr.length() != 30 && this->lb_addr.length() != 31 ) throw std::out_of_range( "Wrong receiving lb_addr" );
        // 14{check_count}
        // next_status_token( ' ', p_pos, c_pos, status );
        // this->check_count = std::stoul( extract_status_token( p_pos, c_pos, status ) );
        // 15{lb_pows}
        // next_status_token( ' ', p_pos, c_pos, status );
        // this->lb_pows = std::stoull( extract_status_token( p_pos, c_pos, status ) );
        // 16{lb_diff}
        // next_status_token( ' ', p_pos, c_pos, status );
        // this->lb_diff = extract_status_token( p_pos, c_pos, status );
        // if ( this->lb_diff.length() != 32 ) throw std::out_of_range( "Wrong receiving lb_diff" );
    }
};

struct CPoolStatus {
    std::uint32_t blck_no;
    std::string lb_hash;
    std::string mn_diff;
    std::string prefix;
    std::string address;
    std::uint64_t till_balance;
    std::uint32_t till_payment;
    std::uint64_t pool_hashrate;
    std::uint32_t payment_block;
    std::uint64_t payment_amount;
    std::string payment_order_id;
    std::uint64_t mnet_hashrate;
    CPoolStatus( const char *ps_line ) {
        assert( ps_line != nullptr && std::strlen( ps_line ) > 0 );
        auto next_status_token = []( char sep, size_t &p_pos, size_t &c_pos, const std::string &status ) {
            p_pos = c_pos;
            c_pos = status.find( sep, c_pos + 1 );
        };
        auto extract_status_token = []( size_t p_pos, size_t c_pos, const std::string& status ) {
            return status.substr( p_pos + 1, c_pos == std::string::npos ? std::string::npos : ( c_pos - p_pos - 1 ) );
        };
        std::string status { ps_line };
        status.erase( status.length() - 2 ); // remove the carriage return and new line charaters
        size_t p_pos = -1, c_pos = -1;
        // {0}OK 1{MinerPrefix} 2{MinerAddress} 3{PoolMinDiff} 4{LBHash} 5{LBNumber}
        // 6{MinerBalance} 7{BlocksTillPayment} 8{LastPayInfo} 9{LastBlockPoolHashrate}
        // 10{MainNetHashrate}
        // 0{OK}
        next_status_token( ' ', p_pos, c_pos, status );
        // std::string status = extract_status_token( p_pos, c_pos, status );
        // 1{prefix}
        next_status_token( ' ', p_pos, c_pos, status );
        this->prefix = extract_status_token( p_pos, c_pos, status );
        if ( this->prefix.length() != 3 ) throw std::out_of_range( "Wrong receiving pool prefix" );
        // 2{address}
        next_status_token( ' ', p_pos, c_pos, status );
        this->address = extract_status_token( p_pos, c_pos, status );
        if ( this->address.length() != 30 && this->address.length() != 31 )  throw std::out_of_range( "Wrong receiving pool address" );
        // 3{mn_diff}
        next_status_token( ' ', p_pos, c_pos, status );
        this->mn_diff = extract_status_token( p_pos, c_pos, status );
        if ( this->mn_diff.length() != 32 ) throw std::out_of_range( "Wrong receiving pool diff" );
        // 4{lb_hash}
        next_status_token( ' ', p_pos, c_pos, status );
        this->lb_hash = extract_status_token( p_pos, c_pos, status );
        if (this->lb_hash.length() != 32 ) throw std::out_of_range( "Wrong receiving lb_hash" );
        // 5{blck_no}
        next_status_token( ' ', p_pos, c_pos, status );
        this->blck_no = std::stoul( extract_status_token( p_pos, c_pos, status ) );
        // 6{till_balance}
        next_status_token( ' ', p_pos, c_pos, status );
        this->till_balance = std::stoull( extract_status_token( p_pos, c_pos, status ) );
        // 7{till_payment}
        next_status_token( ' ', p_pos, c_pos, status );
        this->till_payment = std::stoul( extract_status_token( p_pos, c_pos, status ) );
        // 8{payment_info}
        next_status_token( ' ', p_pos, c_pos, status );
        std::string payment_info = extract_status_token( p_pos, c_pos, status );
        // 9{pool_hashrate}
        next_status_token( ' ', p_pos, c_pos, status );
        this->pool_hashrate = std::stoull( extract_status_token( p_pos, c_pos, status ) );
        // 10{mnet_hashrate}
        next_status_token( ' ', p_pos, c_pos, status );
        this->mnet_hashrate = std::stoull( extract_status_token( p_pos, c_pos, status ) );
        if ( payment_info.length() > 0 ) {
            // 8{LastPayInfo} = Block:ammount:orderID
            size_t p_pos = -1, c_pos = -1;
            // 0{payment_block}
            next_status_token( ':', p_pos, c_pos, payment_info );
            this->payment_block = std::stoul( extract_status_token( p_pos, c_pos, payment_info ) );
            // 0{payment_amount}
            next_status_token( ':', p_pos, c_pos, payment_info );
            this->payment_amount = std::stoull( extract_status_token( p_pos, c_pos, payment_info ) );
            // 0{payment_order_id}
            next_status_token( ':', p_pos, c_pos, payment_info );
            this->payment_order_id = extract_status_token( p_pos, c_pos, payment_info );
        }
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
    std::uint64_t till_balance;
    std::uint32_t till_payment;
    std::uint64_t pool_hashrate;
    std::uint32_t payment_block;
    std::uint64_t payment_amount;
    std::string payment_order_id;
    std::uint64_t mnet_hashrate;
    std::string pool_name;
    CPoolTarget( std::uint32_t blck_no, const std::string &lb_hash, const std::string &mn_diff,
                const std::string &prefix, const std::string &address, std::uint64_t till_balance,
                std::uint32_t till_payment, std::uint64_t pool_hashrate, std::uint32_t payment_block,
                std::uint64_t payment_amount, const std::string &payment_order_id, std::uint64_t mnet_hashrate,
                const std::string& pool_name )
        :   CTarget( blck_no, lb_hash, mn_diff ), till_balance { till_balance }, till_payment { till_payment },
            pool_hashrate { pool_hashrate }, payment_block { payment_block }, payment_amount { payment_amount },
            payment_order_id { payment_order_id }, mnet_hashrate { mnet_hashrate }, pool_name { pool_name } {
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
        assert( std::strlen( base_ ) == 18 && std::strlen( hash_ ) == 32
               && ( std::strlen( diff_ ) == 0 || std::strlen( diff_ ) == 32 ) );
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
public:
    std::uint32_t m_miner_id;
    std::uint32_t m_thread_id;
protected:
    char m_address[32];
    char m_prefix[10];
    std::uint32_t m_blck_no { 0 };
    char m_lb_hash[33];
    char m_mn_diff[33];
    std::uint32_t m_computed_hashes_count { 0 };
    double m_block_mining_duration { 0. };
    mutable std::mutex m_mutex_blck_no;
    mutable std::condition_variable m_condv_blck_no;
    mutable std::mutex m_mutex_summary;
    mutable std::condition_variable m_condv_summary;
public:
    CMineThread( std::uint32_t miner_id, std::uint32_t thread_id )
        :   m_miner_id { miner_id }, m_thread_id { thread_id } {
    }
    virtual ~CMineThread() = default;
    void CleanupSyncState() {
        m_condv_blck_no.notify_one();
        m_condv_summary.notify_one();
    }
    void WaitTarget();
    void DoneTarget();
    void NewTarget( const std::shared_ptr<CTarget> &target );
    void SetBlockSummary( std::uint32_t hashes_count, double duration );
    std::tuple<std::uint32_t, double> GetBlockSummary();
    virtual void Mine( void ( * NewSolFunc )( const std::shared_ptr<CSolution>& ) );
};

class CCommThread { // A singleton pattern class
private:
    mutable std::default_random_engine m_random_engine {
        std::default_random_engine { std::random_device {}() } };
    mutable std::mutex m_mutex_solutions;
    std::vector<std::tuple<std::string, std::string>> m_mining_nodes;
    std::vector<std::tuple<std::string, std::string, std::string>>& m_mining_pools;
    std::uint32_t m_mining_pools_id;
    std::multiset<std::shared_ptr<CSolution>, CSolsSetCompare> m_solutions;
    std::uint64_t m_last_block_hashes_count { 0 };
    double m_last_block_elapsed_secs { 0. };
    double m_last_block_hashrate { 0. };
    std::vector<std::tuple<std::uint32_t, double>> m_last_thread_minings;
    std::uint32_t m_accepted_solutions_count { 0 };
    std::uint32_t m_rejected_solutions_count { 0 };
    std::uint32_t m_failured_solutions_count { 0 };
    std::map<std::uint32_t, int> m_freq_blck_no;
    std::map<std::string  , int> m_freq_lb_hash;
    std::map<std::string  , int> m_freq_mn_diff;
    std::map<std::time_t  , int> m_freq_lb_time;
    std::map<std::string  , int> m_freq_lb_addr;
    char m_inet_buffer[INET_BUFFER_SIZE];
    CCommThread();
    void _ResetMiningBlock();
    void _ReportMiningTarget( const std::shared_ptr<CTarget>& target );
    void _ReportTargetSummary( const std::shared_ptr<CTarget>& target, const std::chrono::duration<double>& elapsed_blck );
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
    std::time_t RequestTimestamp();
    std::vector<std::shared_ptr<CNodeStatus>> RequestNodeSources( std::size_t min_nodes_count );
    int SubmitSoloSolution( std::uint32_t blck, const char base[19], const char address[32], char new_mn_diff[33] );
    std::shared_ptr<CNodeTarget> GetNodeTargetConsensus();
    std::shared_ptr<CPoolTarget> RequestPoolTarget( const char address[32] );
    std::shared_ptr<CPoolTarget> GetPoolTargetFailover();
    int SubmitPoolSolution( std::uint32_t blck_no, const char base[19], const char address[32] );
    std::shared_ptr<CTarget> GetTarget( const char prev_lb_hash[32] );
    void SubmitSolution( const std::shared_ptr<CSolution> &solution, std::shared_ptr<CTarget> &target );
    void Communicate();
};

std::string& ltrim( std::string& s );
std::string& rtrim( std::string& s );
bool iequal( const std::string& s1, const std::string& s2 );

enum class LogLevel { FATAL, ERROR, WARN, INFO, DEBUG, };

template <typename OFS>
class LogEntry {
protected:
    OFS& ofs;
    std::ostringstream oss;
public:
    LogEntry( OFS& ofs ) : ofs { ofs } {};
    LogEntry( const LogEntry& obj ) = delete;
    virtual ~LogEntry() {
        ofs.Output( oss.str() );
    }
    LogEntry& operator=( const LogEntry& obj ) = delete;
    template <LogLevel level>
    std::ostringstream& GetStream() {
        std::time_t now { std::time( 0 ) };
        struct std::tm* ptm = std::localtime( &now );
        oss << "[" << std::put_time( ptm, "%x %X" ) << "]" << LogLevelString<level>() << ":";
        return oss;
    }
    template <LogLevel level>
    static std::string LogLevelString() {
        return
              level == LogLevel::FATAL ? "FATAL" :
            ( level == LogLevel::ERROR ? "ERROR" :
            ( level == LogLevel::WARN  ? " WARN" :
            ( level == LogLevel::INFO  ? " INFO" :
            ( level == LogLevel::DEBUG ? "DEBUG" : "     " ) ) ) );
    }
};

class LogFile {
private:
    std::ostream& m_ost;
public:
    LogFile( std::ostream& ost ) : m_ost { ost } {}
    void Output( const std::string& msg ) {
        m_ost << msg << std::flush;
    }
};

#define LOGGING_STDOUT
#undef LOGGING_STDOUT
#ifdef LOGGING_STDOUT
static LogFile _NOSO_LOGGING_LOGSTREAM( std::cout );
#else
static std::ofstream _NOSO_LOGGING_OFS( DEFAULT_LOGGING_FILENAME );
static LogFile _NOSO_LOGGING_LOGSTREAM( _NOSO_LOGGING_OFS );
#endif
#define LOG_FATAL LogEntry<LogFile>( _NOSO_LOGGING_LOGSTREAM ).GetStream< LogLevel::FATAL >()
#define LOG_ERROR LogEntry<LogFile>( _NOSO_LOGGING_LOGSTREAM ).GetStream< LogLevel::ERROR >()
#define LOG_WARN  LogEntry<LogFile>( _NOSO_LOGGING_LOGSTREAM ).GetStream< LogLevel::WARN  >()
#define LOG_INFO  LogEntry<LogFile>( _NOSO_LOGGING_LOGSTREAM ).GetStream< LogLevel::INFO  >()
#define LOG_DEBUG LogEntry<LogFile>( _NOSO_LOGGING_LOGSTREAM ).GetStream< LogLevel::DEBUG >()
#define NOSO_LOG_INFO  LOG_INFO  << "(" << std::setfill( '0' ) << std::setw( 3 ) << NOSO_BLOCK_AGE << ")"
#define NOSO_LOG_WARN  LOG_WARN  << "(" << std::setfill( '0' ) << std::setw( 3 ) << NOSO_BLOCK_AGE << ")"
#define NOSO_LOG_ERROR LOG_ERROR << "(" << std::setfill( '0' ) << std::setw( 3 ) << NOSO_BLOCK_AGE << ")"
#define NOSO_LOG_FATAL LOG_FATAL << "(" << std::setfill( '0' ) << std::setw( 3 ) << NOSO_BLOCK_AGE << ")"
#define NOSO_LOG_DEBUG LOG_DEBUG << "(" << std::setfill( '0' ) << std::setw( 3 ) << NOSO_BLOCK_AGE << ")"

#define NOSO_STDOUT std::cout
#define NOSO_STDERR std::cerr

int inet_init();
void inet_cleanup();
bool is_valid_address( const std::string& address );
bool is_valid_minerid( std::uint32_t minerid );
bool is_valid_threads( std::uint32_t count );
char hashrate_pretty_unit( std::uint64_t count );
double hashrate_pretty_value( std::uint64_t count );
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
    cxxopts::Options command_options( "noso-2m", "A miner for Nosocryptocurrency Protocol-2" );
    command_options.add_options()
        ( "c,config",   "A configuration file",                 cxxopts::value<std::string>()->default_value( DEFAULT_CONFIG_FILENAME ) )
        ( "a,address",  "An original noso wallet address",      cxxopts::value<std::string>()->default_value( DEFAULT_MINER_ADDRESS ) )
        ( "i,minerid",  "Miner ID - a number between 0-8100",   cxxopts::value<std::uint32_t>()->default_value( std::to_string( DEFAULT_MINER_ID ) ) )
        ( "t,threads",  "Threads count - 2 or more",            cxxopts::value<std::uint32_t>()->default_value( std::to_string( DEFAULT_THREADS_COUNT ) ) )
        (   "pools",    "Mining pools list",                    cxxopts::value<std::string>()->default_value( DEFAULT_POOL_URL_LIST ) )
        (   "solo",     "Solo mining mode" )
        ( "v,version",  "Print version" )
        ( "h,help",     "Print usage" )
        ;
    cxxopts::ParseResult parsed_options;
    try {
        parsed_options = command_options.parse( argc, argv );
    } catch ( const std::exception& e ) {
        NOSO_STDERR << "Invalid option provided: " << e.what() << std::endl;
        NOSO_STDERR << "Use option ’--help’ for usage detail" << std::endl;
        std::exit( EXIT_FAILURE );
    }
    if ( parsed_options.count( "help" ) ) {
        NOSO_STDOUT << command_options.help() << std::endl;
        std::exit( EXIT_SUCCESS );
    }
    if ( parsed_options.count( "version" ) ) {
        NOSO_STDOUT << "version " << NOSO_2M_VERSION_MAJOR << "." << NOSO_2M_VERSION_MINOR << "." << NOSO_2M_VERSION_PATCH << std::endl;
        std::exit( EXIT_SUCCESS );
    }
    NOSO_LOG_INFO << "noso-2m - A miner for Nosocryptocurrency Protocol-2" << std::endl;
    NOSO_LOG_INFO << "f04ever(c) 2022 @https://github.com/f04ever/noso-2m" << std::endl;
    NOSO_LOG_INFO << "version " << NOSO_2M_VERSION_MAJOR << "." << NOSO_2M_VERSION_MINOR << "." << NOSO_2M_VERSION_PATCH << std::endl;
    NOSO_LOG_INFO << "--" << std::endl;
    try {
        std::string cfg_fname = parsed_options["config"].as<std::string>();
        std::string cfg_address;
        std::string cfg_pools;
        long cfg_minerid { -1 };
        std::uint32_t cfg_threads { 2 };
        bool cfg_solo { false };
        std::ifstream cfg_istream( cfg_fname );
        if( !cfg_istream.good() ) {
            std::string msg { "Config file '" + cfg_fname + "' not found!" };
            NOSO_LOG_WARN << msg << std::endl;
            if ( cfg_fname != DEFAULT_CONFIG_FILENAME ) throw std::bad_exception();
            msg = "Use default options";
            NOSO_LOG_INFO << msg << std::endl;
        } else {
            std::string msg { "Load config file '" + cfg_fname + "'" };
            NOSO_LOG_INFO << msg << std::endl;
            int line_no { 0 };
            std::string line_str;
            try {
                int found_cfg { 0 };
                while ( std::getline( cfg_istream, line_str ) && found_cfg < 5) {
                    line_no++;
                    if        ( line_str.rfind( "address ", 0 ) == 0 ) {
                        cfg_address = line_str.substr( 8 );
                        if ( !is_valid_address( cfg_address ) )
                            throw std::invalid_argument( "Invalid address config" );
                        found_cfg++;
                    } else if ( line_str.rfind( "minerid ", 0 ) == 0 ) {
                        cfg_minerid = std::stoi( line_str.substr( 8 ) );
                        if ( !is_valid_minerid( cfg_minerid ) )
                            throw std::invalid_argument( "Invalid minerid config" );
                        found_cfg++;
                    } else if ( line_str.rfind( "threads ", 0 ) == 0 ) {
                        cfg_threads = std::stoi( line_str.substr( 8 ) );
                        if ( !is_valid_threads( cfg_threads ) )
                            throw std::invalid_argument( "Invalid threads count config" );
                        found_cfg++;
                    } else if ( line_str.rfind( "pools ",   0 ) == 0 ) {
                        cfg_pools = line_str.substr( 6 );
                        found_cfg++;
                    } else if ( line_str == "solo" || line_str == "solo true" ) {
                        cfg_solo = true;
                        found_cfg++;
                    }
                }
            } catch( const std::exception& e) {
                std::string msg { std::string( e.what() ) + " in file '" + cfg_fname + "'" };
                NOSO_LOG_FATAL << msg << " line#" << line_no << "[" << line_str << "]" << std::endl;
                throw std::bad_exception();
            }
        }
        std::string opt_address;
        std::uint32_t opt_minerid { 0 };
        std::uint32_t opt_threads { 2 };
        std::string opt_pools;
        std::size_t opt_solo { false };
        try {
            opt_address = parsed_options["address"].as<std::string>();
            if ( !is_valid_address( opt_address ) )
                throw std::invalid_argument( "Invalid miner address option" );
            opt_minerid = parsed_options["minerid"].as<std::uint32_t>();
            if ( !is_valid_minerid( opt_minerid ) )
                throw std::invalid_argument( "Invalid miner id option" );
            opt_threads = parsed_options["threads"].as<std::uint32_t>();
            // if ( !is_valid_threads( opt_threads ) )
            //     throw std::invalid_argument( "Invalid threads count option" );
            opt_pools = parsed_options["pools"].as<std::string>();
            opt_solo = parsed_options.count( "solo" );
        } catch( const std::exception& e) {
            std::string msg { e.what() };
            NOSO_LOG_FATAL << msg << std::endl;
            throw std::bad_exception();
        }
        std::string sel_address { opt_address != DEFAULT_MINER_ADDRESS ? opt_address
            : cfg_address.length() > 0 ? cfg_address : opt_address };
        std::string sel_pools { opt_pools != DEFAULT_POOL_URL_LIST ? opt_pools
            : cfg_pools.length() > 0 ? cfg_pools : opt_pools };
        std::strcpy( g_miner_address, sel_address.c_str() );
        g_miner_id = opt_minerid > 0 ? opt_minerid : cfg_minerid >= 0 ? cfg_minerid : opt_minerid;
        g_threads_count = opt_threads > 2 ? opt_threads: cfg_threads > 2 ? cfg_threads : opt_threads;
        g_mining_pools = parse_pools_argv( sel_pools );
        g_solo_mining = opt_solo > 0 ? true : cfg_solo;
    } catch( const std::exception& e) {
        NOSO_LOG_INFO << "===================================================" << std::endl;
        std::exit( EXIT_FAILURE );
    }
    auto left_pad = []( const std::string& s, std::size_t n, char c ){
        std::string r { s };
        if ( n > r.length() ) r.append( n - r.length(), c );
        return r; };
    std::string msg { "" };
    NOSO_LOG_INFO << msg << std::endl;
    msg = std::string( "- Wallet address: " ) + g_miner_address;
    NOSO_LOG_INFO << msg << std::endl;
    msg = std::string( "-       Miner ID: " ) + std::to_string( g_miner_id );
    NOSO_LOG_INFO << msg << std::endl;
    msg = std::string( "-  Threads count: " ) + std::to_string( g_threads_count );
    NOSO_LOG_INFO << msg << std::endl;
    msg = std::string( "-    Mining mode: " ) + ( g_solo_mining ? "solo" : "pool" );
    NOSO_LOG_INFO << msg << std::endl;
    if ( !g_solo_mining ) {
        for( auto itor = g_mining_pools.cbegin(); itor != g_mining_pools.cend(); itor = std::next( itor ) ) {
            msg = ( itor == g_mining_pools.cbegin() ? "-   Mining pools: " : "                : " )
                    + left_pad( std::get<0>( *itor ), 12, ' ' ).substr( 0, 12 ) + "(" + std::get<1>( *itor ) + ":" + std::get<2>( *itor ) + ")";
            NOSO_LOG_INFO << msg << std::endl;
        }
    }
    try {
        if ( inet_init() < 0 ) throw std::runtime_error( "WSAStartup errors!" );
        std::time_t mainnet_timestamp { CCommThread::GetInstance()->RequestTimestamp() };
        if ( mainnet_timestamp < 0 ) throw std::runtime_error( "Can not check mainnet's timestamp!" );
        else if ( NOSO_TIMESTAMP - mainnet_timestamp > DEFAULT_TIMESTAMP_DIFFERENCES )
            throw std::runtime_error( "Your machine's time is different from mainnet. Synchronize clock!" );
        for ( std::uint32_t thread_id = 0; thread_id < g_threads_count - 1; ++thread_id )
            g_mine_objects.push_back( std::make_shared<CMineThread>( g_miner_id, thread_id ) );
        std::thread comm_thread( &CCommThread::Communicate, CCommThread::GetInstance() );
        constexpr auto const NewSolFunc { []( const std::shared_ptr<CSolution>& solution ){
            CCommThread::GetInstance()->AddSolution( solution ); } };
        for ( std::uint32_t thread_id = 0; thread_id < g_threads_count - 1; ++thread_id )
            g_mine_threads.emplace_back( &CMineThread::Mine, g_mine_objects[thread_id], NewSolFunc );
        signal( SIGINT, []( int /* signum */ ) {
            if ( !g_still_running ) return;
            std::string msg { "Ctrl+C pressed! Wait for finishing all threads..." };
            NOSO_LOG_WARN << msg << std::endl;
            g_still_running = false; });
        comm_thread.join();
        inet_cleanup();
        NOSO_LOG_INFO << "===================================================" << std::endl;
        return EXIT_SUCCESS;
    } catch( const std::exception& e) {
        std::string msg { e.what() };
        NOSO_LOG_FATAL << msg << std::endl;
        inet_cleanup();
        NOSO_LOG_INFO << "===================================================" << std::endl;
        std::exit( EXIT_FAILURE );
    }
}

void CMineThread::SetBlockSummary( std::uint32_t hashes_count, double duration ) {
    m_mutex_summary.lock();
    m_block_mining_duration = duration;
    m_computed_hashes_count = hashes_count;
    m_mutex_summary.unlock();
    m_condv_summary.notify_one();
}

std::tuple<std::uint32_t, double> CMineThread::GetBlockSummary() {
    std::unique_lock unique_lock_summary_lock( m_mutex_summary );
    m_condv_summary.wait( unique_lock_summary_lock, [&]{
                             return !g_still_running || m_computed_hashes_count > 0; } );
    auto summary = std::make_tuple( m_computed_hashes_count, m_block_mining_duration );
    m_computed_hashes_count = 0;
    m_block_mining_duration = 0.;
    unique_lock_summary_lock.unlock();
    return summary;
}

void CMineThread::WaitTarget() {
    std::unique_lock unique_lock_blck_no( m_mutex_blck_no );
    m_condv_blck_no.wait( unique_lock_blck_no, [&]{ return !g_still_running || m_blck_no > 0; } );
    unique_lock_blck_no.unlock();
}

void CMineThread::DoneTarget() {
    std::unique_lock unique_lock_blck_no( m_mutex_blck_no );
    m_blck_no = 0;
    unique_lock_blck_no.unlock();
}

void CMineThread::NewTarget( const std::shared_ptr<CTarget> &target ) {
    std::string thread_prefix = { target->prefix + nosohash_prefix( m_miner_id ) + nosohash_prefix( m_thread_id ) };
    thread_prefix.append( 9 - thread_prefix.size(), '!' );
    m_mutex_blck_no.lock();
    std::strcpy( m_prefix, thread_prefix.c_str() );
    std::strcpy( m_address, target->address.c_str() );
    std::strcpy( m_lb_hash, target->lb_hash.c_str() );
    std::strcpy( m_mn_diff, target->mn_diff.c_str() );
    m_blck_no = target->blck_no + 1;
    m_mutex_blck_no.unlock();
    m_condv_blck_no.notify_one();
}

void CMineThread::Mine( void ( * NewSolFunc )( const std::shared_ptr<CSolution>& ) ) {
    while ( g_still_running ) {
        this->WaitTarget();
        if ( !g_still_running ) break;
        assert( ( std::strlen( m_address ) == 30 || std::strlen( m_address ) == 31 )
                && std::strlen( m_lb_hash ) == 32
                && std::strlen( m_mn_diff ) == 32 );
        char best_diff[33];
        std::strcpy( best_diff, m_mn_diff );
        std::size_t match_len { 0 };
        while ( best_diff[match_len] == '0' ) ++match_len;
        std::uint32_t noso_hash_counter { 0 };
        CNosoHasher noso_hasher( m_prefix, m_address );
        auto begin_mining { std::chrono::steady_clock::now() };
        while ( g_still_running && 1 <= NOSO_BLOCK_AGE && NOSO_BLOCK_AGE <= 585 ) {
            const char *base { noso_hasher.GetBase( noso_hash_counter++ ) };
            const char *hash { noso_hasher.GetHash() };
            assert( std::strlen( base ) == 18 && std::strlen( hash ) == 32 );
            if ( std::strncmp( hash, m_lb_hash, match_len ) == 0 ) {
                if ( g_solo_mining ) {
                    const char *diff { noso_hasher.GetDiff( m_lb_hash ) };
                    assert( std::strlen( diff ) == 32 );
                    if ( std::strcmp( diff, best_diff ) < 0 ) {
                        NewSolFunc( std::make_shared<CSolution>( m_blck_no, base, hash, diff ) );
                        std::strcpy( best_diff, diff );
                        while ( best_diff[match_len] == '0' ) ++match_len;
                    }
                } else NewSolFunc( std::make_shared<CSolution>( m_blck_no, base, hash, "" ) );
            }
        }
        std::chrono::duration<double> elapsed_mining { std::chrono::steady_clock::now() - begin_mining };
        this->SetBlockSummary( noso_hash_counter, elapsed_mining.count() );
        this->DoneTarget();
    } // END while ( g_still_running ) {
}

CCommThread::CCommThread()
    :   m_mining_nodes { g_default_nodes },
        m_mining_pools { g_mining_pools }, m_mining_pools_id { 0 } {
}

std::shared_ptr<CSolution> CCommThread::GetSolution() {
    return g_solo_mining ? this->BestSolution() : this->GoodSolution();
}

std::time_t CCommThread::RequestTimestamp() {
    if ( m_mining_nodes.size() <= 0 ) return std::time_t( -1 );
    std::shuffle( m_mining_nodes.begin(), m_mining_nodes.end(), m_random_engine );
    for (   auto itor = m_mining_nodes.begin();
            g_still_running
                && itor != m_mining_nodes.end();
            itor = std::next( itor ) ) {
        CNodeInet inet { std::get<0>( *itor ), std::get<1>( *itor ), DEFAULT_NODE_INET_TIMEOSEC };
        int rsize { inet.RequestTimestamp( m_inet_buffer, INET_BUFFER_SIZE ) };
        if ( rsize <= 0 ) {
            NOSO_LOG_DEBUG
                << "CCommThread::RequestTimestamp Poor connecting with node "
                << inet.m_host << ":" << inet.m_port
                << std::endl;
        } else {
            try {
                return std::time_t( std::atol( m_inet_buffer ) );
            } catch ( const std::exception &e ) {
                NOSO_LOG_DEBUG
                    << "CCommThread::RequestTimestamp Unrecognised response from node "
                    << inet.m_host << ":" << inet.m_port
                    << "[" << m_inet_buffer << "](size=" << rsize << ")" << e.what()
                    << std::endl;
            }
        }
    }
    return std::time_t( -1 );
}

std::vector<std::shared_ptr<CNodeStatus>> CCommThread::RequestNodeSources( std::size_t min_nodes_count ) {
    std::vector<std::shared_ptr<CNodeStatus>> sources;
    if ( m_mining_nodes.size() < min_nodes_count ) return sources;
    std::shuffle( m_mining_nodes.begin(), m_mining_nodes.end(), m_random_engine );
    std::size_t nodes_count { 0 };
    for (   auto itor = m_mining_nodes.begin();
            g_still_running
                && nodes_count < min_nodes_count
                && itor != m_mining_nodes.end();
            itor = std::next( itor ) ) {
        CNodeInet inet { std::get<0>( *itor ), std::get<1>( *itor ), DEFAULT_NODE_INET_TIMEOSEC };
        int rsize { inet.RequestSource( m_inet_buffer, INET_BUFFER_SIZE ) };
        if ( rsize <= 0 ) {
            NOSO_LOG_DEBUG
                << "CCommThread::RequestNodeSources Poor connecting with node "
                << inet.m_host << ":" << inet.m_port
                << std::endl;
        } else {
            try {
                sources.push_back( std::make_shared<CNodeStatus>( m_inet_buffer ) );
                nodes_count ++;
            } catch ( const std::exception &e ) {
                NOSO_LOG_DEBUG
                    << "CCommThread::RequestNodeSources Unrecognised response from node "
                    << inet.m_host << ":" << inet.m_port
                    << "[" << m_inet_buffer << "](size=" << rsize << ")" << e.what()
                    << std::endl;
            }
        }
    }
    return sources;
}

std::shared_ptr<CNodeTarget> CCommThread::GetNodeTargetConsensus() {
    std::vector<std::shared_ptr<CNodeStatus>> status_of_nodes = this->RequestNodeSources( CONSENSUS_NODES_COUNT );
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
    std::shared_ptr<CNodeTarget> target {
        std::make_shared<CNodeTarget>(
            max_freq( m_freq_blck_no ),
            max_freq( m_freq_lb_hash ),
            max_freq( m_freq_mn_diff ),
            max_freq( m_freq_lb_time ),
            max_freq( m_freq_lb_addr ) ) };
        target->prefix = "";
        target->address = g_miner_address;
    return target;
}

int CCommThread::SubmitSoloSolution( std::uint32_t blck, const char base[19],
                                    const char address[32], char new_mn_diff[33] ) {
    assert( std::strlen( base ) == 18
            && ( std::strlen( address ) == 30 || std::strlen( address ) == 31 ) );
    if ( m_mining_nodes.size() <= 0 ) return ( -1 );
    std::shuffle( m_mining_nodes.begin(), m_mining_nodes.end(), m_random_engine );
    for (   auto itor = m_mining_nodes.begin();
            g_still_running
                && itor != m_mining_nodes.end();
            itor = std::next( itor ) ) {
        CNodeInet inet { std::get<0>( *itor ), std::get<1>( *itor ), DEFAULT_NODE_INET_TIMEOSEC };
        int rsize { inet.SubmitSolution( blck, base, address, m_inet_buffer, INET_BUFFER_SIZE ) };
        if ( rsize <= 0 ) {
            NOSO_LOG_DEBUG
                << "CCommThread::SubmitSoloSolution Poor connecting with node "
                << inet.m_host << ":" << inet.m_port
                << std::endl;
        } else {
            // try {
            // m_inet_buffer ~ len=(70+2)~[True Diff(32) Hash(32)\r\n] OR len=(40+2)~[False Diff(32) Code#(1)\r\n]
            if      ( rsize >= 42
                        && std::strncmp( m_inet_buffer, "False ", 6 ) == 0
                        && m_inet_buffer[38] == ' '
                        && '1' <= m_inet_buffer[39] && m_inet_buffer[39] <= '7' ) {
                // len=(40+2)~[False Diff(32) Code#(1)\r\n]
                std::strncpy( new_mn_diff, m_inet_buffer + 6, 32 );
                new_mn_diff[32] = '\0';
                return m_inet_buffer[39] - '0';
            }
            else if ( rsize >= 72
                        && std::strncmp( m_inet_buffer, "True ", 5 ) == 0
                        && m_inet_buffer[37] == ' ' ) {
                // len=(70+2)~[True Diff(32) Hash(32)\r\n]
                std::strncpy( new_mn_diff, m_inet_buffer + 5, 32 );
                new_mn_diff[32] = '\0';
                return 0;
            }
            // } catch ( const std::exception &e ) {}
            NOSO_LOG_DEBUG
                << "CCommThread::SubmitSoloSolution Unrecognised response from node "
                << inet.m_host << ":" << inet.m_port
                << "[" << m_inet_buffer << "](size=" << rsize << ")"
                << std::endl;
        }
    }
    return ( -1 );
}

std::shared_ptr<CPoolTarget> CCommThread::RequestPoolTarget( const char address[32] ) {
    assert( std::strlen( address ) == 30 || std::strlen( address ) == 31 );
    auto pool { m_mining_pools[m_mining_pools_id] };
    CPoolInet inet { std::get<0>( pool ), std::get<1>( pool ), std::get<2>( pool ), DEFAULT_POOL_INET_TIMEOSEC };
    int rsize { inet.RequestSource( address, m_inet_buffer, INET_BUFFER_SIZE ) };
    if ( rsize <= 0 ) {
        NOSO_LOG_DEBUG
            << "CCommThread::RequestPoolTarget Poor connecting with pool "
            << inet.m_name << "(" << inet.m_host << ":" << inet.m_port << ")"
            << std::endl;
    } else {
        try {
            CPoolStatus ps( m_inet_buffer );
            return std::make_shared<CPoolTarget>(
                ps.blck_no,
                ps.lb_hash,
                ps.mn_diff,
                ps.prefix,
                ps.address,
                ps.till_balance,
                ps.till_payment,
                ps.pool_hashrate,
                ps.payment_block,
                ps.payment_amount,
                ps.payment_order_id,
                ps.mnet_hashrate,
                std::get<0>( pool )
            );
        }
        catch ( const std::exception &e ) {
            NOSO_LOG_DEBUG
                << "CCommThread::RequestPoolTarget Unrecognised response from pool "
                << inet.m_name << "(" << inet.m_host << ":" << inet.m_port << ")"
                << "[" << m_inet_buffer << "](size=" << rsize << ")" << e.what()
                << std::endl;
        }
    }
    return nullptr;
}

std::shared_ptr<CPoolTarget> CCommThread::GetPoolTargetFailover() {
    static const int max_tries_count { 5 };
    bool first_fail { true };
    int tries_count { 1 };
    std::shared_ptr<CPoolTarget> pool_target = this->RequestPoolTarget( g_miner_address );
    while ( g_still_running && pool_target == nullptr ) {
        std::uint32_t old_inet_pools_id { m_mining_pools_id };
        auto old_pool { m_mining_pools[old_inet_pools_id] };
        NOSO_LOG_DEBUG
            << "WAIT TARGET FROM POOL "
            << std::get<0>( old_pool ) << "(" << std::get<1>( old_pool ) << ":" << std::get<2>( old_pool ) << ")"
            << " (Retries " << tries_count << "/" << max_tries_count << ")"
            << std::endl;
        if ( tries_count >= max_tries_count ) {
            tries_count = 0;
            if ( m_mining_pools.size() > 1 ) {
                if ( first_fail ) {
                    m_mining_pools_id = 0;
                    if ( m_mining_pools_id == old_inet_pools_id ) ++m_mining_pools_id;
                    first_fail = false;
                } else {
                    ++m_mining_pools_id;
                    if ( m_mining_pools_id >= m_mining_pools.size() ) m_mining_pools_id = 0;
                }
                auto new_pool { m_mining_pools[m_mining_pools_id] };
                NOSO_LOG_DEBUG
                    << "POOL FAILOVER FROM "
                    << std::get<0>( old_pool ) << "(" << std::get<1>( old_pool ) << ":" << std::get<2>( old_pool ) << ")"
                    << " TO "
                    << std::get<0>( new_pool ) << "(" << std::get<1>( new_pool ) << ":" << std::get<2>( new_pool ) << ")"
                    << std::endl;
            } else {
                NOSO_LOG_DEBUG
                    << "RE-ENTER POOL "
                    << std::get<0>( old_pool ) << "(" << std::get<1>( old_pool ) << ":" << std::get<2>( old_pool ) << ")"
                    << " AS NO ADDITIONAL POOL CONFIGURED FOR FAILOVER"
                    << std::endl;
            }
        }
        std::this_thread::sleep_for( std::chrono::milliseconds( static_cast<int>( tries_count * 1'000 * INET_CIRCLE_SECONDS ) ) );
        pool_target = this->RequestPoolTarget( g_miner_address );
        ++tries_count;
    }
    return pool_target;
}

int CCommThread::SubmitPoolSolution( std::uint32_t blck_no, const char base[19], const char address[32] ) {
    assert( std::strlen( base ) == 18
            && ( std::strlen( address ) == 30 || std::strlen( address ) == 31 ) );
    static const int max_tries_count { 5 };
    auto pool { m_mining_pools[m_mining_pools_id] };
    CPoolInet inet { std::get<0>( pool ), std::get<1>( pool ), std::get<2>( pool ), DEFAULT_POOL_INET_TIMEOSEC };
    for (   int tries_count = 0;
            g_still_running
                && tries_count < max_tries_count;
            ++tries_count ) {
        int rsize { inet.SubmitSolution( blck_no, base, address, m_inet_buffer, INET_BUFFER_SIZE ) };
        if ( rsize <= 0 ) {
            NOSO_LOG_DEBUG
                << "CCommThread::SubmitPoolSolution Poor connecting with pool "
                << inet.m_name << "(" << inet.m_host << ":" << inet.m_port << ")"
                << " Retrying " << tries_count + 1 << "/" << max_tries_count
                << std::endl;
        } else {
            // try {
            // m_inet_buffer ~ len=(4+2)~[True\r\n] OR len=(7+2)~[False Code#(1)\r\n]
            if (        rsize >= 6
                            && std::strncmp( m_inet_buffer, "True", 4 ) == 0 )
                return ( 0 );
            else if (   rsize >= 9
                            && std::strncmp( m_inet_buffer, "False ", 6 ) == 0
                            && '1' <= m_inet_buffer[6] && m_inet_buffer[6] <= '7' )
                return ( m_inet_buffer[6] - '0' );
            // } catch ( const std::exception &e ) {}
            NOSO_LOG_DEBUG
                << "CCommThread::SubmitPoolSolution Unrecognised response from pool "
                << inet.m_name << "(" << inet.m_host << ":" << inet.m_port << ")"
                << "[" << m_inet_buffer << "](size=" << rsize << ")"
                << std::endl;
        }
        std::this_thread::sleep_for( std::chrono::milliseconds( static_cast<int>( 1'000 * INET_CIRCLE_SECONDS ) ) );
    }
    return ( -1 );
}

void CCommThread::_ReportMiningTarget( const std::shared_ptr<CTarget>& target ) {
    char msg[100];
    if ( g_solo_mining ) {
        std::shared_ptr<CNodeTarget> node_target { std::dynamic_pointer_cast<CNodeTarget>( target ) };
        if ( m_last_block_hashrate > 0 ) {
            NOSO_LOG_DEBUG
                << " Computed " << m_last_block_hashes_count << " hashes within "
                << std::fixed << std::setprecision( 2 ) << m_last_block_elapsed_secs / 60 << " minutes"
                << std::endl;
            std::snprintf( msg, 100, " Hashrate(Miner/Pool/Mnet) %5.01f%c /    n/a /    n/a",
                          hashrate_pretty_value( m_last_block_hashrate ),      hashrate_pretty_unit( m_last_block_hashrate ) );
            NOSO_LOG_INFO << msg << std::endl;
        }
        if ( node_target->lb_addr == g_miner_address ) {
            g_mined_block_count++;
            std::snprintf( msg, 100, " Yay! win this block %06u", node_target->blck_no + 1 );
            NOSO_LOG_INFO << msg << std::endl;
        }
        if ( g_mined_block_count > 0 ) std::snprintf( msg, 100, " Won total %3u blocks", g_mined_block_count );
        else std::snprintf( msg, 100, " Not win a block yet!" );
        NOSO_LOG_INFO << msg << std::endl;
    } else {
        std::shared_ptr<CPoolTarget> pool_target { std::dynamic_pointer_cast<CPoolTarget>( target ) };
        if ( m_last_block_hashrate > 0 ) {
            NOSO_LOG_DEBUG
                << " Computed " << m_last_block_hashes_count << " hashed within "
                << std::fixed << std::setprecision( 2 ) << m_last_block_elapsed_secs / 60 << " minutes"
                << std::endl;
            std::snprintf( msg, 100, " Hashrate(Miner/Pool/Mnet) %5.01f%c / %5.01f%c / %5.01f%c",
                          hashrate_pretty_value( m_last_block_hashrate ),      hashrate_pretty_unit( m_last_block_hashrate ),
                          hashrate_pretty_value( pool_target->pool_hashrate ), hashrate_pretty_unit( pool_target->pool_hashrate ),
                          hashrate_pretty_value( pool_target->mnet_hashrate ), hashrate_pretty_unit( pool_target->mnet_hashrate ) );
            NOSO_LOG_INFO << msg << std::endl;
            if ( pool_target->payment_block == pool_target->blck_no ) {
                std::snprintf( msg, 100, " Paid %.8g NOSO", pool_target->payment_amount / 100'000'000.0 );
                NOSO_LOG_INFO << msg << std::endl;
                std::snprintf( msg, 100, " %s", pool_target->payment_order_id.c_str() );
                NOSO_LOG_INFO << msg << std::endl;
            }
        }
    }
    std::snprintf( msg, 100, "---------------------------------------------------" );
    NOSO_LOG_INFO << msg << std::endl;
    std::snprintf( msg, 100, "BLOCK %06u       %-32s", target->blck_no + 1, target->lb_hash.substr( 0, 32 ).c_str() );
    NOSO_LOG_INFO << msg << std::endl;
    if ( g_solo_mining ) {
        std::shared_ptr<CNodeTarget> node_target { std::dynamic_pointer_cast<CNodeTarget>( target ) };
        std::snprintf( msg, 100, " Solo %-12s %-32s",
                        std::string( "Mainnet" ).substr( 0, 12 ).c_str(),
                        node_target->mn_diff.substr( 0, 32).c_str() );
        NOSO_LOG_INFO << msg << std::endl;
    } else {
        std::shared_ptr<CPoolTarget> pool_target { std::dynamic_pointer_cast<CPoolTarget>( target ) };
        std::snprintf( msg, 100, " Pool %-12s %-32s",
                        pool_target->pool_name.substr( 0, 12 ).c_str(),
                        pool_target->mn_diff.substr( 0, 32).c_str() );
        NOSO_LOG_INFO << msg << std::endl;
    }
    if ( g_solo_mining ) {
        std::shared_ptr<CNodeTarget> node_target { std::dynamic_pointer_cast<CNodeTarget>( target ) };
    } else {
        std::shared_ptr<CPoolTarget> pool_target { std::dynamic_pointer_cast<CPoolTarget>( target ) };
    }
}

void CCommThread::_ReportTargetSummary( const std::shared_ptr<CTarget>& target, const std::chrono::duration<double>& elapsed_blck ) {
    m_last_block_hashes_count = 0;
    m_last_block_elapsed_secs = elapsed_blck.count();
    m_last_thread_minings.clear();
    for_each( g_mine_objects.begin(), g_mine_objects.end(), [&]( const auto &object ){
                 auto block_summary = object->GetBlockSummary();
                 m_last_thread_minings.push_back( block_summary );
                 std::uint64_t thread_hashes { std::get<0>( block_summary ) };
                 m_last_block_hashes_count += thread_hashes; } );
    m_last_block_hashrate = m_last_block_hashes_count / m_last_block_elapsed_secs;
    char msg[100];
    if ( g_solo_mining ) {
        std::shared_ptr<CNodeTarget> node_target { std::dynamic_pointer_cast<CNodeTarget>( target ) };
        std::snprintf( msg, 100, " Sent %5u / %4u / %3u",
                        m_accepted_solutions_count, m_rejected_solutions_count, m_failured_solutions_count );
        NOSO_LOG_INFO << msg << std::endl;
    } else {
        std::shared_ptr<CPoolTarget> pool_target { std::dynamic_pointer_cast<CPoolTarget>( target ) };
        std::snprintf( msg, 100, " Sent %5u / %4u / %3u | %14.8g NOSO [%2u]",
                        m_accepted_solutions_count, m_rejected_solutions_count, m_failured_solutions_count,
                        pool_target->till_balance / 100'000'000.0, pool_target->till_payment );
        NOSO_LOG_INFO << msg << std::endl;
    }
};

void CCommThread::_ResetMiningBlock() {
    m_accepted_solutions_count = 0;
    m_rejected_solutions_count = 0;
    m_failured_solutions_count = 0;
    this->ClearSolutions();
};

void CCommThread::_ReportErrorSubmitting( int code, const std::shared_ptr<CSolution> &solution ) {
    if      ( code == 1 ) {
        NOSO_LOG_ERROR
            << "    ERROR"
            << ")base[" << solution->base
            << "]hash[" << solution->hash
            << "]Wrong block#" << solution->blck << " submitted!"
            << std::endl;
    } else if ( code == 2 ) {
        NOSO_LOG_ERROR
            << "    ERROR"
            << ")base[" << solution->base
            << "]hash[" << solution->hash
            << "]Incorrect timestamp submitted!"
            << std::endl;
    } else if ( code == 3 ) {
        NOSO_LOG_ERROR
            << "    ERROR"
            << ")base[" << solution->base
            << "]hash[" << solution->hash
            << "]Invalid address (" << g_miner_address << ") submitted!"
            << std::endl;
    } else if ( code == 7 ) {
        NOSO_LOG_ERROR
            << "    ERROR"
            << ")base[" << solution->base
            << "]hash[" << solution->hash
            << "]Wrong hash base submitted!"
            << std::endl;
    } else if ( code == 4 ) {
        NOSO_LOG_ERROR
            << "    ERROR"
            << ")base[" << solution->base
            << "]hash[" << solution->hash
            << "]Duplicate hash submitted!"
            << std::endl;
    }
}

std::shared_ptr<CTarget> CCommThread::GetTarget( const char prev_lb_hash[32] ) {
    assert( std::strlen( prev_lb_hash ) == 32 );
    std::shared_ptr<CTarget> target { nullptr };
    if ( g_solo_mining ) target = this->GetNodeTargetConsensus();
    else target = this->GetPoolTargetFailover();
    while ( g_still_running && ( target == nullptr || ( target !=nullptr && target->lb_hash == prev_lb_hash ) ) ) {
        std::this_thread::sleep_for( std::chrono::milliseconds( static_cast<int>( 1'000 * INET_CIRCLE_SECONDS ) ) );
        if ( g_solo_mining ) target = this->GetNodeTargetConsensus();
        else target = this->GetPoolTargetFailover();
    }
    return target;
}

void CCommThread::SubmitSolution( const std::shared_ptr<CSolution> &solution, std::shared_ptr<CTarget> &target ) {
    int code { 0 };
    if ( g_solo_mining ) {
        target->mn_diff = solution->diff;
        char new_mn_diff[33] { NOSO_MAX_DIFF };
        code = this->SubmitSoloSolution( solution->blck, solution->base.c_str(), g_miner_address, new_mn_diff );
        if ( target->mn_diff > new_mn_diff ) target->mn_diff = new_mn_diff;
        if ( code == 6 ) {
            if ( solution->diff < new_mn_diff ) this->AddSolution( solution );
            NOSO_LOG_DEBUG
                << " WAITBLCK"
                << ")base[" << solution->base
                << "]hash[" << solution->hash
                << "]Network building block!"
                << std::endl;
        }
    } else {
        code = this->SubmitPoolSolution( solution->blck, solution->base.c_str(), g_miner_address );
    }
    if ( code > 0 ) this->_ReportErrorSubmitting( code, solution ); // rest other error codes 1, 2, 3, 4, 7
    if ( code == 0 ) {
        m_accepted_solutions_count ++;
        NOSO_LOG_DEBUG
            << " ACCEPTED"
            << ")base[" << solution->base
            << "]hash[" << solution->hash
            << "]"
            << std::endl;
    } else if ( code > 0) {
        m_rejected_solutions_count ++;
        NOSO_LOG_DEBUG
            << " REJECTED"
            << ")base[" << solution->base
            << "]hash[" << solution->hash
            << "]"
            << std::endl;
    } else {
        this->AddSolution( solution );
        m_failured_solutions_count ++;
        NOSO_LOG_DEBUG
            << " FAILURED"
            << ")base[" << solution->base
            << "]hash[" << solution->hash
            << "]Will retry submitting!"
            << std::endl;
    }
}

void CCommThread::Communicate() {
    long NOSO_BLOCK_AGE_TARGET_SAFE { 10 };
    char prev_lb_hash[33] { NOSO_NUL_HASH };
    auto begin_blck = std::chrono::steady_clock::now();
    while ( g_still_running ) {
        if ( NOSO_BLOCK_AGE < NOSO_BLOCK_AGE_TARGET_SAFE || 585 < NOSO_BLOCK_AGE ) {
            do {
                std::this_thread::sleep_for( std::chrono::milliseconds( static_cast<int>( 1'000 * INET_CIRCLE_SECONDS ) ) );
            } while ( g_still_running && ( NOSO_BLOCK_AGE < NOSO_BLOCK_AGE_TARGET_SAFE || 585 < NOSO_BLOCK_AGE ) );
            if ( !g_still_running ) break;
        }
        NOSO_BLOCK_AGE_TARGET_SAFE = g_solo_mining ? 1 : 6;
        std::shared_ptr<CTarget> target = this->GetTarget( prev_lb_hash );
        std::strcpy( prev_lb_hash, target->lb_hash.c_str() );
        if ( !g_still_running ) break;
        begin_blck = std::chrono::steady_clock::now();
        for ( auto mo : g_mine_objects ) mo->NewTarget( target );
        this->_ReportMiningTarget( target );
        while ( g_still_running && NOSO_BLOCK_AGE <= 585 ) {
            auto begin_submit = std::chrono::steady_clock::now();
            if ( NOSO_BLOCK_AGE >= 10 ) {
                std::shared_ptr<CSolution> solution = this->GetSolution();
                if ( solution != nullptr && solution->diff < target->mn_diff )
                    this->SubmitSolution( solution, target );
            }
            std::chrono::duration<double> elapsed_submit = std::chrono::steady_clock::now() - begin_submit;
            if ( elapsed_submit.count() < INET_CIRCLE_SECONDS ) {
                std::this_thread::sleep_for( std::chrono::milliseconds( static_cast<int>( 1'000 * INET_CIRCLE_SECONDS ) ) );
            }
        } // END while ( g_still_running && NOSO_BLOCK_AGE <= 585 ) {
        std::chrono::duration<double> elapsed_blck = std::chrono::steady_clock::now() - begin_blck;
        this->_ReportTargetSummary( target, elapsed_blck );
        this->_ResetMiningBlock();
    } // END while ( g_still_running ) {
    for ( auto &obj : g_mine_objects ) obj->CleanupSyncState();
    for ( auto &thr : g_mine_threads ) thr.join();
}

inline bool is_valid_address( const std::string& address ) {
    if ( address.length() < 30 || address.length() > 31 ) return false;
    return true;
}

inline bool is_valid_minerid( std::uint32_t minerid ) {
    if ( minerid < 0 || minerid > 8100 ) return false;
    return true;
}

inline bool is_valid_threads( std::uint32_t count ) {
    if ( count < 2 ) return false;
    return true;
}

inline char hashrate_pretty_unit( std::uint64_t count ) {
    return
          ( count /             1'000.0 ) < 1'000 ? 'K' /* Kilo */
        : ( count /         1'000'000.0 ) < 1'000 ? 'M' /* Mega */
        : ( count /     1'000'000'000.0 ) < 1'000 ? 'G' /* Giga */
        : ( count / 1'000'000'000'000.0 ) < 1'000 ? 'T' /* Tera */
        :                                           'P' /* Peta */;
};

inline double hashrate_pretty_value( std::uint64_t count ) {
    return
          ( count /             1'000.0 ) < 1'000 ? ( count /                 1'000.0 ) /* Kilo */
        : ( count /         1'000'000.0 ) < 1'000 ? ( count /             1'000'000.0 ) /* Mega */
        : ( count /     1'000'000'000.0 ) < 1'000 ? ( count /         1'000'000'000.0 ) /* Giga */
        : ( count / 1'000'000'000'000.0 ) < 1'000 ? ( count /     1'000'000'000'000.0 ) /* Tera */
        :                                           ( count / 1'000'000'000'000'000.0 ) /* Peta */;
};

std::vector<std::tuple<std::string, std::string, std::string>> parse_pools_argv( const std::string& poolstr ) {
    const std::regex re_pool1 { ";|[[:space:]]" };
    const std::regex re_pool2 {
        "^"
        "("
            "[a-zA-Z][a-zA-Z0-9\\-]*[a-zA-Z0-9]"
        ")"
        "(\\:"
            "("
                "("
                    "(25[0-5]|2[0-4][0-9]|1[0-9]{2}|[1-9][0-9]|[0-9])\\."
                    "(25[0-5]|2[0-4][0-9]|1[0-9]{2}|[1-9][0-9]|[0-9])\\."
                    "(25[0-5]|2[0-4][0-9]|1[0-9]{2}|[1-9][0-9]|[0-9])\\."
                    "(25[0-5]|2[0-4][0-9]|1[0-9]{2}|[1-9][0-9]|[0-9])"
                ")"
                "|"
                "("
                    "([a-zA-Z0-9]+(\\-[a-zA-Z0-9]+)*\\.)*[a-zA-Z]{2,}"
                ")"
            ")"
            "(\\:"
                "("
                    "6553[0-5]|655[0-2][0-9]|65[0-4][0-9]{2}|6[0-4][0-9]{3}|[1-5][0-9]{4}|[0-5]{0,5}|[0-9]{1,4}"
                ")$"
            ")?"
        ")?"
        "$"
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
                            std::string port { sm0[13].str() };
                            if ( host.length() <= 0 ) {
                                const auto def_pool = std::find_if(
                                        g_default_pools.begin(), g_default_pools.end(),
                                        [&name]( const auto& val ) {
                                            std::string def_name { std::get<0>( val ) };
                                            bool iequal = std::equal(
                                                    name.begin(), name.end(), def_name.begin(), def_name.end(),
                                                    []( unsigned char a, unsigned char b ) { return tolower( a ) == tolower( b ); });
                                            return iequal ? true : false; } );
                                if ( def_pool != g_default_pools.end() ) {
                                    host = std::get<1>( *def_pool );
                                    if ( port.length() <= 0 ) port = std::get<2>( *def_pool );
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

inline int inet_init() {
    #ifdef _WIN32
    WSADATA wsaData;
    return WSAStartup( MAKEWORD( 2, 2 ), &wsaData ) != NO_ERROR ? -1 : 0;
    #endif
    return 0;
}

inline void inet_cleanup() {
    #ifdef _WIN32
    WSACleanup();
    #endif
}

inline void inet_close_socket( int sockfd ) {
    #ifdef _WIN32
    closesocket( sockfd );
    #else
    close( sockfd );
    #endif
}

inline int inet_set_nonblock( int sockfd ) {
    #ifdef _WIN32
    u_long mode = 1;
    if ( ioctlsocket( sockfd, FIONBIO, &mode ) != NO_ERROR ) return -1;
    #else // LINUX/UNIX
    int flags = 0;
    if ( ( flags = fcntl( sockfd, F_GETFL, 0 ) ) < 0 ) return -1;
    if ( fcntl( sockfd, F_SETFL, flags | O_NONBLOCK ) < 0 ) return -1;
    #endif // END #ifdef _WIN32
    return sockfd;
}

inline int inet_socket( struct addrinfo *serv_info, int timeosec ) {
    struct addrinfo *psi = serv_info;
    struct timeval timeout {
        .tv_sec = timeosec,
        .tv_usec = 0
    };
    int sockfd, rc;
    fd_set rset, wset;
    for( ; psi != NULL; psi = psi->ai_next ) {
        if ( (sockfd = socket( psi->ai_family, psi->ai_socktype,
                               psi->ai_protocol ) ) == -1 ) continue;
        if ( inet_set_nonblock( sockfd ) < 0 ) {
            inet_close_socket( sockfd );
            continue;
        }
        if ( ( rc = connect( sockfd, psi->ai_addr, psi->ai_addrlen ) ) >= 0 )
            return sockfd;
        #ifdef _WIN32
        if ( rc != SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK ) {
            closesocket( sockfd );
            continue;
        }
        #else // LINUX/UNIX
        if ( errno != EINPROGRESS ) {
            close( sockfd );
            continue;
        }
        #endif // END #ifdef _WIN32
        FD_ZERO( &rset );
        FD_ZERO( &wset );
        FD_SET( sockfd, &rset );
        FD_SET( sockfd, &wset );
        int n = select( sockfd + 1, &rset, &wset, NULL, &timeout );
        if ( n <= 0 ) inet_close_socket( sockfd );
        if ( FD_ISSET( sockfd, &rset ) || FD_ISSET( sockfd, &wset ) ) {
            int error = 0;
            socklen_t slen = sizeof( error );
            if ( getsockopt( sockfd, SOL_SOCKET, SO_ERROR, (char*)&error, &slen ) < 0 ) {
                inet_close_socket( sockfd );
                continue;
            }
            if ( error ) {
                inet_close_socket( sockfd );
                continue;
            }
        }
        else continue;
        return sockfd;
    } // END for( ; psi != NULL; psi = psi->ai_next ) {
    return -1;
}

inline int inet_send( int sockfd, int timeosec, const char *message, size_t size ) {
    struct timeval timeout {
        .tv_sec = timeosec,
        .tv_usec = 0
    };
    fd_set fds;
    FD_ZERO( &fds );
    FD_SET( sockfd, &fds );
    int n = select( sockfd + 1, NULL, &fds, NULL, &timeout );
    if ( n <= 0 ) return n; /* n == 0 timeout, n == -1 socket error */
    int slen = send( sockfd, message, size, 0 );
    if ( slen <= 0 ) return slen; /* slen == 0 timeout, slen == -1 socket error */
    return slen;
}

inline int inet_recv( int sockfd, int timeosec, char *buffer, size_t buffsize ) {
    struct timeval timeout {
        .tv_sec = timeosec,
        .tv_usec = 0
    };
    fd_set fds;
    FD_ZERO( &fds );
    FD_SET( sockfd, &fds );
    int n = select( sockfd + 1, &fds, NULL, NULL, &timeout );
    if ( n <= 0 ) return n; /* n == 0 timeout, n == -1 socket error */
    int rlen = recv( sockfd, buffer, buffsize - 1, 0 );
    if ( rlen <= 0 ) return rlen; /* rlen == 0 timeout, nlen == -1 socket error */
    buffer[ rlen ] = '\0';
    return rlen;
}

inline int inet_command( struct addrinfo *serv_info, uint32_t timeosec, char *buffer, size_t buffsize ) {
    int sockfd = inet_socket( serv_info, timeosec );
    if ( sockfd < 0 ) return sockfd;
    int rlen = 0;
    int slen = inet_send( sockfd, timeosec, buffer, buffsize );
    if ( slen > 0 ) rlen = inet_recv( sockfd, timeosec, buffer, buffsize );
    inet_close_socket( sockfd );
    if ( slen <= 0 ) return slen;
    return rlen;
}

inline struct addrinfo * CInet::InitService() {
    struct addrinfo hints, *serv_info;
    std::memset( &hints, 0, sizeof( hints ) );
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    int n = getaddrinfo( this->m_host.c_str(), this->m_port.c_str(), &hints, &serv_info );
    if ( n ) {
        NOSO_LOG_DEBUG
            << "CInet::InitService/getaddrinfo: " << gai_strerror( n )
            << std::endl;
        return NULL;
    }
    return serv_info;
}

inline void CInet::CleanService( struct addrinfo * serv_info ) {
    if ( serv_info == NULL ) return;
    freeaddrinfo( serv_info );
    serv_info = NULL;
}

int CInet::ExecCommand( char *buffer, std::size_t buffsize ) {
    assert( buffer && buffsize > 0 );
    struct addrinfo * serv_info = this->InitService();
    if ( serv_info == NULL ) return -1;
    int n = inet_command( serv_info, m_timeosec, buffer, buffsize );
    this->CleanService( serv_info );
    return n;
}

inline std::string& ltrim( std::string& s ) {
    s.erase( s.begin(), std::find_if( s.begin(), s.end(),
                                     []( unsigned char ch ) {
                                         return !std::isspace( ch ); } ) );
    return s;
}

inline std::string& rtrim( std::string& s ) {
    s.erase( std::find_if( s.rbegin(), s.rend(),
                          []( unsigned char ch ) {
                              return !std::isspace( ch ); } ).base(), s.end() );
    return s;
}

inline bool iequal( const std::string& s1, const std::string& s2 ) {
    return std::equal( s1.cbegin(), s1.cend(), s2.cbegin(), s2.cend(),
                      []( unsigned char a, unsigned char b ) {
                          return tolower( a ) == tolower( b ); } );
}
