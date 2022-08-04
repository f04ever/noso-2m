#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <tuple>
#include <vector>
#include <string>
#include <thread>
#include <signal.h>
#include "cxxopts.hpp"
#include "noso-2m.hpp"
#include "inet.hpp"
#include "misc.hpp"
#include "comm.hpp"
#include "util.hpp"
#include "output.hpp"
#include "mining.hpp"
#include "hashing.hpp"

extern
std::vector<std::tuple<std::string, std::string>> const g_default_nodes {
        { "198.144.190.194" ,   "8080" },
        { "109.230.238.240" ,   "8080" },
        { "107.172.193.176" ,   "8080" },
        { "66.151.117.247"  ,   "8080" },
        { "192.3.73.184"    ,   "8080" },
        { "107.175.24.151"  ,   "8080" },
    }; // seed nodes
extern
std::vector<std::tuple<std::string, std::string, std::string>> const g_default_pools {
        { "f04ever", "209.126.80.203", "8082" },
        /* { "devnoso", "45.146.252.103", "8082" }, */
    };
bool g_solo_mining { false };
bool g_still_running { true };
char g_miner_address[32] { DEFAULT_MINER_ADDRESS };
std::uint32_t g_miner_id { DEFAULT_MINER_ID };
std::uint32_t g_threads_count { DEFAULT_THREADS_COUNT };
std::uint32_t g_mined_block_count { 0 };
std::vector<std::thread> g_mine_threads;
std::vector<std::shared_ptr<CMineThread>> g_mine_objects;
std::vector<std::tuple<std::string, std::string, std::string>> g_mining_pools;
std::vector<std::tuple<std::uint32_t, double>> g_last_block_thread_hashrates;

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
    } catch ( const cxxopts::OptionException& e ) {
        NOSO_STDERR << "Invalid option provided: " << e.what() << std::endl;
        NOSO_STDERR << "Use option ’--help’ for usage detail" << std::endl;
        std::exit( EXIT_FAILURE );
    }
    if ( parsed_options.count( "help" ) ) {
        NOSO_STDOUT << command_options.help() << std::endl;
        std::exit( EXIT_SUCCESS );
    }
    if ( parsed_options.count( "version" ) ) {
        NOSO_STDOUT << "version " << NOSO_2M_VERSION << std::endl;
        std::exit( EXIT_SUCCESS );
    }
    NOSO_LOG_INIT();
    NOSO_LOG_INFO << "noso-2m - A miner for Nosocryptocurrency Protocol-2" << std::endl;
    NOSO_LOG_INFO << "f04ever (c) 2022 https://github.com/f04ever/noso-2m" << std::endl;
    NOSO_LOG_INFO << "version " << NOSO_2M_VERSION << std::endl;
    NOSO_LOG_INFO << "--" << std::endl;
    try {
        NOSO_TUI_StartTUI();
    } catch( const std::runtime_error& e ) {
        NOSO_STDERR << e.what() << std::endl;
        NOSO_LOG_INFO << "===================================================" << std::endl;
        std::exit( EXIT_FAILURE );
    }
    try {
        process_options( parsed_options );
    } catch( const std::bad_exception& e ) {
        NOSO_TUI_WaitKeyPress();
        NOSO_LOG_INFO << "===================================================" << std::endl;
        std::exit( EXIT_FAILURE );
    }
    char buffer[100];
    std::snprintf( buffer, 100, "%-31s [%04d] | %d threads", g_miner_address, g_miner_id, g_threads_count );
    NOSO_TUI_OutputHeadPad( buffer );
    NOSO_TUI_OutputHeadWin();
    std::string msg { "" };
    NOSO_LOG_INFO << msg << std::endl;
    NOSO_TUI_OutputHistPad( msg.c_str() );
    msg = std::string( "- Wallet address: " ) + g_miner_address;
    NOSO_LOG_INFO << msg << std::endl;
    NOSO_TUI_OutputHistPad( msg.c_str() );
    msg = std::string( "-       Miner ID: " ) + std::to_string( g_miner_id );
    NOSO_LOG_INFO << msg << std::endl;
    NOSO_TUI_OutputHistPad( msg.c_str() );
    msg = std::string( "-  Threads count: " ) + std::to_string( g_threads_count );
    NOSO_LOG_INFO << msg << std::endl;
    NOSO_TUI_OutputHistPad( msg.c_str() );
    msg = std::string( "-    Mining mode: " ) + ( g_solo_mining ? "solo" : "pool" );
    NOSO_LOG_INFO << msg << std::endl;
    NOSO_TUI_OutputHistPad( msg.c_str() );
    if ( !g_solo_mining ) {
        for( auto itor = g_mining_pools.cbegin(); itor != g_mining_pools.cend(); itor = std::next( itor ) ) {
            msg = ( itor == g_mining_pools.cbegin() ? "-   Mining pools: " : "                : " )
                    + lpad( std::get<0>( *itor ), 12, ' ' ).substr( 0, 12 )
                    + "(" + std::get<1>( *itor ) + ":" + std::get<2>( *itor ) + ")";
            NOSO_LOG_INFO << msg << std::endl;
            NOSO_TUI_OutputHistPad( msg.c_str() );
        }
    }
    NOSO_TUI_OutputHistWin();
    try {
        if ( inet_init() < 0 ) throw std::runtime_error( "WSAStartup errors!" );
        if ( g_solo_mining ) {
            std::time_t mainnet_timestamp { CCommThread::GetInstance()->RequestTimestamp() };
            if ( mainnet_timestamp == std::time_t( -1 ) ) {
                throw std::runtime_error( "Can not check mainnet's timestamp!" );
            }
            else {
                std::time_t computer_timestamp { NOSO_TIMESTAMP };
                long timestamp_difference = std::abs( computer_timestamp - mainnet_timestamp );
                if ( timestamp_difference > DEFAULT_TIMESTAMP_DIFFERENCES ) {
                    msg = "Your machine's time is different ("
                        + std::to_string( timestamp_difference )
                        + ") from mainnet. Synchronize clock!";
                    throw std::runtime_error( msg );
                }
            }
        }
        for ( std::uint32_t thread_id = 0; thread_id < g_threads_count - 1; ++thread_id )
            g_mine_objects.push_back( std::make_shared<CMineThread>( g_miner_id, thread_id ) );
        std::thread comm_thread( &CCommThread::Communicate, CCommThread::GetInstance() );
        auto const NewSolFunc { []( const std::shared_ptr<CSolution>& solution ){
            CCommThread::GetInstance()->AddSolution( solution ); } };
        for ( std::uint32_t thread_id = 0; thread_id < g_threads_count - 1; ++thread_id )
            g_mine_threads.emplace_back( &CMineThread::Mine, g_mine_objects[thread_id], NewSolFunc );
#ifdef NO_TEXTUI
        signal( SIGINT, []( int /* signum */ ) {
            if ( !g_still_running ) return;
            std::string msg { "Ctrl+C pressed! Wait for finishing all threads..." };
            NOSO_LOG_WARN << msg << std::endl;
            g_still_running = false; });
#else // OF #ifdef NO_TEXTUI
        int last_key = NOSO_TUI_HandleEventLoop();
        g_still_running = false;
        std::string msg { "Wait for finishing all threads..." };
        if ( last_key == KEY_CTRL( 'c' ) ) msg = "Ctrl+C pressed! " + msg;
        else  msg = "Commanded exit! " + msg;
        NOSO_LOG_WARN << msg << std::endl;
        NOSO_TUI_OutputHistPad( msg.c_str() );
        NOSO_TUI_OutputHistWin();
        NOSO_TUI_OutputStatPad( msg.c_str() );
        NOSO_TUI_OutputStatWin();
#endif // OF #ifdef NO_TEXTUI ... #else
        comm_thread.join();
        inet_cleanup();
        NOSO_LOG_INFO << "===================================================" << std::endl;
        return EXIT_SUCCESS;
    } catch( const std::exception& e ) {
        std::string msg { e.what() };
        NOSO_LOG_FATAL << msg << std::endl;
        NOSO_TUI_OutputHistPad( msg.c_str() );
        NOSO_TUI_OutputHistWin();
        NOSO_TUI_WaitKeyPress();
        inet_cleanup();
        NOSO_LOG_INFO << "===================================================" << std::endl;
        std::exit( EXIT_FAILURE );
    }
}
