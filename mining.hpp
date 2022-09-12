#ifndef __NOSO2M__MINING_HPP__
#define __NOSO2M__MINING_HPP__

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <atomic>
#include <cassert>
#include <condition_variable>

#include "hashing.hpp"

struct CSolution {
    std::uint32_t blck;
    std::string base;
    std::string hash;
    std::string diff;
    CSolution( std::uint32_t blck_, const char base_[19], const char hash_[33], const char diff_[33] )
        :   blck { blck_ }, base { base_ }, hash { hash_ }, diff { diff_ } {
        assert( std::strlen( base_ ) == 18 && std::strlen( hash_ ) == 32
               && ( std::strlen( diff_ ) == 0 || std::strlen( diff_ ) == 32 ) );
    }
};

struct CSolsSetCompare {
    bool operator()( const std::shared_ptr<CSolution>& lhs, const std::shared_ptr<CSolution>& rhs ) const {
        return  lhs->blck > rhs->blck ? true :
                lhs->blck < rhs->blck ? false :
                lhs->diff < rhs->diff ? true : false;
    }
};

struct CTarget {
    std::string prefix;
    std::string address;
    std::uint32_t blck_no;
    std::string lb_hash;
    std::string mn_diff;
    CTarget( std::uint32_t blck_no, const std::string &lb_hash, const std::string &mn_diff )
        :   blck_no { blck_no }, lb_hash { lb_hash }, mn_diff { mn_diff } {
        // prefix = "";
        // address = "";
    }
    virtual ~CTarget() = default;
};

struct CNodeTarget : public CTarget {
    std::time_t lb_time;
    std::string lb_addr;
    CNodeTarget( std::uint32_t blck_no, const std::string &lb_hash, const std::string &mn_diff,
              std::time_t lb_time, const std::string &lb_addr )
        :   CTarget( blck_no, lb_hash, mn_diff ), lb_time { lb_time }, lb_addr { lb_addr } {
    }
};

struct CPoolTarget : public CTarget {
    std::uint64_t till_balance;
    std::uint32_t till_payment;
    std::uint64_t pool_hashrate;
    std::uint32_t payment_block;
    std::uint64_t payment_amount;
    std::string payment_order_id;
    std::uint64_t mnet_hashrate;
    std::string pool_name;
    CPoolTarget( std::uint32_t blck_no, const std::string &lb_hash, const std::string &mn_diff,
                const std::string &prefix, const std::string &address, std::uint64_t till_balance,
                std::uint32_t till_payment, std::uint64_t pool_hashrate, std::uint32_t payment_block,
                std::uint64_t payment_amount, const std::string &payment_order_id, std::uint64_t mnet_hashrate,
                const std::string& pool_name )
        :   CTarget( blck_no, lb_hash, mn_diff ), till_balance { till_balance }, till_payment { till_payment },
            pool_hashrate { pool_hashrate }, payment_block { payment_block }, payment_amount { payment_amount },
            payment_order_id { payment_order_id }, mnet_hashrate { mnet_hashrate }, pool_name { pool_name } {
        this->prefix = prefix;
        this->address = address;
    }
};

class CMineThread {
public:
    std::uint32_t const m_miner_id;
    std::uint32_t const m_thread_id;
    std::atomic<short> m_exited { 0 };
protected:
    CNosoHasher m_hasher;
    char m_address[32];
    char m_prefix[10];
    std::uint32_t m_blck_no { 0 };
    char m_lb_hash[33];
    char m_mn_diff[33];
    std::uint32_t m_computed_hashes_count { 0 };
    double m_block_mining_duration { 0. };
    mutable std::mutex m_mutex_blck_no;
    mutable std::condition_variable m_condv_blck_no;
    mutable std::mutex m_mutex_summary;
    mutable std::condition_variable m_condv_summary;
public:
    CMineThread( std::uint32_t miner_id, std::uint32_t thread_id );
    virtual ~CMineThread() = default;
    void CleanupSyncState();
    void WaitTarget();
    void DoneTarget();
    void NewTarget( const std::shared_ptr<CTarget> &target );
    void SetBlockSummary( std::uint32_t hashes_count, double duration );
    std::tuple<std::uint32_t, double> GetBlockSummary();
    virtual void Mine( void ( * NewSolFunc )( const std::shared_ptr<CSolution>& ) );
};

#endif // __NOSO2M__MINING_HPP__
