#include "socket_helper.hpp"
#include "sipp_sockethandler.hpp"  //wsaerrorstr
#include "sipp_globals.hpp"   //_RCAST
#include <cstring>      //memcmp
#include <stdlib.h>
#ifdef WIN32
#include <winsock2.h>
#include <Ws2tcpip.h>  // Ws2_32.lib   getnameinfo 
#define SNPRINTF _snprintf
#else
#include <netinet/in.h> //sockadr_in, in_addr, ntohs
#include <arpa/inet.h>  //inet_ntop
#include <sys/socket.h>
#include <netdb.h>    // for getnameinfo
#define SNPRINTF snprintf
#endif
#include <stdio.h>      //snprintf

// return true if address is equal, false if not
bool is_in_addr_equal(const struct sockaddr_storage *left, const struct sockaddr_storage *right)
{
  if (left->ss_family != right->ss_family) {
    return false;
  }

  if (left->ss_family == AF_INET) {
    return memcmp( &(((struct sockaddr_in*)left)->sin_addr), &(((struct sockaddr_in*)right)->sin_addr), sizeof(struct in_addr) ) == 0;
  }

  return memcmp( &(((struct sockaddr_in6*)left)->sin6_addr), &(((struct sockaddr_in6*)right)->sin6_addr), sizeof(struct in6_addr) ) == 0;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr_storage *sa)
{
  if (sa->ss_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// get port, IPv4 or IPv6:
unsigned short get_in_port(struct sockaddr_storage *sa)
{
  if (sa->ss_family == 0) {
    return 0;
  }
  else if (sa->ss_family == AF_INET) {
    return ntohs(((struct sockaddr_in*)sa)->sin_port);
  }

  return ntohs(((struct sockaddr_in6*)sa)->sin6_port);
}

// If a socket error is known to have occured call to get the error message.
string get_socket_error_message() 
{
#ifdef WIN32
  int error = WSAGetLastError();
  wchar_t *error_msg = wsaerrorstr(error);
  char errorstring[1000];
  const char *errstring = wchar_to_char(error_msg, errorstring);
  return string(errorstring);
#else
  return string(strerror(errno));
#endif
}

string socket_to_ip_string(struct sockaddr_storage *socket)
{
  if (socket->ss_family == 0) {
    return "undefined";
  }
  char ip[INET6_ADDRSTRLEN];
  ip[0] = 0;
#ifdef WIN32
  int flag = NI_NUMERICHOST;
  int err = getnameinfo((struct sockaddr *) socket,sizeof(struct sockaddr_storage), ip, sizeof(ip), NULL, 0, flag );
  if (err){
    fprintf(stderr, "socket_helper.cpp:socket_to_ip_string(): getnameinfo error looking up ip for socket (AF = %d) Error: %s\n",
      socket->ss_family, get_socket_error_message().c_str());
  }
#else
  if (inet_ntop(socket->ss_family, get_in_addr(socket), ip, sizeof(ip)) == 0){
    fprintf(stderr, "socket_helper.cpp:socket_to_ip_string(): inet_ntop error looking up ip for socket (AF = %d) Error: %s\n",
      socket->ss_family, get_socket_error_message().c_str());
  }
#endif
  return string(ip);
}

string socket_to_ip_port_string(struct sockaddr_storage *socket)
{
  const int BUFFER_LENGTH = INET6_ADDRSTRLEN+10;
  char ip_and_port[BUFFER_LENGTH];
  char ip[INET6_ADDRSTRLEN];
  ip[0] = 0;

  if (socket->ss_family == 0) {
    return "undefined";
  }
  int flag = NI_NUMERICHOST;
  int err = getnameinfo((struct sockaddr *) socket, sizeof(struct sockaddr_storage), ip, sizeof(ip), NULL, 0, flag );
  if (err) {
    fprintf(stderr, "socket_helper.cpp:socket_to_ip_port_string(): getnameinfo error looking up ip for socket (AF = %d) Error: %s\n",
      socket->ss_family, get_socket_error_message().c_str());
  }
  if (socket->ss_family == AF_INET6){
    SNPRINTF(ip_and_port, sizeof(ip_and_port), "[%s]:%hu", ip, get_in_port(socket));
  }else{
    SNPRINTF(ip_and_port, sizeof(ip_and_port), "%s:%hu", ip, get_in_port(socket));
  }

  return string(ip_and_port);
}



char * get_inet_address(struct sockaddr_storage * addr)
{
  static char * ip_addr = NULL;

  if (!ip_addr) {
    ip_addr = (char *)malloc(1024*sizeof(char));
  }
  if (getnameinfo(_RCAST( struct sockaddr * , addr),
                  SOCK_ADDR_SIZE(addr),
                  ip_addr,
                  1024,
                  NULL,
                  0,
                  NI_NUMERICHOST) != 0) {
    strcpy(ip_addr, "addr not supported");
  }

  return ip_addr;
}

void get_host_and_port(char * addr, char * host, int * port)
{
  /* Separate the port number (if any) from the host name.
   * Thing is, the separator is a colon (':').  The colon may also exist
   * in the host portion if the host is specified as an IPv6 address (see
   * RFC 2732).  If that's the case, then we need to skip past the IPv6
   * address, which should be contained within square brackets ('[',']').
   */
  char *p;
  p = strchr( addr, '[' );                      /* Look for '['.            */
  if( p != NULL ) {                             /* If found, look for ']'.  */
    p = strchr( p, ']' );
  }
  if( p == NULL ) {                             /* If '['..']' not found,   */
    p = addr;                                   /* scan the whole string.   */
  } else {                                      /* If '['..']' found,       */
    char *p1;                                   /* extract the remote_host  */
    char *p2;
    p1 = strchr( addr, '[' );
    p2 = strchr( addr, ']' );
    *p2 = '\0';
    strcpy(host, p1 + 1);
    *p2 = ']';
  }
  /* Starting at <p>, which is either the start of the host substring
   * or the end of the IPv6 address, find the last colon character.
   */
  p = strchr( p, ':' );
  if( NULL != p ) {
    *p = '\0';
    *port = atol(p + 1);
  } else {
    *port = 0;
  }
}

void set_addr(struct sockaddr_storage *sa, char *ip, bool isIpV6)
{
  if (isIpV6){
    sa->ss_family=AF_INET6;
    struct in6_addr * address = &(((sockaddr_in6*)(sa))->sin6_addr) ;
    inet_pton(AF_INET6, ip, address);
  }else {
    sa->ss_family=AF_INET;
    struct in_addr* address = &(((sockaddr_in*)(sa))->sin_addr) ;
    inet_pton(AF_INET, ip, address);
  }
}

void set_port(struct sockaddr_storage *sa, int port)
{
  if (sa->ss_family == AF_INET6) {
    (_RCAST(struct sockaddr_in6 *, sa))->sin6_port = htons(port);
  } else {
    (_RCAST(struct sockaddr_in *, sa))->sin_port = htons(port);
  }
}



