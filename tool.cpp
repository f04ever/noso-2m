#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <algorithm>

#include "noso-2m.hpp"
#include "tool.hpp"
#include "inet.hpp"
#include "comm.hpp"
#include "util.hpp"
#include "output.hpp"

int CTools::ShowPoolInformation( std::vector<pool_specs_t> const & mining_pools ) {
    char inet_command[DEFAULT_INET_COMMAND_SIZE];
    char inet_buffer[DEFAULT_INET_BUFFER_SIZE];
    char msg[200];
    char msgbuf[200];
    std::snprintf( msg, 200, "POOL INFORMATION" );
    NOSO_TUI_OutputInfoPad( msg );
    std::snprintf( msg, 200, "     | pool name    | pool host            | fee(%%) | miners | poolrate | mnetrate " );
    NOSO_TUI_OutputInfoPad( msg );
    std::snprintf( msg, 200, "-----------------------------------------------------------------------------------" );
    NOSO_TUI_OutputInfoPad( msg );
    std::for_each( std::cbegin( mining_pools ), std::cend( mining_pools ),
            [&, idx = 0]( std::tuple<std::string, std::string, std::string> const & pool ) mutable {
                CPoolInet inet { std::get<0>( pool ), std::get<1>( pool ), std::get<2>( pool ),
                        DEFAULT_POOL_INET_TIMEOSEC };
                int rsize { inet.RequestPoolInfo(
                        DEFAULT_INET_COMMAND_SIZE, inet_command,
                        DEFAULT_INET_BUFFER_SIZE, inet_buffer ) };
                if ( rsize <= 0 ) {
                    std::snprintf( msgbuf, 100,
                            "Poor connection with pool %s(%s:%s)",
                            inet.m_name.c_str(), inet.m_host.c_str(), inet.m_port.c_str() );
                    NOSO_LOG_DEBUG << msgbuf << std::endl;
                    NOSO_TUI_OutputStatPad( msgbuf );
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
                    } catch ( const std::exception &e ) {
                        std::snprintf( msg, 200, " %3u | %-12s | %-20s |    N/A |    N/A |      N/A |      N/A ",
                                idx, inet.m_name.substr( 0, 12 ).c_str(),
                                ( inet.m_host + ":" + inet.m_port ).substr( 0, 20 ).c_str() );
                        std::snprintf( msgbuf, 100,
                                "Unrecognised response from pool %s(%s:%s)",
                                inet.m_name.c_str(), inet.m_host.c_str(), inet.m_port.c_str() );
                        if ( rsize > 2
                                && inet_buffer[rsize - 1] == 10
                                && inet_buffer[rsize - 2] == 13 ) {
                            inet_buffer[rsize - 2 ] = '\0';
                            rsize -= 2;
                        }
                        std::size_t csize = std::strlen( inet_command );
                        if ( csize > 2
                                && inet_command[csize - 1] == 10
                                && inet_command[csize - 2] == 13 ) {
                            inet_command[rsize - 2 ] = '\0';
                            csize -= 2;
                        }
                        NOSO_LOG_DEBUG
                                << "-->Command[" << inet_command << "](size=" << csize << ")"
                                << std::endl;
                        NOSO_LOG_DEBUG
                                << "<--Response[" << inet_buffer << "](size=" << rsize << ")" << e.what()
                                << std::endl;
                        NOSO_LOG_ERROR << msgbuf << std::endl;
                            NOSO_TUI_OutputStatPad( msgbuf );
                            NOSO_TUI_OutputStatWin();
                    }
                    NOSO_TUI_OutputInfoPad( msg );
                    NOSO_TUI_OutputInfoWin();
                }
                ++idx; } );
    std::snprintf( msg, 200, "--" );
    NOSO_TUI_OutputInfoPad( msg );
    NOSO_TUI_OutputInfoWin();
    return (0);
}

int CTools::ShowThreadHashrates( std::vector<std::tuple<std::uint32_t, double>> const & thread_hashrates ) {
    char msg[200];
    std::size_t const thread_count { thread_hashrates.size() };
    if ( NOSO_BLOCK_AGE_OUTER_MINING_PERIOD || thread_count <= 0 ) {
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

