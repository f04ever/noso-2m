#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <algorithm>

#include "tool.hpp"
#include "inet.hpp"
#include "comm.hpp"
#include "util.hpp"
#include "output.hpp"
#include "noso-2m.hpp"

int CTools::ShowNodeInformation( std::vector<std::tuple<std::string, std::string>> const & mining_nodes ) {
    char msg[200];
    std::snprintf( msg, 200, "NODES INFORMATION" );
    NOSO_TUI_OutputInfoPad( msg );
    std::snprintf( msg, 200, "------------------------------------------------------------------------" );
    NOSO_TUI_OutputInfoPad( msg );
    std::for_each(
            std::cbegin( mining_nodes ),
            std::cend( mining_nodes ),
            []( auto const & node ) {
                std::string msg { "- " + std::get<0>( node ) + ":" + std::get<1>( node ) };
                NOSO_TUI_OutputInfoPad( msg.c_str() );
                NOSO_TUI_OutputInfoWin();
            } );
    std::snprintf( msg, 200, "--" );
    NOSO_TUI_OutputInfoPad( msg );
    NOSO_TUI_OutputInfoWin();
    return ( 0 );
}

int CTools::ShowPoolInformation( std::vector<std::tuple<std::string, std::string, std::string>> const & mining_pools ) {
    char inet_buffer[DEFAULT_INET_BUFFER_SIZE];
    char msg[200];
    std::snprintf( msg, 200, "POOL INFORMATION" );
    NOSO_TUI_OutputInfoPad( msg );
    std::snprintf( msg, 200, "     | pool name    | pool host            | fee(%%) | miners | poolrate | mnetrate " );
    NOSO_TUI_OutputInfoPad( msg );
    std::snprintf( msg, 200, "-----------------------------------------------------------------------------------" );
    NOSO_TUI_OutputInfoPad( msg );
    std::for_each( std::cbegin( mining_pools ), std::cend( mining_pools ),
                  [&, idx = 0]( std::tuple<std::string, std::string, std::string> const & pool ) mutable {
                      CPoolInet inet { std::get<0>( pool ), std::get<1>( pool ), std::get<2>( pool ), DEFAULT_POOL_INET_TIMEOSEC };
                      const int rsize { inet.RequestPoolInfo( inet_buffer, DEFAULT_INET_BUFFER_SIZE ) };
                      if ( rsize <= 0 ) {
                          NOSO_LOG_DEBUG
                              << "CUtils::ShowPoolInformation Poor connecting with pool "
                              << inet.m_name << "(" << inet.m_host << ":" << inet.m_port << ")"
                              << std::endl;
                          NOSO_TUI_OutputStatPad( "A poor connecting with pool!" );
                          NOSO_TUI_OutputStatWin();
                      } else {
                          try {
                              auto info { std::make_shared<CPoolInfo>( inet_buffer ) };
                              std::snprintf( msg, 200, " %3u | %-12s | %-20s | %6.02f | %6u | %7.02f%c | %7.02f%c ",
                                            idx, inet.m_name.substr( 0, 12 ).c_str(),
                                            ( inet.m_host + ":" + inet.m_port ).substr( 0, 20 ).c_str(),
                                            info->pool_fee / 100.0, info->pool_miners,
                                            hashrate_pretty_value( info->pool_hashrate ),
                                            hashrate_pretty_unit( info->pool_hashrate ),
                                            hashrate_pretty_value( info->mnet_hashrate ),
                                            hashrate_pretty_unit( info->mnet_hashrate ) );
                          }
                          catch ( const std::exception &e ) {
                              std::snprintf( msg, 200, " %3u | %-12s | %-20s |    N/A |    N/A |      N/A |      N/A ",
                                            idx, inet.m_name.substr( 0, 12 ).c_str(),
                                            ( inet.m_host + ":" + inet.m_port ).substr( 0, 20 ).c_str() );
                              NOSO_LOG_DEBUG
                                  << "CUtils::ShowPoolInformation Unrecognised response from pool "
                                  << inet.m_name << "(" << inet.m_host << ":" << inet.m_port << ")"
                                  << "[" << inet_buffer << "](size=" << rsize << ")" << e.what()
                                  << std::endl;
                              NOSO_TUI_OutputStatPad( "An unrecognised response from pool!" );
                              NOSO_TUI_OutputStatWin();
                          }
                          NOSO_TUI_OutputInfoPad( msg );
                          NOSO_TUI_OutputInfoWin();
                      }
                      ++idx;
                  } );
    std::snprintf( msg, 200, "--" );
    NOSO_TUI_OutputInfoPad( msg );
    NOSO_TUI_OutputInfoWin();
    return (0);
}

int CTools::ShowThreadHashrates( std::vector<std::tuple<std::uint32_t, double>> const & thread_hashrates ) {
    char msg[200];
    std::size_t const thread_count { thread_hashrates.size() };
    if ( NOSO_BLOCK_AGE <= 10 || thread_count <= 0 ) {
        std::snprintf( msg, 200, "Wait for a block finished then try again!" );
        NOSO_TUI_OutputInfoPad( msg );
        std::snprintf( msg, 200, "--" );
        NOSO_TUI_OutputInfoPad( msg );
        NOSO_TUI_OutputInfoWin();
        return (-1);
    }
    std::snprintf( msg, 200, "MINING THREADS (%zu) HASHRATES", thread_count );
    NOSO_TUI_OutputInfoPad( msg );
    char msg1[200];
    char msg2[200];
    std::size_t const threads_per_row { 4 };
    std::size_t const threads_row_count { thread_count / threads_per_row };
    std::size_t const threads_col_remain { thread_count % threads_per_row };
    std::size_t const threads_col_count { threads_row_count > 0 ? threads_per_row : threads_col_remain };
    for ( std::size_t col { 0 }; col < threads_col_count; ++col ) {
        std::snprintf( msg1 + 17 * col, 200, " tid | hashrate |" );
        std::snprintf( msg2 + 17 * col, 200, "-----------------" );
    }
    msg1[17 * threads_col_count - 1] = '\0';
    msg2[17 * threads_col_count - 1] = '\0';
    NOSO_TUI_OutputInfoPad( msg1 );
    NOSO_TUI_OutputInfoPad( msg2 );
    auto next { std::cbegin( thread_hashrates ) };
    auto out_one_column = [&]( std::size_t col ) {
        std::uint32_t const thread_id { std::get<0>( *next ) };
        double const thread_hashrate { std::get<1>( *next ) };
        std::snprintf( msg + 17 * col, 200, " %3u | %7.02f%1c |", thread_id,
                        hashrate_pretty_value( thread_hashrate ),
                        hashrate_pretty_unit( thread_hashrate ) );
    };
    auto out_all_columns = [&]( std::size_t column_count ) {
        for ( std::size_t col { 0 }; col < column_count; ++col ) {
            out_one_column( col );
            next = std::next( next );
        }
        msg[17 * column_count - 1] = '\0';
        NOSO_TUI_OutputInfoPad( msg );
    };
    for ( std::size_t row { 0 }; row < threads_row_count; ++row )
        out_all_columns( threads_per_row );
    out_all_columns( threads_col_remain );
    std::snprintf( msg, 200, "--" );
    NOSO_TUI_OutputInfoPad( msg );
    NOSO_TUI_OutputInfoWin();
    return (0);
}
