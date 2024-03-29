#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <tuple>
#include <vector>
#include <string>
#include <thread>
#include <signal.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else // LINUX/UNIX
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <arpa/inet.h>
#endif // _WIN32

#include "cxxopts.hpp"

#include "noso-2m.hpp"
#include "inet.hpp"
#include "comm.hpp"
#include "output.hpp"

char g_miner_address[32] { DEFAULT_MINER_ADDRESS };
std::atomic<bool> g_still_running { true };
std::uint32_t g_pool_shares_limit { DEFAULT_POOL_SHARES_LIMIT };
std::uint32_t g_pool_threads_count { DEFAULT_POOL_THREADS_COUNT };
char g_binding_address[INET_ADDRSTRLEN] = { '\0' };
CLogLevel g_logging_level { CLogLevel::INFO };
std::vector<pool_specs_t> g_mining_pools;

std::vector<std::tuple<std::uint32_t, double>> g_last_block_thread_hashrates;
awaiting_threads_t g_all_awaiting_threads;

int main( int argc, char *argv[] ) {
    cxxopts::Options command_options( "noso-2m", "A miner for Nosocryptocurrency Protocol-2" );
    command_options.add_options()
        ( "c,config",   "Configuration file",       cxxopts::value<std::string>()->default_value( DEFAULT_CONFIG_FILENAME ) )
        ( "a,address",  "Original noso address",    cxxopts::value<std::string>()->default_value( DEFAULT_MINER_ADDRESS ) )
        ( "t,threads",  "Num. threads per pool",    cxxopts::value<std::uint32_t>()->default_value( std::to_string( DEFAULT_POOL_THREADS_COUNT ) ) )
        ( "s,shares",   "Shares limit per pool",    cxxopts::value<std::uint32_t>()->default_value( std::to_string( DEFAULT_POOL_SHARES_LIMIT ) ) )
        ( "p,pools",    "Mining pools list",        cxxopts::value<std::vector<std::string>>()->default_value( DEFAULT_POOL_URL_LIST ) )
        ( "b,binding",  "Binding none|IPv4",        cxxopts::value<std::string>()->default_value( DEFAULT_BINDING_IPV4ADDR ) )
        ( "l,logging",  "Logging info/debug",       cxxopts::value<std::string>()->default_value( DEFAULT_LOGGING_LEVEL ) )
        ( "v,version",  "Print version" )
        ( "h,help",     "Print usage" )
        ;
    cxxopts::ParseResult parsed_options;
    try {
        parsed_options = command_options.parse( argc, argv );
    } catch( cxxopts::exceptions::exception const & e ) {
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
            std::string msgstr { "No pool provided!" };
            NOSO_LOG_ERROR << msgstr << std::endl;
            NOSO_TUI_OutputHistPad( msgstr.c_str() );
            NOSO_TUI_OutputHistWin();
            throw std::bad_exception();
        }
    } catch( const std::bad_exception& e ) {
        NOSO_TUI_WaitKeyPress();
        NOSO_LOG_INFO << "===================================================" << std::endl;
        std::exit( EXIT_FAILURE );
    }
    char msgbuf[100];
    std::snprintf( msgbuf, 100, "%-31s        | %d threads",
            g_miner_address, g_pool_threads_count );
    NOSO_TUI_OutputHeadPad( msgbuf );
    NOSO_TUI_OutputHeadWin();
    std::string msgstr { "" };
    NOSO_LOG_INFO << msgstr << std::endl;
    NOSO_TUI_OutputHistPad( msgstr.c_str() );
    msgstr = std::string( "-  Wallet address: " )
            + g_miner_address;
    NOSO_LOG_INFO << msgstr << std::endl;
    NOSO_TUI_OutputHistPad( msgstr.c_str() );
    msgstr = std::string( "- Hashing threads: " )
            + std::to_string( g_pool_threads_count )
            + " threads per pool";
    NOSO_LOG_INFO << msgstr << std::endl;
    NOSO_TUI_OutputHistPad( msgstr.c_str() );
    msgstr = std::string( "-    Shares limit: " )
            + std::to_string( g_pool_shares_limit )
            + " shares per pool";
    NOSO_LOG_INFO << msgstr << std::endl;
    NOSO_TUI_OutputHistPad( msgstr.c_str() );
    if ( g_binding_address[0] ) {
        msgstr = std::string( "-    Binding IPv4: " )
                + g_binding_address;
        NOSO_LOG_INFO << msgstr << std::endl;
        NOSO_TUI_OutputHistPad( msgstr.c_str() );
    }
    for( auto itor = std::begin( g_mining_pools );
            itor != std::end( g_mining_pools );
            itor = std::next( itor ) ) {
        msgstr = ( itor == std::begin( g_mining_pools )
                        ? "-    Mining pools: "
                        : "                 : " )
                + lpad( std::get<0>( *itor ), 12, ' ' ).substr( 0, 12 )
                + "(" + std::get<1>( *itor ) + ":" + std::get<2>( *itor ) + ")";
        NOSO_LOG_INFO << msgstr << std::endl;
        NOSO_TUI_OutputHistPad( msgstr.c_str() );
    }
    if ( std::strcmp( g_miner_address, DEFAULT_MINER_ADDRESS ) == 0 ) {
        msgstr = "";
        NOSO_LOG_INFO << msgstr << std::endl;
        NOSO_TUI_OutputHistPad( msgstr.c_str() );
        msgstr = std::string( "YOU ARE MINING TO " ) + g_miner_address;
        NOSO_LOG_INFO << msgstr << std::endl;
        NOSO_TUI_OutputHistPad( msgstr.c_str() );
        msgstr = "This wallet address is the default of noso-2m.";
        NOSO_LOG_INFO << msgstr << std::endl;
        NOSO_TUI_OutputHistPad( msgstr.c_str() );
        msgstr = "Mine to this address is to donate noso-2m's author.";
        NOSO_LOG_INFO << msgstr << std::endl;
        NOSO_TUI_OutputHistPad( msgstr.c_str() );
        msgstr = "If this is not your intention, do correct noso-2m's";
        NOSO_LOG_INFO << msgstr << std::endl;
        NOSO_TUI_OutputHistPad( msgstr.c_str() );
        msgstr = "options you are providing by the command arguments,";
        NOSO_LOG_INFO << msgstr << std::endl;
        NOSO_TUI_OutputHistPad( msgstr.c_str() );
        msgstr = "or/and in the config file.";
        NOSO_LOG_INFO << msgstr << std::endl;
        NOSO_TUI_OutputHistPad( msgstr.c_str() );
        msgstr = "                                      Happy mining!";
        NOSO_LOG_INFO << msgstr << std::endl;
        NOSO_TUI_OutputHistPad( msgstr.c_str() );
        msgstr = "                                         f04ever";
        NOSO_LOG_INFO << msgstr << std::endl;
        NOSO_TUI_OutputHistPad( msgstr.c_str() );
    }
    msgstr = "";
    NOSO_LOG_INFO << msgstr << std::endl;
    NOSO_TUI_OutputHistPad( msgstr.c_str() );
    NOSO_TUI_OutputHistWin();
    struct addrinfo * bind_serv = nullptr;
    try {
        if ( inet_init() < 0 ) throw std::runtime_error( "WSAStartup errors!" );
        if ( g_binding_address[0] ) {
            int rc = inet_local_ipv4( g_binding_address );
            if ( rc < 0 ) {
                throw std::runtime_error( "Network socket errors!" );
            } else if ( rc == 0 ) {
                throw std::runtime_error( "Binding requires an active IPv4 address!" );
            }
            bind_serv = inet_service( g_binding_address, "0" );
            if ( !bind_serv ) {
                throw std::runtime_error( "Binding the IPv4 address failed!" );
            }
        }
#ifdef NO_TEXTUI
        signal( SIGINT, []( int /* signum */ ) {
            if ( g_still_running ) {
                std::string msgstr = "Ctrl+C pressed!";
                NOSO_LOG_WARN << msgstr << std::endl;
            }
            g_still_running = false;
            awaiting_threads_notify( g_all_awaiting_threads );
            std::string msgstr { "Wait for finishing all threads..." };
            NOSO_LOG_WARN << msgstr << std::endl; } );
#else // OF #ifdef NO_TEXTUI
        NOSO_TUI_HandleEventLoop( []( int key ) {
            if ( g_still_running ) {
                std::string msgstr = ( key == KEY_CTRL( 'c' ) )
                        ? "Ctrl+C pressed!"
                        : "Commanded exit!";
                NOSO_LOG_WARN << msgstr << std::endl;
                NOSO_TUI_OutputHistPad( msgstr.c_str() );
                NOSO_TUI_OutputHistWin();
                NOSO_TUI_OutputStatPad( msgstr.c_str() );
                NOSO_TUI_OutputStatWin();
            }
            g_still_running = false;
            awaiting_threads_notify( g_all_awaiting_threads );
            std::string msgstr { "Wait for finishing all threads..." };
            NOSO_LOG_WARN << msgstr << std::endl;
            NOSO_TUI_OutputHistPad( msgstr.c_str() );
            NOSO_TUI_OutputHistWin();
            NOSO_TUI_OutputStatPad( msgstr.c_str() );
            NOSO_TUI_OutputStatWin(); } );
#endif // OF #ifdef NO_TEXTUI ... #else
        std::vector<std::thread> comm_threads;
        for ( auto pool : g_mining_pools ) {
            auto comm_object { std::make_shared<CCommThread>( g_pool_threads_count, pool, bind_serv ) };
            comm_threads.emplace_back( &CCommThread::Communicate, comm_object );
        }
        for ( auto &comm_thread : comm_threads ) comm_thread.join();
        if ( bind_serv ) {
            freeaddrinfo( bind_serv );
        }
        inet_cleanup();
        NOSO_LOG_INFO << "===================================================" << std::endl;
        return EXIT_SUCCESS;
    } catch( const std::exception& e ) {
        msgstr = e.what();
        NOSO_LOG_FATAL << msgstr << std::endl;
        NOSO_TUI_OutputHistPad( msgstr.c_str() );
        NOSO_TUI_OutputHistWin();
        NOSO_TUI_WaitKeyPress();
        if ( bind_serv ) {
            freeaddrinfo( bind_serv );
        }
        inet_cleanup();
        NOSO_LOG_INFO << "===================================================" << std::endl;
        std::exit( EXIT_FAILURE );
    }
}

