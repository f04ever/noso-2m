#include <cstring>
#include "hashing.hpp"

constexpr char const CNosoHasher::hex_dec2char_table[];
constexpr char const CNosoHasher::hex_char2dec_table[];
constexpr std::uint16_t const CNosoHasher::nosohash_chars_table[];

inline
void CNosoHasher::_ito9a( std::uint32_t n, char * buff ) {
    // constexpr static
    // std::uint32_t const number_of_row[] {
    //     100'000'000,
    //      10'000'000,
    //       1'000'000,
    //         100'000,
    //          10'000,
    //           1'000,
    //             100,
    //              10,
    //               1, };
    // n = n % 1'000'000'000;
    // for ( std::uint8_t i = 0; i < 9; i++ ) {
    //     assert( 0 <= n / number_of_row[i] && n / number_of_row[i] <= 9 );
    //     buff[i] = n / number_of_row[i] + '0';
    //     n = n % number_of_row[i];
    // }
    // buff[9] = '\0';
          n = n % 1'000'000'000;
    buff[0] = n /   100'000'000 + '0';
          n = n %   100'000'000;
    buff[1] = n /    10'000'000 + '0';
          n = n %    10'000'000;
    buff[2] = n /     1'000'000 + '0';
          n = n %     1'000'000;
    buff[3] = n /      1'00'000 + '0';
          n = n %      1'00'000;
    buff[4] = n /        10'000 + '0';
          n = n %        10'000;
    buff[5] = n /         1'000 + '0';
          n = n %         1'000;
    buff[6] = n /           100 + '0';
          n = n %           100;
    buff[7] = n /            10 + '0';
          n = n %            10;
    // buff[8] = n /             1 + '0';
    buff[8] = n                + '0';
    buff[9] = '\0';
}

inline
void CNosoHasher::_hash() {
//     constexpr static std::uint8_t const _col2ndid[] {
//   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,
//  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,
//  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,
//  61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,  80,
//  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,  96,  97,  98,  99, 100,
// 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120,
// 121, 122, 123, 124, 125, 126, 127,   0, };
    for( int row = 1; row < 129; row++ ) {
        int const row0_128 = (row-0)*128;
        int const row1_128 = (row-1)*128;
        for( int col = 0; col < 127; col++ ) {
            // m_stat[row*128+col] = nosohash_chars_table[ m_stat[(row-1)*128+col] + m_stat[(row-1)*128+col+1] ];
            m_stat[row0_128+col] = nosohash_chars_table[ m_stat[row1_128+col] + m_stat[row1_128+col+1] ];
        }
        m_stat[row0_128+127] = nosohash_chars_table[ m_stat[row1_128+127] + m_stat[row1_128+0] ];
        // for( int col = 0; col < 128; col++ ) {
        //     // m_stat[row*128+col] = nosohash_chars_table[ m_stat[(row-1)*128+col] + m_stat[(row-1)*128+(col+1)%128] ];
        //     // m_stat[row0_128+col] = nosohash_chars_table[ m_stat[row1_128+col] + m_stat[row1_128+(col+1)%128] ];
        //     m_stat[row0_128+col] = nosohash_chars_table[ m_stat[row1_128+col] + m_stat[row1_128+_col2ndid[col]] ];
        // }
    }
    // for ( int i = 0; i < 32; i++ ) {
    //     int const i_hash = 128*128 + i*4;
    //     m_hash[i] = hex_dec2char_table[ nosohash_chars_table[ m_stat[i_hash+0] + m_stat[i_hash+1] + m_stat[i_hash+2] + m_stat[i_hash+3] ] % 16 ];
    // }
    // m_hash[32] = '\0';
    m_hash[ 0] = hex_dec2char_table[ nosohash_chars_table[ m_stat[16384+  0] + m_stat[16384+  1] + m_stat[16384+  2] + m_stat[16384+  3] ] % 16 ];
    m_hash[ 1] = hex_dec2char_table[ nosohash_chars_table[ m_stat[16384+  4] + m_stat[16384+  5] + m_stat[16384+  6] + m_stat[16384+  7] ] % 16 ];
    m_hash[ 2] = hex_dec2char_table[ nosohash_chars_table[ m_stat[16384+  8] + m_stat[16384+  9] + m_stat[16384+ 10] + m_stat[16384+ 11] ] % 16 ];
    m_hash[ 3] = hex_dec2char_table[ nosohash_chars_table[ m_stat[16384+ 12] + m_stat[16384+ 13] + m_stat[16384+ 14] + m_stat[16384+ 15] ] % 16 ];
    m_hash[ 4] = hex_dec2char_table[ nosohash_chars_table[ m_stat[16384+ 16] + m_stat[16384+ 17] + m_stat[16384+ 18] + m_stat[16384+ 19] ] % 16 ];
    m_hash[ 5] = hex_dec2char_table[ nosohash_chars_table[ m_stat[16384+ 20] + m_stat[16384+ 21] + m_stat[16384+ 22] + m_stat[16384+ 23] ] % 16 ];
    m_hash[ 6] = hex_dec2char_table[ nosohash_chars_table[ m_stat[16384+ 24] + m_stat[16384+ 25] + m_stat[16384+ 26] + m_stat[16384+ 27] ] % 16 ];
    m_hash[ 7] = hex_dec2char_table[ nosohash_chars_table[ m_stat[16384+ 28] + m_stat[16384+ 29] + m_stat[16384+ 30] + m_stat[16384+ 31] ] % 16 ];
    m_hash[ 8] = hex_dec2char_table[ nosohash_chars_table[ m_stat[16384+ 32] + m_stat[16384+ 33] + m_stat[16384+ 34] + m_stat[16384+ 35] ] % 16 ];
    m_hash[ 9] = hex_dec2char_table[ nosohash_chars_table[ m_stat[16384+ 36] + m_stat[16384+ 37] + m_stat[16384+ 38] + m_stat[16384+ 39] ] % 16 ];
    m_hash[10] = hex_dec2char_table[ nosohash_chars_table[ m_stat[16384+ 40] + m_stat[16384+ 41] + m_stat[16384+ 42] + m_stat[16384+ 43] ] % 16 ];
    m_hash[11] = hex_dec2char_table[ nosohash_chars_table[ m_stat[16384+ 44] + m_stat[16384+ 45] + m_stat[16384+ 46] + m_stat[16384+ 47] ] % 16 ];
    m_hash[12] = hex_dec2char_table[ nosohash_chars_table[ m_stat[16384+ 48] + m_stat[16384+ 49] + m_stat[16384+ 50] + m_stat[16384+ 51] ] % 16 ];
    m_hash[13] = hex_dec2char_table[ nosohash_chars_table[ m_stat[16384+ 52] + m_stat[16384+ 53] + m_stat[16384+ 54] + m_stat[16384+ 55] ] % 16 ];
    m_hash[14] = hex_dec2char_table[ nosohash_chars_table[ m_stat[16384+ 56] + m_stat[16384+ 57] + m_stat[16384+ 58] + m_stat[16384+ 59] ] % 16 ];
    m_hash[15] = hex_dec2char_table[ nosohash_chars_table[ m_stat[16384+ 60] + m_stat[16384+ 61] + m_stat[16384+ 62] + m_stat[16384+ 63] ] % 16 ];
    m_hash[16] = hex_dec2char_table[ nosohash_chars_table[ m_stat[16384+ 64] + m_stat[16384+ 65] + m_stat[16384+ 66] + m_stat[16384+ 67] ] % 16 ];
    m_hash[17] = hex_dec2char_table[ nosohash_chars_table[ m_stat[16384+ 68] + m_stat[16384+ 69] + m_stat[16384+ 70] + m_stat[16384+ 71] ] % 16 ];
    m_hash[18] = hex_dec2char_table[ nosohash_chars_table[ m_stat[16384+ 72] + m_stat[16384+ 73] + m_stat[16384+ 74] + m_stat[16384+ 75] ] % 16 ];
    m_hash[19] = hex_dec2char_table[ nosohash_chars_table[ m_stat[16384+ 76] + m_stat[16384+ 77] + m_stat[16384+ 78] + m_stat[16384+ 79] ] % 16 ];
    m_hash[20] = hex_dec2char_table[ nosohash_chars_table[ m_stat[16384+ 80] + m_stat[16384+ 81] + m_stat[16384+ 82] + m_stat[16384+ 83] ] % 16 ];
    m_hash[21] = hex_dec2char_table[ nosohash_chars_table[ m_stat[16384+ 84] + m_stat[16384+ 85] + m_stat[16384+ 86] + m_stat[16384+ 87] ] % 16 ];
    m_hash[22] = hex_dec2char_table[ nosohash_chars_table[ m_stat[16384+ 88] + m_stat[16384+ 89] + m_stat[16384+ 90] + m_stat[16384+ 91] ] % 16 ];
    m_hash[23] = hex_dec2char_table[ nosohash_chars_table[ m_stat[16384+ 92] + m_stat[16384+ 93] + m_stat[16384+ 94] + m_stat[16384+ 95] ] % 16 ];
    m_hash[24] = hex_dec2char_table[ nosohash_chars_table[ m_stat[16384+ 96] + m_stat[16384+ 97] + m_stat[16384+ 98] + m_stat[16384+ 99] ] % 16 ];
    m_hash[25] = hex_dec2char_table[ nosohash_chars_table[ m_stat[16384+100] + m_stat[16384+101] + m_stat[16384+102] + m_stat[16384+103] ] % 16 ];
    m_hash[26] = hex_dec2char_table[ nosohash_chars_table[ m_stat[16384+104] + m_stat[16384+105] + m_stat[16384+106] + m_stat[16384+107] ] % 16 ];
    m_hash[27] = hex_dec2char_table[ nosohash_chars_table[ m_stat[16384+108] + m_stat[16384+109] + m_stat[16384+110] + m_stat[16384+111] ] % 16 ];
    m_hash[28] = hex_dec2char_table[ nosohash_chars_table[ m_stat[16384+112] + m_stat[16384+113] + m_stat[16384+114] + m_stat[16384+115] ] % 16 ];
    m_hash[29] = hex_dec2char_table[ nosohash_chars_table[ m_stat[16384+116] + m_stat[16384+117] + m_stat[16384+118] + m_stat[16384+119] ] % 16 ];
    m_hash[30] = hex_dec2char_table[ nosohash_chars_table[ m_stat[16384+120] + m_stat[16384+121] + m_stat[16384+122] + m_stat[16384+123] ] % 16 ];
    m_hash[31] = hex_dec2char_table[ nosohash_chars_table[ m_stat[16384+124] + m_stat[16384+125] + m_stat[16384+126] + m_stat[16384+127] ] % 16 ];
    m_hash[32] = '\0';
}

inline
void CNosoHasher::_md5() {
    assert( std::strlen( m_hash ) == 32 );
    md5Init( &m_md5_ctx );
    md5Update( &m_md5_ctx, (uint8_t *)m_hash, 32 );
    md5Finalize( &m_md5_ctx );
    // int i_hash = 0;
    // for ( int i = 0; i < 16; i++ ) {
    //     m_hash[i_hash++] = hex_dec2char_table[m_md5_ctx.digest[i] >>  4];
    //     m_hash[i_hash++] = hex_dec2char_table[m_md5_ctx.digest[i] & 0xF];
    // }
    // m_hash[32] = '\0';
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

CNosoHasher::CNosoHasher( char const prefix[10], char const address[32] ) {
    constexpr static const char NOSOHASH_FILLER_CHARS[] = "%)+/5;=CGIOSYaegk";
    constexpr static const int NOSOHASH_FILLER_COUNT = 17;
    assert( std::strlen( prefix ) == 9
            && ( std::strlen( address ) == 30 || std::strlen( address ) == 31 ) );
    std::memcpy( m_base, prefix, 9 );
    std::memcpy( m_stat, m_base, 9 );
    CNosoHasher::_ito9a( 0, m_base + 9 );
    assert( std::strlen( m_base ) == 18 );
    std::memcpy( m_stat + 9, m_base + 9, 9 );
    int addr_len = 30 + (address[30] ? 1 : 0);
    std::memcpy( m_stat + 9 + 9, address, addr_len );
    int len = 18 + addr_len;
    int div = ( 128 - len ) / NOSOHASH_FILLER_COUNT;
    int mod = ( 128 - len ) % NOSOHASH_FILLER_COUNT;
    for ( int i = 0; i < div; i++ ) {
        std::memcpy( m_stat + len, NOSOHASH_FILLER_CHARS, NOSOHASH_FILLER_COUNT );
        len += NOSOHASH_FILLER_COUNT;
    }
    std::memcpy( m_stat + len, NOSOHASH_FILLER_CHARS, mod );
    assert( std::none_of( m_stat, m_stat + 128, []( int c ){ return 33 > c || c > 126; } ) );
}

char const * CNosoHasher::GetBase( std::uint32_t counter ) {
    CNosoHasher::_ito9a( counter, m_base + 9 );
    assert( std::strlen( m_base ) == 18 );
    std::memcpy( m_stat + 9, m_base + 9, 9 );
    return m_base;
}

char const * CNosoHasher::GetHash() {
    this->_hash();
    this->_md5();
    return m_hash;
}

char const * CNosoHasher::GetDiff( char const target[33] ) {
    assert( std::strlen( m_hash ) == 32
           && std::strlen( target ) == 32 );
    // for ( int i = 0; i < 32; i++ )
    //     m_diff[i] = hex_dec2char_table[ std::abs( hex_char2dec_table[ (int)m_hash[i] ] - hex_char2dec_table[ (int)target[i] ] ) ];
    // m_diff[32] = '\0';
    // return m_diff;
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

constexpr static const char NOSOHASH_HASHEABLE_CHARS[] {
    "!\"#$%&')*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~" };
constexpr static const std::size_t NOSOHASH_HASHEABLE_COUNT =  92;
std::string nosohash_prefix( int num ) {
    return std::string {
        NOSOHASH_HASHEABLE_CHARS[ num / NOSOHASH_HASHEABLE_COUNT ],
        NOSOHASH_HASHEABLE_CHARS[ num % NOSOHASH_HASHEABLE_COUNT ], };
}

