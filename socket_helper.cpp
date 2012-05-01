#include "socket_helper.hpp"

#include <cstring>      //memcmp
#include <netinet/in.h> //sockadr_in, in_addr, ntohs
#include <arpa/inet.h>  //inet_ntop

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
    if (sa->ss_family == AF_INET) {
        return ntohs(((struct sockaddr_in*)sa)->sin_port);
    }

    return ntohs(((struct sockaddr_in6*)sa)->sin6_port);
}

string socket_to_ip_string(struct sockaddr_storage *socket)
{
  char ip[INET6_ADDRSTRLEN];
  ip[0] = 0;

  inet_ntop(socket->ss_family, get_in_addr(socket), ip, sizeof(ip));
  return string(ip);
}

string socket_to_ip_port_string(struct sockaddr_storage *socket)
{
  const int BUFFER_LENGTH = INET6_ADDRSTRLEN+10;
  char ip_and_port[BUFFER_LENGTH];
  char ip[INET6_ADDRSTRLEN];
  ip[0] = 0;

  inet_ntop(socket->ss_family, get_in_addr(socket), ip, sizeof(ip));
  snprintf(ip_and_port, sizeof(ip_and_port), "%s:%hu", ip, get_in_port(socket));

  return string(ip_and_port);
}


