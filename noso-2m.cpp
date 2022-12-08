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

std::atomic<bool> g_still_running { true };
char g_miner_address[32] { DEFAULT_MINER_ADDRESS };
std::uint32_t g_threads_count { DEFAULT_THREADS_COUNT };
std::uint32_t g_pool_shares_limit { DEFAULT_POOL_SHARES_LIMIT };
std::vector<pool_specs_t> g_mining_pools;
std::vector<std::tuple<std::uint32_t, double>> g_last_block_thread_hashrates;

int main( int argc, char *argv[] ) {
    cxxopts::Options command_options( "noso-2m", "A miner for Nosocryptocurrency Protocol-2" );
    command_options.add_options()
        ( "c,config",   "A configuration file",                 cxxopts::value<std::string>()->default_value( DEFAULT_CONFIG_FILENAME ) )
        ( "a,address",  "An original noso wallet address",      cxxopts::value<std::string>()->default_value( DEFAULT_MINER_ADDRESS ) )
        ( "p,pools",    "Mining pools list",                    cxxopts::value<std::string>()->default_value( DEFAULT_POOL_URL_LIST ) )
        ( "t,threads",  "Hashing threads per pool", cxxopts::value<std::uint32_t>()->default_value( std::to_string( DEFAULT_THREADS_COUNT ) ) )
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
        NOSO_LOG_ERROR << e.what() << std::endl;
        NOSO_LOG_INFO << "===================================================" << std::endl;
        std::exit( EXIT_FAILURE );
    }
    try {
        process_options( parsed_options );
        if ( g_mining_pools.size() <= 0 ) {
            std::string msg { "No pool provided!" };
            NOSO_LOG_ERROR << msg << std::endl;
            NOSO_TUI_OutputHistPad( msg.c_str() );
            NOSO_TUI_OutputHistWin();
            throw std::bad_exception();
        }
    } catch( const std::bad_exception& e ) {
        NOSO_TUI_WaitKeyPress();
        NOSO_LOG_INFO << "===================================================" << std::endl;
        std::exit( EXIT_FAILURE );
    }
    char buffer[100];
    std::snprintf( buffer, 100, "%-31s        | %d threads", g_miner_address, g_threads_count );
    NOSO_TUI_OutputHeadPad( buffer );
    NOSO_TUI_OutputHeadWin();
    std::string msg { "" };
    NOSO_LOG_INFO << msg << std::endl;
    NOSO_TUI_OutputHistPad( msg.c_str() );
    msg = std::string( "-  Wallet address: " ) + g_miner_address;
    NOSO_LOG_INFO << msg << std::endl;
    NOSO_TUI_OutputHistPad( msg.c_str() );
    msg = std::string( "- Hashing threads: " ) + std::to_string( g_threads_count ) + " threads per pool";
    NOSO_LOG_INFO << msg << std::endl;
    NOSO_TUI_OutputHistPad( msg.c_str() );
    for( auto itor = std::begin( g_mining_pools );
            itor != std::end( g_mining_pools );
            itor = std::next( itor ) ) {
        msg = ( itor == std::begin( g_mining_pools ) ? "-    Mining pools: " : "                 : " )
                + lpad( std::get<0>( *itor ), 12, ' ' ).substr( 0, 12 )
                + "(" + std::get<1>( *itor ) + ":" + std::get<2>( *itor ) + ")";
        NOSO_LOG_INFO << msg << std::endl;
        NOSO_TUI_OutputHistPad( msg.c_str() );
    }
    if ( std::strcmp( g_miner_address, DEFAULT_MINER_ADDRESS ) == 0 ) {
        msg = "";
        NOSO_LOG_INFO << msg << std::endl;
        NOSO_TUI_OutputHistPad( msg.c_str() );
        msg = std::string( "YOU ARE MINING TO " ) + g_miner_address;
        NOSO_LOG_INFO << msg << std::endl;
        NOSO_TUI_OutputHistPad( msg.c_str() );
        msg = "This wallet address is the default of noso-2m.";
        NOSO_LOG_INFO << msg << std::endl;
        NOSO_TUI_OutputHistPad( msg.c_str() );
        msg = "Mine to this address is to donate noso-2m's author.";
        NOSO_LOG_INFO << msg << std::endl;
        NOSO_TUI_OutputHistPad( msg.c_str() );
        msg = "If this is not your intention, do correct noso-2m's";
        NOSO_LOG_INFO << msg << std::endl;
        NOSO_TUI_OutputHistPad( msg.c_str() );
        msg = "options you are providing by the command arguments,";
        NOSO_LOG_INFO << msg << std::endl;
        NOSO_TUI_OutputHistPad( msg.c_str() );
        msg = "or/and in the config file.";
        NOSO_LOG_INFO << msg << std::endl;
        NOSO_TUI_OutputHistPad( msg.c_str() );
        msg = "                                      Happy mining!";
        NOSO_LOG_INFO << msg << std::endl;
        NOSO_TUI_OutputHistPad( msg.c_str() );
        msg = "                                         f04ever";
        NOSO_LOG_INFO << msg << std::endl;
        NOSO_TUI_OutputHistPad( msg.c_str() );
    }
    msg = "";
    NOSO_LOG_INFO << msg << std::endl;
    NOSO_TUI_OutputHistPad( msg.c_str() );
    NOSO_TUI_OutputHistWin();
    try {
        if ( inet_init() < 0 ) throw std::runtime_error( "WSAStartup errors!" );
        std::vector<std::thread> comm_threads;
        for ( auto pool : g_mining_pools ) {
            auto comm_object { std::make_shared<CCommThread>( g_threads_count, pool ) };
            comm_threads.emplace_back( &CCommThread::Communicate, comm_object );
        }
#ifdef NO_TEXTUI
        signal( SIGINT, []( int /* signum */ ) {
            if ( !g_still_running ) return;
            std::string msg { "Ctrl+C pressed! Wait for finishing all threads..." };
            NOSO_LOG_WARN << msg << std::endl;
            g_still_running = false;
            awaiting_threads_notify( ); } );
#else // OF #ifdef NO_TEXTUI
        int last_key = NOSO_TUI_HandleEventLoop();
        g_still_running = false;
        awaiting_threads_notify( );
        std::string msg { "Wait for finishing all threads..." };
        if ( last_key == KEY_CTRL( 'c' ) ) msg = "Ctrl+C pressed! " + msg;
        else  msg = "Commanded exit! " + msg;
        NOSO_LOG_WARN << msg << std::endl;
        NOSO_TUI_OutputHistPad( msg.c_str() );
        NOSO_TUI_OutputHistWin();
        NOSO_TUI_OutputStatPad( msg.c_str() );
        NOSO_TUI_OutputStatWin();
#endif // OF #ifdef NO_TEXTUI ... #else
        for ( auto &comm_thread : comm_threads ) comm_thread.join();
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

