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
    int ExecCommand( char * buffer, std::size_t buffsize, struct addrinfo const * bind_serv=nullptr );
};

class CPoolInet : public CInet {
public:
    std::string m_name;
private:
    struct addrinfo const * m_bind_serv;
public:
    CPoolInet( const std::string& name, const std::string &host, const std::string &port,
            int timeosec, struct addrinfo const * bind_serv=nullptr );
    int RequestPoolInfo( char *buffer, std::size_t buffsize );
    int RequestPoolPublic( char *buffer, std::size_t buffsize );
    int RequestSource( const char address[32], char *buffer, std::size_t buffsize );
    int SubmitSolution( std::uint32_t blck_no, const char base[19], const char address[32],
                        char *buffer, std::size_t buffsize );
};

#endif // __NOSO2M_INET_HPP__

