#ifndef __NOSO2M_MISC_HPP__
#define __NOSO2M_MISC_HPP__

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <tuple>
#include <vector>
#include <string>

#include "cxxopts.hpp"

bool is_valid_address( std::string const & address );
bool is_valid_minerid( std::uint32_t minerid );
bool is_valid_threads( std::uint32_t count );

std::vector<std::tuple<std::string, std::string, std::string>> parse_pools_argv( std::string const & poolstr );

struct mining_options_t {
    int solo;
    int threads;
    long minerid;
    std::string address;
    std::string pools;
    std::string filename;
};

void process_options( cxxopts::ParseResult const & parsed_options );
void process_arg_options( cxxopts::ParseResult const & parsed_options );
void process_cfg_options( cxxopts::ParseResult const & parsed_options );

#endif // __NOSO2M_MISC_HPP__

