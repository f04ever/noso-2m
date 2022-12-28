#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <cassert>
#include <cstring>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#else // LINUX/UNIX
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <arpa/inet.h>
#endif // _WIN32

#include "noso-2m.hpp"
#include "inet.hpp"

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

struct addrinfo * inet_service( char const * host, char const * port ) {
    struct addrinfo hints, * serv { nullptr };
    std::memset( &hints, 0, sizeof( hints ) );
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;
    while ( 1 ) {
        int rc = getaddrinfo( host, port, &hints, &serv );
        if ( rc == EAI_AGAIN ) {
            continue;
        }
        else {
            if ( rc ) {
                // fprintf( stderr, "getaddrinfo: %s\n", gai_strerror( rc ) );
                return nullptr;
            }
            break;
        }
    }
    return serv;
}

inline
int inet_bind( int sockfd, struct addrinfo const * serv_info ) {
    int rc, yes = 1;
    if ( ( rc = setsockopt( sockfd, SOL_SOCKET, SO_REUSEADDR,
                            (char *)&yes, sizeof(int) ) ) == -1 ) {
        return -1;
    }
    for ( struct addrinfo const * psi = serv_info; psi != NULL; psi = psi->ai_next ) {
        if ( ( rc = bind( sockfd, psi->ai_addr,
                        psi->ai_addrlen ) ) == -1 ) {
            continue;
        }
        return 0;
    }
    return -1;
}

inline
int inet_socket( int timeosec,
        struct addrinfo const * serv_info,
        struct addrinfo const * bind_serv ) {
    assert( serv_info );
    struct timeval timeout {
        .tv_sec = timeosec,
        .tv_usec = 0
    };
    int sockfd, rc;
    fd_set rset, wset;
    for ( struct addrinfo const * psi = serv_info; psi != NULL; psi = psi->ai_next ) {
        if ( (sockfd = socket( psi->ai_family, psi->ai_socktype,
                               psi->ai_protocol ) ) == -1 ) {
            continue;
        }
        if ( bind_serv
                && ( rc = inet_bind( sockfd, bind_serv ) ) == -1 ) {
            inet_close_socket( sockfd );
            continue;
        }
        if ( inet_set_nonblock( sockfd ) < 0 ) {
            inet_close_socket( sockfd );
            continue;
        }
        if ( ( rc = connect( sockfd, psi->ai_addr, psi->ai_addrlen ) ) >= 0 ) {
            return sockfd;
        }
        #ifdef _WIN32
        if ( rc != SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK ) {
        #else // LINUX/UNIX
        if ( errno != EINPROGRESS ) {
        #endif // END #ifdef _WIN32 ... #else // LINUX/UNIX
            inet_close_socket( sockfd );
            continue;
        }
        FD_ZERO( &rset );
        FD_ZERO( &wset );
        FD_SET( sockfd, &rset );
        FD_SET( sockfd, &wset );
        int n = select( sockfd + 1, &rset, &wset, NULL, &timeout );
        if ( n <= 0 ) inet_close_socket( sockfd );
        if ( FD_ISSET( sockfd, &rset ) || FD_ISSET( sockfd, &wset ) ) {
            int error = 0;
            socklen_t slen = sizeof( error );
            if ( getsockopt( sockfd, SOL_SOCKET, SO_ERROR, (char *)&error, &slen ) < 0 ) {
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
int inet_send( int sockfd, int timeosec, size_t msgsize, char const * message ) {
    assert( message && msgsize > 0 );
    struct timeval timeout {
        .tv_sec = timeosec,
        .tv_usec = 0
    };
    fd_set fds;
    FD_ZERO( &fds );
    FD_SET( sockfd, &fds );
    int n = select( sockfd + 1, NULL, &fds, NULL, &timeout );
    if ( n <= 0 ) return n; /* n == 0 timeout, n == -1 socket error */
    int slen = send( sockfd, message, msgsize, 0 );
    if ( slen <= 0 ) return slen; /* slen == 0 timeout, slen == -1 socket error */
    return slen;
}

inline
int inet_recv( int sockfd, int timeosec, size_t buffsize, char * buffer ) {
    assert( buffer && buffsize > 0 );
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
int inet_command( uint32_t timeosec,
        size_t command_msgsize, char const * command_message,
        size_t response_buffsize, char * response_buffer,
        struct addrinfo const * serv_info,
        struct addrinfo const * bind_serv ) {
    assert( command_message && command_msgsize > 0
           && response_buffer && response_buffsize > 0
           && serv_info );
    int sockfd = inet_socket( timeosec, serv_info, bind_serv );
    if ( sockfd < 0 ) return sockfd;
    int rlen = 0;
    int slen = inet_send( sockfd, timeosec, command_msgsize, command_message );
    if ( slen > 0 ) {
        rlen = inet_recv( sockfd, timeosec, response_buffsize, response_buffer );
    }
    inet_close_socket( sockfd );
    if ( slen <= 0 ) {
        return slen;
    }
    return rlen;
}

int inet_local_ipv4( char const ipv4_addr[] ) {
    #ifdef _WIN32
    ULONG family = AF_INET;
    ULONG flags = \
              GAA_FLAG_SKIP_ANYCAST
            | GAA_FLAG_SKIP_MULTICAST
            | GAA_FLAG_SKIP_DNS_SERVER
            | GAA_FLAG_SKIP_FRIENDLY_NAME;
    ULONG bufsize = 15000;
    PIP_ADAPTER_ADDRESSES adapters = (IP_ADAPTER_ADDRESSES *)std::malloc( bufsize );
    if ( adapters == NULL ) {
        return -1;
    }
    if ( GetAdaptersAddresses( family, flags, NULL, adapters, &bufsize ) == NO_ERROR ) {
        char inet_addr[INET_ADDRSTRLEN];
        for ( PIP_ADAPTER_ADDRESSES addresses = adapters;
                addresses != NULL; addresses = addresses->Next ) {
            for ( PIP_ADAPTER_UNICAST_ADDRESS unicast = addresses->FirstUnicastAddress;
                    unicast != NULL; unicast = unicast->Next ) {
                if ( unicast->Address.lpSockaddr->sa_family != AF_INET ) {
                    continue;
                }
                inet_ntop( AF_INET, &((sockaddr_in *)(unicast->Address.lpSockaddr))->sin_addr,
                        inet_addr, INET_ADDRSTRLEN );
                if ( std::strcmp( ipv4_addr, inet_addr ) == 0 ) {
                    std::free( adapters );
                    return 1;
                }
            }
        }
    }
    std::free( adapters );
    return 0;
    #else // LINUX/UNIX
    struct ifaddrs * addresses;
    if( getifaddrs( &addresses ) != 0) {
        return -1;
    }
    char inet_addr[INET_ADDRSTRLEN];
    for ( struct ifaddrs * ifa = addresses; ifa != NULL; ifa = ifa->ifa_next ) {
        if ( ifa->ifa_addr == NULL ) {
            continue;
        }
        if ( !( ifa->ifa_flags & IFF_UP ) ) {
            continue;
        }
        if ( ifa->ifa_addr->sa_family != AF_INET ) {
            continue;
        }
        inet_ntop( AF_INET, &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr,
                inet_addr, INET_ADDRSTRLEN );
        if ( std::strcmp( ipv4_addr, inet_addr ) == 0 ) {
            freeifaddrs( addresses );
            return 1;
        }
    }
    freeifaddrs( addresses );
    return 0;
    #endif // END #ifdef _WIN32 #else // LINUX/UNIX
}

inline
CInet::CInet( std::string const & host, std::string const & port, int timeosec )
    :   m_host { host }, m_port { port }, m_timeosec( timeosec ) {
}

inline
int CInet::ExecCommand(
        size_t command_msgsize, char const * command_message,
        size_t response_buffsize, char * response_buffer,
        struct addrinfo const * bind_serv ) {
    assert( command_message && command_msgsize > 0
           && response_buffer && response_buffsize > 0 );
    struct addrinfo * serv_info = inet_service( m_host.c_str(), m_port.c_str() );
    if ( !serv_info ) {
        return -1;
    }
    int n = inet_command( m_timeosec,
            command_msgsize, command_message,
            response_buffsize, response_buffer,
            serv_info, bind_serv );
    freeaddrinfo( serv_info );
    return n;
}

CPoolInet::CPoolInet( const std::string& name, const std::string &host, const std::string &port,
        int timeosec, struct addrinfo const * bind_serv )
    :   CInet( host, port, timeosec ), m_name { name },
        m_bind_serv { bind_serv } {
}

void CPoolInet::BuildCommandRequestPoolInfo(
        size_t command_msgsize, char * command_message ) {
    assert( command_message && command_msgsize > 0 );
    std::snprintf( command_message, command_msgsize, "POOLINFO\n" );
}

int CPoolInet::RequestPoolInfo(
        size_t command_msgsize, char * command_message,
        size_t response_buffsize, char * response_buffer ) {
    assert( command_message && command_msgsize > 0
           && response_buffer && response_buffsize > 0 );
    this->BuildCommandRequestPoolInfo(
            command_msgsize, command_message );
    return this->ExecCommand(
            command_msgsize, command_message,
            response_buffsize, response_buffer,
            m_bind_serv );
}

void CPoolInet::BuildCommandRequestPoolPublic(
        size_t command_msgsize, char * command_message ) {
    assert( command_message && command_msgsize > 0 );
    std::snprintf( command_message, command_msgsize, "POOLPUBLIC\n" );
}

int CPoolInet::RequestPoolPublic(
        size_t command_msgsize, char * command_message,
        size_t response_buffsize, char * response_buffer ) {
    assert( command_message && command_msgsize > 0
           && response_buffer && response_buffsize > 0 );
    this->BuildCommandRequestPoolPublic(
            command_msgsize, command_message );
    return this->ExecCommand(
            command_msgsize, command_message,
            response_buffsize, response_buffer,
            m_bind_serv );
}

void CPoolInet::BuildCommandRequestSource( const char address[32],
        size_t command_msgsize, char * command_message ) {
    assert( std::strlen( address ) == 30 || std::strlen( address ) == 31 );
    assert( command_message && command_msgsize > 0 );
    // SOURCE {address} {MinerName}
    std::snprintf( command_message, command_msgsize,
            "SOURCE %s noso-2m-v%s\n", address, NOSO_2M_VERSION );
}

int CPoolInet::RequestSource( const char address[32],
        size_t command_msgsize, char * command_message,
        size_t response_buffsize, char * response_buffer ) {
    assert( std::strlen( address ) == 30 || std::strlen( address ) == 31 );
    assert( command_message && command_msgsize > 0
           && response_buffer && response_buffsize > 0 );
    this->BuildCommandRequestSource( address, command_msgsize, command_message );
    return this->ExecCommand(
            command_msgsize, command_message,
            response_buffsize, response_buffer,
            m_bind_serv );
}

void CPoolInet::BuildCommandSubmitSolution( std::uint32_t blck_no,
        const char base[19], const char address[32],
        size_t command_msgsize, char * command_message ) {
    assert( std::strlen( base ) == 18
            && std::strlen( address ) == 30 || std::strlen( address ) == 31 );
    assert( command_message && command_msgsize > 0 );
    // SHARE {Address} {Hash} {MinerName}
    std::snprintf( command_message, command_msgsize,
            "SHARE %s %s noso-2m-v%s %d\n",
            address, base, NOSO_2M_VERSION, blck_no );
}

int CPoolInet::SubmitSolution( std::uint32_t blck_no,
        const char base[19], const char address[32],
        size_t command_msgsize, char * command_message,
        size_t response_buffsize, char * response_buffer ) {
    assert( std::strlen( base ) == 18
            && std::strlen( address ) == 30 || std::strlen( address ) == 31 );
    assert( command_message && command_msgsize > 0
           && response_buffer && response_buffsize > 0 );
    this->BuildCommandSubmitSolution( blck_no, base, address,
            command_msgsize, command_message );
    return this->ExecCommand(
            command_msgsize, command_message,
            response_buffsize, response_buffer,
            m_bind_serv );
}

