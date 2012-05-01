#include <string>

#ifdef WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
#else
  #include <sys/socket.h>
#endif

using namespace std;

bool            is_in_addr_equal(const struct sockaddr_storage *left, const struct sockaddr_storage *right);
void            *get_in_addr(struct sockaddr_storage *sa);
unsigned short  get_in_port(struct sockaddr_storage *sa);
string          socket_to_ip_string(struct sockaddr_storage *socket);
string          socket_to_ip_port_string(struct sockaddr_storage *socket);
