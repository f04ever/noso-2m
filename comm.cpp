#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <regex>
#include <thread>
#include <iomanip>
#include <cstring>
#include <algorithm>

#include "inet.hpp"
#include "comm.hpp"
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
    if ( this->prefix.length() != 3 ) throw std::out_of_range( "Wrong receiving pool prefix" );
    // 2{address}
    next_status_token( ' ', p_pos, c_pos, status );
    this->address = extract_status_token( p_pos, c_pos, status );
    if ( this->address.length() != 30 && this->address.length() != 31 )  throw std::out_of_range( "Wrong receiving pool address" );
    // 3{mn_diff}
    next_status_token( ' ', p_pos, c_pos, status );
    this->mn_diff = extract_status_token( p_pos, c_pos, status );
    if ( this->mn_diff.length() != 32 ) throw std::out_of_range( "Wrong receiving pool diff" );
    // 4{lb_hash}
    next_status_token( ' ', p_pos, c_pos, status );
    this->lb_hash = extract_status_token( p_pos, c_pos, status );
    if (this->lb_hash.length() != 32 ) throw std::out_of_range( "Wrong receiving lb_hash" );
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
    // 12{utctime}
    next_status_token( ' ', p_pos, c_pos, status );
    this->utctime = std::stoul( extract_status_token( p_pos, c_pos, status ) );
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
extern std::vector<std::tuple<std::uint32_t, double>> g_last_block_thread_hashrates;

CCommThread::CCommThread( std::uint32_t threads_count, pool_specs_t const &pool )
    :    m_pool { pool } {
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
    this->ClearSolutions();
};

inline
void CCommThread::_ReportMiningTarget( const std::shared_ptr<CTarget>& target ) {
    char msg[100];
    if ( m_last_block_elapsed_secs > 0 ) {
        NOSO_LOG_DEBUG
            << " Computed " << m_last_block_hashes_count << " hashes within "
            << std::fixed << std::setprecision( 2 ) << m_last_block_elapsed_secs / 60 << " minutes"
            << std::endl;
        std::shared_ptr<CPoolTarget> pool_target { std::dynamic_pointer_cast<CPoolTarget>( target ) };
        std::snprintf( msg, 100, " Hashrate(Miner/Pool/Mnet) %5.01f%c / %5.01f%c / %5.01f%c",
                hashrate_pretty_value( m_last_block_hashrate ),      hashrate_pretty_unit( m_last_block_hashrate ),
                hashrate_pretty_value( pool_target->pool_hashrate ), hashrate_pretty_unit( pool_target->pool_hashrate ),
                hashrate_pretty_value( pool_target->mnet_hashrate ), hashrate_pretty_unit( pool_target->mnet_hashrate ) );
        NOSO_LOG_INFO << msg << std::endl;
        NOSO_TUI_OutputHistPad( msg );
        if ( pool_target->payment_block == pool_target->blck_no ) {
            std::snprintf( msg, 100, " Paid %.8g NOSO", pool_target->payment_amount / 100'000'000.0 );
            NOSO_LOG_INFO << msg << std::endl;
            NOSO_TUI_OutputHistPad( msg );
            std::snprintf( msg, 100, " %s", pool_target->payment_order_id.c_str() );
            NOSO_LOG_INFO << msg << std::endl;
            NOSO_TUI_OutputHistPad( msg );
        }
        NOSO_TUI_OutputHistWin();
    }
    std::snprintf( msg, 100, "---------------------------------------------------" );
    NOSO_LOG_INFO << msg << std::endl;
    std::snprintf( msg, 100, "BLOCK %06u       %-32s",
            target->blck_no + 1, target->lb_hash.substr( 0, 32 ).c_str() );
    NOSO_LOG_INFO << msg << std::endl;
    NOSO_TUI_OutputActiWinBlockNum( target->blck_no + 1 );
    NOSO_TUI_OutputActiWinLastHash( target->lb_hash.substr( 0, 32 ) );
    std::shared_ptr<CPoolTarget> pool_target { std::dynamic_pointer_cast<CPoolTarget>( target ) };
    std::snprintf( msg, 100, " Pool %-12s %-32s",
            pool_target->pool_name.substr( 0, 12 ).c_str(),
            pool_target->mn_diff.substr( 0, 32).c_str() );
    NOSO_LOG_INFO << msg << std::endl;
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
    char msg[100];
    std::snprintf( msg, 100, "---------------------------------------------------" );
    NOSO_TUI_OutputHistPad( msg );
    std::snprintf( msg, 100, "BLOCK %06u       %-32s",
            target->blck_no + 1, target->lb_hash.substr( 0, 32 ).c_str() );
    NOSO_TUI_OutputHistPad( msg );
    std::shared_ptr<CPoolTarget> pool_target { std::dynamic_pointer_cast<CPoolTarget>( target ) };
    std::snprintf( msg, 100, " Pool %-12s %-32s",
            pool_target->pool_name.substr( 0, 12 ).c_str(),
            pool_target->mn_diff.substr( 0, 32).c_str() );
    NOSO_TUI_OutputHistPad( msg );
    std::snprintf( msg, 100, " Sent %5u / %4u / %3u | %14.8g NOSO [%2u]",
            m_accepted_solutions_count, m_rejected_solutions_count, m_failured_solutions_count,
            pool_target->till_balance / 100'000'000.0, pool_target->till_payment );
    NOSO_LOG_INFO << msg << std::endl;
    NOSO_TUI_OutputHistPad( msg );
    NOSO_TUI_OutputHistWin();
    NOSO_TUI_OutputActiWinDefault();
};

inline
void CCommThread::_ReportErrorSubmitting( int code, const std::shared_ptr<CSolution> &solution ) {
    char msg[100];
    if      ( code == 1 ) {
        NOSO_LOG_ERROR
            << "    ERROR(--"
            << ")base[" << solution->base
            << "]hash[" << solution->hash
            << "]Wrong block#" << solution->blck << " submitted!"
            << std::endl;
        std::snprintf( msg, 100, "A wrong#%06u submitted!", solution->blck );
        NOSO_TUI_OutputStatPad( msg );
    } else if ( code == 2 ) {
        NOSO_LOG_ERROR
            << "    ERROR(--"
            << ")base[" << solution->base
            << "]hash[" << solution->hash
            << "]Incorrect timestamp submitted!"
            << std::endl;
        NOSO_TUI_OutputStatPad( "An incorrect timestamp submitted!" );
    } else if ( code == 3 ) {
        NOSO_LOG_ERROR
            << "    ERROR(--"
            << ")base[" << solution->base
            << "]hash[" << solution->hash
            << "]Invalid address (" << g_miner_address << ") submitted!"
            << std::endl;
        std::snprintf( msg, 100, "An invalid address (%s) submitted!",
                g_miner_address );
        NOSO_TUI_OutputStatPad( msg );
    } else if ( code == 7 ) {
        NOSO_LOG_ERROR
            << "    ERROR(--"
            << ")base[" << solution->base
            << "]hash[" << solution->hash
            << "]Wrong hash base submitted!"
            << std::endl;
        NOSO_TUI_OutputStatPad( "A wrong hash base submitted!" );
    } else if ( code == 4 ) {
        NOSO_LOG_ERROR
            << "    ERROR(--"
            << ")base[" << solution->base
            << "]hash[" << solution->hash
            << "]Duplicate hash submitted!"
            << std::endl;
        NOSO_TUI_OutputStatPad( "A duplicate hash submitted!" );
    } else if ( code == 5 ) {
        NOSO_LOG_ERROR
            << "    ERROR(--"
            << ")base[" << solution->base
            << "]hash[" << solution->hash
            << "]Hash with wrong diff submitted!"
            << std::endl;
        NOSO_TUI_OutputStatPad( "Has with wrong diff submitted!" );
    } else if ( code == 9 ) {
        NOSO_LOG_ERROR
            << "    ERROR(--"
            << ")base[" << solution->base
            << "]hash[" << solution->hash
            << "]Shares limitation reached!"
            << std::endl;
        NOSO_TUI_OutputStatPad( "Shares limitation reached!" );
    }
    NOSO_TUI_OutputStatWin();
}

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

std::size_t CCommThread::AcceptedSolutionsCount() {
    return m_accepted_solutions_count;
}

inline
std::shared_ptr<CPoolTarget> CCommThread::RequestPoolTarget( const char address[32] ) {
    assert( std::strlen( address ) == 30 || std::strlen( address ) == 31 );
    CPoolInet inet {
            std::get<0>( m_pool ),
            std::get<1>( m_pool ),
            std::get<2>( m_pool ),
            DEFAULT_POOL_INET_TIMEOSEC };
    int rsize { inet.RequestSource( address, m_inet_buffer,
            DEFAULT_INET_BUFFER_SIZE ) };
    if ( rsize <= 0 ) {
        NOSO_LOG_DEBUG
            << "CCommThread::RequestPoolTarget Poor connecting with pool "
            << inet.m_name << "(" << inet.m_host << ":" << inet.m_port << ")"
            << std::endl;
        NOSO_TUI_OutputStatPad( "A poor connectivity while connecting with pool!" );
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
                std::get<0>( m_pool )
            );
        }
        catch ( const std::exception &e ) {
            NOSO_LOG_DEBUG
                << "CCommThread::RequestPoolTarget Unrecognised response from pool "
                << inet.m_name << "(" << inet.m_host << ":" << inet.m_port << ")"
                << "[" << m_inet_buffer << "](size=" << rsize << ")" << e.what()
                << std::endl;
            NOSO_TUI_OutputStatPad( "An unrecognised response from pool!" );
        }
    }
    NOSO_TUI_OutputStatWin();
    return nullptr;
}

inline
std::shared_ptr<CPoolTarget> CCommThread::GetPoolTargetRetrying() {
    std::uint32_t tries_count { 1 };
    std::shared_ptr<CPoolTarget> pool_target = this->RequestPoolTarget( g_miner_address );
    while ( g_still_running
            && NOSO_BLOCK_IS_IN_MINING_AGE
            && pool_target == nullptr ) {
        NOSO_LOG_DEBUG
            << "WAITING ON POOL "
            << std::get<0>( m_pool )
            << "(" << std::get<1>( m_pool ) << ":" << std::get<2>( m_pool ) << ")"
            << " (Retries " << tries_count << "/" << DEFAULT_POOL_RETRIES_COUNT << ")"
            << std::endl;
        NOSO_TUI_OutputStatPad( "Waiting on pool ..." );
        NOSO_TUI_OutputStatWin();
        std::this_thread::sleep_for( std::chrono::milliseconds(
                static_cast<int>( 1'000 * DEFAULT_INET_CIRCLE_SECONDS ) ) );
        pool_target = this->RequestPoolTarget( g_miner_address );
        ++tries_count;
        if ( tries_count > std::uint32_t( DEFAULT_POOL_RETRIES_COUNT ) )
            break;
    }
    return pool_target;
}

inline
std::shared_ptr<CTarget> CCommThread::GetTarget( const char prev_lb_hash[32] ) {
    assert( std::strlen( prev_lb_hash ) == 32 );
    std::shared_ptr<CTarget> target = this->GetPoolTargetRetrying();
    while ( g_still_running
            && NOSO_BLOCK_IS_IN_MINING_AGE
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
    static const int max_tries_count { 5 };
    CPoolInet inet {
            std::get<0>( m_pool ),
            std::get<1>( m_pool ),
            std::get<2>( m_pool ),
            DEFAULT_POOL_INET_TIMEOSEC };
    int ret_code { -1 };
    for (   int tries_count = 0;
            g_still_running
                && tries_count < max_tries_count;
            ++tries_count ) {
        int rsize { inet.SubmitSolution( blck_no, base, address,
                m_inet_buffer, DEFAULT_INET_BUFFER_SIZE ) };
        if ( rsize <= 0 ) {
            NOSO_LOG_DEBUG
                << "CCommThread::SubmitPoolSolution Poor connecting with pool "
                << inet.m_name << "(" << inet.m_host << ":" << inet.m_port << ")"
                << " Retrying " << tries_count + 1 << "/" << max_tries_count
                << std::endl;
            NOSO_TUI_OutputStatPad( "A poor connectivity while connecting with pool!" );
        } else {
            // try {
            // m_inet_buffer ~ len=(4+2)~[True\r\n] OR len=(7+2)~[False Code#(1)\r\n]
            if (        rsize >= 6
                            && std::strncmp( m_inet_buffer, "True", 4 ) == 0 ) {
                ret_code = 0;
                break;
            }
            else if (   rsize >= 9
                            && std::strncmp( m_inet_buffer, "False ", 6 ) == 0 ) {
                if ( '1' <= m_inet_buffer[6] && m_inet_buffer[6] <= '7' ) {
                    ret_code = m_inet_buffer[6] - '0';
                    break;
                }
                if ( rsize >= 18
                        && std::strncmp( m_inet_buffer + 6, "SHARES_LIMIT", 12 ) == 0 ) {
                    ret_code =  9;
                    break;
                }
            }
            // } catch ( const std::exception &e ) {}
            NOSO_LOG_DEBUG
                << "CCommThread::SubmitPoolSolution Unrecognised response from pool "
                << inet.m_name << "(" << inet.m_host << ":" << inet.m_port << ")"
                << "[" << m_inet_buffer << "](size=" << rsize << ")"
                << std::endl;
            NOSO_TUI_OutputStatPad( "An unrecognised response from pool!" );
        }
        NOSO_TUI_OutputStatWin();
        std::this_thread::sleep_for( std::chrono::milliseconds(
                static_cast<int>( 1'000 * DEFAULT_INET_CIRCLE_SECONDS ) ) );
    }
    return ret_code;
}

inline
void CCommThread::SubmitSolution( const std::shared_ptr<CSolution> &solution ) {
    int code = this->SubmitPoolSolution( solution->blck, solution->base.c_str(), g_miner_address );
    if ( code > 0 ) this->_ReportErrorSubmitting( code, solution ); // rest other error codes 1, 2, 3, 4, 7
    char msg[100] { "" };
    if ( code == 0 ) {
        m_accepted_solutions_count ++;
        NOSO_LOG_DEBUG
            << " ACCEPTED("
            << std::setfill( '0' ) << std::setw( 2 ) << m_accepted_solutions_count
            << ")base[" << solution->base
            << "]hash[" << solution->hash
            << "]"
            << std::endl;
        NOSO_TUI_OutputActiWinAcceptedSol( m_accepted_solutions_count );
        std::snprintf( msg, 100, "A submission (%u) accepted!",
                m_accepted_solutions_count );
    } else if ( code > 0) {
        m_rejected_solutions_count ++;
        NOSO_LOG_DEBUG
            << " REJECTED("
            << std::setfill( '0' ) << std::setw( 2 ) << m_rejected_solutions_count
            << ")base[" << solution->base
            << "]hash[" << solution->hash
            << "]"
            << std::endl;
        NOSO_TUI_OutputActiWinRejectedSol( m_rejected_solutions_count );
        std::snprintf( msg, 100, "A submission (%u) rejected!",
                m_rejected_solutions_count );
    } else {
        this->AddSolution( solution );
        m_failured_solutions_count ++;
        NOSO_LOG_DEBUG
            << " FAILURED("
            << std::setfill( '0' ) << std::setw( 2 ) << m_failured_solutions_count
            << ")base[" << solution->base
            << "]hash[" << solution->hash
            << "]Will retry submitting!"
            << std::endl;
        NOSO_TUI_OutputActiWinFailuredSol( m_failured_solutions_count );
        std::snprintf( msg, 100,
                "A submission (%u) failured! The solution will be re-submited.",
                m_failured_solutions_count );
    }
    if ( msg[0] ) NOSO_TUI_OutputStatPad( msg );
    NOSO_TUI_OutputStatWin();
}

void CCommThread::Communicate() {
    std::mutex mutex_wait;
    char prev_lb_hash[33] { NOSO_NUL_HASH };
    auto begin_blck = std::chrono::steady_clock::now();
    auto end_blck = std::chrono::steady_clock::now();
    while ( g_still_running ) {
        if ( ! NOSO_BLOCK_IS_IN_MINING_AGE ) {
            NOSO_TUI_OutputStatPad( "Wait next block..." );
            NOSO_TUI_OutputStatWin();
            NOSO_LOG_DEBUG << "Wait next block..." << std::endl;
            do {
                std::this_thread::sleep_for( std::chrono::milliseconds(
                        static_cast<int>( 1'000 * DEFAULT_INET_CIRCLE_SECONDS ) ) );
            } while ( g_still_running
                    && ! NOSO_BLOCK_IS_IN_MINING_AGE );
            NOSO_LOG_DEBUG << "End wait block..." << std::endl;
            if ( !g_still_running ) break;
        }
        std::shared_ptr<CTarget> target = this->GetTarget( prev_lb_hash );
        if ( !g_still_running ) break;
        if ( target == nullptr ) {
            NOSO_LOG_DEBUG << "No target. Wait the next session..." << std::endl;
            {
            auto condv_wait = std::make_shared<std::condition_variable>();
            awaiting_tasks_append( "CCommThread" + std::get<0>( m_pool ), condv_wait );
            std::unique_lock<std::mutex> unique_lock_wait( mutex_wait );
            condv_wait->wait_for( unique_lock_wait,
                    std::chrono::milliseconds( static_cast<int>(
                            1'000 * ( 585 - NOSO_BLOCK_AGE + 1 ) ) ),
                    [&]() { return !g_still_running || ! NOSO_BLOCK_IS_IN_MINING_AGE; } );
            unique_lock_wait.unlock();
            awaiting_tasks_remove( "CCommThread" + std::get<0>( m_pool ) );
            }
            NOSO_LOG_DEBUG << "Let next session..." << std::endl;
            continue;
        }
        std::strcpy( prev_lb_hash, target->lb_hash.c_str() );
        end_blck = begin_blck = std::chrono::steady_clock::now();
        for ( auto const & mo : m_mine_objects ) mo->NewTarget( target );
        this->_ReportMiningTarget( target );
        while ( g_still_running
                && NOSO_BLOCK_IS_IN_MINING_AGE ) {
            auto begin_submit = std::chrono::steady_clock::now();
            std::shared_ptr<CSolution> solution = this->GetSolution();
            if ( solution != nullptr && solution->diff < target->mn_diff )
                this->SubmitSolution( solution );
            if ( this->AcceptedSolutionsCount() >= DEFAULT_POOL_SHARES_LIMIT ) {
                NOSO_LOG_DEBUG << "Done target. Take a rest..." << std::endl;
                {
                auto condv_wait = std::make_shared<std::condition_variable>();
                awaiting_tasks_append( "CCommThread" + std::get<0>( m_pool ), condv_wait );
                std::unique_lock<std::mutex> unique_lock_wait( mutex_wait );
                condv_wait->wait_for( unique_lock_wait,
                        std::chrono::milliseconds( static_cast<int>(
                                1'000 * ( 585 - NOSO_BLOCK_AGE + 1 ) ) ),
                        [&]() { return !g_still_running || ! NOSO_BLOCK_IS_IN_MINING_AGE; } );
                unique_lock_wait.unlock();
                awaiting_tasks_remove( "CCommThread" + std::get<0>( m_pool ) );
                }
                NOSO_LOG_DEBUG << "End the rest. Let next target..." << std::endl;
                end_blck = std::chrono::steady_clock::now();
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
    } // END while ( g_still_running ) {
    for ( auto &obj : m_mine_objects ) obj->CleanupSyncState();
    for ( auto &thr : m_mine_threads ) thr.join();
}

