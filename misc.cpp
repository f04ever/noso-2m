#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <regex>
#include <thread>
#include <cassert>

#include "misc.hpp"
#include "output.hpp"

extern char g_miner_address[];
extern std::uint32_t g_pool_shares_limit;
extern std::uint32_t g_pool_threads_count;
extern std::vector<pool_specs_t> g_mining_pools;
extern CLogLevel g_logging_level;
extern char g_bind_ipv4[];

inline
bool is_valid_address( std::string const & address ) {
    if ( address.length() < 30 || address.length() > 31 ) return false;
    return true;
}

inline
bool is_valid_threads( std::uint32_t count ) {
    if ( count < 1 ) return false;
    return true;
}

inline
bool is_valid_ipv4addr( std::string const & ipv4_address ) {
    const std::regex re_ipv4 {
            "("
            "(25[0-5]|2[0-4][0-9]|1[0-9]{2}|[1-9][0-9]|[0-9])\\."
            "(25[0-5]|2[0-4][0-9]|1[0-9]{2}|[1-9][0-9]|[0-9])\\."
            "(25[0-5]|2[0-4][0-9]|1[0-9]{2}|[1-9][0-9]|[0-9])\\."
            "(25[0-5]|2[0-4][0-9]|1[0-9]{2}|[1-9][0-9]|[0-9])"
        ")" };
    if ( std::regex_match( ipv4_address, re_ipv4) ) return true;
    return false;
}

inline
std::vector<pool_specs_t> parse_pools_argv( std::string const & poolstr ) {
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
    std::vector<pool_specs_t> mining_pools;
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
                            if ( name.length() <= 0 ) return;
                            if ( host.length() <= 0 ) return;
                            if ( port.length() <= 0 ) port = std::string { "8082" };
                            mining_pools.push_back( { name, host, port } );
                        }
                    );
            }
        );
    return mining_pools;
}

struct _mining_options_t {
    int shares;
    int threads;
    std::string address;
    std::string pools;
    std::string filename;
    std::string logging;
    std::string binding;
}   _g_arg_options = {
        .shares = DEFAULT_POOL_SHARES_LIMIT,
        .threads = DEFAULT_POOL_THREADS_COUNT,
        .logging = DEFAULT_LOGGING_LEVEL,
        .binding = DEFAULT_BINDING_IPV4ADDR,
    },
    _g_cfg_options = {
        .shares = DEFAULT_POOL_SHARES_LIMIT,
        .threads = DEFAULT_POOL_THREADS_COUNT,
        .logging = DEFAULT_LOGGING_LEVEL,
        .binding = DEFAULT_BINDING_IPV4ADDR,
    };

inline
void process_arg_options( cxxopts::ParseResult const & parsed_options ) {
    try {
        _g_arg_options.address = parsed_options["address"].as<std::string>();
        if ( !is_valid_address( _g_arg_options.address ) )
            throw std::invalid_argument( "Invalid miner address argument" );
        _g_arg_options.threads = parsed_options["threads"].as<std::uint32_t>();
        if ( !is_valid_threads( _g_arg_options.threads ) )
            throw std::invalid_argument( "Invalid threads count argument" );
        _g_arg_options.shares = parsed_options["shares"].as<std::uint32_t>();
        _g_arg_options.pools = parsed_options["pools"].as<std::string>();
        _g_arg_options.logging = parsed_options["logging"].as<std::string>();
        if ( _g_arg_options.logging != "info"
                && _g_arg_options.logging != "debug" )
            throw std::invalid_argument( "Invalid logging level argument" );
        _g_arg_options.binding = parsed_options["binding"].as<std::string>();
        if ( !( _g_arg_options.binding == "none" ) 
                && !is_valid_ipv4addr( _g_arg_options.binding ) )
            throw std::invalid_argument( "Invalid binding argument (an IPv4 address)" );
    } catch( const std::invalid_argument& e ) {
        std::string msg { e.what() };
        NOSO_LOG_FATAL << msg << std::endl;
        NOSO_TUI_OutputHistPad( msg.c_str() );
        throw std::bad_exception();
    }
}

inline
void process_cfg_options( cxxopts::ParseResult const & parsed_options ) {
    _g_cfg_options.filename = parsed_options["config"].as<std::string>();
    std::ifstream cfg_istream( _g_cfg_options.filename );
    if( !cfg_istream.good() ) {
        std::string msg { "Config file '" + _g_cfg_options.filename + "' not found!" };
        NOSO_LOG_WARN << msg << std::endl;
        NOSO_TUI_OutputHistPad( msg.c_str() );
        if ( _g_cfg_options.filename != DEFAULT_CONFIG_FILENAME ) throw std::bad_exception();
        msg = "Use default options";
        NOSO_LOG_INFO << msg << std::endl;
        NOSO_TUI_OutputHistPad( msg.c_str() );
        NOSO_TUI_OutputHistWin();
    } else {
        std::string msg { "Load config file '" + _g_cfg_options.filename + "'" };
        NOSO_LOG_INFO << msg << std::endl;
        NOSO_TUI_OutputHistPad( msg.c_str() );
        NOSO_TUI_OutputHistWin();
        int line_no { 0 };
        std::string line_str;
        try {
            while ( std::getline( cfg_istream, line_str ) ) {
                line_no++;
                if        ( line_str.rfind( "address ", 0 ) == 0 ) {
                    _g_cfg_options.address = line_str.substr( 8 );
                    if ( !is_valid_address( _g_cfg_options.address ) )
                        throw std::invalid_argument( "Invalid address config" );
                } else if ( line_str.rfind( "threads ", 0 ) == 0 ) {
                    _g_cfg_options.threads = std::stoul( line_str.substr( 8 ) );
                    if ( !is_valid_threads( _g_cfg_options.threads ) )
                        throw std::invalid_argument( "Invalid threads count config" );
                } else if ( line_str.rfind( "shares ", 0 ) == 0 ) {
                    _g_cfg_options.shares = std::stoul( line_str.substr( 7 ) );
                } else if ( line_str.rfind( "pools ",   0 ) == 0 ) {
                    if ( _g_cfg_options.pools.size() > 0 ) _g_cfg_options.pools += ";";
                    _g_cfg_options.pools += line_str.substr( 6 );
                } else if ( line_str.rfind( "logging ", 0 ) == 0 ) {
                    _g_cfg_options.logging = line_str.substr( 8 );
                    if ( _g_cfg_options.logging != "info"
                            && _g_cfg_options.logging != "debug" )
                        throw std::invalid_argument( "Invalid logging level config" );
                } else if ( line_str.rfind( "binding ", 0 ) == 0 ) {
                    _g_cfg_options.binding = line_str.substr( 8 );
                    if ( !( _g_cfg_options.binding == "none" ) 
                            && !is_valid_ipv4addr( _g_cfg_options.binding ) )
                        throw std::invalid_argument( "Invalid binding config (an IPv4 address)" );
                }
            }
        } catch( const std::invalid_argument& e ) {
            std::string msg { std::string( e.what() ) + " in file '" + _g_cfg_options.filename + "'" };
            NOSO_LOG_FATAL << msg << " line#" << line_no << "[" << line_str << "]" << std::endl;
            NOSO_TUI_OutputHistPad( msg.c_str() );
            throw std::bad_exception();
        }
    }
}

void process_options( cxxopts::ParseResult const & parsed_options ) {
    process_cfg_options( parsed_options );
    process_arg_options( parsed_options );
    std::string sel_address {
        _g_arg_options.address != DEFAULT_MINER_ADDRESS ? _g_arg_options.address
            : _g_cfg_options.address.length() > 0 ? _g_cfg_options.address : DEFAULT_MINER_ADDRESS };
    std::string sel_logging {
        _g_arg_options.logging != DEFAULT_LOGGING_LEVEL ? _g_arg_options.logging
            : _g_cfg_options.logging.length() > 0 ? _g_cfg_options.logging : DEFAULT_LOGGING_LEVEL };
    std::string sel_pools {
        _g_arg_options.pools != DEFAULT_POOL_URL_LIST ? _g_arg_options.pools
            : _g_cfg_options.pools.length() > 0 ? _g_cfg_options.pools : DEFAULT_POOL_URL_LIST };
    std::string sel_binding {
        _g_arg_options.binding != DEFAULT_BINDING_IPV4ADDR  ? _g_arg_options.binding
            : _g_cfg_options.binding.length() > 0 ? _g_cfg_options.binding : DEFAULT_BINDING_IPV4ADDR };
    std::strncpy( g_miner_address, sel_address.c_str(), 32 );
    g_pool_shares_limit = _g_arg_options.shares != DEFAULT_POOL_SHARES_LIMIT ? _g_arg_options.shares
        : _g_cfg_options.shares != DEFAULT_POOL_SHARES_LIMIT ? _g_cfg_options.shares : DEFAULT_POOL_SHARES_LIMIT;
    g_pool_threads_count = _g_arg_options.threads != DEFAULT_POOL_THREADS_COUNT ? _g_arg_options.threads
        : _g_cfg_options.threads != DEFAULT_POOL_THREADS_COUNT ? _g_cfg_options.threads : DEFAULT_POOL_THREADS_COUNT;
    g_logging_level = sel_logging == "info" ? CLogLevel::INFO : CLogLevel::DEBUG;
    if ( sel_binding == "none" ) {
        g_bind_ipv4[0] = '\0';
    } else {
        std::strncpy( g_bind_ipv4, sel_binding.c_str(), 16 );
    }
    g_mining_pools = parse_pools_argv( sel_pools );
}

bool awaiting_threads_handle(
        std::shared_ptr<std::condition_variable> const & condv,
        std::thread::id thread_id, awaiting_threads_t & awaiting_threads ) {
    std::size_t key = std::hash<std::thread::id>()( thread_id );
    std::unique_lock<std::mutex> const lock( awaiting_threads.mutex );
    auto const & end_itor = std::cend( awaiting_threads.awaits );
    auto const & key_itor = awaiting_threads.awaits.find( key );
    assert ( key_itor == end_itor );
    if ( key_itor == end_itor ) {
        awaiting_threads.awaits[key] = condv;
        return true;
    }
    return false;
}

bool awaiting_threads_release(
        std::thread::id thread_id, awaiting_threads_t & awaiting_threads ) {
    std::size_t key = std::hash<std::thread::id>()( thread_id );
    std::unique_lock<std::mutex> const lock( awaiting_threads.mutex );
    auto const & end_itor = std::cend( awaiting_threads.awaits );
    auto const & key_itor = awaiting_threads.awaits.find( key );
    assert ( key_itor != end_itor );
    if ( key_itor != end_itor ) {
        awaiting_threads.awaits.erase( key_itor );
        return true;
    }
    return false;
}

void awaiting_threads_notify( awaiting_threads_t & awaiting_threads ) {
    std::unique_lock<std::mutex> const lock( awaiting_threads.mutex );
    std::for_each(
            std::begin( awaiting_threads.awaits ),
            std::end( awaiting_threads.awaits ),
            []( std::pair<std::size_t, std::shared_ptr<std::condition_variable>> element ){
                    element.second->notify_all(); } );
}

void awaiting_threads_wait(
        std::thread::id thread_id, awaiting_threads_t & awaiting_threads,
        bool ( * awake )() ) {
    auto condv = std::make_shared<std::condition_variable>();
    auto result = awaiting_threads_handle( condv, thread_id, awaiting_threads );
    assert( result );
    if ( result ) {
        std::mutex mutex;
        std::unique_lock<std::mutex> lock( mutex );
        condv->wait( lock, awake );
        awaiting_threads_release( thread_id, awaiting_threads );
    }
}

void awaiting_threads_wait_for( double seconds,
        std::thread::id thread_id, awaiting_threads_t & awaiting_threads,
        bool ( * awake )() ) {
    if ( seconds < 0 ) return;
    auto msec = std::chrono::milliseconds( static_cast<long>( 1'000 * seconds ) );
    auto condv = std::make_shared<std::condition_variable>();
    auto result = awaiting_threads_handle( condv, thread_id, awaiting_threads );
    assert( result );
    if ( result ) {
        std::mutex mutex;
        std::unique_lock<std::mutex> lock( mutex );
        condv->wait_for( lock, msec, awake );
        awaiting_threads_release( thread_id, awaiting_threads );
    }
}
