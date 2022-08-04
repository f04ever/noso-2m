#ifndef __NOSO2M_TOOL_HPP__
#define __NOSO2M_TOOL_HPP__
#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <string>
#include <vector>

class CTools {
public:
    static int ShowNodeInformation( std::vector<std::tuple<std::string, std::string>> const & mining_nodes );
    static int ShowPoolInformation( std::vector<std::tuple<std::string, std::string, std::string>> const & mining_pools );
    static int ShowThreadHashrates( std::vector<std::tuple<std::uint32_t, double>> const & thread_hashrates );
};
#endif // __NOSO2M_TOOL_HPP__
