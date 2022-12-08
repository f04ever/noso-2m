#ifndef __NOSO2M_MISC_HPP__
#define __NOSO2M_MISC_HPP__

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <tuple>
#include <vector>
#include <string>

#include "cxxopts.hpp"

#include "noso-2m.hpp"

bool is_valid_address( std::string const & address );
bool is_valid_threads( std::uint32_t count );

typedef std::tuple<std::string, std::string, std::string> pool_specs_t;

std::vector<pool_specs_t> parse_pools_argv( std::string const & poolstr );

struct mining_options_t {
    int shares;
    int threads;
    std::string address;
    std::string pools;
    std::string filename;
};

void process_options( cxxopts::ParseResult const & parsed_options );
void process_arg_options( cxxopts::ParseResult const & parsed_options );
void process_cfg_options( cxxopts::ParseResult const & parsed_options );

void awaiting_threads_notify( );
void awaiting_threads_wait( std::mutex & mutex_wait, bool ( * wake_up )() );
void awaiting_threads_wait_for( int sec, std::mutex & mutex_wait, bool ( * wake_up )() );

#endif // __NOSO2M_MISC_HPP__

