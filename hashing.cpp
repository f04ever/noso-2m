#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <cstring>
#include "hashing.hpp"

constexpr char const CNosoHasher::hex_dec2char_table[];
constexpr char const CNosoHasher::hex_char2dec_table[];
constexpr std::uint16_t const CNosoHasher::nosohash_chars_table[];

inline
void CNosoHasher::_init( char const prefix[10], char const address[32] ) {
    assert( std::strlen( prefix ) == 9
            && ( std::strlen( address ) == 30 || std::strlen( address ) == 31 ) );
    std::memcpy( m_stat, prefix, 9 );
    std::memcpy( m_stat + 9, "000000000", 9 );
    std::memcpy( m_stat + 18, address, 32 );
    constexpr static char const filler_chars[] = "%)+/5;=CGIOSYaegk%)+/5;=CGIOSYaegk%)+/5;=CGIOSYaegk%)+/5;=CGIOSYaegk%)+/5;=CGIOSYaegk";
    size_t const len = 18 + 30 + (address[30] ? 1 : 0);
    std::memcpy( m_stat + len, filler_chars, 128 - len );
    assert( std::none_of( m_stat, m_stat + 128, []( int c ){ return 33 > c || c > 126; } ) );
    std::memcpy( m_base, m_stat, 18 );
    m_base[18] = '\0';
    assert( std::strlen( m_base ) == 18 );
    m_hash[32] = '\0';
    m_diff[32] = '\0';
}

inline
void CNosoHasher::_itoa( std::uint32_t n ) {
             n = n % 1'000'000'000;
    m_stat[ 9] = n /   100'000'000 + '0';
             n = n %   100'000'000;
    m_stat[10] = n /    10'000'000 + '0';
             n = n %    10'000'000;
    m_stat[11] = n /     1'000'000 + '0';
             n = n %     1'000'000;
    m_stat[12] = n /      1'00'000 + '0';
             n = n %      1'00'000;
    m_stat[13] = n /        10'000 + '0';
             n = n %        10'000;
    m_stat[14] = n /         1'000 + '0';
             n = n %         1'000;
    m_stat[15] = n /           100 + '0';
             n = n %           100;
    m_stat[16] = n /            10 + '0';
             n = n %            10;
    m_stat[17] = n                 + '0';
}

inline
void CNosoHasher::_stat() {
    char * stat_1 = m_stat + 129;
    std::memcpy( stat_1, m_stat, 128 );
    stat_1[128]=stat_1[0];
    for( size_t row = 0; row < 128; ++row ) {
        for( size_t col = 0; col < 128; ++col ) {
            stat_1[col] = nosohash_chars_table[stat_1[col + 0] + stat_1[col + 1]];
        }
        stat_1[128]=stat_1[0];
    }
}

inline
void CNosoHasher::_pack() {
    m_hash[ 0] = hex_dec2char_table[nosohash_chars_table[m_stat[129+  0]+m_stat[129+  1]+m_stat[129+  2]+m_stat[129+  3]]%16];
    m_hash[ 1] = hex_dec2char_table[nosohash_chars_table[m_stat[129+  4]+m_stat[129+  5]+m_stat[129+  6]+m_stat[129+  7]]%16];
    m_hash[ 2] = hex_dec2char_table[nosohash_chars_table[m_stat[129+  8]+m_stat[129+  9]+m_stat[129+ 10]+m_stat[129+ 11]]%16];
    m_hash[ 3] = hex_dec2char_table[nosohash_chars_table[m_stat[129+ 12]+m_stat[129+ 13]+m_stat[129+ 14]+m_stat[129+ 15]]%16];
    m_hash[ 4] = hex_dec2char_table[nosohash_chars_table[m_stat[129+ 16]+m_stat[129+ 17]+m_stat[129+ 18]+m_stat[129+ 19]]%16];
    m_hash[ 5] = hex_dec2char_table[nosohash_chars_table[m_stat[129+ 20]+m_stat[129+ 21]+m_stat[129+ 22]+m_stat[129+ 23]]%16];
    m_hash[ 6] = hex_dec2char_table[nosohash_chars_table[m_stat[129+ 24]+m_stat[129+ 25]+m_stat[129+ 26]+m_stat[129+ 27]]%16];
    m_hash[ 7] = hex_dec2char_table[nosohash_chars_table[m_stat[129+ 28]+m_stat[129+ 29]+m_stat[129+ 30]+m_stat[129+ 31]]%16];
    m_hash[ 8] = hex_dec2char_table[nosohash_chars_table[m_stat[129+ 32]+m_stat[129+ 33]+m_stat[129+ 34]+m_stat[129+ 35]]%16];
    m_hash[ 9] = hex_dec2char_table[nosohash_chars_table[m_stat[129+ 36]+m_stat[129+ 37]+m_stat[129+ 38]+m_stat[129+ 39]]%16];
    m_hash[10] = hex_dec2char_table[nosohash_chars_table[m_stat[129+ 40]+m_stat[129+ 41]+m_stat[129+ 42]+m_stat[129+ 43]]%16];
    m_hash[11] = hex_dec2char_table[nosohash_chars_table[m_stat[129+ 44]+m_stat[129+ 45]+m_stat[129+ 46]+m_stat[129+ 47]]%16];
    m_hash[12] = hex_dec2char_table[nosohash_chars_table[m_stat[129+ 48]+m_stat[129+ 49]+m_stat[129+ 50]+m_stat[129+ 51]]%16];
    m_hash[13] = hex_dec2char_table[nosohash_chars_table[m_stat[129+ 52]+m_stat[129+ 53]+m_stat[129+ 54]+m_stat[129+ 55]]%16];
    m_hash[14] = hex_dec2char_table[nosohash_chars_table[m_stat[129+ 56]+m_stat[129+ 57]+m_stat[129+ 58]+m_stat[129+ 59]]%16];
    m_hash[15] = hex_dec2char_table[nosohash_chars_table[m_stat[129+ 60]+m_stat[129+ 61]+m_stat[129+ 62]+m_stat[129+ 63]]%16];
    m_hash[16] = hex_dec2char_table[nosohash_chars_table[m_stat[129+ 64]+m_stat[129+ 65]+m_stat[129+ 66]+m_stat[129+ 67]]%16];
    m_hash[17] = hex_dec2char_table[nosohash_chars_table[m_stat[129+ 68]+m_stat[129+ 69]+m_stat[129+ 70]+m_stat[129+ 71]]%16];
    m_hash[18] = hex_dec2char_table[nosohash_chars_table[m_stat[129+ 72]+m_stat[129+ 73]+m_stat[129+ 74]+m_stat[129+ 75]]%16];
    m_hash[19] = hex_dec2char_table[nosohash_chars_table[m_stat[129+ 76]+m_stat[129+ 77]+m_stat[129+ 78]+m_stat[129+ 79]]%16];
    m_hash[20] = hex_dec2char_table[nosohash_chars_table[m_stat[129+ 80]+m_stat[129+ 81]+m_stat[129+ 82]+m_stat[129+ 83]]%16];
    m_hash[21] = hex_dec2char_table[nosohash_chars_table[m_stat[129+ 84]+m_stat[129+ 85]+m_stat[129+ 86]+m_stat[129+ 87]]%16];
    m_hash[22] = hex_dec2char_table[nosohash_chars_table[m_stat[129+ 88]+m_stat[129+ 89]+m_stat[129+ 90]+m_stat[129+ 91]]%16];
    m_hash[23] = hex_dec2char_table[nosohash_chars_table[m_stat[129+ 92]+m_stat[129+ 93]+m_stat[129+ 94]+m_stat[129+ 95]]%16];
    m_hash[24] = hex_dec2char_table[nosohash_chars_table[m_stat[129+ 96]+m_stat[129+ 97]+m_stat[129+ 98]+m_stat[129+ 99]]%16];
    m_hash[25] = hex_dec2char_table[nosohash_chars_table[m_stat[129+100]+m_stat[129+101]+m_stat[129+102]+m_stat[129+103]]%16];
    m_hash[26] = hex_dec2char_table[nosohash_chars_table[m_stat[129+104]+m_stat[129+105]+m_stat[129+106]+m_stat[129+107]]%16];
    m_hash[27] = hex_dec2char_table[nosohash_chars_table[m_stat[129+108]+m_stat[129+109]+m_stat[129+110]+m_stat[129+111]]%16];
    m_hash[28] = hex_dec2char_table[nosohash_chars_table[m_stat[129+112]+m_stat[129+113]+m_stat[129+114]+m_stat[129+115]]%16];
    m_hash[29] = hex_dec2char_table[nosohash_chars_table[m_stat[129+116]+m_stat[129+117]+m_stat[129+118]+m_stat[129+119]]%16];
    m_hash[30] = hex_dec2char_table[nosohash_chars_table[m_stat[129+120]+m_stat[129+121]+m_stat[129+122]+m_stat[129+123]]%16];
    m_hash[31] = hex_dec2char_table[nosohash_chars_table[m_stat[129+124]+m_stat[129+125]+m_stat[129+126]+m_stat[129+127]]%16];
    assert( std::strlen( m_hash ) == 32 );
}

inline
void CNosoHasher::_md5d() {
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
    assert( std::strlen( m_hash ) == 32 );
}

void CNosoHasher::_diff( char const target[33] ) {
    assert( std::strlen( m_hash ) == 32
           && std::strlen( target ) == 32 );
    m_diff[ 0] = hex_dec2char_table[std::abs(hex_char2dec_table[(int)m_hash[ 0]]-hex_char2dec_table[(int)target[ 0]])];
    m_diff[ 1] = hex_dec2char_table[std::abs(hex_char2dec_table[(int)m_hash[ 1]]-hex_char2dec_table[(int)target[ 1]])];
    m_diff[ 2] = hex_dec2char_table[std::abs(hex_char2dec_table[(int)m_hash[ 2]]-hex_char2dec_table[(int)target[ 2]])];
    m_diff[ 3] = hex_dec2char_table[std::abs(hex_char2dec_table[(int)m_hash[ 3]]-hex_char2dec_table[(int)target[ 3]])];
    m_diff[ 4] = hex_dec2char_table[std::abs(hex_char2dec_table[(int)m_hash[ 4]]-hex_char2dec_table[(int)target[ 4]])];
    m_diff[ 5] = hex_dec2char_table[std::abs(hex_char2dec_table[(int)m_hash[ 5]]-hex_char2dec_table[(int)target[ 5]])];
    m_diff[ 6] = hex_dec2char_table[std::abs(hex_char2dec_table[(int)m_hash[ 6]]-hex_char2dec_table[(int)target[ 6]])];
    m_diff[ 7] = hex_dec2char_table[std::abs(hex_char2dec_table[(int)m_hash[ 7]]-hex_char2dec_table[(int)target[ 7]])];
    m_diff[ 8] = hex_dec2char_table[std::abs(hex_char2dec_table[(int)m_hash[ 8]]-hex_char2dec_table[(int)target[ 8]])];
    m_diff[ 9] = hex_dec2char_table[std::abs(hex_char2dec_table[(int)m_hash[ 9]]-hex_char2dec_table[(int)target[ 9]])];
    m_diff[10] = hex_dec2char_table[std::abs(hex_char2dec_table[(int)m_hash[10]]-hex_char2dec_table[(int)target[10]])];
    m_diff[11] = hex_dec2char_table[std::abs(hex_char2dec_table[(int)m_hash[11]]-hex_char2dec_table[(int)target[11]])];
    m_diff[12] = hex_dec2char_table[std::abs(hex_char2dec_table[(int)m_hash[12]]-hex_char2dec_table[(int)target[12]])];
    m_diff[13] = hex_dec2char_table[std::abs(hex_char2dec_table[(int)m_hash[13]]-hex_char2dec_table[(int)target[13]])];
    m_diff[14] = hex_dec2char_table[std::abs(hex_char2dec_table[(int)m_hash[14]]-hex_char2dec_table[(int)target[14]])];
    m_diff[15] = hex_dec2char_table[std::abs(hex_char2dec_table[(int)m_hash[15]]-hex_char2dec_table[(int)target[15]])];
    m_diff[16] = hex_dec2char_table[std::abs(hex_char2dec_table[(int)m_hash[16]]-hex_char2dec_table[(int)target[16]])];
    m_diff[17] = hex_dec2char_table[std::abs(hex_char2dec_table[(int)m_hash[17]]-hex_char2dec_table[(int)target[17]])];
    m_diff[18] = hex_dec2char_table[std::abs(hex_char2dec_table[(int)m_hash[18]]-hex_char2dec_table[(int)target[18]])];
    m_diff[19] = hex_dec2char_table[std::abs(hex_char2dec_table[(int)m_hash[19]]-hex_char2dec_table[(int)target[19]])];
    m_diff[20] = hex_dec2char_table[std::abs(hex_char2dec_table[(int)m_hash[20]]-hex_char2dec_table[(int)target[20]])];
    m_diff[21] = hex_dec2char_table[std::abs(hex_char2dec_table[(int)m_hash[21]]-hex_char2dec_table[(int)target[21]])];
    m_diff[22] = hex_dec2char_table[std::abs(hex_char2dec_table[(int)m_hash[22]]-hex_char2dec_table[(int)target[22]])];
    m_diff[23] = hex_dec2char_table[std::abs(hex_char2dec_table[(int)m_hash[23]]-hex_char2dec_table[(int)target[23]])];
    m_diff[24] = hex_dec2char_table[std::abs(hex_char2dec_table[(int)m_hash[24]]-hex_char2dec_table[(int)target[24]])];
    m_diff[25] = hex_dec2char_table[std::abs(hex_char2dec_table[(int)m_hash[25]]-hex_char2dec_table[(int)target[25]])];
    m_diff[26] = hex_dec2char_table[std::abs(hex_char2dec_table[(int)m_hash[26]]-hex_char2dec_table[(int)target[26]])];
    m_diff[27] = hex_dec2char_table[std::abs(hex_char2dec_table[(int)m_hash[27]]-hex_char2dec_table[(int)target[27]])];
    m_diff[28] = hex_dec2char_table[std::abs(hex_char2dec_table[(int)m_hash[28]]-hex_char2dec_table[(int)target[28]])];
    m_diff[29] = hex_dec2char_table[std::abs(hex_char2dec_table[(int)m_hash[29]]-hex_char2dec_table[(int)target[29]])];
    m_diff[30] = hex_dec2char_table[std::abs(hex_char2dec_table[(int)m_hash[30]]-hex_char2dec_table[(int)target[30]])];
    m_diff[31] = hex_dec2char_table[std::abs(hex_char2dec_table[(int)m_hash[31]]-hex_char2dec_table[(int)target[31]])];
    assert( std::strlen( m_diff ) == 32 );
}

void CNosoHasher::Init( char const prefix[10], char const address[32] ) {
    assert( std::strlen( prefix ) == 9
            && ( std::strlen( address ) == 30 || std::strlen( address ) == 31 ) );
    this->_init( prefix, address );
    assert( std::strlen( m_base ) == 18 );
}

char const * CNosoHasher::GetBase( std::uint32_t counter ) {
    this->_itoa( counter );
    std::memcpy( m_base + 9, m_stat + 9, 9 );
    assert( std::strlen( m_base ) == 18 );
    return m_base;
}

char const * CNosoHasher::GetHash() {
    this->_stat();
    this->_pack();
    this->_md5d();
    assert( std::strlen( m_hash ) == 32 );
    return m_hash;
}

char const * CNosoHasher::GetDiff( char const target[33] ) {
    assert( std::strlen( m_hash ) == 32
           && std::strlen( target ) == 32 );
    this->_diff( target );
    assert( std::strlen( m_diff ) == 32 );
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

