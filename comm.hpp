#ifndef __NOSO2M_COMM_HPP__
#define __NOSO2M_COMM_HPP__

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <map>
#include <set>
#include <mutex>
#include <random>
#include <string>
#include <cassert>

#include "noso-2m.hpp"
#include "misc.hpp"
#include "mining.hpp"

struct CPoolInfo {
    std::uint32_t pool_miners;
    std::uint64_t pool_hashrate;
    std::uint32_t pool_fee;
    std::uint64_t mnet_hashrate;
    CPoolInfo( const char *pi );
};

struct CPoolPublic {
    std::string pool_version;
    std::uint32_t pool_ips_count;
    std::uint32_t pool_max_shares;
    std::uint32_t pool_pay_blocks;
    std::string pool_miner_ip;
    CPoolPublic( const char *pp );
};

struct CPoolStatus {
    std::uint32_t blck_no;
    std::string lb_hash;
    std::string mn_diff;
    std::string prefix;
    std::string address;
    std::uint64_t till_balance;
    std::uint32_t till_payment;
    std::uint64_t pool_hashrate;
    std::uint32_t payment_block;
    std::uint64_t payment_amount;
    std::string payment_order_id;
    std::uint64_t mnet_hashrate;
    std::uint32_t pool_fee;
    std::time_t utc_time;
    std::uint32_t num_miners;
    std::uint64_t sum_amount;
    std::uint32_t max_shares;
    CPoolStatus( const char *ps );
};

class CCommThread {
public:
    pool_specs_t const m_pool;
private:
    mutable std::default_random_engine m_random_engine {
            std::default_random_engine { std::random_device {}() } };
    mutable std::mutex m_mutex_solutions;
    std::vector<std::shared_ptr<CSolution>> m_pool_solutions;
    std::uint64_t m_last_block_hashes_count { 0 };
    double m_last_block_elapsed_secs { 0. };
    double m_last_block_hashrate { 0. };
    std::uint32_t m_pool_max_shares { DEFAULT_POOL_SHARES_LIMIT };
    std::uint32_t m_accepted_solutions_count { 0 };
    std::uint32_t m_rejected_solutions_count { 0 };
    std::uint32_t m_failured_solutions_count { 0 };
    char m_inet_buffer[DEFAULT_INET_BUFFER_SIZE];
    std::vector<std::thread> m_mine_threads;
    std::vector<std::shared_ptr<CMineThread>> m_mine_objects;
    const std::shared_ptr<CSolution> GetSolution();
    void ClearSolutions();
    std::size_t SolutionsCount();
    std::shared_ptr<CPoolTarget> RequestPoolTarget( const char address[32] );
    std::shared_ptr<CPoolTarget> GetPoolTargetRetrying();
    std::shared_ptr<CTarget> GetTarget( const char prev_lb_hash[32] );
    int SubmitPoolSolution( std::uint32_t blck_no, const char base[19], const char address[32] );
    void CloseMiningBlock( const std::chrono::duration<double>& elapsed_blck );
    void ResetMiningBlock();
    void _ReportMiningTarget( const std::shared_ptr<CTarget>& target );
    void _ReportTargetSummary( const std::shared_ptr<CTarget>& target );
    void _ReportErrorSubmitting( int code, const std::shared_ptr<CSolution> &solution );
public:
    CCommThread( std::uint32_t threads_count, pool_specs_t const &pool );
    CCommThread( const CCommThread& ) = delete; // Copy prohibited
    CCommThread( CCommThread&& ) = delete; // Move prohibited
    void operator=( const CCommThread& ) = delete; // Assignment prohibited
    CCommThread& operator=( CCommThread&& ) = delete; // Move assignment prohibited
    void AddSolution( const std::shared_ptr<CSolution>& solution );
    void SubmitSolution( const std::shared_ptr<CSolution> &solution );
    std::size_t AcceptedSolutionsReachedMaxShares();
    void Communicate();
};

#endif // __NOSO2M_COMM_HPP__

