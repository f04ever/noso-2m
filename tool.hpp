#ifndef __NOSO2M_TOOL_HPP__
#define __NOSO2M_TOOL_HPP__
#include <string>
#include <vector>

class CTools {
public:
    static int ShowPoolInformation( std::vector<std::tuple<std::string, std::string, std::string>> const & mining_pools );
    static int ShowThreadHashrates( std::vector<std::tuple<std::uint32_t, double>> const & thread_hashrates );
};
#endif // __NOSO2M_TOOL_HPP__
