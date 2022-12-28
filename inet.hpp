#ifndef __NOSO2M_INET_HPP__
#define __NOSO2M_INET_HPP__

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <string>

int inet_init();
void inet_cleanup();
struct addrinfo * inet_service( char const * host, char const * port );
int inet_local_ipv4( char const ipv4_addr[] );

class CInet {
public:
    std::string const & m_host;
    std::string const & m_port;
    const int m_timeosec;
public:
    CInet( std::string const & host, std::string const & port, int timeosec );
    int ExecCommand(
            size_t command_msgsize, char const * command_message,
            size_t response_buffsize, char * response_buffer,
            struct addrinfo const * bind_serv=nullptr );
};

class CPoolInet : public CInet {
public:
    std::string m_name;
private:
    struct addrinfo const * m_bind_serv;
public:
    CPoolInet( const std::string& name, const std::string &host, const std::string &port,
            int timeosec, struct addrinfo const * bind_serv=nullptr );
    void BuildCommandRequestPoolInfo(
            size_t command_msgsize, char * command_message );
    int RequestPoolInfo(
            size_t command_msgsize, char * command_message,
            size_t response_buffsize, char * response_buffer );
    void BuildCommandRequestPoolPublic(
            size_t command_msgsize, char * command_message );
    int RequestPoolPublic(
            size_t command_msgsize, char * command_message,
            size_t response_buffsize, char * response_buffer );
    void BuildCommandRequestSource( const char address[32],
        size_t command_msgsize, char * command_message );
    int RequestSource( const char address[32],
            size_t command_msgsize, char * command_message,
            size_t response_buffsize, char * response_buffer );
    void BuildCommandSubmitSolution( std::uint32_t blck_no,
            const char base[19], const char address[32],
            size_t command_msgsize, char * command_message );
    int SubmitSolution( std::uint32_t blck_no,
            const char base[19], const char address[32],
            size_t command_msgsize, char * command_message,
            size_t response_buffsize, char * response_buffer );
};

#endif // __NOSO2M_INET_HPP__

