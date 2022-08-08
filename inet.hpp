#ifndef __NOSO2M_INET_HPP__
#define __NOSO2M_INET_HPP__

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <string>

int inet_init();
void inet_cleanup();

class CInet {
public:
    const std::string m_host;
    const std::string m_port;
    const int m_timeosec;
    CInet( const std::string &host, const std::string &port, int timeosec );
private:
    struct addrinfo * InitService();
    static void CleanService( struct addrinfo * serv_info );
public:
    int ExecCommand( char *buffer, std::size_t buffsize );
};

class CNodeInet : public CInet {
public:
    CNodeInet( const std::string &host, const std::string &port , int timeosec );
    int RequestTimestamp( char *buffer, std::size_t buffsize );
    int RequestMNList( char *buffer, std::size_t buffsize );
    int RequestSource( char *buffer, std::size_t buffsize );
    int SubmitSolution( std::uint32_t blck, const char base[19], const char address[32],
                        char *buffer, std::size_t buffsize );
};

class CPoolInet : public CInet {
public:
    std::string m_name;
    CPoolInet( const std::string& name, const std::string &host, const std::string &port , int timeosec );
    int RequestPoolInfo( char *buffer, std::size_t buffsize );
    int RequestSource( const char address[32], char *buffer, std::size_t buffsize );
    int SubmitSolution( std::uint32_t blck_no, const char base[19], const char address[32],
                        char *buffer, std::size_t buffsize );
};

#endif // __NOSO2M_INET_HPP__