#include "sipp_globals.hpp"
//#include "infile.hpp" // FileContents::getField

#ifdef WIN32
#include <Winsock2.h>
#define  ssize_t SSIZE_T
#else
#include <sys/socket.h>  //sockaddr_storage
#endif


#ifdef WIN32
#define POLLREF WSAPOLLFD
#define SOCKREF SOCKET
#define CLOSESOCKET closesocket
#define POLL WSAPoll
#else  //WIN32
#include <poll.h>
#define SOCKREF int   //sock_fd
#define POLLREF struct pollfd

//msft defines as 0, -1 for posix?
//todo makesure all socket fd compare to INVALID_SOCKET for consistency
#define INVALID_SOCKET -1
#define CLOSESOCKET close  //make sure its a socket, not a file close to use this
#define POLL poll
#endif  //WIN32

// windows socket errors vs err.h
// WSAGetLastError    vs perror,errno
#ifdef WIN32
#define ERRORNUMBER errorcode
//from WinSock2.h v6.0a header file
//  note these are #if 0 disabled in winsock2.h to avoid name conflict
//  with err.h. Since we do use errno for non socket errors, only 
//  define the specific errors from err.h that we use to minimize 
//  potential name collision with errno for non socket errors.
#define EAGAIN      WSAEWOULDBLOCK
#define EINTR       WSAEINTR
#define EWOULDBLOCK WSAEWOULDBLOCK
#define EOPNOTSUPP  WSAEOPNOTSUPP
#define EADDRINUSE  WSAEADDRINUSE
#else
#define ERRORNUMBER errno
#endif


// for setsockopt TCP_NODELAY, not defined for windows xp
#ifndef SOL_TCP
#define SOL_TCP 6
#endif


//todo some of these dont need to be publicly available 
// residual from fact that they were moved one by one and made public to ensure nothing broke along the way
void                         initialize_sockets();
void                         cleanup_sockets();
void                         sipp_socket_invalidate(struct sipp_socket *socket);
void                         sipp_close_socket(struct sipp_socket *socket);
struct sipp_socket           *new_sipp_socket(bool use_ipv6, int transport);
int                          sipp_bind_socket(struct sipp_socket *socket, struct sockaddr_storage *saddr, int *port);
struct sipp_socket           *sipp_allocate_socket(bool use_ipv6, int transport, SOCKREF fd, int accepting) ;
struct sipp_socket           *sipp_allocate_socket(bool use_ipv6, int transport, SOCKREF fd);
SOCKREF                      socket_fd(bool use_ipv6, int transport);
void                         determine_remote_and_local_ip();
int                          empty_socket(struct sipp_socket *socket);
int                          check_for_message(struct sipp_socket *socket);
int                          write_socket(struct sipp_socket *socket, char *buffer, ssize_t len, int flags, struct sockaddr_storage *dest);
int                          flush_socket(struct sipp_socket *socket);
void                         free_socketbuf(struct socketbuf *socketbuf);
struct socketbuf             *alloc_socketbuf(char *buffer, size_t size, int copy, struct sockaddr_storage *dest) ;
void                         sipp_customize_socket(struct sipp_socket *socket);
int                          sipp_reconnect_socket(struct sipp_socket *socket);
struct sipp_socket           *sipp_accept_socket(struct sipp_socket *accept_socket, struct sockaddr_storage *source);
int                          sipp_connect_socket(struct sipp_socket *socket, struct sockaddr_storage *dest);
//int                          sipp_do_connect_socket(struct sipp_socket *socket);
int                          open_connections();
void                         connect_to_peer(char *peer_host, int peer_port, struct sockaddr_storage *peer_sockaddr, char *peer_ip, struct sipp_socket **peer_socket);
void                         connect_to_all_peers();
void                         connect_local_twin_socket(char * twinSippHost);
void                         print_if_error(int rc);

#ifdef WIN32 
wchar_t*                    wsaerrorstr(int errnumber);
char*                       wchar_to_char(wchar_t* orig,char* nstring);
#endif

extern struct pollfd         pollfiles[SIPP_MAXFDS]; 


#ifdef _USE_OPENSSL
const char *sip_tls_error_string(SSL *ssl, int size);
extern SSL_CTX  *sip_trp_ssl_ctx ; /* For SSL cserver context */
extern SSL_CTX  *sip_trp_ssl_ctx_client; /* For SSL cserver context */
extern SSL_CTX  *twinSipp_sip_trp_ssl_ctx_client; /* For SSL cserver context */
#endif

/* Socket Buffer Management. */
#define NO_COPY 0
#define DO_COPY 1

#define WS_EAGAIN 1 /* Return EAGAIN if there is no room for writing the message. */
#define WS_BUFFER 2 /* Buffer the message if there is no room for writing the message. */

/****************************** Network Interface *******************/

/* Our message detection states: */
#define CFM_NORMAL 0 /* No CR Found, searching for \r\n\r\n. */
#define CFM_CONTROL 1 /* Searching for 27 */
#define CFM_CR 2 /* CR Found, Searching for \n\r\n */
#define CFM_CRLF 3 /* CRLF Found, Searching for \r\n */
#define CFM_CRLFCR 4 /* CRLFCR Found, Searching for \n */
#define CFM_CRLFCRLF 5 /* We've found the end of the headers! */

extern int errorcode;

