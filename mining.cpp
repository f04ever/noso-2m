#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <mutex>
#include <thread>
#include <cstring>

#include "comm.hpp"
#include "misc.hpp"
#include "mining.hpp"

extern std::atomic<bool> g_still_running;

CMineThread::CMineThread( std::uint32_t thread_id )
    :   m_thread_id { thread_id } {
}

void CMineThread::CleanupSyncState() {
    m_condv_blck_no.notify_one();
    m_condv_summary.notify_one();
}

void CMineThread::SetBlockSummary( std::uint32_t hashes_count, double duration ) {
    m_mutex_summary.lock();
    m_block_mining_duration = duration;
    m_computed_hashes_count = hashes_count;
    m_mutex_summary.unlock();
    m_condv_summary.notify_one();
}

std::tuple<std::uint32_t, double> CMineThread::GetBlockSummary() {
    std::unique_lock<std::mutex> unique_lock_summary_lock( m_mutex_summary );
    m_condv_summary.wait( unique_lock_summary_lock, [&]() {
            return !g_still_running || m_computed_hashes_count > 0; } );
    auto summary = std::make_tuple( m_computed_hashes_count, m_block_mining_duration );
    m_computed_hashes_count = 0;
    m_block_mining_duration = 0.;
    unique_lock_summary_lock.unlock();
    return summary;
}

inline
void CMineThread::WaitTarget() {
    std::unique_lock<std::mutex> unique_lock_blck_no( m_mutex_blck_no );
    m_condv_blck_no.wait( unique_lock_blck_no, [&]() {
            return !g_still_running || m_blck_no > 0; } );
    unique_lock_blck_no.unlock();
}

inline
void CMineThread::DoneTarget() {
    std::unique_lock<std::mutex> unique_lock_blck_no( m_mutex_blck_no );
    m_blck_no = 0;
    unique_lock_blck_no.unlock();
    this->CleanupSyncState();
}

void CMineThread::NewTarget( const std::shared_ptr<CTarget> &target ) {
    std::string thread_prefix = {
        target->prefix
        + nosohash_prefix( m_thread_id ) };
    thread_prefix.append( 9 - thread_prefix.size(), '!' );
    m_mutex_blck_no.lock();
    if ( m_prefix != thread_prefix
        || m_address != target->address ) {
        std::strcpy( m_prefix, thread_prefix.c_str() );
        std::strcpy( m_address, target->address.c_str() );
        m_hasher.Init( m_prefix, m_address );
    }
    std::strcpy( m_lb_hash, target->lb_hash.c_str() );
    std::strcpy( m_mn_diff, target->mn_diff.c_str() );
    m_blck_no = target->blck_no + 1;
    m_mutex_blck_no.unlock();
    m_condv_blck_no.notify_one();
}

void CMineThread::Mine( CCommThread * pCommThread ) {
    std::mutex mutex_wait;
    m_exited = 0;
    char best_diff[33];
    while ( g_still_running ) {
        this->WaitTarget();
        if ( !g_still_running ) break;
        assert( ( std::strlen( m_address ) == 30 || std::strlen( m_address ) == 31 )
                && std::strlen( m_lb_hash ) == 32
                && std::strlen( m_mn_diff ) == 32 );
        std::strcpy( best_diff, m_mn_diff );
        std::size_t match_len { 0 };
        while ( best_diff[match_len] == '0' ) ++match_len;
        std::uint32_t hashes_counter { 0 };
        auto begin_mining { std::chrono::steady_clock::now() };
        while ( g_still_running
                && NOSO_BLOCK_IS_IN_MINING_AGE ) {
            if ( pCommThread->AcceptedSolutionsReachedMaxShares() ) {
                {
                auto condv_wait = std::make_shared<std::condition_variable>();
                awaiting_tasks_append( "CMineThread" + std::get<0>( pCommThread->m_pool ), condv_wait );
                std::unique_lock<std::mutex> unique_lock_wait( mutex_wait );
                condv_wait->wait_for( unique_lock_wait,
                        std::chrono::milliseconds( static_cast<int>(
                                1'000 * ( 585 - NOSO_BLOCK_AGE + 1 ) ) ),
                        [&]() { return !g_still_running || ! NOSO_BLOCK_IS_IN_MINING_AGE; } );
                unique_lock_wait.unlock();
                awaiting_tasks_remove( "CMineThread" + std::get<0>( pCommThread->m_pool ) );
                }
            } else {
                const char *base { m_hasher.GetBase( hashes_counter++ ) };
                const char *hash { m_hasher.GetHash() };
                assert( std::strlen( base ) == 18 && std::strlen( hash ) == 32 );
                if ( std::strncmp( hash, m_lb_hash, match_len ) == 0 ) {
                    pCommThread->AddSolution( std::make_shared<CSolution>( m_blck_no, base, hash, "" ) );
                }
            }
        }
        auto end_mining { std::chrono::steady_clock::now() };
        std::chrono::duration<double> elapsed_mining { end_mining - begin_mining };
        this->SetBlockSummary( hashes_counter, elapsed_mining.count() );
        this->DoneTarget();
    } // END while ( g_still_running ) {
    m_exited = 1;
}

