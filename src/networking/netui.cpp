#include <config.h>

#include "const.h"
#include "netui.h"
#include "vsnet_serversocket.h"
#include "vsnet_debug.h"
#include "vsnet_headers.h"

static void static_initNetwork( )
{
#if defined(_WIN32) && !defined(__CYGWIN__)
    static bool first_time = true;
    if( first_time )
    {
        first_time = false;

        COUT <<"Initializing Winsock"<<std::endl;
        WORD wVersionRequested = MAKEWORD( 1, 1 );
        WSADATA wsaData; 
        int res = WSAStartup(wVersionRequested,&wsaData);
        if( res != 0)
            COUT <<"Error initializing Winsock"<<std::endl;
    }
#endif
}

/**************************************************************/
/**************************************************************/
/**** Create (and bind) a socket on host                   ****/
/**************************************************************/

// Creates and bind the socket designed to receive coms
// host == NULL -> localhost

SOCKETALT NetUITCP::createSocket( char * host, unsigned short srv_port, SocketSet& set )
{
    COUT << "enter " << __PRETTY_FUNCTION__ << std::endl;

    static_initNetwork( );

    int            local_fd;
    AddressIP      remote_ip;

    struct hostent  *he = NULL;
#if defined(_WIN32) && !defined(__CYGWIN__)
    int sockerr= INVALID_SOCKET;
#else
    int sockerr= -1;
#endif

    if( (local_fd = socket( PF_INET, SOCK_STREAM, 0))==sockerr)
    {
        COUT << "Could not create socket" << std::endl;
        SOCKETALT ret; // ( -1, SOCKETALT::TCP, remote_ip );
        return ret;
    }

    // If port is not given, use the defaults ones --> do not work with specified ones yet... well, didn't try
    if( srv_port == 0 )
    {
        srv_port = SERVER_PORT;
    }

    // Gets the host info for host
    if( host[0]<48 || host[0]>57)
    {
        COUT <<"Resolving host name... ";
        if( (he = gethostbyname( host)) == NULL)
        {
            COUT << "Could not resolve hostname" << std::endl;
            close_socket( local_fd );
            SOCKETALT ret; // ( -1, SOCKETALT::TCP, remote_ip );
            return ret;
        }
        memcpy( &remote_ip.sin_addr.s_addr, he->h_addr_list[0], he->h_length);
        COUT <<"found : "<<inet_ntoa( remote_ip.sin_addr)<<std::endl;
    }
    else
    {
#if defined(_WIN32) && !defined(__CYGWIN__)
        if( (remote_ip.sin_addr.s_addr=inet_addr( host)) == 0)
        {
            COUT << "Error inet_addr" << std::endl;
            close_socket( local_fd );
            SOCKETALT ret; // ( -1, SOCKETALT::TCP, remote_ip );
            return ret;
        }
#else           
        if( inet_aton( host, &remote_ip.sin_addr) == 0)
        {
            COUT << "Error inet_aton" << std::endl;
            close_socket( local_fd );
            SOCKETALT ret; // ( -1, SOCKETALT::TCP, remote_ip );
            return ret;
        }
#endif
    }
    // Store it in srv_ip struct
    remote_ip.sin_port   = htons( srv_port );
    remote_ip.sin_family = AF_INET;

    COUT << "Connecting to " << inet_ntoa( remote_ip.sin_addr) << " on port " << srv_port << std::endl;
#if defined(_WIN32) && !defined(__CYGWIN__)
    if( ::connect( local_fd, (sockaddr *)&remote_ip, sizeof( struct sockaddr))==SOCKET_ERROR)
#else
    if( ::connect( local_fd, (sockaddr *)&remote_ip, sizeof( struct sockaddr)) < 0 )
#endif
    {
        perror( "Can't connect to server ");
        close_socket( local_fd );
        SOCKETALT ret; // ( -1, SOCKETALT::TCP, remote_ip );
        return ret;
    }
    COUT << "Connected to " << inet_ntoa( remote_ip.sin_addr) << ":" << srv_port << std::endl;

    SOCKETALT ret( local_fd, SOCKETALT::TCP, remote_ip, set );
    COUT << "SOCKETALT n� : " << ret.get_fd() << std::endl;
    return ret;
}

ServerSocket* NetUITCP::createServerSocket( unsigned short port, SocketSet& set )
{
    COUT << "enter " << __PRETTY_FUNCTION__ << std::endl;

    static_initNetwork( );
    
    int       local_fd;
    AddressIP local_ip;

#if defined(_WIN32) && !defined(__CYGWIN__)
    int sockerr= INVALID_SOCKET;
#else
    int sockerr= -1;
#endif

    // this->server = 1;

    if( (local_fd = socket( PF_INET, SOCK_STREAM, 0))==sockerr )
    {
        COUT << "Could not create socket" << std::endl;
        return NULL;
    }

    // If port is not given, use the defaults ones --> do not work with specified ones yet... well, didn't try
    if( port == 0 ) port = SERVER_PORT;

    memset( &local_ip, 0, sizeof(AddressIP) );
    local_ip.sin_addr.s_addr = htonl(INADDR_ANY);
    local_ip.sin_port        = htons( port );
    local_ip.sin_family      = AF_INET;
    // binds socket
    COUT << "Bind on " << ntohl(local_ip.sin_addr.s_addr) << ", port "
         << ntohs( local_ip.sin_port) << std::endl;
    if( bind( local_fd, (sockaddr *)&local_ip, sizeof( struct sockaddr_in) )==SOCKET_ERROR )
    {
        perror( "Problem binding socket" );
        close_socket( local_fd );
        return NULL;
    }

    COUT << "Accepting max : " << SOMAXCONN << std::endl;
    if( listen( local_fd, SOMAXCONN)==SOCKET_ERROR)
    {
        perror( "Problem listening on socket" );
        close_socket( local_fd );
        return NULL;
    }
    COUT << "Listening on socket " << local_fd << std::endl
         << "*** ServerSocket n� : " << local_fd << std::endl;
    return new ServerSocketTCP( local_fd, local_ip, set );
}

/**************************************************************/
/**** Create (and bind) a socket on host                   ****/
/**************************************************************/

// Creates and bind the socket designed to receive coms
// host == NULL -> localhost

SOCKETALT NetUIUDP::createSocket( char * host, unsigned short srv_port, SocketSet& set )
{
    COUT << " enter " << __PRETTY_FUNCTION__ << std::endl;
    static_initNetwork( );

    AddressIP local_ip;
    AddressIP remote_ip;
    int       local_fd;
#if defined(_WIN32) && !defined(__CYGWIN__)
    static const int sockerr = INVALID_SOCKET;
#else
    static const int sockerr = -1;
#endif

    // this->server = 0;

    // If port is not given, use the defaults ones --> do not work with specified ones yet... well, didn't try
    if( srv_port==0 ) srv_port = SERVER_PORT;

    if( (local_fd = socket( PF_INET, SOCK_DGRAM, 0))==sockerr)
    {
        perror( "Could not create socket");
        SOCKETALT ret; // ( -1, SOCKETALT::UDP, remote_ip );
        return ret;
    }

    // Gets the host info for host
    struct hostent  *he = NULL;

    if( host[0]<48 || host[0]>57)
    {
        COUT <<"Resolving host name... ";
        if( (he = gethostbyname( host)) == NULL)
        {
            COUT << "Could not resolve hostname" << std::endl;
            close_socket( local_fd );
            SOCKETALT ret; // ( -1, SOCKETALT::UDP, remote_ip );
            return ret;
        }
        memcpy( &remote_ip.sin_addr.s_addr, he->h_addr_list[0], he->h_length);
        COUT <<"found : "<<inet_ntoa( remote_ip.sin_addr)<<std::endl;
    }
    else
    {
#if defined(_WIN32) && !defined(__CYGWIN__)
        if( (remote_ip.sin_addr.s_addr=inet_addr( host)) == 0)
#else           
        if( inet_aton( host, &remote_ip.sin_addr) == 0)
#endif
        {
            COUT << "Error inet_aton" << std::endl;
            close_socket( local_fd );
            SOCKETALT ret; // ( -1, SOCKETALT::UDP, remote_ip );
            return ret;
        }
    }
    // Store it in srv_ip struct
    remote_ip.sin_port= htons( srv_port);
    remote_ip.sin_family= AF_INET;

    local_ip.sin_addr.s_addr = htonl(INADDR_ANY);
    local_ip.sin_port = htons( 0);
    local_ip.sin_family = AF_INET;

    // binds socket
    if( bind( local_fd, (sockaddr *)&local_ip, sizeof(struct sockaddr_in))==SOCKET_ERROR )
    {
        perror( "Can't bind socket" );
        close_socket( local_fd );
        SOCKETALT ret; // ( -1, SOCKETALT::UDP, remote_ip );
        return ret;
    }

    SOCKETALT ret( local_fd, SOCKETALT::UDP, remote_ip, set );

    if( ret.set_nonblock() == false )
    {
        ret.disconnect( "Could not set socket to nonblocking state" );
        SOCKETALT ret; // ( -1, SOCKETALT::UDP, remote_ip );
        return ret;
    }
    COUT << "Bind on localhost, " << ret << std::endl;
    return ret;
}

ServerSocket* NetUIUDP::createServerSocket( unsigned short port, SocketSet& set )
{
    COUT << "enter " << __PRETTY_FUNCTION__ << std::endl;
    static_initNetwork( );

    int       local_fd;
    AddressIP local_ip;

#if defined(_WIN32) && !defined(__CYGWIN__)
    static const int sockerr= INVALID_SOCKET;
#else
    static const int sockerr= -1;
#endif

    // this->server = 1;
    // If port is not given, use the defaults ones --> do not work with specified ones yet... well, didn't try
    if( port == 0 ) port = SERVER_PORT;

    if( (local_fd = socket( PF_INET, SOCK_DGRAM, 0 ))==sockerr )
    {
        COUT << "Could not create socket" << std::endl;
        return NULL;
    }

    memset( &local_ip, 0, sizeof(AddressIP) );
    local_ip.sin_addr.s_addr = htonl(INADDR_ANY);
    local_ip.sin_port        = htons( port );
    local_ip.sin_family      = AF_INET;
    // binds socket
    if( bind( local_fd, (sockaddr *)&local_ip, sizeof( struct sockaddr_in ) )==SOCKET_ERROR )
    {
        perror( "Cannot bind socket");
        close_socket( local_fd );
        return NULL;
    }

    ServerSocket* ret = new ServerSocketUDP( local_fd, local_ip, set );

    if( ret->set_nonblock() == false )
    {
        ret->disconnect( "Setting server socket mode to nonblocking failed", true );
        delete ret;
        return NULL;
    }

    COUT << "Bind on localhost, " << *ret << std::endl;
    return ret;
}

