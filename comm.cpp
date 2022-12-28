#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <regex>
#include <thread>
#include <iomanip>
#include <cstring>
#include <algorithm>

#include "comm.hpp"
#include "inet.hpp"
#include "util.hpp"
#include "misc.hpp"
#include "output.hpp"

inline
void next_status_token( char sep, size_t &p_pos, size_t &c_pos, const std::string &status ) {
    p_pos = c_pos;
    c_pos = status.find( sep, c_pos + 1 );
};

inline
std::string extract_status_token( size_t p_pos, size_t c_pos, const std::string& status ) {
    return status.substr( p_pos + 1, c_pos == std::string::npos ? std::string::npos : ( c_pos - p_pos - 1 ) );
};

CPoolInfo::CPoolInfo( const char *pi ) {
    assert( pi != nullptr && std::strlen( pi ) > 2 );
    std::string status { pi };
    status.erase( status.length() - 2 ); // remove the carriage return and new line charaters
    size_t p_pos = -1, c_pos = -1;
    next_status_token( ' ', p_pos, c_pos, status );
    this->pool_miners = std::stoul( extract_status_token( p_pos, c_pos, status ) );
    next_status_token( ' ', p_pos, c_pos, status );
    this->pool_hashrate = std::stoull( extract_status_token( p_pos, c_pos, status ) );
    next_status_token( ' ', p_pos, c_pos, status );
    this->pool_fee = std::stoul( extract_status_token( p_pos, c_pos, status ) );
    next_status_token( ' ', p_pos, c_pos, status );
    this->mnet_hashrate = std::stoul( extract_status_token( p_pos, c_pos, status ) );
}

CPoolPublic::CPoolPublic( const char *pp ) {
    assert( pp != nullptr && std::strlen( pp ) > 2 );
    std::string status { pp };
    status.erase( status.length() - 2 ); // remove the carriage return and new line charaters
    size_t p_pos = -1, c_pos = -1;
    next_status_token( ' ', p_pos, c_pos, status );
    this->pool_version = extract_status_token( p_pos, c_pos, status );
    next_status_token( ' ', p_pos, c_pos, status );
    this->pool_ips_count = std::stoul( extract_status_token( p_pos, c_pos, status ) );
    next_status_token( ' ', p_pos, c_pos, status );
    this->pool_max_shares = std::stoul( extract_status_token( p_pos, c_pos, status ) );
    next_status_token( ' ', p_pos, c_pos, status );
    this->pool_pay_blocks = std::stoul( extract_status_token( p_pos, c_pos, status ) );
    next_status_token( ' ', p_pos, c_pos, status );
    this->pool_miner_ip = extract_status_token( p_pos, c_pos, status );
}

inline
CPoolStatus::CPoolStatus( const char *ps ) {
    assert( ps != nullptr && std::strlen( ps ) > 2 );
    std::string status { ps };
    status.erase( status.length() - 2 ); // remove the carriage return and new line charaters
    size_t p_pos = -1, c_pos = -1;
    // 1{MinerPrefix} 2{MinerAddress} 3{PoolMinDiff} 4{LBHash} 5{LBNumber} 6{MinerBalance}
    // 7{TillPayment} 8{LastPayInfo} 9{LastBlockPoolHashrate} {10}MainnetHashRate {11}PoolFee 12{PoolUTCTime}
    //
    // 0{OK}
    next_status_token( ' ', p_pos, c_pos, status );
    // std::string status = extract_status_token( p_pos, c_pos, status );
    // 1{prefix}
    next_status_token( ' ', p_pos, c_pos, status );
    this->prefix = extract_status_token( p_pos, c_pos, status );
    if ( this->prefix.length() != 3 ) throw std::out_of_range( "Wrong pool prefix" );
    // 2{address}
    next_status_token( ' ', p_pos, c_pos, status );
    this->address = extract_status_token( p_pos, c_pos, status );
    if ( this->address.length() != 30 && this->address.length() != 31 )  throw std::out_of_range( "Wrong pool address" );
    // 3{mn_diff}
    next_status_token( ' ', p_pos, c_pos, status );
    this->mn_diff = extract_status_token( p_pos, c_pos, status );
    if ( this->mn_diff.length() != 32 ) throw std::out_of_range( "Wrong pool diff" );
    // 4{lb_hash}
    next_status_token( ' ', p_pos, c_pos, status );
    this->lb_hash = extract_status_token( p_pos, c_pos, status );
    if (this->lb_hash.length() != 32 ) throw std::out_of_range( "Wrong lb_hash" );
    // 5{blck_no}
    next_status_token( ' ', p_pos, c_pos, status );
    this->blck_no = std::stoul( extract_status_token( p_pos, c_pos, status ) );
    // 6{till_balance}
    next_status_token( ' ', p_pos, c_pos, status );
    this->till_balance = std::stoull( extract_status_token( p_pos, c_pos, status ) );
    // 7{till_payment}
    next_status_token( ' ', p_pos, c_pos, status );
    this->till_payment = std::stoul( extract_status_token( p_pos, c_pos, status ) );
    // 8{payment_info}
    next_status_token( ' ', p_pos, c_pos, status );
    std::string payment_info = extract_status_token( p_pos, c_pos, status );
    // 9{pool_hashrate}
    next_status_token( ' ', p_pos, c_pos, status );
    this->pool_hashrate = std::stoull( extract_status_token( p_pos, c_pos, status ) );
    // 10{mnet_hashrate}
    next_status_token( ' ', p_pos, c_pos, status );
    this->mnet_hashrate = std::stoull( extract_status_token( p_pos, c_pos, status ) );
    // 11{pool_fee}
    next_status_token( ' ', p_pos, c_pos, status );
    this->pool_fee = std::stoul( extract_status_token( p_pos, c_pos, status ) );
    // 12{utc_time}
    next_status_token( ' ', p_pos, c_pos, status );
    this->utc_time = std::stoul( extract_status_token( p_pos, c_pos, status ) );
    // 13{num_miners}
    next_status_token( ' ', p_pos, c_pos, status );
    this->num_miners = std::stoul( extract_status_token( p_pos, c_pos, status ) );
    // 14{pool_fee} duplicate the 11, skip
    next_status_token( ' ', p_pos, c_pos, status );
    // this->pool_fee = std::stoul( extract_status_token( p_pos, c_pos, status ) );
    // 15{sum_amount}
    next_status_token( ' ', p_pos, c_pos, status );
    this->sum_amount = std::stoull( extract_status_token( p_pos, c_pos, status ) );
    // 16{max_shares}
    next_status_token( ' ', p_pos, c_pos, status );
    this->max_shares = std::stoul( extract_status_token( p_pos, c_pos, status ) );
    //
    // 8{payment_info}
    if ( payment_info.length() > 0 ) {
        // 8{LastPayInfo} = Block:ammount:orderID
        size_t p_pos = -1, c_pos = -1;
        // 0{payment_block}
        next_status_token( ':', p_pos, c_pos, payment_info );
        this->payment_block = std::stoul( extract_status_token( p_pos, c_pos, payment_info ) );
        // 0{payment_amount}
        next_status_token( ':', p_pos, c_pos, payment_info );
        this->payment_amount = std::stoull( extract_status_token( p_pos, c_pos, payment_info ) );
        // 0{payment_order_id}
        next_status_token( ':', p_pos, c_pos, payment_info );
        this->payment_order_id = extract_status_token( p_pos, c_pos, payment_info );
    }
}

extern char g_miner_address[];
extern std::atomic<bool> g_still_running;
extern std::uint32_t g_pool_shares_limit;
extern std::vector<std::tuple<std::uint32_t, double>> g_last_block_thread_hashrates;
extern awaiting_threads_t g_all_awaiting_threads;

CCommThread::CCommThread( std::uint32_t threads_count, pool_specs_t const & pool,
        struct addrinfo const * bind_serv )
    :   m_pool { pool }, m_bind_serv { bind_serv } {
    for ( std::uint32_t thread_id = 0; thread_id < threads_count; ++thread_id ) {
        auto mine_object { std::make_shared<CMineThread>( thread_id ) };
        m_mine_objects.push_back( mine_object );
        m_mine_threads.emplace_back( &CMineThread::Mine, mine_object, this );
    }
}

inline
void CCommThread::CloseMiningBlock( const std::chrono::duration<double>& elapsed_blck ) {
    m_last_block_hashes_count = 0;
    m_last_block_elapsed_secs = elapsed_blck.count();
    g_last_block_thread_hashrates.clear();
    for_each( m_mine_objects.begin(), m_mine_objects.end(),
            [&]( auto const & object ) {
                if ( object->m_exited < 2 ) {
                    auto block_summary = object->GetBlockSummary();
                    std::uint64_t thread_hashes { std::get<0>( block_summary ) };
                    double thread_duration { std::get<1>( block_summary ) };
                    double thread_hashrate { thread_hashes / thread_duration };
                    g_last_block_thread_hashrates.push_back(
                             std::make_tuple( object->m_thread_id, thread_hashrate ) );
                    m_last_block_hashes_count += thread_hashes;
                 } else if ( object->m_exited == 1 ) { // has just exited
                     object->m_exited = 2; // exited and did summaries the last one
                 }
             } );
    m_last_block_hashrate = m_last_block_hashes_count / m_last_block_elapsed_secs;
}

inline
void CCommThread::ResetMiningBlock() {
    m_accepted_solutions_count = 0;
    m_rejected_solutions_count = 0;
    m_failured_solutions_count = 0;
    m_reached_pool_max_shares = false;
    this->ClearSolutions();
};

inline
void CCommThread::_ReportMiningTarget( const std::shared_ptr<CTarget>& target ) {
    char msgbuf[100];
    if ( m_last_block_elapsed_secs > 0 ) {
        NOSO_LOG_DEBUG
                << " Computed " << m_last_block_hashes_count << " hashes within "
                << std::fixed << std::setprecision( 2 )
                << m_last_block_elapsed_secs / 60 << " minutes"
                << std::endl;
        auto pool_target { std::dynamic_pointer_cast<CPoolTarget>( target ) };
        std::snprintf( msgbuf, 100,
                " Hashrate(Miner/Pool/Mnet) %5.01f%c / %5.01f%c / %5.01f%c",
                hashrate_pretty_value( m_last_block_hashrate ),
                hashrate_pretty_unit( m_last_block_hashrate ),
                hashrate_pretty_value( pool_target->pool_hashrate ),
                hashrate_pretty_unit( pool_target->pool_hashrate ),
                hashrate_pretty_value( pool_target->mnet_hashrate ),
                hashrate_pretty_unit( pool_target->mnet_hashrate ) );
        NOSO_LOG_INFO << msgbuf << std::endl;
        NOSO_TUI_OutputHistPad( msgbuf );
        if ( pool_target->payment_block == pool_target->blck_no ) {
            std::snprintf( msgbuf, 100, " Paid %.8g NOSO",
                    pool_target->payment_amount / 100'000'000.0 );
            NOSO_LOG_INFO << msgbuf << std::endl;
            NOSO_TUI_OutputHistPad( msgbuf );
            std::snprintf( msgbuf, 100, " Order %s",
                    pool_target->payment_order_id.c_str() );
            NOSO_LOG_INFO << msgbuf << std::endl;
            NOSO_TUI_OutputHistPad( msgbuf );
        }
        NOSO_TUI_OutputHistWin();
    }
    std::snprintf( msgbuf, 100, "---------------------------------------------------" );
    NOSO_LOG_INFO << msgbuf << std::endl;
    std::snprintf( msgbuf, 100, "BLOCK %06u       %-32s",
            target->blck_no + 1, target->lb_hash.substr( 0, 32 ).c_str() );
    NOSO_LOG_INFO << msgbuf << std::endl;
    NOSO_TUI_OutputActiWinBlockNum( target->blck_no + 1 );
    NOSO_TUI_OutputActiWinLastHash( target->lb_hash.substr( 0, 32 ) );
    auto pool_target { std::dynamic_pointer_cast<CPoolTarget>( target ) };
    std::snprintf( msgbuf, 100, " Pool %-12s %-32s",
            pool_target->pool_name.substr( 0, 12 ).c_str(),
            pool_target->mn_diff.substr( 0, 32).c_str() );
    NOSO_LOG_INFO << msgbuf << std::endl;
    NOSO_TUI_OutputActiWinMiningDiff( target->mn_diff.substr( 0, 32 ) );
    NOSO_TUI_OutputActiWinAcceptedSol( m_accepted_solutions_count );
    NOSO_TUI_OutputActiWinRejectedSol( m_rejected_solutions_count );
    NOSO_TUI_OutputActiWinFailuredSol( m_failured_solutions_count );
    NOSO_TUI_OutputActiWinMiningSource( pool_target->pool_name.substr( 0, 12 ) );
    NOSO_TUI_OutputActiWinTillBalance( pool_target->till_balance );
    NOSO_TUI_OutputActiWinTillPayment( pool_target->till_payment );
    NOSO_TUI_OutputStatPad( "Press Ctrl+C to stop!" );
    NOSO_TUI_OutputStatWin();
}

inline
void CCommThread::_ReportTargetSummary( const std::shared_ptr<CTarget>& target ) {
    char msgbuf[100];
    std::snprintf( msgbuf, 100, "---------------------------------------------------" );
    NOSO_TUI_OutputHistPad( msgbuf );
    std::snprintf( msgbuf, 100, "BLOCK %06u       %-32s",
            target->blck_no + 1, target->lb_hash.substr( 0, 32 ).c_str() );
    NOSO_TUI_OutputHistPad( msgbuf );
    auto pool_target { std::dynamic_pointer_cast<CPoolTarget>( target ) };
    std::snprintf( msgbuf, 100, " Pool %-12s %-32s",
            pool_target->pool_name.substr( 0, 12 ).c_str(),
            pool_target->mn_diff.substr( 0, 32).c_str() );
    NOSO_TUI_OutputHistPad( msgbuf );
    std::snprintf( msgbuf, 100, " Sent %5u / %4u / %3u | %14.8g NOSO [%2u]",
            m_accepted_solutions_count, m_rejected_solutions_count, m_failured_solutions_count,
            pool_target->till_balance / 100'000'000.0, pool_target->till_payment );
    NOSO_LOG_INFO << msgbuf << std::endl;
    NOSO_TUI_OutputHistPad( msgbuf );
    NOSO_TUI_OutputHistWin();
    NOSO_TUI_OutputActiWinDefault();
};

void CCommThread::AddSolution( const std::shared_ptr<CSolution>& solution ) {
    m_mutex_solutions.lock();
    m_pool_solutions.push_back( solution );
    m_mutex_solutions.unlock();
}

inline
void CCommThread::ClearSolutions() {
    m_mutex_solutions.lock();
    m_pool_solutions.clear();
    m_mutex_solutions.unlock();
}

inline
std::size_t CCommThread::SolutionsCount() {
    std::size_t size { 0 };
    m_mutex_solutions.lock();
    size = m_pool_solutions.size();
    m_mutex_solutions.unlock();
    return size;
}

inline
const std::shared_ptr<CSolution> CCommThread::GetSolution() {
    std::shared_ptr<CSolution> good_solution { nullptr };
    m_mutex_solutions.lock();
    if ( m_pool_solutions.begin() != m_pool_solutions.end() ) {
        good_solution = m_pool_solutions.back();
        m_pool_solutions.pop_back();
    }
    m_mutex_solutions.unlock();
    return good_solution;
}

bool CCommThread::IsBandedByPool() {
    return m_been_banded_by_pool;
}

bool CCommThread::ReachedMaxShares() {
    return m_reached_pool_max_shares
            || !(   ( g_pool_shares_limit <= 0
                            && m_accepted_solutions_count <= m_pool_max_shares )
                    || ( g_pool_shares_limit > 0
                            && m_accepted_solutions_count < g_pool_shares_limit )
                );
}

inline
std::shared_ptr<CPoolTarget> CCommThread::RequestPoolTarget( const char address[32] ) {
    assert( std::strlen( address ) == 30 || std::strlen( address ) == 31 );
    char msgbuf[100];
    CPoolInet inet {
            std::get<0>( m_pool ),
            std::get<1>( m_pool ),
            std::get<2>( m_pool ),
            DEFAULT_POOL_INET_TIMEOSEC,
            m_bind_serv };
    int rsize { inet.RequestSource( address, 
            DEFAULT_INET_COMMAND_SIZE, m_inet_command,
            DEFAULT_INET_BUFFER_SIZE, m_inet_buffer ) };
    if ( rsize <= 0 ) {
        std::snprintf( msgbuf, 100,
                "Poor connection with pool %s(%s:%s)",
                inet.m_name.c_str(), inet.m_host.c_str(), inet.m_port.c_str() );
        NOSO_LOG_DEBUG << msgbuf << std::endl;
        NOSO_TUI_OutputStatPad( msgbuf );
    } else {
        try {
            CPoolStatus ps( m_inet_buffer );
            return std::make_shared<CPoolTarget>(
                ps.blck_no,
                ps.lb_hash,
                ps.mn_diff,
                ps.prefix,
                ps.address,
                ps.till_balance,
                ps.till_payment,
                ps.pool_hashrate,
                ps.payment_block,
                ps.payment_amount,
                ps.payment_order_id,
                ps.mnet_hashrate,
                ps.pool_fee,
                ps.utc_time,
                ps.num_miners,
                ps.sum_amount,
                ps.max_shares,
                std::get<0>( m_pool )
            );
        }
        catch ( const std::exception & e ) {
            if ( rsize >= 13
                    && std::strncmp( m_inet_buffer, "WRONG_ADDRESS", 13 ) == 0 ) {
                std::snprintf( msgbuf, 100,
                        "Submit by a wrong address %s", address );
            } else {
                std::snprintf( msgbuf, 100,
                        "Unrecognised response from pool %s(%s:%s)",
                        inet.m_name.c_str(), inet.m_host.c_str(), inet.m_port.c_str() );
                if ( rsize > 2
                        && m_inet_buffer[rsize - 1] == 10
                        && m_inet_buffer[rsize - 2] == 13 ) {
                    m_inet_buffer[rsize - 2 ] = '\0';
                    rsize -= 2;
                }
                std::size_t csize = std::strlen( m_inet_command );
                if ( csize > 2
                        && m_inet_command[csize - 1] == 10
                        && m_inet_command[csize - 2] == 13 ) {
                    m_inet_command[rsize - 2 ] = '\0';
                    csize -= 2;
                }
                NOSO_LOG_DEBUG
                        << "-->Command[" << m_inet_command << "](size=" << csize << ")"
                        << std::endl;
                NOSO_LOG_DEBUG
                        << "<--Response[" << m_inet_buffer << "](size=" << rsize << ")" << e.what()
                        << std::endl;
            }
            NOSO_LOG_ERROR << msgbuf << std::endl;
            NOSO_TUI_OutputHistPad( msgbuf );
            NOSO_TUI_OutputStatPad( msgbuf );
            NOSO_TUI_OutputHistWin();
        }
    }
    NOSO_TUI_OutputStatWin();
    return nullptr;
}

inline
std::shared_ptr<CPoolTarget> CCommThread::GetPoolTargetRetrying() {
    char msgbuf[100];
    std::shared_ptr<CPoolTarget> pool_target = this->RequestPoolTarget( g_miner_address );
    std::uint32_t tries_count { 0 };
    while ( g_still_running
                && NOSO_BLOCK_AGE_INNER_MINING_PERIOD
                && tries_count < std::uint32_t( DEFAULT_POOL_RETRIES_COUNT )
            && pool_target == nullptr ) {
        std::snprintf( msgbuf, 100,
                "Retry (%d/%d) sourcing from pool %s...",
                tries_count + 1, DEFAULT_POOL_RETRIES_COUNT,
                std::get<0>( m_pool ).c_str() );
        NOSO_LOG_WARN << msgbuf << std::endl;
        NOSO_TUI_OutputStatPad( msgbuf );
        NOSO_TUI_OutputStatWin();
        std::this_thread::sleep_for( std::chrono::milliseconds(
                static_cast<int>( 1'000 * DEFAULT_INET_CIRCLE_SECONDS ) ) );
        pool_target = this->RequestPoolTarget( g_miner_address );
        ++tries_count;
    }
    return pool_target;
}

inline
std::shared_ptr<CTarget> CCommThread::GetTarget( const char prev_lb_hash[32] ) {
    assert( std::strlen( prev_lb_hash ) == 32 );
    std::shared_ptr<CTarget> target = this->GetPoolTargetRetrying();
    while ( g_still_running
            && NOSO_BLOCK_AGE_INNER_MINING_PERIOD
            && ( target !=nullptr
                    && target->lb_hash == prev_lb_hash ) ) {
        std::this_thread::sleep_for( std::chrono::milliseconds(
                static_cast<int>( 1'000 * DEFAULT_INET_CIRCLE_SECONDS ) ) );
        target = this->GetPoolTargetRetrying();
    }
    return target;
}

inline
int CCommThread::SubmitPoolSolution( std::uint32_t blck_no, const char base[19], const char address[32] ) {
    assert( std::strlen( base ) == 18
            && ( std::strlen( address ) == 30 || std::strlen( address ) == 31 ) );
    int ret_code { -1 };
    char msgbuf[100];
    CPoolInet inet {
            std::get<0>( m_pool ),
            std::get<1>( m_pool ),
            std::get<2>( m_pool ),
            DEFAULT_POOL_INET_TIMEOSEC,
            m_bind_serv };
    for (   std::uint32_t tries_count { 0 };
            g_still_running
                && NOSO_BLOCK_AGE_INNER_MINING_PERIOD
                && tries_count < std::uint32_t( DEFAULT_POOL_RETRIES_COUNT );
            ++tries_count ) {
        int rsize { inet.SubmitSolution( blck_no, base, address,
                DEFAULT_INET_COMMAND_SIZE, m_inet_command,
                DEFAULT_INET_BUFFER_SIZE, m_inet_buffer ) };
        if ( rsize <= 0 ) {
            std::snprintf( msgbuf, 100,
                    "Poor connection with pool %s(%s:%s)",
                    inet.m_name.c_str(), inet.m_host.c_str(), inet.m_port.c_str() );
            NOSO_LOG_DEBUG << msgbuf << std::endl;
            NOSO_TUI_OutputStatPad( msgbuf );
            std::snprintf( msgbuf, 100,
                    "Retry (%d/%d) submitting to pool %s...",
                    tries_count + 1, DEFAULT_POOL_RETRIES_COUNT,
                    std::get<0>( m_pool ).c_str() );
            NOSO_LOG_WARN << msgbuf << std::endl;
            NOSO_TUI_OutputStatPad( msgbuf );
        } else {
            // try {
            // m_inet_buffer ~ len=(4+2)~[True\r\n] OR len=(7+2)~[False Code#(1-2)\r\n]
            if (        rsize >= 6
                            && std::strncmp( m_inet_buffer, "True", 4 ) == 0 ) {
                ret_code = 0;
                break;
            }
            else if (   rsize >= 9
                            && std::strncmp( m_inet_buffer, "False ", 6 ) == 0 ) {
                if ( rsize >= 18
                        && std::strncmp( m_inet_buffer + 6, "SHARES_LIMIT", 12 ) == 0 ) {
                    ret_code =  9;
                    break;
                }
                std::uint32_t err_code = std::stoul( m_inet_buffer + 6 );
                assert( err_code == 1
                        || err_code == 2
                        || err_code == 3
                        || err_code == 4
                        || err_code == 5
                        || err_code == 7
                        || err_code == 11
                        || err_code == 12 );
                if ( err_code == 1
                        || err_code == 2
                        || err_code == 3
                        || err_code == 4
                        || err_code == 5
                        || err_code == 7
                        || err_code == 11
                        || err_code == 12 ) {
                    ret_code = err_code;
                    break;
                }
            }
            // } catch ( const std::exception &e ) {}
            std::snprintf( msgbuf, 100,
                    "Unrecognised response from pool %s(%s:%s)",
                    inet.m_name.c_str(), inet.m_host.c_str(), inet.m_port.c_str() );
            if ( rsize > 2
                    && m_inet_buffer[rsize - 1 ] == 10
                    && m_inet_buffer[rsize - 2 ] == 13 ) {
                m_inet_buffer[rsize - 2 ] = '\0';
                rsize -= 2;
            }
            std::size_t csize = std::strlen( m_inet_command );
            if ( csize > 2
                    && m_inet_command[csize - 1] == 10
                    && m_inet_command[csize - 2] == 13 ) {
                m_inet_command[rsize - 2 ] = '\0';
                csize -= 2;
            }
            NOSO_LOG_DEBUG
                    << "-->Command[" << m_inet_command << "](size=" << csize << ")"
                    << std::endl;
            NOSO_LOG_DEBUG
                    << "<--Response[" << m_inet_buffer << "](size=" << rsize << ")"
                    << std::endl;
            NOSO_LOG_ERROR << msgbuf << std::endl;
            NOSO_TUI_OutputHistPad( msgbuf );
            NOSO_TUI_OutputStatPad( msgbuf );
        }
        NOSO_TUI_OutputHistWin();
        NOSO_TUI_OutputStatWin();
        std::this_thread::sleep_for( std::chrono::milliseconds(
                static_cast<int>( 1'000 * DEFAULT_INET_CIRCLE_SECONDS ) ) );
    }
    return ret_code;
}

inline
void CCommThread::SubmitSolution( std::shared_ptr<CSolution> const & solution,
            std::shared_ptr<CTarget> const & target ) {
    int code = this->SubmitPoolSolution( solution->blck,
            solution->base.c_str(), g_miner_address );
    char msgbuf[100];
    auto  pool_target { std::dynamic_pointer_cast<CPoolTarget>( target ) };
    if ( code == 0 ) {
        m_accepted_solutions_count ++;
        if ( m_accepted_solutions_count >= pool_target->max_shares )
            m_reached_pool_max_shares = true;
        std::snprintf( msgbuf, 100,
                "Pool %s has accepted the %u%s of %u max shares",
                std::get<0>( m_pool ).c_str(),
                m_accepted_solutions_count,
                m_accepted_solutions_count == 1 ? "st"
                      : m_accepted_solutions_count == 2 ? "nd"
                            : m_accepted_solutions_count == 3 ? "rd" : "th",
                pool_target->max_shares );
        NOSO_LOG_INFO << msgbuf << std::endl;
        // NOSO_LOG_DEBUG
        //     << "Pool[" << std::get<0>( m_pool )
        //     << "]ACCEPTED("
        //     << std::setfill( '0' ) << std::setw( 2 )
        //     << m_accepted_solutions_count
        //     << "/"
        //     << pool_target->max_shares
        //     << ")base[" << solution->base
        //     << "]hash[" << solution->hash
        //     << "]"
        //     << std::endl;
        NOSO_TUI_OutputActiWinAcceptedSol( m_accepted_solutions_count );
    } else if ( code > 0 ) {
        m_rejected_solutions_count ++;
        std::snprintf( msgbuf, 100, "Pool %s has rejected",
                std::get<0>( m_pool ).c_str() );
        if      ( code == 1 )
            std::snprintf( msgbuf, 100, "%s the wrong block number share", msgbuf );
        else if ( code == 2 )
            std::snprintf( msgbuf, 100, "%s the wrong timestamp share", msgbuf );
        else if ( code == 3 )
            std::snprintf( msgbuf, 100, "%s the wrong noso address share", msgbuf );
        else if ( code == 4 )
            std::snprintf( msgbuf, 100, "%s the duplicate hash share", msgbuf );
        else if ( code == 5 )
            std::snprintf( msgbuf, 100, "%s the wrong hashdiff share", msgbuf );
        else if ( code == 7 )
            std::snprintf( msgbuf, 100, "%s the wrong hashbase share", msgbuf );
        else if ( code == 9 )
            std::snprintf( msgbuf, 100, "%s the exceeded limit share", msgbuf );
        else if ( code == 11 )
            std::snprintf( msgbuf, 100, "%s the used TOR share (BANNED!!!)", msgbuf );
        else if ( code == 12 )
            std::snprintf( msgbuf, 100, "%s the used VPN share (BANNED!!!)", msgbuf );
        else
            std::snprintf( msgbuf, 100, "%s as the unknown response", msgbuf );
        NOSO_LOG_WARN << msgbuf << std::endl;
        NOSO_LOG_DEBUG
            << "Pool[" << std::get<0>( m_pool )
            << "]REJECTED("
            << std::setfill( '0' ) << std::setw( 2 )
            << m_rejected_solutions_count
            << ")base[" << solution->base
            << "]hash[" << solution->hash
            << "]"
            << std::endl;
        NOSO_TUI_OutputActiWinRejectedSol( m_rejected_solutions_count );
    } else { /* code < 0 */
        this->AddSolution( solution );
        m_failured_solutions_count ++;
        std::snprintf( msgbuf, 100, "Pool %s has failed to summit %u share(s)",
                std::get<0>( m_pool ).c_str(), m_failured_solutions_count );
        NOSO_LOG_INFO << msgbuf << std::endl;
        NOSO_LOG_DEBUG
            << " Pool[" << lpad( std::get<0>( m_pool ), 12, ' ' ).substr( 0, 12 )
            << "]FAILURED("
            << std::setfill( '0' ) << std::setw( 2 ) << m_failured_solutions_count
            << ")base[" << solution->base
            << "]hash[" << solution->hash
            << "]"
            << std::endl;
        NOSO_TUI_OutputActiWinFailuredSol( m_failured_solutions_count );
    }
    NOSO_TUI_OutputStatPad( msgbuf );
    NOSO_TUI_OutputStatWin();
    if ( code == 9 ) {
        m_reached_pool_max_shares = true;
    } else if ( code == 11 || code == 12 ) {
        m_been_banded_by_pool = true;
    }
}

void CCommThread::Communicate() {
    char msgbuf[100];
    char prev_lb_hash[33] { NOSO_NUL_HASH };
    auto begin_blck = std::chrono::steady_clock::now();
    auto end_blck = std::chrono::steady_clock::now();
    while ( g_still_running ) {
        std::snprintf( msgbuf, 100,
                "Wait target from pool %s...",
                std::get<0>( m_pool ).c_str() );
        NOSO_LOG_INFO << msgbuf << std::endl;
        NOSO_TUI_OutputHistPad( msgbuf );
        NOSO_TUI_OutputStatPad( msgbuf );
        NOSO_TUI_OutputHistWin();
        NOSO_TUI_OutputStatWin();
        if ( NOSO_BLOCK_AGE_OUTER_MINING_PERIOD ) {
            awaiting_threads_wait_for(
                    ( NOSO_BLOCK_AGE_BEHIND_MINING_PERIOD
                            ? ( 600 - NOSO_BLOCK_AGE + 10 )
                            : ( 10 - NOSO_BLOCK_AGE ) ) + 1,
                    std::this_thread::get_id(),
                    g_all_awaiting_threads,
                    []() -> bool { return !g_still_running
                            || NOSO_BLOCK_AGE_INNER_MINING_PERIOD; } );
            if ( !g_still_running ) break;
        }
        std::shared_ptr<CTarget> target = this->GetTarget( prev_lb_hash );
        if ( !g_still_running ) break;
        if ( target == nullptr ) {
            std::snprintf( msgbuf, 100,
                    "None target from pool %s. Take a rest",
                    std::get<0>( m_pool ).c_str() );
            NOSO_LOG_INFO << msgbuf << std::endl;
            NOSO_TUI_OutputHistPad( msgbuf );
            NOSO_TUI_OutputStatPad( msgbuf );
            NOSO_TUI_OutputHistWin();
            NOSO_TUI_OutputStatWin();
            awaiting_threads_wait_for( ( 585 - NOSO_BLOCK_AGE ) + 1,
                    std::this_thread::get_id(),
                    g_all_awaiting_threads,
                    []() -> bool { return !g_still_running
                            || NOSO_BLOCK_AGE_OUTER_MINING_PERIOD; } );
            continue;
        }
        std::strcpy( prev_lb_hash, target->lb_hash.c_str() );
        end_blck = begin_blck = std::chrono::steady_clock::now();
        for ( auto const & mo : m_mine_objects ) mo->NewTarget( target );
        this->_ReportMiningTarget( target );
        auto  pool_target { std::dynamic_pointer_cast<CPoolTarget>( target ) };
        m_pool_max_shares = pool_target->max_shares;
        while ( g_still_running
                && NOSO_BLOCK_AGE_INNER_MINING_PERIOD ) {
            auto begin_submit = std::chrono::steady_clock::now();
            std::shared_ptr<CSolution> solution = this->GetSolution();
            if ( solution != nullptr && solution->diff < target->mn_diff )
                this->SubmitSolution( solution, target );
            if ( this->IsBandedByPool() ) {
                break;
            } else if ( this->ReachedMaxShares() ) {
                end_blck = std::chrono::steady_clock::now();
                std::snprintf( msgbuf, 100,
                        "Done target from pool %s. Take a rest",
                        std::get<0>( m_pool ).c_str() );
                NOSO_LOG_INFO << msgbuf << std::endl;
                NOSO_TUI_OutputHistPad( msgbuf );
                NOSO_TUI_OutputStatPad( msgbuf );
                NOSO_TUI_OutputHistWin();
                NOSO_TUI_OutputStatWin();
                awaiting_threads_wait_for( ( 585 - NOSO_BLOCK_AGE ) + 1,
                        std::this_thread::get_id(),
                        g_all_awaiting_threads,
                        []() -> bool { return !g_still_running
                                || NOSO_BLOCK_AGE_OUTER_MINING_PERIOD; } );
            } else {
                if ( this->SolutionsCount() > 0 ) continue;
                auto end_submit = std::chrono::steady_clock::now();
                std::chrono::duration<double> elapsed_submit = end_submit - begin_submit;
                if ( elapsed_submit.count() < DEFAULT_INET_CIRCLE_SECONDS ) {
                    std::this_thread::sleep_for( std::chrono::milliseconds(
                            static_cast<int>( 1'000 * DEFAULT_INET_CIRCLE_SECONDS ) ) );
                }
            }
        }
        this->CloseMiningBlock( end_blck - begin_blck );
        this->_ReportTargetSummary( target );
        this->ResetMiningBlock();
        if ( this->IsBandedByPool() ) {
            break;
        }
    } // END while ( g_still_running ) {
    for ( auto &obj : m_mine_objects ) obj->CleanupSyncState();
    for ( auto &thr : m_mine_threads ) thr.join();
}

