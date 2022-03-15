#include <algorithm>
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
// #define DEFAULT_MINER_ADDRESS "NWtKL76qKjCAGBg6f2sz4ggoaKAcCx"    // f04ever
// #define DEFAULT_MINER_ADDRESS "N4SQeiGb2F3AUDfK4YPXootpmM55LCV"
#define DEFAULT_MINER_ADDRESS "N3G1HhkpXvmLcsWFXySdAxX3GZpkMFS"
#define DEFAULT_MINER_ID 1000
#define DEFAULT_THREADS_COUNT 2
#define DEFAULT_INET_TIMEOUT 10

#define HASHES_COUNTER_BEGINNING 100'000'000
#define CONSENSUS_NODES_COUNT 3
#define UPDATE_CIRCLE_SECONDS 10.0
#define SUBMIT_CIRCLE_SECONDS 0.1
#define HASHING_BATCH_SIZE 100
#define INET_BUFFER_SIZE 1024
#define NOSO_ADDRESS_SIZE 31
#define NOSO_MAX_DIFF "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
// #define B58_ALPHABET "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz"

const auto seed_nodes { std::to_array<std::tuple<std::string, std::string>>(
        {
            { "23.94.21.83", "8080" },
            { "45.146.252.103", "8080" },
            { "107.172.5.8", "8080" },
            { "109.230.238.240", "8080" },
            { "172.245.52.208", "8080" },
            { "192.210.226.118", "8080" },
            { "194.156.88.117", "8080" },
            // MY NODES
            // { "209.126.80.203", "8080" },
            // { "141.144.236.130", "8080" },
            // { "130.61.53.192", "8080" },
            // { "130.61.250.115", "8080" },
            // { "152.70.177.209", "8080" },
            // // { "3.122.235.191", "8080" },
            // { "18.156.76.83", "8080" },
            // { "34.70.75.74", "8080" },
            // { "20.126.46.54", "8080" },
        } ) };

class CNodeInet {
public:
    const std::string m_host;
    const std::string m_port;
    const int m_timeout { DEFAULT_INET_TIMEOUT };
    CNodeInet( const std::string &host, const std::string &port )
        : m_host { host }, m_port { port },
        m_serv_info { NULL } {
        this->initService();
    }
    ~CNodeInet() {
        this->cleanService();
    }
    void initService();
    void cleanService();
    const char* fetchNodestatus();
    const char* submitSolution( const char miner[32], const char base[28], std::uint32_t blck );
private:
    struct addrinfo *m_serv_info;
    char m_fetch_buffer[INET_BUFFER_SIZE];
    char m_submit_buffer[INET_BUFFER_SIZE];
};

struct CNodeStatus {
    std::uint32_t peer;
    std::uint32_t blck_no;
    std::uint32_t pending;
    std::uint32_t delta;
    std::string branch;
    std::string version;
    std::time_t utctime;
    std::string mn_hash;
    std::uint32_t mn_count;
    std::string lb_hash;
    std::string mn_diff;
    std::time_t lb_time;
    std::string lb_addr;

    CNodeStatus& from_nodestatus( const char *ns );
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
    void print() {
        std::cout
            // << "peer["      << peer     << "]"
            << "blck_no["   << blck_no  << "]"
            // << "pending["   << pending  << "]"
            // << "delta["     << delta    << "]"
            // << "branch["    << branch   << "]"
            // << "version["   << version  << "]"
            // << "utctime["   << utctime  << "]"
            // << "mn_hash["   << mn_hash  << "]"
            // << "mn_count["  << mn_count << "]"
            << "mn_diff["   << mn_diff  << "]"
            << "lb_hash["   << lb_hash  << "]"
            << "lb_time["   << lb_time  << "]"
            << "lb_addr["   << lb_addr  << "]" << std::endl;
    }
};

struct CSolution {
    std::uint32_t m_blck;
    std::string m_diff;
    std::string m_base;
    std::string m_hash;
    CSolution( std::uint32_t blck, const char base[28], const char hash[33], const char diff[33] )
        : m_blck { blck }, m_base { base }, m_hash { hash }, m_diff { diff } {
    }
};

struct CSolsSetCompare {
    bool operator()( const std::shared_ptr<CSolution>& lhs, const std::shared_ptr<CSolution>& rhs ) const {
        return  lhs->m_blck > rhs->m_blck ? true :
                lhs->m_blck < rhs->m_blck ? false :
                lhs->m_diff < rhs->m_diff ? true : false;
    }
};

class CMineThread {
private:
    char m_prefix[19];
    std::uint32_t m_hashes_counter { HASHES_COUNTER_BEGINNING };
    mutable std::shared_mutex m_mutex_computed_hashes_count;
    std::uint32_t m_computed_hashes_count { 0 };
    MD5Context m_md5_context;
    char m_noso_hash_buffer[33];
    char m_noso_diff_buffer[33];
    char m_noso_stat_buffer[129][128];
public:
    CMineThread( const char prefix[19] ) {
        assert( strlen( prefix ) == 18 );
        strcpy( m_prefix, prefix );
    }

    std::uint32_t getComputedHashesCount() {
        std::shared_lock shared_lock_computed_hashes_count( m_mutex_computed_hashes_count );
        return m_computed_hashes_count;
    }
    void mine();
};

std::time_t utc_time( );
std::string makePrefix( int num );
char* makeNosohash( const char input[129], char output[33], char noso_stat_buffer[129][128], MD5Context *md5_context );
char* makeNosodiff( const char hash[33], const char target[33], char diff[33] );
CConsensus makeConsensus( const std::vector<std::shared_ptr<CNodeStatus>> &nodes );
std::vector<std::shared_ptr<CNodeStatus>> syncSources( int nodes_max );
std::tuple<bool, bool, int> syncSolution( const char miner[32], const char base[28], std::uint32_t blck, char new_mn_diff[33] );
void comm_thread();
void signal_callback_handler( int signum );

const char NOSOHASH_INPUT_FILLER_CHARS[] { "%)+/5;=CGIOSYaegk" };
const int NOSOHASH_INPUT_FILLER_COUNT = 17; // strlen( NOSOHASH_INPUT_FILLER_CHARS );

auto g_node_inets = [](){
    std::vector<std::shared_ptr<CNodeInet>> vec;
    for( auto sn : seed_nodes ) vec.push_back( std::make_shared<CNodeInet>(std::get<0>( sn ), std::get<1>( sn ) ) );
    return vec; }();
std::vector<std::shared_ptr<CNodeInet>> g_node_inets_poor;

std::string g_source { DEFAULT_SOURCE };
bool g_loop_main { true };
char g_miner_address[32];
std::uint32_t g_miner_id;
std::uint32_t g_threads_count;
std::uint32_t g_total_pendings_solutions_count = { 0 };
std::uint32_t g_total_accepted_solutions_count = { 0 };
std::uint32_t g_total_rejected_solutions_count = { 0 };
std::uint32_t g_total_failured_solutions_count = { 0 };
std::uint32_t g_total_disposed_solutions_count = { 0 };
std::uint32_t g_total_passover_solutions_count = { 0 };
std::set<std::uint32_t> g_mined_blcks;
std::vector<std::shared_ptr<CMineThread>> g_mineObjects;
std::vector<std::thread> g_mineThreads;
auto g_random_engine = std::default_random_engine { std::random_device {}() };

std::shared_mutex g_mutex_blck_no;
std::shared_mutex g_mutex_lb_hash;
std::shared_mutex g_mutex_mn_diff;
std::mutex g_mutex_pendings_solutions;
std::uint32_t   s_blck_no { 0 };
char s_lb_hash[33];
char s_mn_diff[33];
std::multiset<std::shared_ptr<CSolution>, CSolsSetCompare> s_pendings_solutions;

int main( int argc, char *argv[] ) {
    signal(SIGINT, signal_callback_handler);
    cxxopts::Options options("noso-2m", "Noso protocol 2 miner");
    options.add_options()
        ("a,address",   "An original noso wallet address", cxxopts::value<std::string>()->default_value( DEFAULT_MINER_ADDRESS ) )
        ("i,minerid",   "Miner ID - a number between 0-8100", cxxopts::value<std::uint32_t>()->default_value( std::to_string( DEFAULT_MINER_ID ) ) )
        ("t,threads",   "Number of threads use for mining", cxxopts::value<std::int32_t>()->default_value( std::to_string( DEFAULT_THREADS_COUNT ) ) )
        ("h,help",      "Print this usage")
        ;
    auto result = options.parse( argc, argv );
    if ( result.count( "help" ) ) {
        std::cout << options.help( ) << std::endl;
        exit( 0 );
    }
    std::string miner_address = result["address"].as<std::string>();
    // TODO validate address, len = 31 (NOSO_ADDRESS_SIZE)
    strcpy( g_miner_address, miner_address.c_str() );
    assert( strlen( g_miner_address ) == NOSO_ADDRESS_SIZE );
    g_miner_id = result["minerid"].as<std::uint32_t>();
    // TODO validate minerid value
    g_threads_count = result["threads"].as<std::int32_t>();
    // TODO validate number of threads
    std::cout << "- Wallet address: " << g_miner_address << std::endl;
    std::cout << "-       Miner ID: " << g_miner_id << std::endl;
    std::cout << "-  Threads count: " << g_threads_count << std::endl;
    const std::string miner_prefix { makePrefix( g_miner_id ) };
    auto minePrefix = [ &prefix = std::as_const( miner_prefix ) ]( int num ) {
        std::string result = std::string { prefix + makePrefix( num ) };
        result.append( 18 - result.size(), '!' );
        return result;
    };
    std::thread commThread( comm_thread );
    for ( int i = 0; i < g_threads_count - 1; i++ ) {
        g_mineObjects.push_back( std::make_shared<CMineThread>( minePrefix( i ).c_str() ) );
        g_mineThreads.push_back( std::move( std::thread( &CMineThread::mine, g_mineObjects[i] ) ) );
    }
    commThread.join();
    std::cout << "TOTAL "
        << g_total_accepted_solutions_count << " ACCEPTED "
        << g_total_rejected_solutions_count << " REJECTED "
        << g_total_failured_solutions_count << " FAILURED "
        << g_total_disposed_solutions_count << " DISPOSED "
        << g_total_passover_solutions_count << " PASSOVER " 
        << g_total_pendings_solutions_count << " PENDINGS" << " SOLUTION(S)" << std::endl;
    std::cout << "MINED " << g_mined_blcks.size() << " BLOCKS" << std::endl;
    if ( g_mined_blcks.size() > 0 ) {
        for( auto b : g_mined_blcks ) std::cout << b << " ";
        std::cout << std::endl;
    }
    return 0;
}

void signal_callback_handler(int signum) {
    std::cout << "\nCtrl+C pressed!\nFinising all threads..." << std::endl;
    g_loop_main = false;
}

void CMineThread::mine() {
    const int prefix_size = 18; // strlen( m_prefix );
    const int counter_size = 9; // std::to_string( HASHES_COUNTER_BEGINNING ).size(); // counter_size = 9 -> max 999'999'999 - 100'000'000 hashes
    char input[129];
    char base[28]; // 18-chars-prefix + 9-chars-counter = 27 chars

    memcpy( base, m_prefix, prefix_size );
    sprintf( base + prefix_size, "%d", HASHES_COUNTER_BEGINNING ); // placehold for 9-chars-counter

    memcpy( input, m_prefix, prefix_size );
    sprintf( input + prefix_size, "%d", HASHES_COUNTER_BEGINNING ); // placehold for 9-chars-counter
    memcpy( input + prefix_size + counter_size, g_miner_address, NOSO_ADDRESS_SIZE );
    int len = 58; // prefix_size + counter_size + NOSO_ADDRESS_SIZE;
    int div = ( 128 - len ) / NOSOHASH_INPUT_FILLER_COUNT;
    int mod = ( 128 - len ) % NOSOHASH_INPUT_FILLER_COUNT;
    for ( int i = 0; i < div; i++ ) {
        memcpy( input + len, NOSOHASH_INPUT_FILLER_CHARS, NOSOHASH_INPUT_FILLER_COUNT );
        len += NOSOHASH_INPUT_FILLER_COUNT;
    }
    memcpy( input + len, NOSOHASH_INPUT_FILLER_CHARS, mod );
    input[len + mod] = '\0';
    assert( strlen( input ) == 128 );

    std::uint32_t blck_no { 0 };
    char lb_hash[33];
    char mn_diff[33];
    std::shared_lock<std::shared_mutex> shared_lock_blck_no( g_mutex_blck_no, std::defer_lock );
    std::shared_lock<std::shared_mutex> shared_lock_lb_hash( g_mutex_lb_hash, std::defer_lock );
    std::unique_lock<std::shared_mutex> unique_lock_mn_diff( g_mutex_mn_diff, std::defer_lock );
    std::shared_lock<std::shared_mutex> shared_lock_mn_diff( g_mutex_mn_diff, std::defer_lock );
    std::unique_lock<std::mutex> unique_lock_pendings_solutions( g_mutex_pendings_solutions, std::defer_lock );
    std::unique_lock<std::shared_mutex> unique_lock_computed_hashes_count( m_mutex_computed_hashes_count, std::defer_lock );

    while ( g_loop_main ) {
        bool first_takenap { true };
        do {
            if ( first_takenap ) {
                first_takenap = false;
            }
            std::this_thread::sleep_for( std::chrono::milliseconds( static_cast<int>( 1000 * SUBMIT_CIRCLE_SECONDS ) ) );
            shared_lock_blck_no.lock();
            blck_no = s_blck_no;
            shared_lock_blck_no.unlock();
        } while ( g_loop_main && blck_no <= 0 );
        std::uint32_t prev_blck_no { 0 };
        while ( g_loop_main && blck_no > 0 ) {
            shared_lock_blck_no.lock();
            blck_no = s_blck_no;
            shared_lock_blck_no.unlock();
            if ( blck_no <= 0 ) {
                break;
            }
            if ( blck_no > prev_blck_no ) {
                shared_lock_lb_hash.lock();
                strcpy( lb_hash, s_lb_hash );
                shared_lock_lb_hash.unlock();
                unique_lock_computed_hashes_count.lock();
                m_computed_hashes_count = 0;
                unique_lock_computed_hashes_count.unlock();
                prev_blck_no = blck_no;
                m_hashes_counter = HASHES_COUNTER_BEGINNING;
            }
            shared_lock_mn_diff.lock();
            strcpy( mn_diff, s_mn_diff );
            shared_lock_mn_diff.unlock();
            std::uint32_t computed_hashes_count { 0 };
            auto begin_batch = std::chrono::steady_clock::now();
            for ( std::uint32_t i = 0; g_loop_main && i < HASHING_BATCH_SIZE; i++ ) {
                sprintf( base + prefix_size, "%d", m_hashes_counter ); // update counter part (size = 9 chars)
                assert( strlen( base ) == 27 ); // 27 = 18 + 9 = prefix_size + counter_size
                memcpy( input + prefix_size, base + prefix_size, counter_size );  // update counter part as it is updated in base
                assert( strlen( input ) == 128 ); // 128 = 18 + 9 + 31 + the rest of fills
                const char *hash { makeNosohash( input, m_noso_hash_buffer, m_noso_stat_buffer, &m_md5_context ) };
                const char *diff { makeNosodiff( hash, lb_hash, m_noso_diff_buffer ) };
                if ( strcmp( diff, mn_diff ) < 0 ) { // && strcmp( mn_diff, NOSO_MAX_DIFF ) < 0 )
                    unique_lock_mn_diff.lock();
                    strcpy( s_mn_diff, diff );
                    unique_lock_mn_diff.unlock();
                    strcpy( mn_diff, diff );
                    unique_lock_pendings_solutions.lock();
                    s_pendings_solutions.insert(  std::make_shared<CSolution>( blck_no + 1, base, hash, diff ) );
                    unique_lock_pendings_solutions.unlock();
                }
                m_hashes_counter ++;
                computed_hashes_count ++;
            } // END for ( int i = 0; g_loop_main && i < HASHING_BATCH_SIZE; i++ ) {
            std::chrono::duration<double> elapsed_batch = std::chrono::steady_clock::now() - begin_batch;
            double hashrate = computed_hashes_count / elapsed_batch.count();
            // std::cout << "HASHRATE: " << hashrate << std::endl;
            unique_lock_computed_hashes_count.lock();
            m_computed_hashes_count += computed_hashes_count;
            unique_lock_computed_hashes_count.unlock();
        } // END while ( g_loop_main && blck_no > 0 ) {
    } // END while ( g_loop_main ) {
}

void comm_thread() {
    std::uint32_t accepted_solutions_count = { 0 };
    std::uint32_t rejected_solutions_count = { 0 };
    std::uint32_t failured_solutions_count = { 0 };
    std::uint32_t disposed_solutions_count = { 0 };
    std::uint32_t pendings_solutions_count = { 0 };
    std::uint32_t passover_solutions_count = { 0 };
    std::uint32_t blck_no { 0 };
    char lb_hash[33];
    char mn_diff[33];
    char new_mn_diff[33];
    std::unique_lock<std::shared_mutex> unique_lock_blck_no( g_mutex_blck_no, std::defer_lock );
    std::shared_lock<std::shared_mutex> shared_lock_blck_no( g_mutex_blck_no, std::defer_lock );
    std::unique_lock<std::shared_mutex> unique_lock_lb_hash( g_mutex_lb_hash, std::defer_lock  );
    std::unique_lock<std::shared_mutex> unique_lock_mn_diff( g_mutex_mn_diff, std::defer_lock );
    std::shared_lock<std::shared_mutex> shared_lock_mn_diff( g_mutex_mn_diff, std::defer_lock );
    std::unique_lock<std::mutex> unique_lock_pendings_solutions( g_mutex_pendings_solutions, std::defer_lock );
    std::vector<std::string> accepted_hashes;

    auto report = [&]( bool won, auto begin_blck ) {
        std::chrono::duration<double> elapsed_blck = std::chrono::steady_clock::now() - begin_blck;
        std::uint32_t computed_hashes_count = std::accumulate(
                g_mineObjects.begin(), g_mineObjects.end(), 0,
                []( int a, const std::shared_ptr<CMineThread>& o ) { return a + o->getComputedHashesCount(); } );
        std::cout << "SUMMARY BLOCK#" << blck_no << (won ? "(Yay!): " : ": ") << computed_hashes_count<< " hashes computed within "
            << elapsed_blck.count() / 60 << " min (" << computed_hashes_count / elapsed_blck.count() << " h/s)\n\t"
            << " accepted " << accepted_solutions_count
            << " rejected " << rejected_solutions_count
            << " failured " << failured_solutions_count
            << " disposed " << disposed_solutions_count
            << " passover " << passover_solutions_count
            << " pendings " << pendings_solutions_count << " solution(s)" << std::endl;
        std::cout << "MINED " << g_mined_blcks.size() << " BLOCKS" << std::endl;
        g_total_accepted_solutions_count += accepted_solutions_count;
        g_total_rejected_solutions_count += rejected_solutions_count;
        g_total_failured_solutions_count += failured_solutions_count;
        g_total_disposed_solutions_count += disposed_solutions_count;
        g_total_passover_solutions_count += passover_solutions_count;
        g_total_pendings_solutions_count += pendings_solutions_count;
        accepted_solutions_count = 0;
        rejected_solutions_count = 0;
        failured_solutions_count = 0;
        disposed_solutions_count = 0;
        passover_solutions_count = 0;
        pendings_solutions_count = 0;
    };

    bool firstIter = true;
    auto begin_blck = std::chrono::steady_clock::now();
    auto begin_update = std::chrono::steady_clock::now();
    auto begin_submit = std::chrono::steady_clock::now();
    // auto begin_rate4s = std::chrono::steady_clock::now();
    // std::uint32_t accu_computed_hashes_count { 0 };
    while ( g_loop_main ) {
        // std::chrono::duration<double> elapsed_rate4s = std::chrono::steady_clock::now() - begin_rate4s;
        // if ( elapsed_rate4s.count() >= 4 ) {
        //     std::uint32_t computed_hashes_count = std::accumulate(
        //             g_mineObjects.begin(), g_mineObjects.end(), 0,
        //             []( int a, const std::shared_ptr<CMineThread>& o ) { return a + o->getComputedHashesCount(); } );
        //     std::cout << "HASHRATE: " << ( computed_hashes_count - accu_computed_hashes_count ) / elapsed_rate4s.count() << "h/s" << std::endl;
        //     accu_computed_hashes_count += computed_hashes_count;
        //     begin_rate4s = std::chrono::steady_clock::now();
        // }
        std::chrono::duration<double> elapsed_submit = std::chrono::steady_clock::now() - begin_submit;
        if ( !firstIter && elapsed_submit.count() >= SUBMIT_CIRCLE_SECONDS ) {
            std::shared_ptr<CSolution> solution = nullptr;
            do { // BEGIN } while ( g_loop_main && solution != nullptr );
                unique_lock_pendings_solutions.lock();
                if ( s_pendings_solutions.size() > 0 ) {
                    auto itor_best = s_pendings_solutions.begin();
                    solution = *itor_best;
                    s_pendings_solutions.erase( itor_best );
                    // for ( auto s : s_pendings_solutions ) {
                    //     g_disposed_solutions.insert( s );
                    //     std::cout << "\tPASSOVER>>"
                    //         << "blck[" << s->m_blck << "]diff[" << s->m_diff
                    //         << "]hash[" << s->m_hash << "]base[" << s->m_base << "]" << std::endl;
                    // }
                    passover_solutions_count += s_pendings_solutions.size();
                    s_pendings_solutions.clear();
                }
                else solution = nullptr;
                unique_lock_pendings_solutions.unlock();
                if ( solution != nullptr ) {
                    shared_lock_blck_no.lock();
                    blck_no = s_blck_no;
                    shared_lock_blck_no.unlock();
                    shared_lock_mn_diff.lock();
                    strcpy( mn_diff, s_mn_diff );
                    shared_lock_mn_diff.unlock();
                    std::cout << "\tSOLUTION>>"
                        << "blck[" << solution->m_blck << "]diff[" << solution->m_diff
                        << "]hash[" << solution->m_hash << "]base[" << solution->m_base << "]" << std::endl;
                    if ( solution->m_blck > blck_no && solution->m_diff <= mn_diff ) {
                        // std::cout << "\tSOLUTION>>"
                        //     << "blck[" << solution->m_blck << "]diff[" << solution->m_diff
                        //     << "]hash[" << solution->m_hash << "]base[" << solution->m_base << "]" << std::endl;
                        auto [ submited, accepted, code ] = syncSolution( g_miner_address, solution->m_base.c_str(), solution->m_blck, new_mn_diff );
                        if ( submited ) {
                            if ( accepted ) {
                                accepted_solutions_count ++;
                                accepted_hashes.push_back( solution->m_hash );
                                std::cout << "\t\tACCEPTED>>"
                                    << "blck[" << solution->m_blck << "]diff[" << solution->m_diff
                                    << "]hash[" << solution->m_hash << "]base[" << solution->m_base << "]" << std::endl;
                                std::cout << utc_time() << ")UPTODATED)"
                                    << "blck_no[" << blck_no << "]"
                                    << "mn_diff[" << mn_diff << "]"
                                    << "lb_hash[" << lb_hash << "]"
                                    << std::endl;
                            }
                            else {
                                rejected_solutions_count ++;
                                /* ******************CODES*******************
                                * 1: Wrong block number
                                * 2: Incorrect timestamp
                                * 3: Miner is not a valid hash address
                                * 4: Solution already received
                                * 5: Solution is not a besthash
                                * 6: Mainnet is building the next block
                                * 7: Wrong hash length (18-33 chars)
                                * ******************************************/
                                if      ( code == 1 ) {
                                    std::cout << "\t\tERROR OCCURED>>Wrong block# " << solution->m_blck << " submitted! Next CONSENSUS!" << std::endl;
                                }
                                else if ( code == 2 ) {
                                    std::cout << "\t\tERROR OCCURED>>Incorrect timestamp submitted! DO SYNC YOUR COMPUTER TIME!" << std::endl;
                                    g_loop_main = false;
                                }
                                else if ( code == 3 ) {
                                    std::cout << "\t\tERROR OCCURED>>Miner address " << g_miner_address << " is not valid!" << std::endl;
                                    g_loop_main = false;
                                }
                                else if ( code == 7 ) {
                                    std::cout << "\t\tERROR OCCURED>>Wrong hash base (" << solution->m_base << ") has submitted (THIS SHOULD NOT HAPPEND)!" << std::endl;
                                    g_loop_main = false;
                                }
                                else if ( code == 4 ) {
                                    std::cout << "\t\tDUPLICAT>>"
                                        << "blck[" << solution->m_blck << "]diff[" << solution->m_diff
                                        << "]hash[" << solution->m_hash << "]base[" << solution->m_base << "] IGNORED!" << std::endl;
                                }
                                else{
                                    std::cout << "\t\tREJECTED>>"
                                        << "blck[" << solution->m_blck << "]diff[" << solution->m_diff
                                        << "]hash[" << solution->m_hash << "]base[" << solution->m_base << "]" << std::endl;
                                    if ( code == 5 ) {
                                        unique_lock_mn_diff.lock();
                                        strcpy( s_mn_diff, new_mn_diff );
                                        unique_lock_mn_diff.unlock();
                                        strcpy( mn_diff, new_mn_diff );
                                        std::cout << utc_time() << ")UPTODATED)"
                                            << "blck_no[" << blck_no << "]"
                                            << "mn_diff[" << mn_diff << "]"
                                            << "lb_hash[" << lb_hash << "]"
                                            << std::endl;
                                    }
                                }
                            }
                        }
                        else {
                            failured_solutions_count ++;
                            std::cout << "\t\tFAILURED>>"
                                << "blck[" << solution->m_blck << "]diff[" << solution->m_diff
                                << "]hash[" << solution->m_hash << "]base[" << solution->m_base << "]" << std::endl;
                        }
                    } // END if ( solution->m_blck > blck_no && solution->m_diff <= mn_diff ) {
                    else {
                        disposed_solutions_count ++;
                        std::cout << "\t\tDISPOSED>>"
                            << "blck[" << solution->m_blck << "]diff[" << solution->m_diff
                            << "]hash[" << solution->m_hash << "]base[" << solution->m_base << "]" << std::endl;
                        // std::cout << "\t\t CURRENT>>"
                        //     << "blck[" << blck_no << "]mn_diff[" << mn_diff << "]lb_hash[" << lb_hash << "]" << std::endl;
                    }
                } // END if ( solution != nullptr ) {
            } while ( g_loop_main && solution != nullptr );
            if ( !g_loop_main ) break;
            begin_submit = std::chrono::steady_clock::now();
        } // END if ( !first && elapsed_submit.count() >= SUBMIT_CIRCLE_SECONDS ) {
        std::chrono::duration<double> elapsed_update = std::chrono::steady_clock::now() - begin_update;
        if ( firstIter || elapsed_update.count() >= UPDATE_CIRCLE_SECONDS ) {
            std::vector<std::shared_ptr<CNodeStatus>> nodes = syncSources( CONSENSUS_NODES_COUNT );
            while ( g_loop_main && nodes.size() < CONSENSUS_NODES_COUNT ) {
                std::cout << utc_time() << "))TRYING SYNC NODES ..." << std::endl;
                nodes = syncSources( CONSENSUS_NODES_COUNT );
            }
            if ( !g_loop_main ) break;
            CConsensus consensus = makeConsensus( nodes );
            shared_lock_blck_no.lock();
            blck_no = s_blck_no;
            shared_lock_blck_no.unlock();
            if ( blck_no < consensus.blck_no ) {
                bool won = false;
                if ( consensus.lb_addr == g_miner_address ) {
                    won = true;
                    g_mined_blcks.insert( consensus.blck_no );
                    bool this_machine = false;
                    if ( std::find( accepted_hashes.begin(), accepted_hashes.end(), consensus.lb_hash ) != accepted_hashes.end() )
                        this_machine = true;
                    accepted_hashes.clear();
                    std::cout << utc_time() << "))MINED BLOCK#" << consensus.blck_no
                        << (this_machine ? " BY THIS MACHINE!!!" : " TO THIS ")
                        << "ADDRESS " << consensus.lb_addr<< std::endl;
                }
                if ( blck_no > 0 ) {
                    report( won, begin_blck );
                    unique_lock_pendings_solutions.lock();
                    s_pendings_solutions.clear();
                    unique_lock_pendings_solutions.unlock();
                }
                begin_blck = std::chrono::steady_clock::now();
            }
            shared_lock_mn_diff.lock();
            strcpy( mn_diff, s_mn_diff );
            shared_lock_mn_diff.unlock();
            if (    blck_no != consensus.blck_no  ||
                    mn_diff != consensus.mn_diff ) {
                blck_no = consensus.blck_no;
                unique_lock_blck_no.lock();
                s_blck_no = consensus.blck_no;
                unique_lock_blck_no.unlock();
                unique_lock_lb_hash.lock();
                strcpy( s_lb_hash, consensus.lb_hash.c_str() );
                unique_lock_lb_hash.unlock();
                strcpy( lb_hash, consensus.lb_hash.c_str() );
                unique_lock_mn_diff.lock();
                strcpy( s_mn_diff, consensus.mn_diff.c_str() );
                unique_lock_mn_diff.unlock();
                strcpy( mn_diff, consensus.mn_diff.c_str() );
                std::cout << utc_time() << ")CONSENSUS)" << std::flush;
                consensus.print();
            }
            begin_update = std::chrono::steady_clock::now();
        } // END if ( first || elapsed_update.count() >= UPDATE_CIRCLE_SECONDS ) {
        if ( elapsed_submit.count() < SUBMIT_CIRCLE_SECONDS && elapsed_update.count() < UPDATE_CIRCLE_SECONDS ) {
            std::this_thread::sleep_for( std::chrono::milliseconds( static_cast<int>( 1000 * SUBMIT_CIRCLE_SECONDS ) ) );
        }
        firstIter = false;
    } // END while ( g_loop_main ) {
    report( false, begin_blck );
    for ( auto &thr : g_mineThreads ) thr.join();
}

// bool is_solo_mining() {
//     return true;
//     // return makeUppercase( g_source ) == "MAINNET" ? true : false;
// }

std::time_t utc_time() {
    std::time_t loc_now = std::time( 0 ) ;
    std::tm* utc_tm = std::gmtime( &loc_now );
    return std::mktime( utc_tm );
}

// char *make_uppercase( char *str ) {
//     char *p = str;
//     while ( *p != '\0' ) {
//         *p = toupper( *p );
//         p ++;
//     }
//     return str;
// }

const char NOSOHASH_HASHEABLE_CHARS[] { "!\"#$%&')*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~" };
const std::size_t NOSOHASH_HASHEABLE_COUNT =  93 - 1; // strlen( NOSOHASH_HASHEABLE_COUNT );
std::string makePrefix( int num ) {
    int div = num / NOSOHASH_HASHEABLE_COUNT;
    int mov = num % NOSOHASH_HASHEABLE_COUNT;
    return std::string { NOSOHASH_HASHEABLE_CHARS[ num / NOSOHASH_HASHEABLE_COUNT ], NOSOHASH_HASHEABLE_CHARS[ num % NOSOHASH_HASHEABLE_COUNT ] };
}

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

int clean_char( int num ) {
    while ( num > 126 ) num -= 95;
    return num;
};

char* makeNosohash_hash( const char input[129], char output[33], char noso_stat_buffer[][128] ) {
    assert( strlen( input ) == 128 );
    memcpy( noso_stat_buffer[0], input, 128 );
    for( int row = 1; row < 129; row++ ) {
        for( int col = 0; col < 127; col++ )
            noso_stat_buffer[row][col] = clean_char( noso_stat_buffer[row-1][col] + noso_stat_buffer[row-1][col+1] );
        noso_stat_buffer[row][127] = clean_char( noso_stat_buffer[row-1][127] + noso_stat_buffer[row-1][0] );
    }
    for( int i = 0; i < 32; i++ )
        output[i] = hex_dec2char( clean_char(
                                    noso_stat_buffer[128][ ( i * 4 ) + 0 ] +
                                    noso_stat_buffer[128][ ( i * 4 ) + 1 ] +
                                    noso_stat_buffer[128][ ( i * 4 ) + 2 ] +
                                    noso_stat_buffer[128][ ( i * 4 ) + 3 ] ) % 16 );
    output[32] = '\0';
    assert( strlen( output ) == 32 );
    return output;
}

char* makeNosohash_md5( const char input[33], char output[33], MD5Context *ctx ) {
    assert( strlen( input ) == 32 );
    md5Init( ctx );
    md5Update( ctx, (uint8_t *)input, 32 );
    md5Finalize( ctx );
    sprintf( output,
            "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
            ctx->digest[ 0], ctx->digest[ 1], ctx->digest[ 2], ctx->digest[ 3],
            ctx->digest[ 4], ctx->digest[ 5], ctx->digest[ 6], ctx->digest[ 7],
            ctx->digest[ 8], ctx->digest[ 9], ctx->digest[10], ctx->digest[11],
            ctx->digest[12], ctx->digest[13], ctx->digest[14], ctx->digest[15] );
    assert( strlen( output ) == 32 );
    return output;
}

char* makeNosohash( const char input[129], char output[33], char noso_stat_buffer[129][128], MD5Context *md5_context ) {
    assert( strlen( input ) == 128 && std::none_of( input, input + strlen( input ), []( int c ){ return 33 > c || c > 126; } ) );
    char *hash = makeNosohash_hash( input, output, noso_stat_buffer );
    char *md5d = makeNosohash_md5( hash, output, md5_context );
    return md5d;
}

char* makeNosodiff( const char hash[33], const char target[33], char diff[33] ) {
    assert( strlen( hash ) == 32 && strlen( target ) == 32 );
    for ( std::size_t i = 0; i < 32; i ++ ) diff[i] = toupper( hex_dec2char( abs( hex_char2dec( hash[ i ] ) - hex_char2dec( target[ i ] ) ) ) );
    diff[32] = '\0';
    assert( strlen( diff ) == 32 );
    return diff;
}

void CNodeInet::initService() {
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

void CNodeInet::cleanService() {
    if ( m_serv_info == NULL ) return;
    freeaddrinfo(m_serv_info);
    m_serv_info = NULL;
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

int inet_recv( int sockfd, uint32_t timesec, char *buffer, size_t buflen ) {
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
    int rlen = recv( sockfd, buffer, buflen - 1, 0 );
    // if (rlen < 0) { //TODO
    if ( rlen < 1 ) {
        close( sockfd );
        perror( "recv: error" );
        return -1;
    }
    // buffer[ rlen - 2 ] = '\0';
    // return rlen - 2;
    buffer[ rlen ] = '\0';
    return rlen;
}

int inet_command( struct addrinfo *serv_info, uint32_t timesec, char *buffer, size_t buflen ) {
    int sockfd = inet_socket( serv_info, timesec );
    if (sockfd < 0) return -1;
    int slen = inet_send( sockfd, timesec, buffer, strlen( buffer ) );
    if ( slen < 0 ) return slen;
    int rlen = inet_recv( sockfd, timesec, buffer, buflen );
    if ( rlen < 0 ) return rlen;
    close( sockfd );
    return rlen;
}

const char* CNodeInet::fetchNodestatus() {
    strcpy( m_fetch_buffer, "NODESTATUS\n" );
    int n = inet_command( m_serv_info, m_timeout, m_fetch_buffer, INET_BUFFER_SIZE - 1 );
    if ( n < 0 ) return nullptr;
    // std::cout << "NODESTATUS:[" << std::string( m_fetch_buffer ).erase( strlen( m_fetch_buffer ) - 2 ) << "\\r\\n]" << std::endl;
    return m_fetch_buffer;
}

const char* CNodeInet::submitSolution( const char miner[32], const char base[28], std::uint32_t blck ) {
    snprintf( m_submit_buffer, INET_BUFFER_SIZE - 1, "BESTHASH 1 2 3 4 %s %s %d %lu\n", miner, base, blck, utc_time() );
    // std::cout << "SUBMITTING:[" << std::string( m_submit_buffer ).erase( strlen( buffer ) - 2 ) << "\\r\\n]" << std::endl;
    int n = inet_command( m_serv_info, m_timeout, m_submit_buffer, INET_BUFFER_SIZE - 1 );
    if ( n < 0 ) return nullptr;
    // std::cout << "RESPONSED :[" << std::string( m_submit_buffer ).erase( strlen( m_submit_buffer ) - 2 ) << "\\r\\n]" << std::endl;
    return m_submit_buffer;
}

CNodeStatus& CNodeStatus::from_nodestatus( const char *ns ) {
    std::string nodestatus { ns };
    nodestatus.erase( nodestatus.length() - 2 ); // remove the carriage return and new line charaters

    size_t p_pos = -1, c_pos = -1;
    auto next_nodestatus_token = [&p_pos, &c_pos, &nodestatus]() {
        p_pos = c_pos;
        c_pos = nodestatus.find(' ', c_pos + 1);
    };
    auto extract_nodestatus_token = [&p_pos, &c_pos, &nodestatus]() {
        std::string token = nodestatus.substr(p_pos + 1, c_pos == std::string::npos ? std::string::npos : (c_pos - p_pos - 1));
        return token;
    };
    //NODESTATUS 1{Peers} 2{LastBlock} 3{Pendings} 4{Delta} 5{headers} 6{version} 7{UTCTime} 8{MNsHash} 9{MNscount}
    //           10{LastBlockHash} 11{BestHashDiff} 12{LastBlockTimeEnd} 13{LBMiner}
    // 0{nodestatus}
    next_nodestatus_token();
    // std::string nodestatus = extract_nodestatus_token();
    // 1{peers}
    next_nodestatus_token();
    this->peer = std::stoul(extract_nodestatus_token());
    // 2{blck}
    next_nodestatus_token();
    this->blck_no = std::stoul(extract_nodestatus_token());
    // 3{pendings}
    next_nodestatus_token();
    this->pending = std::stoul(extract_nodestatus_token());
    // 4{delta}
    next_nodestatus_token();
    this->delta = std::stoul(extract_nodestatus_token());
    // 5{headers/branch}
    next_nodestatus_token();
    this->branch = extract_nodestatus_token();
    // 6{version}
    next_nodestatus_token();
    this->version = extract_nodestatus_token();
    // 7{utctime}
    next_nodestatus_token();
    this->utctime = std::stoul(extract_nodestatus_token());
    // 8{mn_hash}
    next_nodestatus_token();
    this->mn_hash = extract_nodestatus_token();
    // 9{mn_count}
    next_nodestatus_token();
    this->mn_count = std::stoul(extract_nodestatus_token());
    // 10{lb_hash}
    next_nodestatus_token();
    this->lb_hash = extract_nodestatus_token();
    // 11{bh_diff/mn_diff}
    next_nodestatus_token();
    this->mn_diff = extract_nodestatus_token();
    // 12{lb_time}
    next_nodestatus_token();
    this->lb_time = std::stoul(extract_nodestatus_token());
    // 13{lb_addr}
    next_nodestatus_token();
    this->lb_addr = extract_nodestatus_token();
    return *this;
}

std::vector<std::shared_ptr<CNodeStatus>> syncSources( int min_nodes_count ) {
    if ( g_node_inets.size() < min_nodes_count ) {
        for ( auto ni : g_node_inets_poor ) {
            ni->initService();
            g_node_inets.push_back( ni );
        }
        g_node_inets_poor.clear();
        std::cout << "poor network reset: " << g_node_inets.size() << "/" << g_node_inets_poor.size() << std::endl;
    }
    std::shuffle( g_node_inets.begin(), g_node_inets.end(), g_random_engine );
    std::size_t nodes_count { 0 };
    std::vector<std::shared_ptr<CNodeStatus>> vec;
    // for( auto ni : g_node_inets ) {
    for ( auto it = g_node_inets.begin(); it != g_node_inets.end(); ) {
        const char *ns { (*it)->fetchNodestatus() };
        if( ns == nullptr ) {
            (*it)->cleanService();
            g_node_inets_poor.push_back( *it );
            it = g_node_inets.erase( it );
            std::cout << "poor network found: " << g_node_inets.size() << "/" << g_node_inets_poor.size() << std::endl;
            continue;
        }
        ++it;
        try {
            auto nd = std::make_shared<CNodeStatus>();
            nd->from_nodestatus( ns );
            vec.push_back( nd );
            nodes_count ++;
            if ( nodes_count >= min_nodes_count ) break;
        }
        catch ( const std::exception &e ) {
            std::cout << e.what();
        }
    }
    return vec;
}

std::tuple<bool, bool, int> syncSolution( const char miner[32], const char base[28], std::uint32_t blck, char new_mn_diff[33] ) {
    if ( g_node_inets.size() < 1 ) {
        for ( auto ni : g_node_inets_poor ) {
            ni->initService();
            g_node_inets.push_back( ni );
        }
        g_node_inets_poor.clear();
        std::cout << "poor network reset: " << g_node_inets.size() << "/" << g_node_inets_poor.size() << std::endl;
    }
    std::shuffle( g_node_inets.begin(), g_node_inets.end(), g_random_engine );
    int count { 0 };
    for ( auto it = g_node_inets.begin(); it != g_node_inets.end(); ) {
        const char *sr { (*it)->submitSolution( miner, base, blck ) };
        if ( sr == nullptr ) {
            (*it)->cleanService();
            g_node_inets_poor.push_back( *it );
            it = g_node_inets.erase( it );
            std::cout << "poor network found: " << g_node_inets.size() << "/" << g_node_inets_poor.size() << std::endl;
            continue;
        }
        ++it;
        assert( strlen( sr ) >= 40 ); //[True Diff(32) Hash(32)] or [False Diff(32) #(1)]
        std::cout << "RESULT:[" << std::string( sr ).erase( strlen( sr ) - 2 ) << "]" << std::endl;
        if ( strncmp( sr, "True", 4 ) == 0 ) {
            return std::make_tuple( true, true, 0 );
        }
        else {
            assert( strncmp( sr, "False", 5 ) == 0 && '1' <= sr[39] && sr[39] <= '7' );
            if ( sr[39] == '5' ) {
                strncpy( new_mn_diff, sr + 6, 32 );
                new_mn_diff[32] = '\0';
            }
            return std::make_tuple( true, false, sr[39] - '0' );
        }
        count ++;
        if ( count > 0 ) break;
    }
    return std::make_tuple( count > 0, false, 0 );
}

CConsensus makeConsensus( const std::vector<std::shared_ptr<CNodeStatus>> &nodes ) {
    const auto max_freq = []( const auto &freq ) {
        return std::max_element(
            std::begin( freq ), std::end( freq ),
            [] ( const auto &p1, const auto &p2 ) {
                return p1.second < p2.second; } )->first;
    };
    // std::map<std::uint32_t, int> freq_peer;
    std::map<std::uint32_t, int> freq_blck_no;
    // std::map<std::uint32_t, int> freq_pending;
    // std::map<std::uint32_t, int> freq_delta;
    // std::map<std::string  , int> freq_branch;
    // std::map<std::string  , int> freq_version;
    // std::map<std::time_t  , int> freq_utctime;
    // std::map<std::string  , int> freq_mn_hash;
    // std::map<std::uint32_t, int> freq_mn_count;
    std::map<std::string  , int> freq_lb_hash;
    std::map<std::string  , int> freq_mn_diff;
    std::map<std::time_t  , int> freq_lb_time;
    std::map<std::string  , int> freq_lb_addr;
    for( auto nd : nodes ) {
        // ++freq_peer    [nd->peer];
        ++freq_blck_no [nd->blck_no];
        // ++freq_pending [nd->pending ];
        // ++freq_delta   [nd->delta   ];
        // ++freq_branch  [nd->branch  ];
        // ++freq_version [nd->version ];
        // ++freq_utctime [nd->utctime ];
        // ++freq_mn_hash [nd->mn_hash ];
        // ++freq_mn_count[nd->mn_count];
        ++freq_lb_hash [nd->lb_hash];
        ++freq_mn_diff [nd->mn_diff];
        ++freq_lb_time [nd->lb_time];
        ++freq_lb_addr [nd->lb_addr];
    }
    return CConsensus {
        // .peer     = max_freq( freq_peer ),
        .blck_no  = max_freq( freq_blck_no ),
        // .pending  = max_freq( freq_pending ),
        // .delta    = max_freq( freq_delta   ),
        // .branch   = max_freq( freq_branch  ),
        // .version  = max_freq( freq_version ),
        // .utctime  = max_freq( freq_utctime ),
        // .mn_hash  = max_freq( freq_mn_hash ),
        // .mn_count = max_freq( freq_mn_count),
        .lb_hash  = max_freq( freq_lb_hash ),
        .mn_diff  = max_freq( freq_mn_diff ),
        .lb_time  = max_freq( freq_lb_time ),
        .lb_addr  = max_freq( freq_lb_addr ),
    };
}
