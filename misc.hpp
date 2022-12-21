#ifndef __NOSO2M_MISC_HPP__
#define __NOSO2M_MISC_HPP__

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <map>
#include <mutex>
#include <thread>

#include "cxxopts.hpp"

#include "noso-2m.hpp"

void process_options( cxxopts::ParseResult const & parsed_options );


struct awaiting_threads_t {
    std::mutex mutex;
    std::unordered_map<std::size_t, std::shared_ptr<std::condition_variable>> awaits {};
};

bool awaiting_threads_handle(
        std::shared_ptr<std::condition_variable> const & condv,
        std::thread::id thread_id, awaiting_threads_t & awaiting_threads );
bool awaiting_threads_release(
        std::thread::id thread_id, awaiting_threads_t & awaiting_threads );
void awaiting_threads_notify( awaiting_threads_t & awaiting_threads );
void awaiting_threads_wait(
        std::thread::id thread_id, awaiting_threads_t & awaiting_threads,
        bool ( * awake )() );
void awaiting_threads_wait_for( double seconds,
        std::thread::id thread_id, awaiting_threads_t & awaiting_threads,
        bool ( * awake )() );

#endif // __NOSO2M_MISC_HPP__

