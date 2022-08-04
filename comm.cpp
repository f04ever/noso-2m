#include <regex>
#include <thread>
#include <iomanip>
#include <cstring>
#include <algorithm>
#include "inet.hpp"
#include "comm.hpp"
#include "util.hpp"
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

inline
CNodeStatus::CNodeStatus( const char *ns_line ) {
    assert( ns_line != nullptr && std::strlen( ns_line ) > 0 );
    std::string status { ns_line };
    status.erase( status.length() - 2 ); // remove the carriage return and new line charaters
    size_t p_pos = -1, c_pos = -1;
    //NODESTATUS 1{Peers} 2{LastBlock} 3{Pendings} 4{Delta} 5{headers} 6{version} 7{UTCTime} 8{MNsHash} 9{MNscount}
    //           10{LasBlockHash} 11{BestHashDiff} 12{LastBlockTimeEnd} 13{LBMiner} 14{ChecksCount} 15{LastBlockPoW}
    //           16{LastBlockDiff}
    // 0{NODESTATUS}
    next_status_token( ' ', p_pos, c_pos, status );
    // std::string nodestatus = extract_status_token( p_pos, c_pos, status );
    // 1{peer}
    next_status_token( ' ', p_pos, c_pos, status );
    // this->peer = std::stoul( extract_status_token( p_pos, c_pos, status ) );
    // 2{blck}
    next_status_token( ' ', p_pos, c_pos, status );
    this->blck_no = std::stoul( extract_status_token( p_pos, c_pos, status ) );
    // 3{pending}
    next_status_token( ' ', p_pos, c_pos, status );
    // this->pending = std::stoul( extract_status_token( p_pos, c_pos, status ) );
    // 4{delta}
    next_status_token( ' ', p_pos, c_pos, status );
    // this->delta = std::stoul( extract_status_token( p_pos, c_pos, status ) );
    // 5{header/branch}
    next_status_token( ' ', p_pos, c_pos, status );
    // this->branch = extract_status_token( p_pos, c_pos, status );
    // 6{version}
    next_status_token( ' ', p_pos, c_pos, status );
    // this->version = extract_status_token( p_pos, c_pos, status );
    // 7{utctime}
    next_status_token( ' ', p_pos, c_pos, status );
    // this->utctime = std::stoul( extract_status_token( p_pos, c_pos, status ) );
    // 8{mn_hash}
    next_status_token( ' ', p_pos, c_pos, status );
    // this->mn_hash = extract_status_token( p_pos, c_pos, status );
    // 9{mn_count}
    next_status_token( ' ', p_pos, c_pos, status );
    // this->mn_count = std::stoul( extract_status_token( p_pos, c_pos, status ) );
    // 10{lb_hash}
    next_status_token( ' ', p_pos, c_pos, status );
    this->lb_hash = extract_status_token( p_pos, c_pos, status );
    if ( this->lb_hash.length() != 32 ) throw std::out_of_range( "Wrong receiving lb_hash" );
    // 11{bh_diff/mn_diff}
    next_status_token( ' ', p_pos, c_pos, status );
    this->mn_diff = extract_status_token( p_pos, c_pos, status );
    if ( this->mn_diff.length() != 32 ) throw std::out_of_range( "Wrong receiving mn_diff" );
    // 12{lb_time}
    next_status_token( ' ', p_pos, c_pos, status );
    this->lb_time = std::stoul( extract_status_token( p_pos, c_pos, status ) );
    // 13{lb_addr}
    next_status_token( ' ', p_pos, c_pos, status );
    this->lb_addr = extract_status_token( p_pos, c_pos, status );
    if ( this->lb_addr.length() != 30 && this->lb_addr.length() != 31 ) throw std::out_of_range( "Wrong receiving lb_addr" );
    // 14{check_count}
    // next_status_token( ' ', p_pos, c_pos, status );
    // this->check_count = std::stoul( extract_status_token( p_pos, c_pos, status ) );
    // 15{lb_pows}
    // next_status_token( ' ', p_pos, c_pos, status );
    // this->lb_pows = std::stoull( extract_status_token( p_pos, c_pos, status ) );
    // 16{lb_diff}
    // next_status_token( ' ', p_pos, c_pos, status );
    // this->lb_diff = extract_status_token( p_pos, c_pos, status );
    // if ( this->lb_diff.length() != 32 ) throw std::out_of_range( "Wrong receiving lb_diff" );
}

CPoolInfo::CPoolInfo( const char *pi ) {
    assert( pi != nullptr && std::strlen( pi ) > 0 );
    std::string status { pi };
    status.erase( status.length() - 2 ); // remove the carriage return and new line charaters
    size_t p_pos = -1, c_pos = -1;
    next_status_token( ' ', p_pos, c_pos, status );
    this->pool_miners = std::stoul( extract_status_token( p_pos, c_pos, status ) );
    next_status_token( ' ', p_pos, c_pos, status );
    this->pool_hashrate = std::stoull( extract_status_token( p_pos, c_pos, status ) );
    next_status_token( ' ', p_pos, c_pos, status );
    this->pool_fee = std::stoul( extract_status_token( p_pos, c_pos, status ) );
}

inline
CPoolStatus::CPoolStatus( const char *ps_line ) {
    assert( ps_line != nullptr && std::strlen( ps_line ) > 0 );
    std::string status { ps_line };
    status.erase( status.length() - 2 ); // remove the carriage return and new line charaters
    size_t p_pos = -1, c_pos = -1;
    // {0}OK 1{MinerPrefix} 2{MinerAddress} 3{PoolMinDiff} 4{LBHash} 5{LBNumber}
    // 6{MinerBalance} 7{BlocksTillPayment} 8{LastPayInfo} 9{LastBlockPoolHashrate}
    // 10{MainNetHashrate} 11{PoolFee}
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
extern std::vector<std::tuple<std::string, std::string>> const g_default_nodes;
extern std::vector<std::tuple<std::string, std::string, std::string>> g_mining_pools;
extern std::vector<std::tuple<std::uint32_t, double>> g_last_block_thread_hashrates;
extern std::vector<std::shared_ptr<CMineThread>> g_mine_objects;
extern std::vector<std::thread> g_mine_threads;
extern std::uint32_t g_mined_block_count;
extern bool g_still_running;
extern bool g_solo_mining;

CCommThread::CCommThread()
    :    m_mining_pools { g_mining_pools }, m_mining_pools_id { 0 } {
    this->UpdateMiningNodesInSoloModeIfNeed();
}

inline
std::vector<std::tuple<std::string, std::string>> const & CCommThread::GetDefaultNodes() {
    // TODO: load default nodes from mainnet.cfg when it is implemented
    return g_default_nodes;
}

inline
bool CCommThread::SaveHintNodes( std::vector<std::tuple<std::string, std::string>> const &nodes ) {
    std::ofstream nodes_ostream( "hint_nodes.txt" );
    if( !nodes_ostream.good() ) {
        std::string msg { "Failed to save hint nodes to file 'hint_nodes.txt'!" };
        NOSO_LOG_ERROR << msg << std::endl;
        NOSO_TUI_OutputHistPad( msg.c_str() );
        NOSO_TUI_OutputHistWin();
        return false;
    }
    for( auto node : nodes ) {
        nodes_ostream << std::get<0>( node ) << ":" << std::get<1>( node ) << std::endl;
    }
    std::string msg { "Saved hint nodes to file 'hint_nodes.txt'" };
    NOSO_LOG_INFO << msg << std::endl;
    return true;
}

inline
std::vector<std::tuple<std::string, std::string>> CCommThread::LoadHintNodes() {
    std::vector<std::tuple<std::string, std::string>> nodes;
    std::ifstream nodes_istream( "hint_nodes.txt" );
    if( !nodes_istream.good() ) {
        std::string msg { "Hint nodes file 'hint_nodes.txt' not found!" };
        NOSO_LOG_WARN << msg << std::endl;
        NOSO_TUI_OutputHistPad( msg.c_str() );
        msg = "Trying with default hint nodes.";
        NOSO_LOG_INFO << msg << std::endl;
        NOSO_TUI_OutputHistPad( msg.c_str() );
        NOSO_TUI_OutputHistWin();
    } else {
        std::string msg { "Load hint nodes from file 'hint_nodes.txt'" };
        NOSO_LOG_INFO << msg << std::endl;
        std::regex const re { // IP-ADDRESS:PORT
                "^"
                "("
                    "(25[0-5]|2[0-4][0-9]|1[0-9]{2}|[1-9][0-9]|[0-9])\\."
                    "(25[0-5]|2[0-4][0-9]|1[0-9]{2}|[1-9][0-9]|[0-9])\\."
                    "(25[0-5]|2[0-4][0-9]|1[0-9]{2}|[1-9][0-9]|[0-9])\\."
                    "(25[0-5]|2[0-4][0-9]|1[0-9]{2}|[1-9][0-9]|[0-9])"
                ")"
                "\\:"
                "("
                    "6553[0-5]|655[0-2][0-9]|65[0-4][0-9]{2}|6[0-4][0-9]{3}|[1-5][0-9]{4}|[0-5]{0,5}|[0-9]{1,4}"
                ")"
                "$" };
        std::smatch sm;
        for_each(
                std::istream_iterator<std::string>{ nodes_istream },
                std::istream_iterator<std::string>{},
                [&]( auto const &line ) {
                    if( std::regex_match( line, sm, re ) ) {
                        std::string const host { sm[1] };
                        std::string const port { sm[6] };
                        nodes.push_back( { host, port } );
                    }
                } );
    }
    return nodes;
}

inline
std::vector<std::tuple<std::string, std::string>> CCommThread::GetValidators(
        std::vector<std::tuple<std::string, std::string>> const &hints ) {
    std::vector<std::tuple<std::string, std::string>> nodes;
    std::size_t const NSLMNS_INET_BUFFER_SIZE { 32'000 }; // appro. max 500 * 64 bytes per MN
    char inet_buffer[NSLMNS_INET_BUFFER_SIZE];
    for (   auto itor = hints.begin();
            itor != hints.end();
            itor = std::next( itor ) ) {
        CNodeInet inet { std::get<0>( *itor ), std::get<1>( *itor ), DEFAULT_NODE_INET_TIMEOSEC };
        int rsize { inet.RequestMNList( inet_buffer, NSLMNS_INET_BUFFER_SIZE ) };
        if ( rsize <= 0 ) {
            NOSO_LOG_DEBUG
                << "sync_nodes <- CNodeInet::RequestMNList Poor connecting with node "
                << inet.m_host << ":" << inet.m_port
                << std::endl;
            NOSO_TUI_OutputStatPad( "A poor connectivity while connecting with a node!" );
        } else {
            std::regex const re {
                    "("
                        "(25[0-5]|2[0-4][0-9]|1[0-9]{2}|[1-9][0-9]|[0-9])\\."
                        "(25[0-5]|2[0-4][0-9]|1[0-9]{2}|[1-9][0-9]|[0-9])\\."
                        "(25[0-5]|2[0-4][0-9]|1[0-9]{2}|[1-9][0-9]|[0-9])\\."
                        "(25[0-5]|2[0-4][0-9]|1[0-9]{2}|[1-9][0-9]|[0-9])"
                    ")"
                    ";"
                    "("
                        "6553[0-5]|655[0-2][0-9]|65[0-4][0-9]{2}|6[0-4][0-9]{3}|[1-5][0-9]{4}|[0-5]{0,5}|[0-9]{1,4}"
                    ")"
                    "\\:"
                    "N[0-9A-Za-z]{29,30}"
                    "\\:"
                    "([0-9]{1,})" };
            std::string mn_list { inet_buffer };
            std::vector<std::tuple<std::string, std::string, std::uint32_t>> validators;
            try {
                std::for_each(
                        std::sregex_iterator { mn_list.begin(), mn_list.end(), re },
                        std::sregex_iterator {},
                        [&]( auto const &sm0 ) {
                            std::string host { sm0[1].str() };
                            std::string port { sm0[6].str() };
                            std::string ages { sm0[7].str() };
                            validators.push_back( { host, port, std::stol( ages ) } );
                        } );
            } catch ( const std::exception &e ) {
                NOSO_LOG_DEBUG
                    << "CCommThread::GetValidators Unrecognised response from node "
                    << inet.m_host << ":" << inet.m_port
                    << "[" << inet_buffer << "](size=" << rsize << ")" << e.what()
                    << std::endl;
                NOSO_TUI_OutputStatPad( "An unrecognised response from a node!" );
                continue;
            }
            std::sort( validators.begin(), validators.end(),
                    []( auto const &a, auto const &b ){
                        return std::get<2>( a ) > std::get<2>( b );
                    } );
            std::size_t validators_count { validators.size() / 10 + 3 };
            auto const &validators_begin = validators.begin();
            std::for_each( validators_begin, validators_begin + validators_count,
                    [&]( auto const &a ) {
                        nodes.push_back( { std::get<0>( a ), std::get<1>( a ) } );
                    } );
            if ( nodes.size() > 0 ) break;
        }
        NOSO_TUI_OutputStatWin();
    }
    return nodes;
}

inline
void CCommThread::UpdateMiningNodesInSoloModeIfNeed() {
    if( !g_solo_mining ) return;
    if( m_mining_nodes.size() < DEFAULT_CONSENSUS_NODES_COUNT ) {
        std::vector<std::tuple<std::string, std::string>> hints;
        if ( m_mining_nodes.size() <= 0 ) {
            hints = CCommThread::LoadHintNodes();
            if ( hints.size() <= 0 ) hints = this->GetDefaultNodes();
            if ( hints.size() <= 0 ) hints.push_back( { "localhost", "8080" } );
        } else {
            hints = m_mining_nodes;
        }
        std::shuffle( m_mining_nodes.begin(), m_mining_nodes.end(), m_random_engine );
        m_mining_nodes = CCommThread::GetValidators( hints );
        std::stringstream msg;
        msg << "Do mining randomly on nodes:";
        NOSO_LOG_INFO << msg.str() << std::endl;
        NOSO_TUI_OutputHistPad( msg.str().c_str() );
        for ( auto node : m_mining_nodes ) {
            msg.str( std::string() );
            msg << "- " << std::get<0>( node ) << ":" << std::get<1>( node );
            NOSO_LOG_INFO << msg.str() << std::endl;
            NOSO_TUI_OutputHistPad( msg.str().c_str() );
        }
        NOSO_TUI_OutputHistWin();
        CCommThread::SaveHintNodes( m_mining_nodes );
    }
}

std::shared_ptr<CCommThread> CCommThread::GetInstance() {
    static std::shared_ptr<CCommThread> singleton { new CCommThread() };
    return singleton;
}

inline
void CCommThread::CloseMiningBlock( const std::chrono::duration<double>& elapsed_blck ) {
    m_last_block_hashes_count = 0;
    m_last_block_elapsed_secs = elapsed_blck.count();
    g_last_block_thread_hashrates.clear();
    for_each( g_mine_objects.begin(), g_mine_objects.end(), [&]( auto const & object ) {
                 auto block_summary = object->GetBlockSummary();
                 std::uint64_t thread_hashes { std::get<0>( block_summary ) };
                 double thread_duration { std::get<1>( block_summary ) };
                 double thread_hashrate { thread_hashes / thread_duration };
                 g_last_block_thread_hashrates.push_back( std::make_tuple( object->m_thread_id, thread_hashrate ) );
                 m_last_block_hashes_count += thread_hashes;
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
    if ( m_last_block_hashrate > 0 ) {
        NOSO_LOG_DEBUG
            << " Computed " << m_last_block_hashes_count << " hashes within "
            << std::fixed << std::setprecision( 2 ) << m_last_block_elapsed_secs / 60 << " minutes"
            << std::endl;
        if ( g_solo_mining ) {
            std::shared_ptr<CNodeTarget> node_target { std::dynamic_pointer_cast<CNodeTarget>( target ) };
            std::snprintf( msg, 100, " Hashrate(Miner/Pool/Mnet) %5.01f%c /    n/a /    n/a",
                        hashrate_pretty_value( m_last_block_hashrate ),      hashrate_pretty_unit( m_last_block_hashrate ) );
            NOSO_LOG_INFO << msg << std::endl;
            NOSO_TUI_OutputHistPad( msg );
            if ( node_target->lb_addr == g_miner_address ) {
                g_mined_block_count++;
                std::snprintf( msg, 100, " Yay! win this block %06u", node_target->blck_no + 1 );
                NOSO_LOG_INFO << msg << std::endl;
                NOSO_TUI_OutputHistPad( msg );
            }
            if ( g_mined_block_count > 0 ) {
                std::snprintf( msg, 100, " Won total %3u blocks", g_mined_block_count );
                NOSO_LOG_INFO << msg << std::endl;
                NOSO_TUI_OutputHistPad( msg );
            }
        } else {
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
        }
        NOSO_TUI_OutputHistWin();
    }
    std::snprintf( msg, 100, "---------------------------------------------------" );
    NOSO_LOG_INFO << msg << std::endl;
    std::snprintf( msg, 100, "BLOCK %06u       %-32s", target->blck_no + 1, target->lb_hash.substr( 0, 32 ).c_str() );
    NOSO_LOG_INFO << msg << std::endl;
    NOSO_TUI_OutputActiWinBlockNum( target->blck_no + 1 );
    NOSO_TUI_OutputActiWinLastHash( target->lb_hash.substr( 0, 32 ) );
    if ( g_solo_mining ) {
        std::shared_ptr<CNodeTarget> node_target { std::dynamic_pointer_cast<CNodeTarget>( target ) };
        std::snprintf( msg, 100, " Solo %-12s %-32s",
                        std::string( "Mainnet" ).substr( 0, 12 ).c_str(),
                        node_target->mn_diff.substr( 0, 32).c_str() );
        NOSO_LOG_INFO << msg << std::endl;
    } else {
        std::shared_ptr<CPoolTarget> pool_target { std::dynamic_pointer_cast<CPoolTarget>( target ) };
        std::snprintf( msg, 100, " Pool %-12s %-32s",
                        pool_target->pool_name.substr( 0, 12 ).c_str(),
                        pool_target->mn_diff.substr( 0, 32).c_str() );
        NOSO_LOG_INFO << msg << std::endl;
    }
    NOSO_TUI_OutputActiWinMiningMode( g_solo_mining );
    NOSO_TUI_OutputActiWinMiningDiff( target->mn_diff.substr( 0, 32 ) );
    NOSO_TUI_OutputActiWinAcceptedSol( m_accepted_solutions_count );
    NOSO_TUI_OutputActiWinRejectedSol( m_rejected_solutions_count );
    NOSO_TUI_OutputActiWinFailuredSol( m_failured_solutions_count );
    if ( g_solo_mining ) {
        std::shared_ptr<CNodeTarget> node_target { std::dynamic_pointer_cast<CNodeTarget>( target ) };
        NOSO_TUI_OutputActiWinMiningSource( std::string( "Mainnet" ).substr( 0, 12 ) );
    } else {
        std::shared_ptr<CPoolTarget> pool_target { std::dynamic_pointer_cast<CPoolTarget>( target ) };
        NOSO_TUI_OutputActiWinMiningSource( pool_target->pool_name.substr( 0, 12 ) );
        NOSO_TUI_OutputActiWinTillBalance( pool_target->till_balance );
        NOSO_TUI_OutputActiWinTillPayment( pool_target->till_payment );
    }
    NOSO_TUI_OutputStatPad( "Press Ctrl+C to stop!" );
    NOSO_TUI_OutputStatWin();
}

inline
void CCommThread::_ReportTargetSummary( const std::shared_ptr<CTarget>& target ) {
    char msg[100];
    std::snprintf( msg, 100, "---------------------------------------------------" );
    NOSO_TUI_OutputHistPad( msg );
    std::snprintf( msg, 100, "BLOCK %06u       %-32s", target->blck_no + 1, target->lb_hash.substr( 0, 32 ).c_str() );
    NOSO_TUI_OutputHistPad( msg );
    if ( g_solo_mining ) {
        std::shared_ptr<CNodeTarget> node_target { std::dynamic_pointer_cast<CNodeTarget>( target ) };
        std::snprintf( msg, 100, " Solo %-12s %-32s",
                        std::string( "Mainnet" ).substr( 0, 12 ).c_str(),
                        node_target->mn_diff.substr( 0, 32).c_str() );
        NOSO_TUI_OutputHistPad( msg );
        std::snprintf( msg, 100, " Sent %5u / %4u / %3u",
                        m_accepted_solutions_count, m_rejected_solutions_count, m_failured_solutions_count );
        NOSO_LOG_INFO << msg << std::endl;
        NOSO_TUI_OutputHistPad( msg );
    } else {
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
    }
    NOSO_TUI_OutputHistWin();
    NOSO_TUI_OutputActiWinDefault();
};

inline
void CCommThread::_ReportErrorSubmitting( int code, const std::shared_ptr<CSolution> &solution ) {
    char msg[100];
    if      ( code == 1 ) {
        NOSO_LOG_ERROR
            << "    ERROR"
            << ")base[" << solution->base
            << "]hash[" << solution->hash
            << "]Wrong block#" << solution->blck << " submitted!"
            << std::endl;
        std::snprintf( msg, 100, "A wrong#%06u submitted!", solution->blck );
        NOSO_TUI_OutputStatPad( msg );
    } else if ( code == 2 ) {
        NOSO_LOG_ERROR
            << "    ERROR"
            << ")base[" << solution->base
            << "]hash[" << solution->hash
            << "]Incorrect timestamp submitted!"
            << std::endl;
        NOSO_TUI_OutputStatPad( "An incorrect timestamp submitted!" );
    } else if ( code == 3 ) {
        NOSO_LOG_ERROR
            << "    ERROR"
            << ")base[" << solution->base
            << "]hash[" << solution->hash
            << "]Invalid address (" << g_miner_address << ") submitted!"
            << std::endl;
        std::snprintf( msg, 100, "An invalid address (%s) submitted!", g_miner_address );
        NOSO_TUI_OutputStatPad( msg );
    } else if ( code == 7 ) {
        NOSO_LOG_ERROR
            << "    ERROR"
            << ")base[" << solution->base
            << "]hash[" << solution->hash
            << "]Wrong hash base submitted!"
            << std::endl;
        NOSO_TUI_OutputStatPad( "A wrong hash base submitted!" );
    } else if ( code == 4 ) {
        NOSO_LOG_ERROR
            << "    ERROR"
            << ")base[" << solution->base
            << "]hash[" << solution->hash
            << "]Duplicate hash submitted!"
            << std::endl;
        NOSO_TUI_OutputStatPad( "A duplicate hash submitted!" );
    }
    NOSO_TUI_OutputStatWin();
}

void CCommThread::AddSolution( const std::shared_ptr<CSolution>& solution ) {
    m_mutex_solutions.lock();
    if ( g_solo_mining ) m_solo_solutions.insert( solution );
    else m_pool_solutions.push_back( solution );
    m_mutex_solutions.unlock();
}

inline
void CCommThread::ClearSolutions() {
    m_mutex_solutions.lock();
    if ( g_solo_mining ) m_solo_solutions.clear();
    else m_pool_solutions.clear();
    m_mutex_solutions.unlock();
}

inline
const std::shared_ptr<CSolution> CCommThread::BestSolution() {
    std::shared_ptr<CSolution> best_solution { nullptr };
    m_mutex_solutions.lock();
    if ( m_solo_solutions.begin() != m_solo_solutions.end() ) {
        auto itor_best_solution = m_solo_solutions.begin();
        best_solution = *itor_best_solution;
        m_solo_solutions.clear();
    }
    m_mutex_solutions.unlock();
    return best_solution;
}

inline
const std::shared_ptr<CSolution> CCommThread::GoodSolution() {
    std::shared_ptr<CSolution> good_solution { nullptr };
    m_mutex_solutions.lock();
    if ( m_pool_solutions.begin() != m_pool_solutions.end() ) {
        good_solution = m_pool_solutions.back();
        m_pool_solutions.pop_back();
    }
    m_mutex_solutions.unlock();
    return good_solution;
}

inline
std::shared_ptr<CSolution> CCommThread::GetSolution() {
    return g_solo_mining ? this->BestSolution() : this->GoodSolution();
}

std::time_t CCommThread::RequestTimestamp() {
    if ( m_mining_nodes.size() <= 0 ) return std::time_t( -1 );
    std::shuffle( m_mining_nodes.begin(), m_mining_nodes.end(), m_random_engine );
    std::time_t ret_time { -1 };
    std::vector<std::vector<std::tuple<std::string, std::string>>::iterator> bad_mining_nodes;
    for (   auto itor = m_mining_nodes.begin();
            g_still_running
                && itor != m_mining_nodes.end();
            itor = std::next( itor ) ) {
        CNodeInet inet { std::get<0>( *itor ), std::get<1>( *itor ), DEFAULT_NODE_INET_TIMEOSEC };
        int rsize { inet.RequestTimestamp( m_inet_buffer, DEFAULT_INET_BUFFER_SIZE ) };
        if ( rsize <= 0 ) {
            bad_mining_nodes.push_back( itor );
            NOSO_LOG_DEBUG
                << "CCommThread::RequestTimestamp Poor connecting with node "
                << inet.m_host << ":" << inet.m_port
                << std::endl;
            NOSO_TUI_OutputStatPad( "A poor connectivity while connecting with a node!" );
        } else {
            try {
                ret_time = std::time_t( std::atol( m_inet_buffer ) );
                break;
            } catch ( const std::exception &e ) {
                bad_mining_nodes.push_back( itor );
                NOSO_LOG_DEBUG
                    << "CCommThread::RequestTimestamp Unrecognised response from node "
                    << inet.m_host << ":" << inet.m_port
                    << "[" << m_inet_buffer << "](size=" << rsize << ")" << e.what()
                    << std::endl;
                NOSO_TUI_OutputStatPad( "An unrecognised response from a node!" );
            }
        }
        NOSO_TUI_OutputStatWin();
    }
    std::for_each( bad_mining_nodes.begin(), bad_mining_nodes.end(),
            [&]( auto const &itor ) { m_mining_nodes.erase( itor ); } );
    return ret_time;
}

inline
std::vector<std::shared_ptr<CNodeStatus>> CCommThread::RequestNodeSources( std::size_t min_nodes_count ) {
    std::vector<std::shared_ptr<CNodeStatus>> sources;
    if ( m_mining_nodes.size() < min_nodes_count ) return sources;
    std::shuffle( m_mining_nodes.begin(), m_mining_nodes.end(), m_random_engine );
    std::size_t nodes_count { 0 };
    std::vector<std::vector<std::tuple<std::string, std::string>>::iterator> bad_mining_nodes;
    for (   auto itor = m_mining_nodes.begin();
            g_still_running
                && nodes_count < min_nodes_count
                && itor != m_mining_nodes.end();
            itor = std::next( itor ) ) {
        CNodeInet inet { std::get<0>( *itor ), std::get<1>( *itor ), DEFAULT_NODE_INET_TIMEOSEC };
        int rsize { inet.RequestSource( m_inet_buffer, DEFAULT_INET_BUFFER_SIZE ) };
        if ( rsize <= 0 ) {
            bad_mining_nodes.push_back( itor );
            NOSO_LOG_DEBUG
                << "CCommThread::RequestNodeSources Poor connecting with node "
                << inet.m_host << ":" << inet.m_port
                << std::endl;
            NOSO_TUI_OutputStatPad( "A poor connectivity while connecting with a node!" );
        } else {
            try {
                sources.push_back( std::make_shared<CNodeStatus>( m_inet_buffer ) );
                nodes_count ++;
            } catch ( const std::exception &e ) {
                bad_mining_nodes.push_back( itor );
                NOSO_LOG_DEBUG
                    << "CCommThread::RequestNodeSources Unrecognised response from node "
                    << inet.m_host << ":" << inet.m_port
                    << "[" << m_inet_buffer << "](size=" << rsize << ")" << e.what()
                    << std::endl;
                NOSO_TUI_OutputStatPad( "An unrecognised response from a node!" );
            }
        }
        NOSO_TUI_OutputStatWin();
    }
    std::for_each( bad_mining_nodes.begin(), bad_mining_nodes.end(),
            [&]( auto const &itor ) { m_mining_nodes.erase( itor ); } );
    return sources;
}

inline
std::shared_ptr<CNodeTarget> CCommThread::GetNodeTargetConsensus() {
    std::vector<std::shared_ptr<CNodeStatus>> status_of_nodes = this->RequestNodeSources( DEFAULT_CONSENSUS_NODES_COUNT );
    if ( status_of_nodes.size() < DEFAULT_CONSENSUS_NODES_COUNT ) return nullptr;
    const auto max_freq = []( const auto &freq ) {
        return std::max_element(
            std::begin( freq ), std::end( freq ),
            [] ( const auto &p1, const auto &p2 ) {
                return p1.second < p2.second; } )->first;
    };
    m_freq_blck_no.clear();
    m_freq_lb_hash.clear();
    m_freq_mn_diff.clear();
    m_freq_lb_time.clear();
    m_freq_lb_addr.clear();
    for( auto ns : status_of_nodes ) {
        ++m_freq_blck_no [ns->blck_no];
        ++m_freq_lb_hash [ns->lb_hash];
        ++m_freq_mn_diff [ns->mn_diff];
        ++m_freq_lb_time [ns->lb_time];
        ++m_freq_lb_addr [ns->lb_addr];
    }
    std::shared_ptr<CNodeTarget> target {
        std::make_shared<CNodeTarget>(
            max_freq( m_freq_blck_no ),
            max_freq( m_freq_lb_hash ),
            max_freq( m_freq_mn_diff ),
            max_freq( m_freq_lb_time ),
            max_freq( m_freq_lb_addr ) ) };
        target->prefix = "";
        target->address = g_miner_address;
    return target;
}

inline
std::shared_ptr<CPoolTarget> CCommThread::RequestPoolTarget( const char address[32] ) {
    assert( std::strlen( address ) == 30 || std::strlen( address ) == 31 );
    auto pool { m_mining_pools[m_mining_pools_id] };
    CPoolInet inet { std::get<0>( pool ), std::get<1>( pool ), std::get<2>( pool ), DEFAULT_POOL_INET_TIMEOSEC };
    int rsize { inet.RequestSource( address, m_inet_buffer, DEFAULT_INET_BUFFER_SIZE ) };
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
                std::get<0>( pool )
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
std::shared_ptr<CPoolTarget> CCommThread::GetPoolTargetFailover() {
    static const int max_tries_count { 5 };
    bool first_fail { true };
    int tries_count { 1 };
    std::shared_ptr<CPoolTarget> pool_target = this->RequestPoolTarget( g_miner_address );
    while ( g_still_running && pool_target == nullptr ) {
        std::uint32_t old_inet_pools_id { m_mining_pools_id };
        auto old_pool { m_mining_pools[old_inet_pools_id] };
        NOSO_LOG_DEBUG
            << "WAIT TARGET FROM POOL "
            << std::get<0>( old_pool ) << "(" << std::get<1>( old_pool ) << ":" << std::get<2>( old_pool ) << ")"
            << " (Retries " << tries_count << "/" << max_tries_count << ")"
            << std::endl;
        NOSO_TUI_OutputStatPad( "Waiting target from pool..." );
        NOSO_TUI_OutputStatWin();
        if ( tries_count >= max_tries_count ) {
            tries_count = 0;
            if ( m_mining_pools.size() > 1 ) {
                if ( first_fail ) {
                    m_mining_pools_id = 0;
                    if ( m_mining_pools_id == old_inet_pools_id ) ++m_mining_pools_id;
                    first_fail = false;
                } else {
                    ++m_mining_pools_id;
                    if ( m_mining_pools_id >= m_mining_pools.size() ) m_mining_pools_id = 0;
                }
                auto new_pool { m_mining_pools[m_mining_pools_id] };
                NOSO_LOG_DEBUG
                    << "POOL FAILOVER FROM "
                    << std::get<0>( old_pool ) << "(" << std::get<1>( old_pool ) << ":" << std::get<2>( old_pool ) << ")"
                    << " TO "
                    << std::get<0>( new_pool ) << "(" << std::get<1>( new_pool ) << ":" << std::get<2>( new_pool ) << ")"
                    << std::endl;
                NOSO_TUI_OutputStatPad( "Failover to new pool!" );
                NOSO_TUI_OutputStatWin();
            } else {
                NOSO_LOG_DEBUG
                    << "RE-ENTER POOL "
                    << std::get<0>( old_pool ) << "(" << std::get<1>( old_pool ) << ":" << std::get<2>( old_pool ) << ")"
                    << " AS NO POOL CONFIGURED FOR FAILOVER"
                    << std::endl;
                NOSO_TUI_OutputStatPad( "No pool for failover. Re-attempt current pool." );
                NOSO_TUI_OutputStatWin();
            }
        }
        std::this_thread::sleep_for( std::chrono::milliseconds( static_cast<int>( tries_count * 1'000 * DEFAULT_INET_CIRCLE_SECONDS ) ) );
        pool_target = this->RequestPoolTarget( g_miner_address );
        ++tries_count;
    }
    return pool_target;
}

inline
std::shared_ptr<CTarget> CCommThread::GetTarget( const char prev_lb_hash[32] ) {
    assert( std::strlen( prev_lb_hash ) == 32 );
    std::shared_ptr<CTarget> target { nullptr };
    if ( g_solo_mining ) target = this->GetNodeTargetConsensus();
    else target = this->GetPoolTargetFailover();
    while ( g_still_running && ( target == nullptr || ( target !=nullptr && target->lb_hash == prev_lb_hash ) ) ) {
        std::this_thread::sleep_for( std::chrono::milliseconds( static_cast<int>( 1'000 * DEFAULT_INET_CIRCLE_SECONDS ) ) );
        if ( g_solo_mining ) target = this->GetNodeTargetConsensus();
        else target = this->GetPoolTargetFailover();
    }
    return target;
}

inline
int CCommThread::SubmitSoloSolution( std::uint32_t blck, const char base[19],
                                    const char address[32], char new_mn_diff[33] ) {
    assert( std::strlen( base ) == 18
            && ( std::strlen( address ) == 30 || std::strlen( address ) == 31 ) );
    if ( m_mining_nodes.size() <= 0 ) return ( -1 );
    std::shuffle( m_mining_nodes.begin(), m_mining_nodes.end(), m_random_engine );
    int ret_code { -1 };
    std::vector<std::vector<std::tuple<std::string, std::string>>::iterator> bad_mining_nodes;
    for (   auto itor = m_mining_nodes.begin();
            g_still_running
                && itor != m_mining_nodes.end();
            itor = std::next( itor ) ) {
        CNodeInet inet { std::get<0>( *itor ), std::get<1>( *itor ), DEFAULT_NODE_INET_TIMEOSEC };
        int rsize { inet.SubmitSolution( blck, base, address, m_inet_buffer, DEFAULT_INET_BUFFER_SIZE ) };
        if ( rsize <= 0 ) {
            bad_mining_nodes.push_back( itor );
            NOSO_LOG_DEBUG
                << "CCommThread::SubmitSoloSolution Poor connecting with node "
                << inet.m_host << ":" << inet.m_port
                << std::endl;
            NOSO_TUI_OutputStatPad( "A poor connectivity while connecting with a node!" );
        } else {
            // try {
            // m_inet_buffer ~ len=(70+2)~[True Diff(32) Hash(32)\r\n] OR len=(40+2)~[False Diff(32) Code#(1)\r\n]
            if      ( rsize >= 42
                        && std::strncmp( m_inet_buffer, "False ", 6 ) == 0
                        && m_inet_buffer[38] == ' '
                        && '1' <= m_inet_buffer[39] && m_inet_buffer[39] <= '7' ) {
                // len=(40+2)~[False Diff(32) Code#(1)\r\n]
                std::strncpy( new_mn_diff, m_inet_buffer + 6, 32 );
                new_mn_diff[32] = '\0';
                ret_code  = m_inet_buffer[39] - '0';
                break;
            }
            else if ( rsize >= 72
                        && std::strncmp( m_inet_buffer, "True ", 5 ) == 0
                        && m_inet_buffer[37] == ' ' ) {
                // len=(70+2)~[True Diff(32) Hash(32)\r\n]
                std::strncpy( new_mn_diff, m_inet_buffer + 5, 32 );
                new_mn_diff[32] = '\0';
                ret_code = 0;
                break;
            }
            // } catch ( const std::exception &e ) {}
            bad_mining_nodes.push_back( itor );
            NOSO_LOG_DEBUG
                << "CCommThread::SubmitSoloSolution Unrecognised response from node "
                << inet.m_host << ":" << inet.m_port
                << "[" << m_inet_buffer << "](size=" << rsize << ")"
                << std::endl;
            NOSO_TUI_OutputStatPad( "An unrecognised response from a node!" );
        }
        NOSO_TUI_OutputStatWin();
    }
    std::for_each( bad_mining_nodes.begin(), bad_mining_nodes.end(),
            [&]( auto const &itor ) { m_mining_nodes.erase( itor ); } );
    return ret_code;
}

inline
int CCommThread::SubmitPoolSolution( std::uint32_t blck_no, const char base[19], const char address[32] ) {
    assert( std::strlen( base ) == 18
            && ( std::strlen( address ) == 30 || std::strlen( address ) == 31 ) );
    static const int max_tries_count { 5 };
    auto pool { m_mining_pools[m_mining_pools_id] };
    CPoolInet inet { std::get<0>( pool ), std::get<1>( pool ), std::get<2>( pool ), DEFAULT_POOL_INET_TIMEOSEC };
    int ret_code { -1 };
    for (   int tries_count = 0;
            g_still_running
                && tries_count < max_tries_count;
            ++tries_count ) {
        int rsize { inet.SubmitSolution( blck_no, base, address, m_inet_buffer, DEFAULT_INET_BUFFER_SIZE ) };
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
                            && std::strncmp( m_inet_buffer, "False ", 6 ) == 0
                            && '1' <= m_inet_buffer[6] && m_inet_buffer[6] <= '7' ) {
                ret_code = m_inet_buffer[6] - '0';
                break;
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
        std::this_thread::sleep_for( std::chrono::milliseconds( static_cast<int>( 1'000 * DEFAULT_INET_CIRCLE_SECONDS ) ) );
    }
    return ret_code;
}

inline
void CCommThread::SubmitSolution( const std::shared_ptr<CSolution> &solution, std::shared_ptr<CTarget> &target ) {
    int code { 0 };
    if ( g_solo_mining ) {
        target->mn_diff = solution->diff;
        char new_mn_diff[33] { NOSO_MAX_DIFF };
        code = this->SubmitSoloSolution( solution->blck, solution->base.c_str(), g_miner_address, new_mn_diff );
        if ( code >= 0 ) {
            if ( target->mn_diff > new_mn_diff ) {
                target->mn_diff = new_mn_diff;
                NOSO_TUI_OutputActiWinMiningDiff( target->mn_diff.substr( 0, 32 ) );
            }
            if ( code == 6 ) {
                if ( solution->diff < new_mn_diff ) this->AddSolution( solution );
                NOSO_LOG_DEBUG
                    << " WAITBLCK"
                    << ")base[" << solution->base
                    << "]hash[" << solution->hash
                    << "]Network building block!"
                    << std::endl;
                NOSO_TUI_OutputStatPad( "A submission while network building block! The solution will be re-submited." );
            }
        }
    } else {
        code = this->SubmitPoolSolution( solution->blck, solution->base.c_str(), g_miner_address );
    }
    if ( code > 0 ) this->_ReportErrorSubmitting( code, solution ); // rest other error codes 1, 2, 3, 4, 7
    char msg[100] { "" };
    if ( code == 0 ) {
        m_accepted_solutions_count ++;
        NOSO_LOG_DEBUG
            << " ACCEPTED"
            << ")base[" << solution->base
            << "]hash[" << solution->hash
            << "]"
            << std::endl;
        NOSO_TUI_OutputActiWinAcceptedSol( m_accepted_solutions_count );
        std::snprintf( msg, 100, "A submission (%u) accepted!", m_accepted_solutions_count );
    } else if ( code > 0) {
        m_rejected_solutions_count ++;
        NOSO_LOG_DEBUG
            << " REJECTED"
            << ")base[" << solution->base
            << "]hash[" << solution->hash
            << "]"
            << std::endl;
        NOSO_TUI_OutputActiWinRejectedSol( m_rejected_solutions_count );
        std::snprintf( msg, 100, "A submission (%u) rejected!", m_rejected_solutions_count );
    } else {
        this->AddSolution( solution );
        m_failured_solutions_count ++;
        NOSO_LOG_DEBUG
            << " FAILURED"
            << ")base[" << solution->base
            << "]hash[" << solution->hash
            << "]Will retry submitting!"
            << std::endl;
        NOSO_TUI_OutputActiWinFailuredSol( m_failured_solutions_count );
        std::snprintf( msg, 100, "A submission (%u) failured!  The solution will be re-submited.", m_failured_solutions_count );
    }
    if ( msg[0] ) NOSO_TUI_OutputStatPad( msg );
    NOSO_TUI_OutputStatWin();
}

void CCommThread::Communicate() {
    long NOSO_BLOCK_AGE_TARGET_SAFE { 10 };
    char prev_lb_hash[33] { NOSO_NUL_HASH };
    auto begin_blck = std::chrono::steady_clock::now();
    while ( g_still_running ) {
        if ( NOSO_BLOCK_AGE < NOSO_BLOCK_AGE_TARGET_SAFE || 585 < NOSO_BLOCK_AGE ) {
            NOSO_TUI_OutputStatPad( "Wait next block..." );
            NOSO_TUI_OutputStatWin();
            do {
                std::this_thread::sleep_for( std::chrono::milliseconds( static_cast<int>( 1'000 * DEFAULT_INET_CIRCLE_SECONDS ) ) );
            } while ( g_still_running && ( NOSO_BLOCK_AGE < NOSO_BLOCK_AGE_TARGET_SAFE || 585 < NOSO_BLOCK_AGE ) );
            if ( !g_still_running ) break;
        }
        NOSO_BLOCK_AGE_TARGET_SAFE = g_solo_mining ? 1 : 6;
        this->UpdateMiningNodesInSoloModeIfNeed();
        std::shared_ptr<CTarget> target = this->GetTarget( prev_lb_hash );
        if ( !g_still_running || target == nullptr ) break;
        std::strcpy( prev_lb_hash, target->lb_hash.c_str() );
        begin_blck = std::chrono::steady_clock::now();
        for ( auto const & mo : g_mine_objects ) mo->NewTarget( target );
        this->_ReportMiningTarget( target );
        while ( g_still_running && NOSO_BLOCK_AGE <= 585 ) {
            auto begin_submit = std::chrono::steady_clock::now();
            if ( NOSO_BLOCK_AGE >= 10 ) {
                std::shared_ptr<CSolution> solution = this->GetSolution();
                if ( solution != nullptr && solution->diff < target->mn_diff )
                    this->SubmitSolution( solution, target );
            }
            this->UpdateMiningNodesInSoloModeIfNeed();
            std::chrono::duration<double> elapsed_submit = std::chrono::steady_clock::now() - begin_submit;
            if ( elapsed_submit.count() < DEFAULT_INET_CIRCLE_SECONDS ) {
                std::this_thread::sleep_for( std::chrono::milliseconds( static_cast<int>( 1'000 * DEFAULT_INET_CIRCLE_SECONDS ) ) );
            }
        }
        this->CloseMiningBlock( std::chrono::steady_clock::now() - begin_blck );
        this->_ReportTargetSummary( target );
        this->ResetMiningBlock();
    } // END while ( g_still_running ) {
    for ( auto &obj : g_mine_objects ) obj->CleanupSyncState();
    for ( auto &thr : g_mine_threads ) thr.join();
}
