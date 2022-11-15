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
        { "109.230.238.240" ,   "8080" },
        { "149.57.235.14"   ,   "8080" },
        { "149.57.226.244"  ,   "8080" },
        { "81.22.38.101"    ,   "8080" },
        { "66.151.117.247"  ,   "8080" },
        { "149.57.229.81"   ,   "8080" },
        { "149.57.242.211"  ,   "8080" },
        { "149.57.138.12"   ,   "8080" },
        { "159.196.1.198"   ,   "8080" },
        { "101.100.138.125" ,   "8080" },
    }; // seed nodes
extern
std::vector<std::tuple<std::string, std::string, std::string>> const g_default_pools {
        { "f04ever", "f04ever.com", "8082" },
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
    if ( g_solo_mining ) {
        NOSO_LOG_INFO << "===================================================" << std::endl;
        NOSO_LOG_INFO << std::endl;
        NOSO_LOG_INFO << "SOLO MINING HAS BEEN DISABLE ON NOSO NETWORK" << std::endl;
        NOSO_LOG_INFO << "More detail in the #ANNOUNCEMENTS on the Discord channel in Nov.08 2022" << std::endl;
        NOSO_LOG_INFO << "https://discord.com/channels/816716075747901470/816717022812831765/1039427185078964305" << std::endl;
        NOSO_LOG_INFO << std::endl;
        NOSO_LOG_INFO << "===================================================" << std::endl;
        NOSO_TUI_OutputHistPad( "===================================================" );
        NOSO_TUI_OutputHistPad( "" );;
        NOSO_TUI_OutputHistPad( "SOLO MINING HAS BEEN DISABLE ON NOSO NETWORK" );
        NOSO_TUI_OutputHistPad( "More detail in the #ANNOUNCEMENTS on the Discord channel in Nov.08 2022" );
        NOSO_TUI_OutputHistPad( "https://discord.com/channels/816716075747901470/816717022812831765/1039427185078964305" );
        NOSO_TUI_OutputHistPad( "" );
        NOSO_TUI_OutputHistPad( "===================================================" );
        NOSO_TUI_WaitKeyPress();
        std::exit( EXIT_FAILURE );
    }
    if ( !g_solo_mining ) {
        for( auto itor = g_mining_pools.cbegin(); itor != g_mining_pools.cend(); itor = std::next( itor ) ) {
            msg = ( itor == g_mining_pools.cbegin() ? "-   Mining pools: " : "                : " )
                    + lpad( std::get<0>( *itor ), 12, ' ' ).substr( 0, 12 )
                    + "(" + std::get<1>( *itor ) + ":" + std::get<2>( *itor ) + ")";
            NOSO_LOG_INFO << msg << std::endl;
            NOSO_TUI_OutputHistPad( msg.c_str() );
        }
    }
    if ( std::strcmp( g_miner_address, DEFAULT_MINER_ADDRESS ) == 0 ) {
        msg = "";
        NOSO_LOG_INFO << msg << std::endl;
        NOSO_TUI_OutputHistPad( msg.c_str() );
        msg = std::string( "YOU ARE MINING TO " ) + g_miner_address;
        NOSO_LOG_INFO << msg << std::endl;
        NOSO_TUI_OutputHistPad( msg.c_str() );
        msg = "This address is the default of miner noso-2m.";
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
        msg = "Happy mining!";
        NOSO_LOG_INFO << msg << std::endl;
        NOSO_TUI_OutputHistPad( msg.c_str() );
    }
    msg = "";
    NOSO_LOG_INFO << msg << std::endl;
    NOSO_TUI_OutputHistPad( msg.c_str() );
    NOSO_TUI_OutputHistWin();
    try {
        if ( inet_init() < 0 ) throw std::runtime_error( "WSAStartup errors!" );
        std::time_t mainnet_timestamp { CCommThread::GetInstance()->RequestTimestamp() };
        if ( mainnet_timestamp == std::time_t( -1 ) ) {
            throw std::runtime_error( "Can not check mainnet's timestamp!" );
        }
        else {
            std::time_t computer_timestamp { static_cast<time_t>( NOSO_TIMESTAMP ) };
            long timestamp_difference = std::abs( computer_timestamp - mainnet_timestamp );
            if ( timestamp_difference > DEFAULT_TIMESTAMP_DIFFERENCES ) {
                msg = "Your machine's time is different ("
                    + std::to_string( timestamp_difference )
                    + ") from mainnet. Synchronize clock!";
                throw std::runtime_error( msg );
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
