#include <cassert>
#include <cstring>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#endif // _WIN32
#include "inet.hpp"
#include "output.hpp"
#include "noso-2m.hpp"

int inet_init() {
    #ifdef _WIN32
    WSADATA wsaData;
    return WSAStartup( MAKEWORD( 2, 2 ), &wsaData ) != NO_ERROR ? -1 : 0;
    #endif
    return 0;
}

void inet_cleanup() {
    #ifdef _WIN32
    WSACleanup();
    #endif
}

inline
void inet_close_socket( int sockfd ) {
    #ifdef _WIN32
    closesocket( sockfd );
    #else
    close( sockfd );
    #endif
}

inline
int inet_set_nonblock( int sockfd ) {
    #ifdef _WIN32
    u_long mode = 1;
    if ( ioctlsocket( sockfd, FIONBIO, &mode ) != NO_ERROR ) return -1;
    #else // LINUX/UNIX
    int flags = 0;
    if ( ( flags = fcntl( sockfd, F_GETFL, 0 ) ) < 0 ) return -1;
    if ( fcntl( sockfd, F_SETFL, flags | O_NONBLOCK ) < 0 ) return -1;
    #endif // END #ifdef _WIN32
    return sockfd;
}

inline
int inet_socket( struct addrinfo *serv_info, int timeosec ) {
    struct addrinfo *psi = serv_info;
    struct timeval timeout {
        .tv_sec = timeosec,
        .tv_usec = 0
    };
    int sockfd, rc;
    fd_set rset, wset;
    for( ; psi != NULL; psi = psi->ai_next ) {
        if ( (sockfd = socket( psi->ai_family, psi->ai_socktype,
                               psi->ai_protocol ) ) == -1 ) continue;
        if ( inet_set_nonblock( sockfd ) < 0 ) {
            inet_close_socket( sockfd );
            continue;
        }
        if ( ( rc = connect( sockfd, psi->ai_addr, psi->ai_addrlen ) ) >= 0 )
            return sockfd;
        #ifdef _WIN32
        if ( rc != SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK ) {
            closesocket( sockfd );
            continue;
        }
        #else // LINUX/UNIX
        if ( errno != EINPROGRESS ) {
            close( sockfd );
            continue;
        }
        #endif // END #ifdef _WIN32
        FD_ZERO( &rset );
        FD_ZERO( &wset );
        FD_SET( sockfd, &rset );
        FD_SET( sockfd, &wset );
        int n = select( sockfd + 1, &rset, &wset, NULL, &timeout );
        if ( n <= 0 ) inet_close_socket( sockfd );
        if ( FD_ISSET( sockfd, &rset ) || FD_ISSET( sockfd, &wset ) ) {
            int error = 0;
            socklen_t slen = sizeof( error );
            if ( getsockopt( sockfd, SOL_SOCKET, SO_ERROR, (char*)&error, &slen ) < 0 ) {
                inet_close_socket( sockfd );
                continue;
            }
            if ( error ) {
                inet_close_socket( sockfd );
                continue;
            }
        }
        else continue;
        return sockfd;
    } // END for( ; psi != NULL; psi = psi->ai_next ) {
    return -1;
}

inline
int inet_send( int sockfd, int timeosec, const char *message, size_t size ) {
    struct timeval timeout {
        .tv_sec = timeosec,
        .tv_usec = 0
    };
    fd_set fds;
    FD_ZERO( &fds );
    FD_SET( sockfd, &fds );
    int n = select( sockfd + 1, NULL, &fds, NULL, &timeout );
    if ( n <= 0 ) return n; /* n == 0 timeout, n == -1 socket error */
    int slen = send( sockfd, message, size, 0 );
    if ( slen <= 0 ) return slen; /* slen == 0 timeout, slen == -1 socket error */
    return slen;
}

inline
int inet_recv( int sockfd, int timeosec, char *buffer, size_t buffsize ) {
    struct timeval timeout {
        .tv_sec = timeosec,
        .tv_usec = 0
    };
    fd_set fds;
    FD_ZERO( &fds );
    FD_SET( sockfd, &fds );
    int n = select( sockfd + 1, &fds, NULL, NULL, &timeout );
    if ( n <= 0 ) return n; /* n == 0 timeout, n == -1 socket error */
    int rlen = recv( sockfd, buffer, buffsize - 1, 0 );
    if ( rlen <= 0 ) return rlen; /* rlen == 0 timeout, nlen == -1 socket error */
    buffer[ rlen ] = '\0';
    return rlen;
}

inline
int inet_command( struct addrinfo *serv_info, uint32_t timeosec, char *buffer, size_t buffsize ) {
    int sockfd = inet_socket( serv_info, timeosec );
    if ( sockfd < 0 ) return sockfd;
    int rlen = 0;
    int slen = inet_send( sockfd, timeosec, buffer, buffsize );
    if ( slen > 0 ) rlen = inet_recv( sockfd, timeosec, buffer, buffsize );
    inet_close_socket( sockfd );
    if ( slen <= 0 ) return slen;
    return rlen;
}

inline
CInet::CInet( const std::string &host, const std::string &port, int timeosec )
    :   m_host { host }, m_port { port }, m_timeosec( timeosec ) {
}

inline
struct addrinfo * CInet::InitService() {
    struct addrinfo hints, *serv_info;
    std::memset( &hints, 0, sizeof( hints ) );
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    int n = getaddrinfo( this->m_host.c_str(), this->m_port.c_str(), &hints, &serv_info );
    if ( n ) {
        NOSO_LOG_DEBUG
            << "CInet::InitService/getaddrinfo: " << gai_strerror( n )
            << std::endl;
        return NULL;
    }
    return serv_info;
}

inline
void CInet::CleanService( struct addrinfo * serv_info ) {
    if ( serv_info == NULL ) return;
    freeaddrinfo( serv_info );
    serv_info = NULL;
}

inline
int CInet::ExecCommand( char *buffer, std::size_t buffsize ) {
    assert( buffer && buffsize > 0 );
    struct addrinfo * serv_info = this->InitService();
    if ( serv_info == NULL ) return -1;
    int n = inet_command( serv_info, m_timeosec, buffer, buffsize );
    this->CleanService( serv_info );
    return n;
}

CNodeInet::CNodeInet( const std::string &host, const std::string &port , int timeosec )
    :   CInet( host, port, timeosec ) {
}

int CNodeInet::RequestTimestamp( char *buffer, std::size_t buffsize ) {
    assert( buffer && buffsize > 0 );
    std::strcpy( buffer, "NSLTIME\n" );
    return this->ExecCommand( buffer, buffsize );
}

int CNodeInet::RequestMNList( char *buffer, std::size_t buffsize ) {
    assert( buffer && buffsize > 0 );
    std::strcpy( buffer, "NSLMNS\n" );
    return this->ExecCommand( buffer, buffsize );
}

int CNodeInet::RequestSource( char *buffer, std::size_t buffsize ) {
    assert( buffer && buffsize > 0 );
    std::strcpy( buffer, "NODESTATUS\n" );
    return this->ExecCommand( buffer, buffsize );
}

int CNodeInet::SubmitSolution( std::uint32_t blck, const char base[19], const char address[32],
                    char *buffer, std::size_t buffsize ) {
    assert( buffer && buffsize > 0 );
    assert( std::strlen( base ) == 18
            && ( std::strlen( address ) == 30 || std::strlen( address ) == 31 ) );
    std::snprintf( buffer, buffsize, "BESTHASH 1 2 3 4 %s %s %d %lld\n", address, base, blck, NOSO_TIMESTAMP );
    return this->ExecCommand( buffer, buffsize );
}

CPoolInet::CPoolInet( const std::string& name, const std::string &host, const std::string &port , int timeosec )
    :   CInet( host, port, timeosec ), m_name { name } {
}

int CPoolInet::RequestPoolInfo( char *buffer, std::size_t buffsize ) {
    assert( buffer && buffsize > 0 );
    std::snprintf( buffer, buffsize, "POOLINFO\n" );
    return this->ExecCommand( buffer, buffsize );
}

int CPoolInet::RequestSource( const char address[32], char *buffer, std::size_t buffsize ) {
    assert( buffer && buffsize > 0 );
    assert( std::strlen( address ) == 30 || std::strlen( address ) == 31 );
    // SOURCE {address} {MinerName}
    std::snprintf( buffer, buffsize, "SOURCE %s noso-2m-v%s\n", address, NOSO_2M_VERSION );
    return this->ExecCommand( buffer, buffsize );
}

int CPoolInet::SubmitSolution( std::uint32_t blck_no, const char base[19], const char address[32],
                    char *buffer, std::size_t buffsize ) {
    assert( buffer && buffsize > 0 );
    assert( std::strlen( base ) == 18
            && std::strlen( address ) == 30 || std::strlen( address ) == 31 );
    // SHARE {Address} {Hash} {MinerName}
    std::snprintf( buffer, buffsize, "SHARE %s %s noso-2m-v%s %d\n", address, base, NOSO_2M_VERSION, blck_no );
    return this->ExecCommand( buffer, buffsize );
}
