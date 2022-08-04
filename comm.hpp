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
#include "mining.hpp"

struct CNodeStatus {
    // std::uint32_t peer;
    std::uint32_t blck_no;
    // std::uint32_t pending;
    // std::uint32_t delta;
    // std::string branch;
    // std::string version;
    // std::time_t utctime;
    // std::string mn_hash;
    // std::uint32_t mn_count;
    std::string lb_hash;
    std::string mn_diff;
    std::time_t lb_time;
    std::string lb_addr;
    // std::uint32_t check_count;
    // std::uint64_t lb_pows;
    // std::string lb_diff;
    CNodeStatus( const char *ns_line );
};

struct CPoolInfo {
    std::uint32_t pool_miners;
    std::uint64_t pool_hashrate;
    std::uint32_t pool_fee;
    CPoolInfo( const char *pi );
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
    CPoolStatus( const char *ps_line );
};

class CCommThread { // A singleton pattern class
private:
    mutable std::default_random_engine m_random_engine {
        std::default_random_engine { std::random_device {}() } };
    mutable std::mutex m_mutex_solutions;
    std::vector<std::tuple<std::string, std::string>> m_mining_nodes;
    std::vector<std::tuple<std::string, std::string, std::string>>& m_mining_pools;
    std::uint32_t m_mining_pools_id;
    std::multiset<std::shared_ptr<CSolution>, CSolsSetCompare> m_solo_solutions;
    std::vector<std::shared_ptr<CSolution>> m_pool_solutions;
    std::uint64_t m_last_block_hashes_count { 0 };
    double m_last_block_elapsed_secs { 0. };
    double m_last_block_hashrate { 0. };
    std::uint32_t m_accepted_solutions_count { 0 };
    std::uint32_t m_rejected_solutions_count { 0 };
    std::uint32_t m_failured_solutions_count { 0 };
    std::map<std::uint32_t, int> m_freq_blck_no;
    std::map<std::string  , int> m_freq_lb_hash;
    std::map<std::string  , int> m_freq_mn_diff;
    std::map<std::time_t  , int> m_freq_lb_time;
    std::map<std::string  , int> m_freq_lb_addr;
    char m_inet_buffer[DEFAULT_INET_BUFFER_SIZE];
    CCommThread();
    std::vector<std::tuple<std::string, std::string>> const & GetDefaultNodes();
    static std::vector<std::tuple<std::string, std::string>> LoadHintNodes();
    static bool SaveHintNodes( std::vector<std::tuple<std::string, std::string>> const &nodes );
    static std::vector<std::tuple<std::string, std::string>> GetValidators(
            std::vector<std::tuple<std::string, std::string>> const &hints );
    void UpdateMiningNodesInSoloModeIfNeed();
    const std::shared_ptr<CSolution> BestSolution();
    const std::shared_ptr<CSolution> GoodSolution();
    std::shared_ptr<CSolution> GetSolution();
    void ClearSolutions();
    std::vector<std::shared_ptr<CNodeStatus>> RequestNodeSources( std::size_t min_nodes_count );
    std::shared_ptr<CNodeTarget> GetNodeTargetConsensus();
    std::shared_ptr<CPoolTarget> RequestPoolTarget( const char address[32] );
    std::shared_ptr<CPoolTarget> GetPoolTargetFailover();
    int SubmitSoloSolution( std::uint32_t blck, const char base[19], const char address[32], char new_mn_diff[33] );
    int SubmitPoolSolution( std::uint32_t blck_no, const char base[19], const char address[32] );
    void CloseMiningBlock( const std::chrono::duration<double>& elapsed_blck );
    void ResetMiningBlock();
    void _ReportMiningTarget( const std::shared_ptr<CTarget>& target );
    void _ReportTargetSummary( const std::shared_ptr<CTarget>& target );
    void _ReportErrorSubmitting( int code, const std::shared_ptr<CSolution> &solution );
public:
    CCommThread( const CCommThread& ) = delete; // Copy prohibited
    CCommThread( CCommThread&& ) = delete; // Move prohibited
    void operator=( const CCommThread& ) = delete; // Assignment prohibited
    CCommThread& operator=( CCommThread&& ) = delete; // Move assignment prohibited
    static std::shared_ptr<CCommThread> GetInstance();
    std::vector<std::tuple<std::string, std::string>> const & GetMiningNodes();
    void AddSolution( const std::shared_ptr<CSolution>& solution );
    std::time_t RequestTimestamp();
    std::shared_ptr<CTarget> GetTarget( const char prev_lb_hash[32] );
    void SubmitSolution( const std::shared_ptr<CSolution> &solution, std::shared_ptr<CTarget> &target );
    void Communicate();
};
#endif // __NOSO2M_COMM_HPP__
