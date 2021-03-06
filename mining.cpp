#include <mutex>
#include <cstring>
#include "mining.hpp"
#include "noso-2m.hpp"
#include "hashing.hpp"

extern bool g_still_running;
extern bool g_solo_mining;

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

void CMineThread::WaitTarget() {
    std::unique_lock<std::mutex> unique_lock_blck_no( m_mutex_blck_no );
    m_condv_blck_no.wait( unique_lock_blck_no, [&]() { return !g_still_running || m_blck_no > 0; } );
    unique_lock_blck_no.unlock();
}

void CMineThread::DoneTarget() {
    std::unique_lock<std::mutex> unique_lock_blck_no( m_mutex_blck_no );
    m_blck_no = 0;
    unique_lock_blck_no.unlock();
}

void CMineThread::NewTarget( const std::shared_ptr<CTarget> &target ) {
    std::string thread_prefix = {
        target->prefix
        + nosohash_prefix( m_miner_id )
        + nosohash_prefix( m_thread_id ) };
    thread_prefix.append( 9 - thread_prefix.size(), '!' );
    m_mutex_blck_no.lock();
    std::strcpy( m_prefix, thread_prefix.c_str() );
    std::strcpy( m_address, target->address.c_str() );
    std::strcpy( m_lb_hash, target->lb_hash.c_str() );
    std::strcpy( m_mn_diff, target->mn_diff.c_str() );
    m_blck_no = target->blck_no + 1;
    m_mutex_blck_no.unlock();
    m_condv_blck_no.notify_one();
}

void CMineThread::Mine( void ( * NewSolFunc )( const std::shared_ptr<CSolution>& ) ) {
    while ( g_still_running ) {
        this->WaitTarget();
        if ( !g_still_running ) break;
        assert( ( std::strlen( m_address ) == 30 || std::strlen( m_address ) == 31 )
                && std::strlen( m_lb_hash ) == 32
                && std::strlen( m_mn_diff ) == 32 );
        char best_diff[33];
        std::strcpy( best_diff, m_mn_diff );
        std::size_t match_len { 0 };
        while ( best_diff[match_len] == '0' ) ++match_len;
        std::uint32_t noso_hash_counter { 0 };
        CNosoHasher noso_hasher( m_prefix, m_address );
        auto begin_mining { std::chrono::steady_clock::now() };
        while ( g_still_running && 1 <= NOSO_BLOCK_AGE && NOSO_BLOCK_AGE <= 585 ) {
            const char *base { noso_hasher.GetBase( noso_hash_counter++ ) };
            const char *hash { noso_hasher.GetHash() };
            assert( std::strlen( base ) == 18 && std::strlen( hash ) == 32 );
            if ( std::strncmp( hash, m_lb_hash, match_len ) == 0 ) {
                if ( g_solo_mining ) {
                    const char *diff { noso_hasher.GetDiff( m_lb_hash ) };
                    assert( std::strlen( diff ) == 32 );
                    if ( std::strcmp( diff, best_diff ) < 0 ) {
                        NewSolFunc( std::make_shared<CSolution>( m_blck_no, base, hash, diff ) );
                        std::strcpy( best_diff, diff );
                        while ( best_diff[match_len] == '0' ) ++match_len;
                    }
                } else NewSolFunc( std::make_shared<CSolution>( m_blck_no, base, hash, "" ) );
            }
        }
        std::chrono::duration<double> elapsed_mining { std::chrono::steady_clock::now() - begin_mining };
        this->SetBlockSummary( noso_hash_counter, elapsed_mining.count() );
        this->DoneTarget();
    } // END while ( g_still_running ) {
}
