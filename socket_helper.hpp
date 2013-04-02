/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 *  Author : 
 *           Polycom Inc. (Edward Estabrook, Richard Lum).  Contributions (c) 2010 - 2013
 *
 */

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
char            *get_inet_address(struct sockaddr_storage * addr);
void            get_host_and_port(char * addr, char * host, int * port);

void            set_addr(struct sockaddr_storage *sa, char *ip, bool isIpV6);
void            set_port(struct sockaddr_storage *sa, int port);

string          get_socket_error_message();