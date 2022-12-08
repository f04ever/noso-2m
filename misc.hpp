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

void process_options( cxxopts::ParseResult const & parsed_options );

void awaiting_threads_notify( );
void awaiting_threads_wait( std::mutex & mutex_wait, bool ( * wake_up )() );
void awaiting_threads_wait_for( int sec, std::mutex & mutex_wait, bool ( * wake_up )() );

#endif // __NOSO2M_MISC_HPP__

