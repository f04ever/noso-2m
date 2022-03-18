#include <algorithm>
#include <iomanip>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <ostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <thread>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <signal.h>
#include <getopt.h>

#include <cstdint>
#include <string>
#include <thread>
#include <iostream>
#include <ctime>
#include <tuple>
#include <vector>
#include <array>
#include <memory>
#include <map>
#include <set>
#include <chrono>
#include <sstream>
#include <cassert>
#include <numeric>
#include <algorithm>
#include <random>
#include <shared_mutex>

#include "md5-c.hpp"
#include "cxxopts.hpp"

#define DEFAULT_SOURCE "mainnet"
#define DEFAULT_MINER_ADDRESS "N3G1HhkpXvmLcsWFXySdAxX3GZpkMFS"
#define DEFAULT_MINER_ID 1000
#define DEFAULT_THREADS_COUNT 2
#define DEFAULT_INET_TIMEOSEC 10

#define CONSENSUS_NODES_COUNT 3
#define UPDATE_CIRCLE_SECONDS 10.0
#define SUBMIT_CIRCLE_SECONDS 0.01
#define INET_BUFFER_SIZE 1024

#define NOSO_MAX_DIFF "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
#define NOSOHASH_COUNTER_MIN 100'000'000;

// #define NOSO_B58_ALPHABET "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz"

#define NOSO_TIMESTAMP std::time( 0 )
#define NOSO_BLOCK_AGE ( NOSO_TIMESTAMP % 600 )

const auto g_seed_nodes { std::to_array<std::tuple<std::string, std::string>>(
        {
            { "23.94.21.83", "8080" },
            { "45.146.252.103", "8080" },
            { "107.172.5.8", "8080" },
            { "109.230.238.240", "8080" },
            { "172.245.52.208", "8080" },
            { "192.210.226.118", "8080" },
            { "194.156.88.117", "8080" },
            // // MY NODES
            // { "209.126.80.203", "8080" },
            // { "141.144.236.130", "8080" },
            // { "130.61.53.192", "8080" },
            // { "130.61.250.115", "8080" },
            // { "152.70.177.209", "8080" },
            // { "3.122.235.191", "8080" },
            // { "18.156.76.83", "8080" },
            // { "34.70.75.74", "8080" },
            // { "20.126.46.54", "8080" },
        } ) };

int inet_command( struct addrinfo *serv_info, uint32_t timeosec, char *buffer, size_t buffsize );
int hex_char2dec(char hexchar);
char hex_dec2char( int hexdec );
int nosohash_char( int num );
std::string nosohash_prefix( int num );

class CNosoHasher {
private:
    char m_input[129]; // 128 = 18-chars-prefix + 9 chars-counter + 31-chars-address + fill chars
    char m_base[28]; // 27 = 18-chars-prefix + 9-chars-counter
    char m_hash[33];
    char m_diff[33];
    char m_stat[129][128];
    MD5Context m_md5_ctx;
    void _input() {
        memcpy( m_input + 18, m_base + 18, 9 );  // update counter part as it was updated in base
        assert( strlen( m_input ) == 128 && std::none_of( m_input, m_input + strlen( m_input ), []( int c ){ return 33 > c || c > 126; } ) );
    }
    void _hash() {
        assert( strlen( m_input ) == 128 );
        memcpy( m_stat[0], m_input, 128 );
        for( int row = 1; row < 129; row++ ) {
            for( int col = 0; col < 127; col++ )
                m_stat[row][col] = nosohash_char( m_stat[row-1][col] + m_stat[row-1][col+1] );
            m_stat[row][127] = nosohash_char( m_stat[row-1][127] + m_stat[row-1][0] );
        }
        for( int i = 0; i < 32; i++ )
            m_hash[i] = hex_dec2char( nosohash_char(
                                        m_stat[128][ ( i * 4 ) + 0 ] +
                                        m_stat[128][ ( i * 4 ) + 1 ] +
                                        m_stat[128][ ( i * 4 ) + 2 ] +
                                        m_stat[128][ ( i * 4 ) + 3 ] ) % 16 );
        m_hash[32] = '\0';
        assert( strlen( m_hash ) == 32 );
    }
    void _md5() {
        assert( strlen( m_hash ) == 32 );
        md5Init( &m_md5_ctx );
        md5Update( &m_md5_ctx, (uint8_t *)m_hash, 32 );
        md5Finalize( &m_md5_ctx );
        sprintf( m_hash,
                "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
                m_md5_ctx.digest[ 0], m_md5_ctx.digest[ 1],
                m_md5_ctx.digest[ 2], m_md5_ctx.digest[ 3],
                m_md5_ctx.digest[ 4], m_md5_ctx.digest[ 5],
                m_md5_ctx.digest[ 6], m_md5_ctx.digest[ 7],
                m_md5_ctx.digest[ 8], m_md5_ctx.digest[ 9],
                m_md5_ctx.digest[10], m_md5_ctx.digest[11],
                m_md5_ctx.digest[12], m_md5_ctx.digest[13],
                m_md5_ctx.digest[14], m_md5_ctx.digest[15] );
        assert( strlen( m_hash ) == 32 );
    }
public:
    CNosoHasher( const char prefix[19], const char address[32] ) {
        constexpr static const char NOSOHASH_INPUT_FILLER_CHARS[] = "%)+/5;=CGIOSYaegk";
        constexpr static const int NOSOHASH_INPUT_FILLER_COUNT = 17; // strlen( NOSOHASH_INPUT_FILLER_CHARS );
        assert( strlen( prefix ) == 18 && strlen( address ) == 31 );
        memcpy( m_base, prefix, 18 );
        sprintf( m_base + 18, "%09d", 0 ); // placehold for 9-digits-counter
        assert( strlen( m_base ) == 27 ); // 27 = 18 + 9 = 18-chars-prefix + 9-digits-counter
        memcpy( m_input, prefix, 18 );
        sprintf( m_input + 18, "%09d", 0 ); // placehold for 9-digits-counter
        memcpy( m_input + 18 + 9, address, 31 );
        int len = 58; // 58 = 18 + 9 + 31 = 18-chars-prefix + 9-digits-counter + 31-chars-address
        int div = ( 128 - len ) / NOSOHASH_INPUT_FILLER_COUNT;
        int mod = ( 128 - len ) % NOSOHASH_INPUT_FILLER_COUNT;
        for ( int i = 0; i < div; i++ ) {
            memcpy( m_input + len, NOSOHASH_INPUT_FILLER_CHARS, NOSOHASH_INPUT_FILLER_COUNT );
            len += NOSOHASH_INPUT_FILLER_COUNT;
        }
        memcpy( m_input + len, NOSOHASH_INPUT_FILLER_CHARS, mod );
        m_input[len + mod] = '\0';
        assert( strlen( m_input ) == 128 && std::none_of( m_input, m_input + strlen( m_input ), []( int c ){ return 33 > c || c > 126; } ) );
    }
    const char* GetBase( std::uint32_t counter ) {
        sprintf( m_base + 18, "%09d", counter ); // update 9-digits-counter part
        assert( strlen( m_base ) == 27 ); // 27 = 18 + 9 = 18-chars-prefix + 9-digits-counter
        return m_base;
    }
    const char* GetHash() {
        _input();
        _hash();
        _md5();
        return m_hash;
    }
    const char* GetDiff( const char target[33] ) {
        assert( strlen( m_hash ) == 32 && strlen( target ) == 32 );
        for ( std::size_t i = 0; i < 32; i ++ )
            m_diff[i] = toupper( hex_dec2char( abs( hex_char2dec( m_hash[ i ] ) - hex_char2dec( target[ i ] ) ) ) );
        m_diff[32] = '\0';
        assert( strlen( m_diff ) == 32 );
        return m_diff;
    }
};

class CNodeInet {
private:
    struct addrinfo *m_serv_info;
public:
    const std::string m_host;
    const std::string m_port;
    const int m_timeosec { DEFAULT_INET_TIMEOSEC };
    CNodeInet( const std::string &host, const std::string &port )
        : m_host { host }, m_port { port }, m_serv_info { NULL } {
        this->InitService();
    }
    ~CNodeInet() {
        this->CleanService();
    }
    void InitService() {
        if ( m_serv_info != NULL ) return;
        struct addrinfo hints, *serv_info;
        memset( &hints, 0, sizeof( hints ) );
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        if ( int n = getaddrinfo( this->m_host.c_str(), this->m_port.c_str(),
                &hints, &serv_info ); n != 0 ) {
            fprintf( stderr, "getaddrinfo: %s\n", gai_strerror(n) );
            m_serv_info = NULL;
        }
        m_serv_info = serv_info;
    }
    void CleanService() {
        if ( m_serv_info == NULL ) return;
        freeaddrinfo(m_serv_info);
        m_serv_info = NULL;
    }
    int FetchNodestatus( char *buffer, std::size_t buffsize ) {
        strcpy( buffer, "NODESTATUS\n" );
        return inet_command( m_serv_info, m_timeosec, buffer, buffsize );
    }
    int SubmitSolution( std::uint32_t blck, const char base[28], const char miner[32], char *buffer, std::size_t buffsize ) {
        snprintf( buffer, INET_BUFFER_SIZE - 1, "BESTHASH 1 2 3 4 %s %s %d %lu\n", miner, base, blck, NOSO_TIMESTAMP );
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
        assert( ns_line != nullptr && strlen( ns_line ) > 1 );
        std::string nodestatus { ns_line };
        nodestatus.erase( nodestatus.length() - 2 ); // remove the carriage return and new line charaters

        size_t p_pos = -1, c_pos = -1;
        auto next_nodestatus_token = [&p_pos, &c_pos, &nodestatus]() {
            p_pos = c_pos;
            c_pos = nodestatus.find(' ', c_pos + 1);
        };
        auto extract_nodestatus_token = [&p_pos, &c_pos, &nodestatus]() {
            std::string token = nodestatus.substr( p_pos + 1, c_pos == std::string::npos ? std::string::npos : (c_pos - p_pos - 1) );
            return token;
        };
        //NODESTATUS 1{Peers} 2{LastBlock} 3{Pendings} 4{Delta} 5{headers} 6{version} 7{UTCTime} 8{MNsHash} 9{MNscount}
        //           10{LastBlockHash} 11{BestHashDiff} 12{LastBlockTimeEnd} 13{LBMiner}
        // 0{nodestatus}
        next_nodestatus_token();
        // std::string nodestatus = extract_nodestatus_token();
        // 1{peer}
        next_nodestatus_token();
        // this->peer = std::stoul( extract_nodestatus_token() );
        // 2{blck}
        next_nodestatus_token();
        this->blck_no = std::stoul( extract_nodestatus_token() );
        // 3{pending}
        next_nodestatus_token();
        // this->pending = std::stoul( extract_nodestatus_token() );
        // 4{delta}
        next_nodestatus_token();
        // this->delta = std::stoul( extract_nodestatus_token() );
        // 5{header/branch}
        next_nodestatus_token();
        // this->branch = extract_nodestatus_token();
        // 6{version}
        next_nodestatus_token();
        // this->version = extract_nodestatus_token();
        // 7{utctime}
        next_nodestatus_token();
        // this->utctime = std::stoul( extract_nodestatus_token() );
        // 8{mn_hash}
        next_nodestatus_token();
        // this->mn_hash = extract_nodestatus_token();
        // 9{mn_count}
        next_nodestatus_token();
        // this->mn_count = std::stoul( extract_nodestatus_token() );
        // 10{lb_hash}
        next_nodestatus_token();
        this->lb_hash = extract_nodestatus_token();
        // 11{bh_diff/mn_diff}
        next_nodestatus_token();
        this->mn_diff = extract_nodestatus_token();
        // 12{lb_time}
        next_nodestatus_token();
        this->lb_time = std::stoul( extract_nodestatus_token() );
        // 13{lb_addr}
        next_nodestatus_token();
        this->lb_addr = extract_nodestatus_token();
    }
};

struct CConsensus {
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
    CConsensus( std::uint32_t blck_no_, const std::string& lb_hash_, const std::string& mn_diff_, std::time_t lb_time_, const std::string& lb_addr_ )
        : blck_no { blck_no_ }, lb_hash { lb_hash_ }, mn_diff { mn_diff_ }, lb_time { lb_time_ }, lb_addr { lb_addr_ } {
    }
};

struct CSolution {
    std::uint32_t blck;
    std::string base;
    std::string hash;
    std::string diff;
    CSolution( std::uint32_t blck_, const char base_[28], const char hash_[33], const char diff_[33] )
        : blck { blck_ }, base { base_ }, hash { hash_ }, diff { diff_ } {
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
private:
    char m_address[32];
    char m_prefix[19];
    std::uint32_t m_blck_no { 0 };
    char m_lb_hash[33];
    char m_mn_diff[33] { NOSO_MAX_DIFF };
    std::uint32_t m_computed_hashes_count { 0 };
    mutable std::shared_mutex m_mutex_computed_hashes_count;
public:
    CMineThread( const char prefix[19], const char address[32] ) {
        assert( strlen( prefix ) == 18 && strlen( address ) == 31 );
        strcpy( m_prefix, prefix );
        strcpy( m_address, address );
    }
    void UpdateLastBlock( std::uint32_t blck_no, const char lb_hash[33] ) {
        m_blck_no = blck_no;
        strcpy( m_lb_hash, lb_hash );
    }
    void UpdateHashDiff( const char mn_diff[33] ) {
        strcpy( m_mn_diff, mn_diff );
    }
    void UpdateComputedHashesCount( std::uint32_t more ) {
        m_mutex_computed_hashes_count.lock();
        m_computed_hashes_count += more;
        m_mutex_computed_hashes_count.unlock();
    }
    void ResetComputedHashesCount() {
        m_mutex_computed_hashes_count.lock();
        m_computed_hashes_count = 0;
        m_mutex_computed_hashes_count.unlock();
    }
    std::uint32_t GetComputedHashesCount() {
        m_mutex_computed_hashes_count.lock_shared();
        std::uint32_t computed_hashes_count { m_computed_hashes_count };
        m_mutex_computed_hashes_count.unlock_shared();
        return computed_hashes_count;
    }
    void Mine();
};

class CCommThread {
private:
    mutable std::default_random_engine m_random_engine {
        std::default_random_engine { std::random_device {}() } };
    mutable std::mutex m_mutex_solutions;
    std::vector<std::shared_ptr<CNodeInet>> m_node_inets_good;
    std::vector<std::shared_ptr<CNodeInet>> m_node_inets_poor;
    std::multiset<std::shared_ptr<CSolution>, CSolsSetCompare> m_solutions;
    std::uint32_t m_accepted_solutions_count { 0 };
    std::uint32_t m_rejected_solutions_count { 0 };
    std::uint32_t m_failured_solutions_count { 0 };
    std::uint32_t m_disposed_solutions_count { 0 };
    std::uint32_t m_pendings_solutions_count { 0 };
    std::uint32_t m_passover_solutions_count { 0 };
    char m_status_buffer[INET_BUFFER_SIZE];
    char m_submit_buffer[INET_BUFFER_SIZE];
    std::map<std::uint32_t, int> m_freq_blck_no;
    std::map<std::string  , int> m_freq_lb_hash;
    std::map<std::string  , int> m_freq_mn_diff;
    std::map<std::time_t  , int> m_freq_lb_time;
    std::map<std::string  , int> m_freq_lb_addr;
    CCommThread() {
        for( auto sn : g_seed_nodes ) m_node_inets_good.push_back(
            std::make_shared<CNodeInet>(std::get<0>( sn ), std::get<1>( sn ) ) );
    }
    std::vector<std::shared_ptr<CNodeStatus>> SyncSources( int min_nodes_count ) {
        if ( m_node_inets_good.size() < min_nodes_count ) {
            for ( auto ni : m_node_inets_poor ) {
                ni->InitService();
                m_node_inets_good.push_back( ni );
            }
            m_node_inets_poor.clear();
            std::cout << "poor network reset: " << m_node_inets_good.size() << "/" << m_node_inets_poor.size() << std::endl;
        }
        std::shuffle( m_node_inets_good.begin(), m_node_inets_good.end(), m_random_engine );
        std::size_t nodes_count { 0 };
        std::vector<std::shared_ptr<CNodeStatus>> vec;
        for ( auto it = m_node_inets_good.begin(); it != m_node_inets_good.end(); ) {
            if ( (*it)->FetchNodestatus( m_status_buffer, INET_BUFFER_SIZE ) <= 0 ) {
                (*it)->CleanService();
                m_node_inets_poor.push_back( *it );
                it = m_node_inets_good.erase( it );
                std::cout << "poor network found: " << m_node_inets_good.size() << "/" << m_node_inets_poor.size() << std::endl;
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
            m_solutions.erase( itor_best_solution );
            // for ( auto s : m_solutions ) {
            //     std::cout << "\tPASSOVER>>"
            //         << "blck[" << s->blck << "]diff[" << s->diff
            //         << "]hash[" << s->hash << "]base[" << s->base << "]" << std::endl;
            // }
            m_passover_solutions_count += m_solutions.size();
            m_solutions.clear();
        }
        m_mutex_solutions.unlock();
        return best_solution;
    }
    std::tuple<bool, bool, int> PushSolution( std::uint32_t blck, const char base[28], const char address[32], char new_mn_diff[33] ) {
        if ( m_node_inets_good.size() < 1 ) {
            for ( auto ni : m_node_inets_poor ) {
                ni->InitService();
                m_node_inets_good.push_back( ni );
            }
            m_node_inets_poor.clear();
            std::cout << "poor network reset: " << m_node_inets_good.size() << "/" << m_node_inets_poor.size() << std::endl;
        }
        std::shuffle( m_node_inets_good.begin(), m_node_inets_good.end(), m_random_engine );
        for ( auto it = m_node_inets_good.begin(); it != m_node_inets_good.end(); ) {
            if ( (*it)->SubmitSolution( blck, base, address, m_submit_buffer, INET_BUFFER_SIZE ) <= 0 ) {
                (*it)->CleanService();
                m_node_inets_poor.push_back( *it );
                it = m_node_inets_good.erase( it );
                std::cout << "poor network found: " << m_node_inets_good.size() << "/" << m_node_inets_poor.size() << std::endl;
                continue;
            }
            ++it;
            assert( strlen( m_submit_buffer ) >= 40 + 2 ); //len=70[True Diff(32) Hash(32)] OR len=40[False Diff(32) #(1)] + "\r\n"
            if ( strncmp( m_submit_buffer, "True", 4 ) == 0 ) {
                assert( strlen( m_submit_buffer ) == 70 + 2 ); //len=70[True Diff(32) Hash(32)] + "\r\n"
                strncpy( new_mn_diff, m_submit_buffer + 5, 32 );
                new_mn_diff[32] = '\0';
                return std::make_tuple( true, true, 0 );
            }
            else {
                assert( strlen( m_submit_buffer ) == 40 + 2 && strncmp( m_submit_buffer, "False", 5 ) == 0
                    && '1' <= m_submit_buffer[39] && m_submit_buffer[39] <= '7' ); // len=40[False Diff(32) #(1)] + "\r\n"
                strncpy( new_mn_diff, m_submit_buffer + 6, 32 );
                new_mn_diff[32] = '\0';
                return std::make_tuple( true, false, m_submit_buffer[39] - '0' );
            }
        }
        return std::make_tuple( false, false, 0 );
    }
    std::shared_ptr<CConsensus> MakeConsensus() {
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
        return std::make_shared<CConsensus>(
            max_freq( m_freq_blck_no ),
            max_freq( m_freq_lb_hash ),
            max_freq( m_freq_mn_diff ),
            max_freq( m_freq_lb_time ),
            max_freq( m_freq_lb_addr ) );
    }
    void Communicate();
};

std::string g_mining_source { DEFAULT_SOURCE };
char g_miner_address[32] { DEFAULT_MINER_ADDRESS };
std::uint32_t g_miner_id { DEFAULT_MINER_ID };
std::uint32_t g_threads_count { DEFAULT_THREADS_COUNT };
std::uint32_t g_total_pendings_solutions_count { 0 };
std::uint32_t g_total_accepted_solutions_count { 0 };
std::uint32_t g_total_rejected_solutions_count { 0 };
std::uint32_t g_total_failured_solutions_count { 0 };
std::uint32_t g_total_disposed_solutions_count { 0 };
std::uint32_t g_total_passover_solutions_count { 0 };
std::set<std::uint32_t> g_mined_blocks;
std::vector<std::shared_ptr<CMineThread>> g_mine_objects;
std::vector<std::thread> g_mine_threads;
bool g_still_running { true };

int main( int argc, char *argv[] ) {
    signal( SIGINT, []( int signum ) {
        std::cout << "\nCtrl+C pressed! Wait for finishing all mining threads..." << std::endl;
        g_still_running = false; });
    cxxopts::Options options( "noso-2m", "Noso protocol 2 miner" );
    options.add_options()
        ( "a,address",   "An original noso wallet address",     cxxopts::value<std::string>()->default_value( DEFAULT_MINER_ADDRESS ) )
        ( "i,minerid",   "Miner ID - a number between 0-8100",  cxxopts::value<std::uint32_t>()->default_value( std::to_string( DEFAULT_MINER_ID ) ) )
        ( "t,threads",   "Number of threads use for mining",    cxxopts::value<std::uint32_t>()->default_value( std::to_string( DEFAULT_THREADS_COUNT ) ) )
        ( "h,help",      "Print this usage" )
        ;
    auto result = options.parse( argc, argv );
    if ( result.count( "help" ) ) {
        std::cout << options.help() << std::endl;
        exit( 0 );
    }
    std::string miner_address = result["address"].as<std::string>();
    // TODO validate address, len = 31 (NOSO_ADDRESS_LENGTH)
    strcpy( g_miner_address, miner_address.c_str() );
    assert( strlen( g_miner_address ) == 31 );
    if ( strlen( g_miner_address ) != 31 ) exit(0);
    g_miner_id = result["minerid"].as<std::uint32_t>();
    assert( 0 <= g_miner_id && g_miner_id <= 8100 );
    if ( g_miner_id < 0 || g_miner_id > 8100 ) exit(0);
    // TODO validate minerid value
    g_threads_count = result["threads"].as<std::uint32_t>();
    // TODO validate number of threads
    std::cout << "- Wallet address: " << g_miner_address << std::endl;
    std::cout << "-       Miner ID: " << g_miner_id << std::endl;
    std::cout << "-  Threads count: " << g_threads_count << std::endl;
    const std::string miner_prefix { nosohash_prefix( g_miner_id ) };
    auto mine_thread_prefix = [ &prefix = std::as_const( miner_prefix ) ]( int num ) {
        std::string result = std::string { prefix + nosohash_prefix( num ) };
        result.append( 18 - result.size(), '!' );
        return result;
    };
    for ( int i = 0; i < g_threads_count - 1; i++ )
        g_mine_objects.push_back( std::make_shared<CMineThread>( mine_thread_prefix( i ).c_str(), g_miner_address ) );
    std::thread comm_thread( &CCommThread::Communicate, CCommThread::GetInstance() );
    for ( int i = 0; i < g_threads_count - 1; i++ )
        g_mine_threads.push_back( std::move( std::thread( &CMineThread::Mine, g_mine_objects[i] ) ) );
    comm_thread.join();
    std::cout << "================================================================================================================\n"
        << "TOTAL "
        << g_total_accepted_solutions_count << " ACCEPTED "
        << g_total_rejected_solutions_count << " REJECTED "
        << g_total_failured_solutions_count << " FAILURED "
        << g_total_disposed_solutions_count << " DISPOSED "
        << g_total_passover_solutions_count << " PASSOVER "
        << g_total_pendings_solutions_count << " PENDINGS" << " SOLUTION(S)" << std::endl;
    std::cout << "MINED " << g_mined_blocks.size() << " BLOCKS" << std::endl;
    if ( g_mined_blocks.size() > 0 ) {
        for( auto b : g_mined_blocks ) std::cout << b << " ";
        std::cout << std::endl;
    }
    return 0;
}

void CMineThread::Mine() {
    CNosoHasher noso_hasher( m_prefix, m_address );
    std::uint32_t noso_hash_counter { 0 };
    while ( g_still_running ) {
        std::uint32_t prev_blck_no { 0 };
        bool first_takenap { true };
        do {
            if ( first_takenap ) first_takenap = false;
            std::this_thread::sleep_for( std::chrono::milliseconds( static_cast<int>( 1000 * SUBMIT_CIRCLE_SECONDS ) ) );
        } while ( g_still_running && m_blck_no <= 0 );
        while ( g_still_running && m_blck_no > 0 ) {
            if ( m_blck_no <= 0 ) break;
            if ( m_blck_no > prev_blck_no ) {
                this->ResetComputedHashesCount();
                prev_blck_no = m_blck_no;
                noso_hash_counter = NOSOHASH_COUNTER_MIN;
            }
            const char *base { noso_hasher.GetBase( noso_hash_counter++ ) };
            const char *hash { noso_hasher.GetHash() };
            const char *diff { noso_hasher.GetDiff( m_lb_hash ) };
            if ( strcmp( diff, m_mn_diff ) < 0 ) { // && strcmp( m_mn_diff, NOSO_MAX_DIFF ) < 0 )
                CCommThread::GetInstance()->AddSolution( std::make_shared<CSolution>( m_blck_no + 1, base, hash, diff ) );
            }
            this->UpdateComputedHashesCount( 1 );
            // std::cout << base << "|" << hash << "|" << diff << std::endl;
        } // END while ( g_still_running && blck_no > 0 ) {
    } // END while ( g_still_running ) {
}

#define COUT_NOSO_TIME std::cout << NOSO_TIMESTAMP << "(" << std::setfill('0') << std::setw(3) << NOSO_BLOCK_AGE << "))"

void CCommThread::Communicate() {
    std::uint32_t blck_no { 0 };
    char lb_hash[33];
    char mn_diff[33] { NOSO_MAX_DIFF };
    char new_mn_diff[33] { NOSO_MAX_DIFF };
    std::vector<std::string> accepted_hashes;
    auto print_block_summary = [&]( auto begin_blck ) {
        std::chrono::duration<double> elapsed_blck = std::chrono::steady_clock::now() - begin_blck;
        std::uint32_t computed_hashes_count = std::accumulate(
                g_mine_objects.begin(), g_mine_objects.end(), 0,
                []( int a, const std::shared_ptr<CMineThread>& o ) { return a + o->GetComputedHashesCount(); } );
        std::cout << "SUMMARY BLOCK#" << blck_no << " : " << computed_hashes_count<< " hashes computed within "
            << elapsed_blck.count() / 60 << " minutes (" << computed_hashes_count / elapsed_blck.count() << " h/s)\n\t"
            << " accepted " << m_accepted_solutions_count
            << " rejected " << m_rejected_solutions_count
            << " failured " << m_failured_solutions_count
            << " disposed " << m_disposed_solutions_count
            << " passover " << m_passover_solutions_count
            << " pendings " << m_pendings_solutions_count << " solution(s)" << std::endl;
        std::cout << "MINED " << g_mined_blocks.size() << " BLOCKS" << std::endl;
        g_total_accepted_solutions_count += m_accepted_solutions_count;
        g_total_rejected_solutions_count += m_rejected_solutions_count;
        g_total_failured_solutions_count += m_failured_solutions_count;
        g_total_disposed_solutions_count += m_disposed_solutions_count;
        g_total_passover_solutions_count += m_passover_solutions_count;
        g_total_pendings_solutions_count += m_pendings_solutions_count;
    };
    auto reset_mining_block = [&]() {
        m_accepted_solutions_count = 0;
        m_rejected_solutions_count = 0;
        m_failured_solutions_count = 0;
        m_disposed_solutions_count = 0;
        m_passover_solutions_count = 0;
        m_pendings_solutions_count = 0;
        accepted_hashes.clear();
        this->ClearSolutions();
    };
    bool firstIter { true };
    auto begin_blck = std::chrono::steady_clock::now();
    auto begin_update = std::chrono::steady_clock::now();
    auto begin_submit = std::chrono::steady_clock::now();
    // auto begin_rate4s = std::chrono::steady_clock::now();
    // std::uint32_t accu_computed_hashes_count { 0 };
    while ( g_still_running ) {
        // std::chrono::duration<double> elapsed_rate4s = std::chrono::steady_clock::now() - begin_rate4s;
        // if ( elapsed_rate4s.count() >= 4 ) {
        //     std::uint32_t computed_hashes_count = std::accumulate(
        //             g_mine_objects.begin(), g_mine_objects.end(), 0,
        //             []( int a, const std::shared_ptr<CMineThread>& o ) { return a + o->GetComputedHashesCount(); } );
        //     std::cout << "HASHRATE: " << ( computed_hashes_count - accu_computed_hashes_count ) / elapsed_rate4s.count() << "h/s" << std::endl;
        //     accu_computed_hashes_count += computed_hashes_count;
        //     begin_rate4s = std::chrono::steady_clock::now();
        // }
        std::chrono::duration<double> elapsed_submit = std::chrono::steady_clock::now() - begin_submit;
        // if ( !firstIter && elapsed_submit.count() >= SUBMIT_CIRCLE_SECONDS { // && NOSO_BLOCK_AGE >= 5 ) {
            if ( std::shared_ptr<CSolution> solution { this->BestSolution() }; solution != nullptr ) {
                if ( solution->blck > blck_no && solution->diff < mn_diff ) {
                    strcpy( mn_diff, solution->diff.c_str() );
                    for ( auto mo : g_mine_objects ) mo->UpdateHashDiff( mn_diff );
                    auto [ submited, accepted, code ] = this->PushSolution( solution->blck, solution->base.c_str(), g_miner_address, new_mn_diff );
                    if ( submited ) {
                        if ( accepted ) {
                            m_accepted_solutions_count ++;
                            accepted_hashes.push_back( solution->hash );
                            COUT_NOSO_TIME << " ACCEPTED"
                                << ")blck[" << solution->blck
                                << "]diff[" << solution->diff
                                << "]hash[" << solution->hash
                                << "]base[" << solution->base << "]" << std::endl;
                        } else {
                            m_rejected_solutions_count ++;
                            if      ( code == 1 ) {
                                COUT_NOSO_TIME << "    ERROR)Wrong block number " << solution->blck << " submitted!" << std::endl;
                            } else if ( code == 2 ) {
                                COUT_NOSO_TIME << "    ERROR)Incorrect timestamp submitted!" << std::endl;
                                // g_still_running = false;
                            } else if ( code == 3 ) {
                                COUT_NOSO_TIME << "    ERROR)Invalid mining address (" << g_miner_address << ")!" << std::endl;
                                // g_still_running = false;
                            } else if ( code == 7 ) {
                                COUT_NOSO_TIME << "    ERROR)Wrong hash base (" << solution->base << ")!" << std::endl;
                            } else if ( code == 4 ) {
                                COUT_NOSO_TIME << " DUPLICAT"
                                    << ")blck[" << solution->blck
                                    << "]diff[" << solution->diff
                                    << "]hash[" << solution->hash
                                    << "]base[" << solution->base << "]THIS SHOULD NOT HAPPEND!" << std::endl;
                            } else if ( code == 5 ) {
                                assert( strcmp( mn_diff, new_mn_diff ) > 0 );
                                strcpy( mn_diff, new_mn_diff );
                                for ( auto mo : g_mine_objects ) mo->UpdateHashDiff( mn_diff );
                                COUT_NOSO_TIME << " REJECTED"
                                    << ")blck[" << solution->blck
                                    << "]diff[" << solution->diff
                                    << "]hash[" << solution->hash
                                    << "]base[" << solution->base << "]" << std::endl;
                                COUT_NOSO_TIME << "UPTODATED"
                                    << ")blck[" << blck_no + 1
                                    << "]diff[" << mn_diff
                                    << "]hash[" << lb_hash << "]" << std::endl;
                            } else { // code == 6
                                COUT_NOSO_TIME << "WAITBLOCK"
                                    << ")blck[" << blck_no + 1
                                    << "]diff[" << mn_diff
                                    << "]hash[" << lb_hash << "]"
                                    << "Network building block!" << std::endl;
                            }
                        } // OF if ( accepted ) { ... } else {
                    } else { // OF if ( submited ) {
                        m_failured_solutions_count ++;
                        COUT_NOSO_TIME << " FAILURED"
                            << ")blck[" << solution->blck
                            << "]diff[" << solution->diff
                            << "]hash[" << solution->hash
                            << "]base[" << solution->base << "]" << std::endl;
                    }
                } else { // OF if ( solution->m_blck > blck_no && solution->m_diff < mn_diff ) {
                    m_disposed_solutions_count ++;
                    COUT_NOSO_TIME << " DISPOSED"
                        << ")blck[" << solution->blck
                        << "]diff[" << solution->diff
                        << "]hash[" << solution->hash
                        << "]base[" << solution->base << "]" << std::endl;
                }
            } // END if ( std::shared_ptr<CSolution> solution { this->BestSolution() }; solution != nullptr ) {
            begin_submit = std::chrono::steady_clock::now();
        // } // END if ( !first && elapsed_submit.count() >= SUBMIT_CIRCLE_SECONDS ) {
        std::chrono::duration<double> elapsed_update = std::chrono::steady_clock::now() - begin_update;
        if ( firstIter || elapsed_update.count() >= UPDATE_CIRCLE_SECONDS ) {
            std::shared_ptr<CConsensus> consensus = this->MakeConsensus();
            while ( g_still_running && consensus == nullptr ) {
                COUT_NOSO_TIME << "MAKING CONSENSUS..." << std::endl;
                consensus = this->MakeConsensus();
            }
            if ( blck_no < consensus->blck_no ) {
                if ( consensus->lb_addr == g_miner_address ) {
                    g_mined_blocks.insert( consensus->blck_no );
                    bool this_machine = std::find( accepted_hashes.begin(), accepted_hashes.end(), consensus->lb_hash ) != accepted_hashes.end();
                    COUT_NOSO_TIME << "YAY! YOU HAVE MINED BLOCK#" << consensus->blck_no
                        << " TO ADDRESS " << consensus->lb_addr
                        << (this_machine ? " BY THIS MACHINE!!!" : "") << std::endl;
                }
                if ( blck_no > 0 ) print_block_summary( begin_blck );
                reset_mining_block();
                begin_blck = std::chrono::steady_clock::now();
                std::cout << "-----------------------------------------------------------------------------------------------------------------" << std::endl;
                blck_no = consensus->blck_no;
                strcpy( lb_hash, consensus->lb_hash.c_str() );
                for ( auto mo : g_mine_objects ) mo->UpdateLastBlock( blck_no, lb_hash );
                COUT_NOSO_TIME << "PREVBLOCK"
                    << ")blck[" << consensus->blck_no
                    << "]addr[" << consensus->lb_addr
                    << "]time[" << consensus->lb_time << "]" << std::endl;
            }
            if ( mn_diff != consensus->mn_diff ) {
                strcpy( mn_diff, consensus->mn_diff.c_str() );
                for ( auto mo : g_mine_objects ) mo->UpdateHashDiff( mn_diff );
                COUT_NOSO_TIME << "CONSENSUS"
                    << ")blck["   << consensus->blck_no + 1
                    << "]diff["   << consensus->mn_diff
                    << "]hash["   << consensus->lb_hash << "]" << std::endl;
            }
            begin_update = std::chrono::steady_clock::now();
        } // END if ( first || elapsed_update.count() >= UPDATE_CIRCLE_SECONDS ) {
        if ( elapsed_submit.count() < SUBMIT_CIRCLE_SECONDS && elapsed_update.count() < UPDATE_CIRCLE_SECONDS ) {
            std::this_thread::sleep_for( std::chrono::milliseconds( static_cast<int>( 1000 * SUBMIT_CIRCLE_SECONDS ) ) );
        }
        firstIter = false;
    } // END while ( g_still_running ) {
    print_block_summary( begin_blck );
    for ( auto &thr : g_mine_threads ) thr.join();
}

const char NOSOHASH_HASHEABLE_CHARS[] { "!\"#$%&')*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~" };
const std::size_t NOSOHASH_HASHEABLE_COUNT =  93 - 1; // strlen( NOSOHASH_HASHEABLE_COUNT );
std::string nosohash_prefix( int num ) {
    return std::string { NOSOHASH_HASHEABLE_CHARS[ num / NOSOHASH_HASHEABLE_COUNT ], NOSOHASH_HASHEABLE_CHARS[ num % NOSOHASH_HASHEABLE_COUNT ] };
}

int nosohash_char( int num ) {
    while ( num > 126 ) num -= 95;
    return num;
};

#ifndef NDEBUG
const std::array<char, 16> HEXCHAR_DOMAIN { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F' };
#endif
int hex_char2dec(char hexchar) {
    assert( std::find( HEXCHAR_DOMAIN.begin(), HEXCHAR_DOMAIN.end(), hexchar ) != HEXCHAR_DOMAIN.end() );
    return  ( '0' <= hexchar && hexchar <= '9' ) ? hexchar - '0' :
            ( 'A' <= hexchar && hexchar <= 'F' ) ? hexchar - 'A' + 10 : -1;
}

#ifndef NDEBUG
const std::array<int, 16> HEXDEC_DOMAIN{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
#endif
char hex_dec2char( int hexdec ) {
    assert( std::find( HEXDEC_DOMAIN.begin(), HEXDEC_DOMAIN.end(), hexdec ) != HEXDEC_DOMAIN.end() );
    return  (  0 <= hexdec && hexdec <=  9 ) ? hexdec + '0' :
            ( 10 <= hexdec && hexdec <= 15 ) ? hexdec + 'A' - 10 : '\0';
}

int inet_socket( struct addrinfo *serv_info, int timesec ) {
    struct addrinfo *psi = serv_info;
    struct timeval timeout {
        .tv_sec = timesec,
        .tv_usec = 0
    };
    int sockfd;
    fd_set rset, wset;
    for( ; psi != NULL; psi = psi->ai_next ) {
        if ( (sockfd = socket( psi->ai_family, psi->ai_socktype,
                               psi->ai_protocol ) ) == -1 ) {
            perror( "socket: error" );
            continue;
        }
        int flags = 0;
        if ( ( flags = fcntl( sockfd, F_GETFL, 0 ) ) < 0 ) {
            close( sockfd );
            perror( "fcntl/socket: error" );
            continue;
        }
        if ( fcntl( sockfd, F_SETFL, flags | O_NONBLOCK ) < 0 ) {
            close( sockfd );
            perror( "connect/socket: error" );
            continue;
        }
        if ( connect( sockfd, psi->ai_addr, psi->ai_addrlen ) >= 0 ) {
            // if ( fcntl( sockfd, F_SETFL, flags ) < 0 ) {
            //     close( sockfd );
            //     perror( "connect/socket: error" );
            //     continue;
            // }
            // break;
            return sockfd;
        }
        if ( errno != EINPROGRESS ) {
            close( sockfd );
            perror( "connect: error" );
            continue;
        }
        FD_ZERO( &rset );
        FD_ZERO( &wset );
        FD_SET( sockfd, &rset );
        FD_SET( sockfd, &wset );
        // wset = rset;
        int n = select( sockfd + 1, &rset, &wset, NULL, &timeout );
        if ( n == 0 ) {
            close( sockfd );
            perror("select/socket: timeout");
            continue;
        }
        if ( n == -1 ) {
            close( sockfd );
            perror( "select/socket: failed" );
            continue;
        }
        if ( FD_ISSET( sockfd, &rset ) || FD_ISSET( sockfd, &wset ) ) {
            int error = 0;
            socklen_t slen = sizeof( error );
            if ( getsockopt( sockfd, SOL_SOCKET, SO_ERROR, &error, &slen ) < 0 ) {
                close( sockfd );
                perror( "getsockopt/socket: failed" );
                continue;
            }
            if ( error ) {
                close( sockfd );
                perror( "getsockopt/socket: failed" );
                continue;
            }
        }
        else {
            perror( "select/socket: failed" );
            continue;
        }
        // if ( fcntl( sockfd, F_SETFL, flags ) < 0 ) {
        //     close( sockfd );
        //     perror( "connect/socket: error" );
        //     continue;
        // }
        return sockfd;
    }
    return -1;
}

int inet_send( int sockfd, uint32_t timesec, const char *message, size_t size ) {
    struct timeval timeout {
        .tv_sec = timesec,
        .tv_usec = 0
    };
    fd_set fds;
    FD_ZERO( &fds );
    FD_SET( sockfd, &fds );
    int n = select( sockfd + 1, NULL, &fds, NULL, &timeout );
    if ( n == 0 ) {
        close( sockfd );
        perror( "select/send: timeout" );
        return -2; // timeout!
    }
    if ( n == -1 ) {
        close( sockfd );
        perror( "select/send: failed" );
        return -1; // error
    }
    int slen = send( sockfd, message, size, 0 );
    if ( slen < 1 ) {
        perror( "send: error" );
        close( sockfd );
        return -1;
    }
    return slen;
}

int inet_recv( int sockfd, uint32_t timesec, char *buffer, size_t buffsize ) {
    struct timeval timeout {
        .tv_sec = timesec,
        .tv_usec = 0
    };
    fd_set fds;
    FD_ZERO( &fds );
    FD_SET( sockfd, &fds );
    int n = select( sockfd + 1, &fds, NULL, NULL, &timeout );
    if ( n == 0 ) {
        close( sockfd );
        perror( "select/recv: timeout" );
        return -2; // timeout!
    }
    if ( n == -1 ) {
        close( sockfd );
        perror( "select/recv: failed" );
        return -1; // error
    }
    int rlen = recv( sockfd, buffer, buffsize - 1, 0 );
    // if (rlen < 0) { //TODO
    if ( rlen < 1 ) {
        close( sockfd );
        perror( "recv: error" );
        return -1;
    }
    buffer[ rlen ] = '\0';
    return rlen;
}

int inet_command( struct addrinfo *serv_info, uint32_t timeosec, char *buffer, size_t buffsize ) {
    int sockfd = inet_socket( serv_info, timeosec );
    if (sockfd < 0) return -1;
    int slen = inet_send( sockfd, timeosec, buffer, strlen( buffer ) );
    if ( slen < 0 ) return slen;
    int rlen = inet_recv( sockfd, timeosec, buffer, buffsize );
    if ( rlen < 0 ) return rlen;
    close( sockfd );
    return rlen;
}
