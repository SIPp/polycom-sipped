/****************************************
  cross platform (linux, cygwin, win32)
  unit test sipp_sockethandler.cpp and socket_helper.cpp
  20120615

  rlum@polycom.com


*******************************************/

#include "gtest/gtest.h"
#include <string>
#include <cstdio>
#include <cstdlib>
#include "sipp_sockethandler.hpp"
#include "sipp_globals.hpp"
#include <wchar.h>

#ifdef WIN32
#include <Winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h> //gethostname
#include <netdb.h>  //addrinfo
#include <arpa/inet.h>
#include <string.h>
#include <netinet/tcp.h> // TCP_NODELAY
#endif

#include <string>
#include <vector>
#include <iostream>

#ifdef WIN32
#define SOCKREF SOCKET
#else
#define SOCKREF int   //sock_fd
//msft defines as 0, -1 for posix  
//todo makesure all socket fd compare to INVALID_SOCKET for consitency
#define INVALID_SOCKET -1
#endif

#include "win32_compatibility.hpp"  //inet_ntop
#include "socket_helper.hpp" // socket_to_ip_string, is_in_addr_equal, socket_to_ip_port_string

using namespace std;

/******************** helper routines ********************/
// will only work after sockfd has been through bind or connect
string fd_to_ipstring(SOCKREF sockfd){
    struct sockaddr_storage name;
    socklen_t namelen = sizeof (sockaddr_storage);
    int rv;
    if ((rv=getsockname(sockfd,(struct sockaddr*)&name, &namelen)) != 0){
      print_if_error(rv);
      return "GETSOCKNAME FAILURE";
    }
    //printf("address from socket: %s\n",socket_to_ip_port_string((struct sockaddr_storage*)&name).c_str());
    return socket_to_ip_port_string((struct sockaddr_storage*)&name);
}

// get all ip addresses of local machine
vector<string> getall_local_ip(int addressfamily)
{
  vector<string> result;
  addrinfo hints, *res;
  int status;
  char ip[INET6_ADDRSTRLEN]={0};
  //char* ip = (char*)malloc(INET6_ADDRSTRLEN);
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = addressfamily;
  hints.ai_flags = AI_PASSIVE;

  char hostname[80];
    if (gethostname(hostname,sizeof(hostname))!=0){
    printf("failed to get hostname\n");	
    exit(1);
  }
  
  if ( (status = getaddrinfo(hostname, "0" , &hints, &res)) != 0){
#ifdef WIN32
    print_if_error(status);
#else
    printf("getaddrinfo error: %s\n", gai_strerror(status));
#endif
    exit(1);
  };

  addrinfo* r = res;
  while (r) {	
    if (r->ai_family == AF_INET){
      inet_ntop(AF_INET,
          &(((sockaddr_in*)(r->ai_addr))->sin_addr.s_addr), 
          ip,
          sizeof(ip));
      //printf ("ipv4 = %s, port type = %d, socket = %d\n",
      //		ip, r->ai_socktype,  
      //		htons(((sockaddr_in*)(r->ai_addr))->sin_port));
      result.push_back(ip);
    }else if (r->ai_family == AF_INET6){
      inet_ntop(AF_INET6,
          &(((sockaddr_in6*)(r->ai_addr))->sin6_addr.s6_addr), 
          ip,
          sizeof(ip));
     //  printf ("ipv6 = %s, port type = %d, socket = %d\n",
          //ip, r->ai_socktype,  
          //htons(((sockaddr_in6*)(r->ai_addr))->sin6_port));
          result.push_back(string(ip));
    }else{
      printf ("Encountered unkown ai_family = %d\n", res->ai_family);
    }
    r=r->ai_next;
  };
  return result;
}

// helper routine to get the first local machine of the right address family
string get_local_ip(int addressfamily)
{
  string result = "";
  addrinfo hints, *res;
  int status;
  char ip[INET6_ADDRSTRLEN];
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = addressfamily;
  //hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  char hostname[80];
    if (gethostname(hostname,sizeof(hostname))!=0){
    printf("failed to get hostname\n");	
    exit(1);
  }

  if ( (status = getaddrinfo(hostname, "0" , &hints, &res)) != 0){
#ifdef WIN32
    print_if_error(status);
#else
    printf("getaddrinfo error: %s\n", gai_strerror(status));
#endif
    exit(1);
  };

  addrinfo* r = res;
  // return the first address in the linked list
  while (r) {	
    if (r->ai_family == AF_INET){
      inet_ntop(AF_INET,
          &(((sockaddr_in*)(r->ai_addr))->sin_addr.s_addr), 
          ip,
          sizeof(ip));
      //printf ("ipv4 = %s, port type = %d, socket = %d\n",
      //		ip, r->ai_socktype,  
      //		htons(((sockaddr_in*)(r->ai_addr))->sin_port));
      result = ip;
      return result;
    }else if (r->ai_family == AF_INET6){
      inet_ntop(AF_INET6,
          &(((sockaddr_in6*)(r->ai_addr))->sin6_addr.s6_addr), 
          ip,
          sizeof(ip));
      //printf ("ipv6 = %s, port type = %d, socket = %d\n",
      //		ip, r->ai_socktype,  
      //		htons(((sockaddr_in6*)(r->ai_addr))->sin6_port));
          result = ip ;
          return result;
    }else{
      printf ("unkown ai_family = %d\n", res->ai_family);
      return "unknown ai_family";
    }
    r=r->ai_next;
  };
  return string("NO ADDRESSES FOUND!");
}

/**********test important setup routines **************/

//determine_remote_ip
// get access to sipp_sockethandler routine to test it
extern void determine_remote_ip();
// test ipv4 remote address 
TEST(sockethandler, determine_remote_ip_ipv4 ){
  initialize_sockets();

  char ipaddr[256] = "172.23.2.49:65432";
  // remote_host can only be empty if MODE_SERVER 
  // expects to get remote_host from commandline parameter, provide here manually
  strncpy(remote_host, ipaddr ,255);
  determine_remote_ip();
  // calls getaddrinfo which can fail without error messages visible to unit test
  // comment out determine_remote_ip and uncomment the following
  // to get details on error from getaddrinfo on windows
      //struct addrinfo   hints;
      //struct addrinfo * local_addr;
      //memset((char*)&hints, 0, sizeof(hints));
      //hints.ai_flags  = AI_PASSIVE;
      //hints.ai_family = PF_UNSPEC;
      //int rv;
      //if ((rv = getaddrinfo(remote_host,
      //                NULL,
      //                &hints,
      //                &local_addr)) != 0) {
      //  printf("Unknown remote host '%s'.\n err = %x\n", remote_host, rv);
      //  wchar_t* error = gai_strerror(rv);
      //  wprintf (L"%s\n",error);
      //}

  // test that globals should be set up
  EXPECT_STREQ("172.23.2.49", remote_host);  // removes port as part of processing
  EXPECT_EQ(65432,remote_port)<< "failed to set global";
  EXPECT_STREQ("172.23.2.49",remote_ip)<< "failed to set global";
  // sockaddr_storage remote_sockaddr 
  EXPECT_STREQ("172.23.2.49",get_inet_address(&remote_sockaddr)) << "sockethelper routine failed";
  EXPECT_STREQ(ipaddr,socket_to_ip_port_string(&remote_sockaddr).c_str()) << "sockethelper routine failed";
  EXPECT_STREQ("172.23.2.49",socket_to_ip_string(&remote_sockaddr).c_str()) << "sockethelper routine failed";
  EXPECT_EQ(65432,get_in_port(&remote_sockaddr)) << "sockethelper routine failed";

  cleanup_sockets();
}

// test ipv6 remote address
TEST(sockethandler, determine_remote_ip_ipv6 ){
  initialize_sockets();

  const char* ipaddr = "[fe80::7c34:8d1f:a328:ac82]:65432";
  // remote_host can only be empty if MODE_SERVER 
  // expect to get remote_host from commandline parameter, provide here manually
  strncpy(remote_host, ipaddr ,255);
  determine_remote_ip();

  EXPECT_STREQ("fe80::7c34:8d1f:a328:ac82", remote_host);  // removes port as part of processing
  EXPECT_EQ(65432,remote_port)<< "failed to set global";
  EXPECT_STREQ("fe80::7c34:8d1f:a328:ac82",remote_ip)<< "failed to set global";
  // sockaddr_storage remote_sockaddr
  EXPECT_STREQ("fe80::7c34:8d1f:a328:ac82",get_inet_address(&remote_sockaddr)) << "sockethelper routine failed";
  EXPECT_STREQ(ipaddr,socket_to_ip_port_string(&remote_sockaddr).c_str()) << "sockethelper routine failed";
  EXPECT_STREQ("fe80::7c34:8d1f:a328:ac82",socket_to_ip_string(&remote_sockaddr).c_str()) << "sockethelper routine failed";
  EXPECT_EQ(65432,get_in_port(&remote_sockaddr)) << "sockethelper routine failed";

  cleanup_sockets();
}


// get access to sipp_socket_handler.cpp determine_local_ip to test it.
extern void determine_local_ip();
// test setting of local_ip when no explicit address given
// 
// addrinfo*        local_addr_storage
// char             hostname[80]
// char             local_ip[40]
// sockaddr_storage local_sockaddr
//
// when no args specified, local_addr_storage just uses first 
// result from getaddrinfo which may not match local_sockaddr address
//   eg actual ip address vs loop around.
TEST(sockethandler, determine_local_ip_noargs ){
// test no local_ip specified 
  vector<string>::iterator it;
  initialize_sockets();
  //local_ip from commandline arg or taken from gethostname
  //no input for local_ip, could get ipv4 or ipv6
  // this is the most complicated to test since we dont specify input
  determine_local_ip();

  bool found = false;
  // get all possible expected IP addresses and check that local_ip is one of them
  vector<string> all_local_ip = getall_local_ip(AF_UNSPEC);
  string local_ip_string = string(local_ip);
  int matchindex = -1;
  int index =0;
  for(it=all_local_ip.begin();it<all_local_ip.end();it++){
    //printf("local_ip = %s, it = %s\n",local_ip_string.c_str(), it->c_str());
    //local_ip may contain ipv6 zone, local_ip contains *it string, 
    //"fe80::7c34:8d1f:a328:ac82%11".find("fe80::7c34:8d1f:a328:ac82")  is a match
    if(local_ip_string.find(*it) != string::npos){
      found = true;
      matchindex=index;
      break;
    }
    index++;
  }
  EXPECT_TRUE(found) << "local_ip should match one of the local machine ip address";

  // check that hostname was set 
  char expected_hostname[80];
  gethostname(expected_hostname,80);
  EXPECT_STREQ(expected_hostname,hostname);

  // determine_local_ip should populate local_addr_storage (call_socket bind)
  // one of the addresses in local_addr_storage should be our matchindex, search through them
  found=false;
  struct addrinfo* adr = local_addr_storage;
  do{
    //printf ("local_ip = %s\t", all_local_ip[matchindex].c_str());
    //printf("address = %s\n", get_inet_address((sockaddr_storage*)adr->ai_addr));
    string a_local_address(get_inet_address((sockaddr_storage*)adr->ai_addr));
    if ((a_local_address).find(all_local_ip[matchindex]) != string::npos){
      found=true;
      break;
    }
    adr = adr->ai_next;
  }while (adr);
  EXPECT_TRUE(found) << "determine_local_ip did not populate local_addr_storage with expected local ip address";

  //local_sockaddr (main_socket bind), only set from gethostname if local_ip is set.
  //otherwise only the ss_family is set 
  if (local_sockaddr.ss_family == AF_INET6){
    EXPECT_TRUE('[' == local_ip_escaped[0]);
    EXPECT_TRUE(']' == local_ip_escaped[strlen(local_ip_escaped)-1]);
  }else{
    EXPECT_STREQ(local_ip, local_ip_escaped) <<"not ipv6, no difference should exist"; 
  }

  cleanup_sockets();
}


// addrinfo*        local_addr_storage
// char             hostname[80]
// char             local_ip[40]
// sockaddr_storage local_sockaddr
// test local_ip when ipv4 address specified
TEST(sockethandler, determine_local_ip_ipv4 ){
  initialize_sockets();
/*********** test local_ip specified as ipv4***********************/
  //set local_ip to one of the ipv4 local addresses
  string local_ip_string = get_local_ip(AF_INET);
  strncpy(local_ip, local_ip_string.c_str(),40);
  determine_local_ip();
  // did global hostname get updated
  char expected_hostname[80];
  gethostname(expected_hostname,80);
  EXPECT_STREQ(expected_hostname,hostname);
  // did global local_ip retain update
  EXPECT_STREQ(local_ip_string.c_str(),local_ip) << "determin_local_ip should rewrite local_ip back to value or substring of value that was set";
  // did global local_sockaddr get updated (used for binding to main_socket, which does accept)
  EXPECT_STREQ(local_ip_string.c_str(), get_inet_address(&local_sockaddr))<<"local_sockaddr should have local hosts ip address";
  EXPECT_STREQ(local_ip_string.c_str(), socket_to_ip_string(&local_sockaddr).c_str()) ;
  EXPECT_EQ(AF_INET, local_sockaddr.ss_family);
  // did global local_ip_is_ipv6 get updated 
  EXPECT_FALSE(local_ip_is_ipv6);
  // did global local_ip_escaped get updated
  EXPECT_STREQ(local_ip, local_ip_escaped) <<"ipv4 should have identical values";
  // did global local_addr_storage get updated (used for binding to call_socket, which we send from and is used in connect_socket_if_needed
  EXPECT_EQ(AF_INET,local_addr_storage->ai_family);
  EXPECT_STREQ(local_ip, get_inet_address((sockaddr_storage*)(local_addr_storage->ai_addr)));

  //printf("local_sockaddr ip = %s\n", socket_to_ip_port_string(&local_sockaddr).c_str());
  //printf("addr->ai_addr  ip = %s\n", socket_to_ip_port_string((sockaddr_storage*)local_addr_storage->ai_addr).c_str());
  bool local_sockaddr_matches_local_addr_storage = 
          is_in_addr_equal(&local_sockaddr, (sockaddr_storage*)local_addr_storage->ai_addr);
  EXPECT_TRUE(local_sockaddr_matches_local_addr_storage) << "determine_local_ip did not set up local_sockaddr and local_addr_storage to consistent addresses";
  
  cleanup_sockets();
}

//test local_ip when ipv6 address specified
TEST(sockethandler, determine_local_ip_ipv6 ){
  initialize_sockets();
/*********** test local_ip specified as ipv6***********************/
  //set local_ip to one of hte ipv6 local addresses
  // this works if test machines have valid dns entry
  // string local_ipv6_string = get_local_ip(AF_INET6);
  //change to hardcoded ::1  to accomodate testing on bacon
  string local_ipv6_string = string("::1");
  strncpy(local_ip, local_ipv6_string.c_str(),40);

  if (gethostname(hostname,64) != 0){
    perror("hostname failed:");
  }
  struct addrinfo * tlocal_addr;
  struct addrinfo   thints;
    memset((char*)&thints, 0, sizeof(thints));
    thints.ai_flags  = AI_PASSIVE;
    thints.ai_family = PF_INET6;
    if (getaddrinfo(hostname, NULL, &thints, &tlocal_addr) != 0) {
      perror("getaddrinfo failed:");
      printf("Can't get local IP address in getaddrinfo, local_host='%s', local_ip='%s'",
                   hostname,
                   local_ip);
    }

  determine_local_ip();
  // did global hostname get updated
  char expected_hostname[80];
  gethostname(expected_hostname,80);
  EXPECT_STREQ(expected_hostname,hostname);
  // did local_ip retain input
  EXPECT_STREQ(local_ipv6_string.c_str(),local_ip) << "determin_local_ip should rewrite local_ip back to value or substring of value that was set";
// did global local_sockaddr get updated (used for binding to main_socket, which does accept)
  EXPECT_STREQ(local_ipv6_string.c_str(), get_inet_address(&local_sockaddr))<<"local_sockaddr should have local hosts ip address";
  EXPECT_STREQ(local_ipv6_string.c_str(), socket_to_ip_string(&local_sockaddr).c_str()) ;
  EXPECT_EQ(AF_INET6, local_sockaddr.ss_family);
// did global local_ip_is_ipv6 get updated 
  EXPECT_TRUE(local_ip_is_ipv6);
//did local_ip_escaped address get square brackets
  char* extract = strndup(local_ip_escaped+1,strlen(local_ip_escaped)-2);
  EXPECT_STREQ(local_ip, extract);
  EXPECT_EQ ('[', local_ip_escaped[0]);
  EXPECT_EQ (']', local_ip_escaped[strlen(local_ip_escaped)-1]);
// did global local_addr_storage get updated (used for binding to call_socket, which we send from and is used in connect_socket_if_needed
  EXPECT_EQ(AF_INET6,local_addr_storage->ai_family);
  EXPECT_STREQ(local_ip, get_inet_address((sockaddr_storage*)(local_addr_storage->ai_addr)));

  //printf("local_sockaddr ip = %s\n", socket_to_ip_port_string(&local_sockaddr).c_str());
  //printf("addr->ai_addr  ip = %s\n", socket_to_ip_port_string((sockaddr_storage*)local_addr_storage->ai_addr).c_str());
  bool local_sockaddr_matches_local_addr_storage = 
          is_in_addr_equal(&local_sockaddr, (sockaddr_storage*)local_addr_storage->ai_addr);
  EXPECT_TRUE(local_sockaddr_matches_local_addr_storage) << "determine_local_ip did not set up local_sockaddr and local_addr_storage to consistent addresses";

  cleanup_sockets();
}


/******************* sipp_allocate_socket ************************/
// focus on testing this method and required support methods
// test all combinations of ipv6(true, false) and transport(udp,tcp,default)
// verify sipp_socket contents(int,int) 
// verify globals are updated appropriately
// verify we can bind to socket and sipp_socket and globalsparm updated
// verify we can close socket over socket and sipp_socket and globalsparm updated
// verify sipp_allocate_socket (int,int,bool) - called directly with accepting=true

//sipp_allocate_socket only allocates sipp_socket and takes a socket as input
//sockets are generally created by socket_fd 


/****************sockethandler routines ***********************/
// 
//for the all combinations of inputs (af,transport)
//  initialize
//  socket_fd
//  sipp_allocate_socket - 3parm,4parm version
//  sipp_bind_socket
//  sipp_close_socket - sipp_invalidate_socket
//  cleanup
//
// This tests uses local_ip = null which creates local address of
//   0.0.0.0   or :: 
TEST(sockethandler, allocate_noaddr_notipv6_DFLT_TRANSPORT){
  initialize_sockets();

  //note socket_fd maps
  //                      0          1            2           3 
  //sipp transport_type   T_UDP      T_TCP        T_TLS
  //socket_type           SOCK_DGRAM SOCK_STREAM  none
  //sipp uses DEFAULT_TRANSPORT = T_UDP = 0 
  int  proto = 0;  // all sockets created in sipp just use this default
  int  sipp_transport_type = DEFAULT_TRANSPORT; // T_UDP, T_TCP, T_TLS
  bool ipv6 = false;
  int  address_domain = (ipv6 ? AF_INET6 : AF_INET);
  int  socket_type = ((sipp_transport_type == T_UDP) ? SOCK_DGRAM : SOCK_STREAM);
  
  /****test socket_fd correctly creates socket *******/
  SOCKREF sock_fd = socket_fd(ipv6,sipp_transport_type);
  if (sock_fd == INVALID_SOCKET)
#ifdef WIN32
  ASSERT_NE(INVALID_SOCKET, sock_fd) << WSAGetLastError();
#else
  //if (sock_fd == -1)
  {
    perror("failed to create socket");
    ASSERT_NE(-1, sock_fd) << "failed to create socket";
  }
#endif
  
  //test sipp_allocate_socket using socket from sock_fd, verify sipp_socket 
  EXPECT_EQ(0,pollnfds) << "pollnfds should have zero value before any sockets are allocated";
  struct sipp_socket* ss = sipp_allocate_socket(ipv6, sipp_transport_type, sock_fd, proto);
  ASSERT_TRUE (ss != NULL)       << "failed to allocate sipp_socket structure";
  EXPECT_EQ(1, ss->ss_count);
  EXPECT_EQ(false, ss->ss_ipv6);
  EXPECT_EQ(sipp_transport_type, ss->ss_transport) ;
  EXPECT_EQ(false, ss->ss_control) ;
  EXPECT_EQ(false, ss->ss_call_socket) << "This should not be marked as a call socket";
  EXPECT_EQ(false, ss->ss_changed_dest) << "No destination was changed, should be false";
  EXPECT_EQ(sock_fd, ss->ss_fd) << "sipp_socket does not have socket we passed in";
  EXPECT_EQ(NULL, ss->ss_comp_state);
#ifdef _USE_OPENSSL
  EXPECT_EQ(NULL, ss->ss_ssl) << "No SSL objects should exist";
  EXPECT_EQ(NULL, ss->ss_bio) << "No BIO objet should exist";
#endif
  EXPECT_EQ(false,ss->ss_congested);
  EXPECT_EQ(false,ss->ss_invalid);
  EXPECT_EQ(NULL,ss->ss_in);
  EXPECT_EQ((size_t)0,ss->ss_msglen);
  EXPECT_EQ(NULL,ss->ss_out);

  // check globals are updated
  EXPECT_EQ(1, pollnfds) << "pollnfds should have been incremented by sipp_allocate_sokcet";
  EXPECT_EQ(pollnfds-1,ss->ss_pollidx) << "pollidx should be set to one less than current pollnfds";
  EXPECT_EQ(ss, sockets[ss->ss_pollidx])<< "socket array should have this socket at position ss_pollidx";

  // prepare to call sipp_bind_socket 
  // normally done by determine_local_ip but explictly broken out here to
  // isolate testing of use of local_addr_storage vs creation
  // get local address and bind to the socket
  struct addrinfo hints;
  struct addrinfo *localaddr;
  memset (&hints, 0, sizeof(hints));
  hints.ai_family = address_domain;  
  hints.ai_socktype = socket_type;   
  hints.ai_flags = AI_PASSIVE;  //use my ip  AI_NUMERICHOST
  const char* myport = "6111";
  int portnumber = atoi(myport);
  const char*local_ip = NULL;

  // localaddr to be used for binding
  int rv = getaddrinfo(local_ip, myport , &hints, &localaddr);
  EXPECT_EQ(0, rv) << gai_strerror(rv);
  // get the address out of localaddr result 
  struct in_addr* sinaddr =  &(((sockaddr_in*)(localaddr->ai_addr))->sin_addr);
  char addrstr[INET_ADDRSTRLEN];
  inet_ntop(localaddr->ai_family, sinaddr,addrstr, INET_ADDRSTRLEN);
  EXPECT_STREQ("0.0.0.0", addrstr) << "Setting up in_addr incorrectly";
  // get the port out of localaddr
  unsigned short theport = htons(((sockaddr_in*)(localaddr->ai_addr))->sin_port);
  EXPECT_EQ(portnumber,theport)<< "Setting up in_addr port incorrectly";
  
  //side track, test is_in_addr_equal while we have data to use
  struct sockaddr_storage saddr;
  memset(&saddr, 0, sizeof(struct sockaddr_storage));
  EXPECT_TRUE(is_in_addr_equal(&saddr,&saddr));
  EXPECT_TRUE(is_in_addr_equal( (sockaddr_storage*)(localaddr->ai_addr),
                                (sockaddr_storage*)(localaddr->ai_addr) ) );
  EXPECT_FALSE(is_in_addr_equal( &saddr ,
                                (sockaddr_storage*)(localaddr->ai_addr) ) );

  //test sipp_bind_socket
  int port;
  int rc = sipp_bind_socket(ss,(sockaddr_storage*)(localaddr->ai_addr),&port);
  EXPECT_EQ(portnumber, port);
  EXPECT_EQ(0,rc)<<"sipp_bind_socket failed to bind";

  // does the created socket have the right parameters?
  // getsockname only works after connect or bind 
  struct sockaddr addr;
  socklen_t namelen = sizeof (addr);
  EXPECT_EQ(0,getsockname(sock_fd, &addr,&namelen)) << "Failed to retreive sockaddr from socket";
  EXPECT_EQ(address_domain, addr.sa_family) << "socket address family doesnt match input to fd_socket";
  //exercise  socket_helper function as well as verify our socket is as expected
  EXPECT_EQ(portnumber, (int) get_in_port((struct sockaddr_storage*)&addr))<<"socket port number should match";
  EXPECT_STREQ("0.0.0.0", socket_to_ip_string((struct sockaddr_storage*)&addr).c_str());
  char dst[INET6_ADDRSTRLEN];
  inet_ntop(addr.sa_family,get_in_addr((struct sockaddr_storage*)&addr), dst, sizeof(dst));
  EXPECT_STREQ("0.0.0.0",dst) <<  "get_in_addr did not return expected address string";

  sipp_close_socket(ss);
  EXPECT_EQ(0,pollnfds) << "sipp_close_socket should have decremented pollnfds";

  cleanup_sockets();
}

//socket_fd
//sipp_allocate_socket
//sipp_bind_socket
//sipp_close_socket
TEST(sockethandler, allocate_noaddr_ipv6_DFLT_TRANSPORT){
  initialize_sockets();

  int  proto = 0;  // all sockets created in sipp just use this default
  int  sipp_transport_type = DEFAULT_TRANSPORT; // T_UDP, T_TCP, T_TLS
  bool ipv6 = true;
  int  address_domain = (ipv6 ? AF_INET6 : AF_INET);
  int  socket_type = ((sipp_transport_type == T_UDP) ? SOCK_DGRAM : SOCK_STREAM);

  // make a socket
  SOCKREF sock_fd = socket_fd(ipv6,sipp_transport_type);
  if (sock_fd == INVALID_SOCKET)
#ifdef WIN32
  ASSERT_NE(INVALID_SOCKET, sock_fd) << WSAGetLastError();
#else
  {
    perror("failed to create socket");
    ASSERT_NE(-1, sock_fd) << "failed to create socket";
  }
#endif
  
  //test sipp_allocate_socket using socket from sock_fd, verify sipp_socket structure
  EXPECT_EQ(0,pollnfds) << "pollnfds should have zero value before any sockets are allocated";
  struct sipp_socket* ss = sipp_allocate_socket(ipv6, sipp_transport_type, sock_fd, proto);
  ASSERT_TRUE (ss != NULL)       << "failed to allocate sipp_socket structure";
  EXPECT_EQ(1, ss->ss_count);
  EXPECT_EQ(true, ss->ss_ipv6);
  EXPECT_EQ(sipp_transport_type, ss->ss_transport) ;
  EXPECT_EQ(false, ss->ss_control) ;
  EXPECT_EQ(false, ss->ss_call_socket) << "This should not be marked as a call socket";
  EXPECT_EQ(false, ss->ss_changed_dest) << "No destination was changed, should be false";
  EXPECT_EQ(sock_fd, ss->ss_fd) << "sipp_socket does not have socket we passed in";
  EXPECT_EQ(NULL, ss->ss_comp_state);
#ifdef _USE_OPENSSL
  // important to set compiler flag here to match unit under test or sipp_socket structure will be wrong
  EXPECT_EQ(NULL, ss->ss_ssl) << "No SSL objects should exist";
  EXPECT_EQ(NULL, ss->ss_bio) << "No BIO objet should exist";
#endif
  EXPECT_EQ(false,ss->ss_congested);
  EXPECT_EQ(false,ss->ss_invalid);
  EXPECT_EQ(NULL,ss->ss_in);
  EXPECT_EQ((size_t)0,ss->ss_msglen);
  EXPECT_EQ(NULL,ss->ss_out);
  // check that globals are updated as well
  EXPECT_EQ(1, pollnfds) << "pollnfds should have been incremented by sipp_allocate_sokcet";
  EXPECT_EQ(pollnfds-1,ss->ss_pollidx) << "pollidx should be set to one less than current pollnfds";
  EXPECT_EQ(ss, sockets[ss->ss_pollidx])<< "socket array should have this socket at position ss_pollidx";

  //prepare to call sipp_bind_socket 
  //get local address and bind to the socket
  struct addrinfo hints;
  struct addrinfo *localaddr;
  memset (&hints, 0, sizeof(hints));
  hints.ai_family = address_domain;  
  hints.ai_socktype = socket_type;   
  hints.ai_flags = AI_PASSIVE;  //use my ip  AI_NUMERICHOST
  const char* myport = "6111";
  int portnumber = atoi(myport);
  const char*local_ip = NULL;

  // localaddr to be used for binding
  int rv = getaddrinfo(local_ip, myport , &hints, &localaddr);
  EXPECT_EQ(0, rv) << gai_strerror(rv);
  // get the address out of localaddr result 
  struct in6_addr* sinaddr =  &(((sockaddr_in6*)(localaddr->ai_addr))->sin6_addr);
  char addrstr[INET6_ADDRSTRLEN];
  inet_ntop(localaddr->ai_family, sinaddr,addrstr, INET6_ADDRSTRLEN);
  // ipv6 '::'  is all zeros ip address
  EXPECT_STREQ("::", addrstr) << "Setting up in_addr incorrectly";
  // get the port out of localaddr
  unsigned short theport = htons(((sockaddr_in6*)(localaddr->ai_addr))->sin6_port);
  EXPECT_EQ(portnumber,theport)<< "Setting up in_addr port incorrectly";
  
  //side track, test is_in_addr_equal while we have data to use
  struct sockaddr_storage saddr;
  memset(&saddr, 0, sizeof(struct sockaddr_storage));
  EXPECT_TRUE(is_in_addr_equal(&saddr,&saddr));
  EXPECT_TRUE(is_in_addr_equal( (sockaddr_storage*)(localaddr->ai_addr),
                                (sockaddr_storage*)(localaddr->ai_addr) ) );
  EXPECT_FALSE(is_in_addr_equal( &saddr ,
                                (sockaddr_storage*)(localaddr->ai_addr) ) );
  
  // bind to socket
  int port;
  int rc = sipp_bind_socket(ss,(sockaddr_storage*)(localaddr->ai_addr),&port);
  //printf("port = %d\n", port);
  EXPECT_EQ(portnumber, port);
  EXPECT_EQ(0,rc)<<"sipp_bind_socket failed to bind";

  // does the created socket have the right parameters?
  // getsockname only works after connect or bind 
  struct sockaddr_storage addr;
  socklen_t namelen = sizeof (addr);
  EXPECT_EQ(0,getsockname(sock_fd, (sockaddr*) &addr,&namelen)) << "Failed to retreive sockaddr from socket";
  EXPECT_EQ(address_domain, ((sockaddr_in6*)&addr)->sin6_family) << "socket address family doesnt match input to fd_socket";
  //exercise  socket_helper function as well as verify our socket is as expected
  EXPECT_EQ(portnumber, (int) get_in_port((struct sockaddr_storage*)&addr))<<"socket port number should match";
  EXPECT_STREQ("::", socket_to_ip_string((struct sockaddr_storage*)&addr).c_str());
  char dst[INET6_ADDRSTRLEN];
  inet_ntop(addr.ss_family,get_in_addr((struct sockaddr_storage*)&addr), dst, sizeof(dst));
  EXPECT_STREQ("::",dst) <<  "get_in_addr did not return expected address string";
  

  sipp_close_socket(ss);
  EXPECT_EQ(0,pollnfds);

  cleanup_sockets();

}

//socket_fd
//sipp_allocate_socket
//sipp_bind_socket
//sipp_close_socket
TEST(sockethandler, allocate_notipv6_T_UDP){
  initialize_sockets();
  int  proto = 0;  // all sockets created in sipp just use this default
  int  sipp_transport_type = T_UDP; // T_UDP, T_TCP, T_TLS
  bool ipv6 = false;
  int  address_domain = (ipv6 ? AF_INET6 : AF_INET);
  int  socket_type = ((sipp_transport_type == T_UDP) ? SOCK_DGRAM : SOCK_STREAM);
  
  //test socket_fd correctly creates socket 
  SOCKREF sock_fd = socket_fd(ipv6,sipp_transport_type);
  if (sock_fd == INVALID_SOCKET)
#ifdef WIN32
  ASSERT_NE(INVALID_SOCKET, sock_fd) << WSAGetLastError();
#else
  //if (sock_fd == -1)
  {
    perror("failed to create socket");
    ASSERT_NE(-1, sock_fd) << "failed to create socket";
  }
#endif

  EXPECT_EQ(0,pollnfds) << "pollnfds should have zero value before any sockets are allocated";
  /*****test sipp_allocate_socket using socket from sock_fd, verify sipp_socket structure ******/

  struct sipp_socket* ss = sipp_allocate_socket(ipv6, sipp_transport_type, sock_fd, proto);
  ASSERT_TRUE (ss != NULL)       << "failed to allocate sipp_socket structure";
  EXPECT_EQ(1, ss->ss_count);
  EXPECT_EQ(false, ss->ss_ipv6);
  EXPECT_EQ(sipp_transport_type, ss->ss_transport) ;
  EXPECT_EQ(false, ss->ss_control) ;
  EXPECT_EQ(false, ss->ss_call_socket) << "This should not be marked as a call socket";
  EXPECT_EQ(false, ss->ss_changed_dest) << "No destination was changed, should be false";
  EXPECT_EQ(sock_fd, ss->ss_fd) << "sipp_socket does not have socket we passed in";
  EXPECT_EQ(NULL, ss->ss_comp_state);
#ifdef _USE_OPENSSL
  EXPECT_EQ(NULL, ss->ss_ssl) << "No SSL objects should exist";
  EXPECT_EQ(NULL, ss->ss_bio) << "No BIO objet should exist";
#endif
  EXPECT_EQ(false,ss->ss_congested);
  EXPECT_EQ(false,ss->ss_invalid);
  EXPECT_EQ(NULL,ss->ss_in);
  EXPECT_EQ((size_t)0,ss->ss_msglen);
  EXPECT_EQ(NULL,ss->ss_out);

  // check globals are updated
  EXPECT_EQ(1, pollnfds) << "pollnfds should have been incremented by sipp_allocate_sokcet";
  EXPECT_EQ(pollnfds-1,ss->ss_pollidx) << "pollidx should be set to one less than current pollnfds";
  EXPECT_EQ(ss, sockets[ss->ss_pollidx])<< "socket array should have this socket at position ss_pollidx";

  //prepare to call sipp_bind_socket 
  //get local address and bind to the socket
  struct addrinfo hints;
  struct addrinfo *localaddr;
  memset (&hints, 0, sizeof(hints));
  hints.ai_family = address_domain;  
  hints.ai_socktype = socket_type;   
  hints.ai_flags = AI_PASSIVE;  //use my ip  AI_NUMERICHOST
  const char* myport = "6111";
  int portnumber = atoi(myport);
  const char*local_ip = NULL;

  // localaddr to be used for binding
  int rv = getaddrinfo(local_ip, myport , &hints, &localaddr);
  EXPECT_EQ(0, rv) << gai_strerror(rv);
  // get the address out of localaddr result 
  struct in_addr* sinaddr =  &(((sockaddr_in*)(localaddr->ai_addr))->sin_addr);
  char addrstr[INET_ADDRSTRLEN];
  inet_ntop(localaddr->ai_family, sinaddr,addrstr, INET_ADDRSTRLEN);
  EXPECT_STREQ("0.0.0.0", addrstr) << "Setting up in_addr incorrectly";
  // get the port out of localaddr
  unsigned short theport = htons(((sockaddr_in*)(localaddr->ai_addr))->sin_port);
  EXPECT_EQ(portnumber,theport)<< "Setting up in_addr port incorrectly";
  
  //side track, test socket_helper.cpp:is_in_addr_equal while we have data to use
  struct sockaddr_storage saddr;
  memset(&saddr, 0, sizeof(struct sockaddr_storage));
  EXPECT_TRUE(is_in_addr_equal(&saddr,&saddr));
  EXPECT_TRUE(is_in_addr_equal( (sockaddr_storage*)(localaddr->ai_addr),
                                (sockaddr_storage*)(localaddr->ai_addr) ) );
  EXPECT_FALSE(is_in_addr_equal( &saddr ,
                                (sockaddr_storage*)(localaddr->ai_addr) ) );

  // test sipp_bind_socket 
  int port;
  int rc = sipp_bind_socket(ss,(sockaddr_storage*)(localaddr->ai_addr),&port);
  EXPECT_EQ(portnumber, port);
  EXPECT_EQ(0,rc)<<"sipp_bind_socket failed to bind";
  struct sockaddr addr;
  socklen_t namelen = sizeof (addr);
  // note getsockname only works after connect or bind
  EXPECT_EQ(0,getsockname(sock_fd, &addr,&namelen)) << "Failed to retreive sockaddr from socket";
  EXPECT_EQ(address_domain, addr.sa_family) << "socket address family doesnt match input to fd_socket";
  //exercise  socket_helper function as well as verify our socket is as expected
  EXPECT_EQ(portnumber, (int) get_in_port((struct sockaddr_storage*)&addr))<<"socket port number should match";
  EXPECT_STREQ("0.0.0.0", socket_to_ip_string((struct sockaddr_storage*)&addr).c_str());
  char dst[INET6_ADDRSTRLEN];
  inet_ntop(addr.sa_family,get_in_addr((struct sockaddr_storage*)&addr), dst, sizeof(dst));
  EXPECT_STREQ("0.0.0.0",dst) <<  "get_in_addr did not return expected address string";

  sipp_close_socket(ss);
  //sipp_socket is freed, ss data is no longer be valid, nothing to check
  EXPECT_EQ(0,pollnfds) << "sipp_close_socket should have decremented pollnfds";

  cleanup_sockets();

}

//test methods and verify globals
//sock_fd
//sipp_allocate_socket
//sipp_bind_socket
TEST(sockethandler, allocate_ipv6_T_UDP){
  initialize_sockets();
  int  proto = 0;  // all sockets created in sipp just use this default
  int  sipp_transport_type = T_UDP; // T_UDP, T_TCP, T_TLS
  bool ipv6 = true;
  int  address_domain = (ipv6 ? AF_INET6 : AF_INET);
  int  socket_type = ((sipp_transport_type == T_UDP) ? SOCK_DGRAM : SOCK_STREAM);

  // make a socket
  SOCKREF sock_fd = socket_fd(ipv6,sipp_transport_type);
  if (sock_fd == INVALID_SOCKET)
#ifdef WIN32
  ASSERT_NE(INVALID_SOCKET, sock_fd) << WSAGetLastError();
#else
  {
    perror("failed to create socket");
    ASSERT_NE(-1, sock_fd) << "failed to create socket";
  }
#endif

  // fill out sipp_socket structure and associate with socket
  EXPECT_EQ(0,pollnfds) << "pollnfds should have zero value before any sockets are allocated";
/*****test sipp_allocate_socket using socket from sock_fd, verify sipp_socket structure ******/
  struct sipp_socket* ss = sipp_allocate_socket(ipv6, sipp_transport_type, sock_fd, proto);
  ASSERT_TRUE (ss != NULL)       << "failed to allocate sipp_socket structure";
  EXPECT_EQ(1, ss->ss_count);
  EXPECT_EQ(true, ss->ss_ipv6);
  EXPECT_EQ(sipp_transport_type, ss->ss_transport) ;
  EXPECT_EQ(false, ss->ss_control) ;
  EXPECT_EQ(false, ss->ss_call_socket) << "This should not be marked as a call socket";
  EXPECT_EQ(false, ss->ss_changed_dest) << "No destination was changed, should be false";
  EXPECT_EQ(sock_fd, ss->ss_fd) << "sipp_socket does not have socket we passed in";
  EXPECT_EQ(NULL, ss->ss_comp_state);
#ifdef _USE_OPENSSL
  // important to set compiler flag here to match unit under test or sipp_socket structure will be wrong
  EXPECT_EQ(NULL, ss->ss_ssl) << "No SSL objects should exist";
  EXPECT_EQ(NULL, ss->ss_bio) << "No BIO objet should exist";
#endif
  EXPECT_EQ(false,ss->ss_congested);
  EXPECT_EQ(false,ss->ss_invalid);
  EXPECT_EQ(NULL,ss->ss_in);
  EXPECT_EQ((size_t)0,ss->ss_msglen);
  EXPECT_EQ(NULL,ss->ss_out);
  // check that globals are updated as well
  EXPECT_EQ(1, pollnfds) << "pollnfds should have been incremented by sipp_allocate_sokcet";
  EXPECT_EQ(pollnfds-1,ss->ss_pollidx) << "pollidx should be set to one less than current pollnfds";
  EXPECT_EQ(ss, sockets[ss->ss_pollidx])<< "socket array should have this socket at position ss_pollidx";

  /**** prepare to call sipp_bind_socket ***********/
  //get local address and bind to the socket
  struct addrinfo hints;
  struct addrinfo *localaddr;
  memset (&hints, 0, sizeof(hints));
  hints.ai_family = address_domain;  
  hints.ai_socktype = socket_type;   
  hints.ai_flags = AI_PASSIVE;  //use my ip  AI_NUMERICHOST
  const char* myport = "6111";
  int portnumber = atoi(myport);
  const char*local_ip = NULL;

  // localaddr to be used for binding
  int rv = getaddrinfo(local_ip, myport , &hints, &localaddr);
  EXPECT_EQ(0, rv) << gai_strerror(rv);
  // get the address out of localaddr result 
  struct in6_addr* sinaddr =  &(((sockaddr_in6*)(localaddr->ai_addr))->sin6_addr);
  char addrstr[INET6_ADDRSTRLEN];
  inet_ntop(localaddr->ai_family, sinaddr,addrstr, INET6_ADDRSTRLEN);
  //printf ("addr = %s\n", addrstr);
  // ipv6 '::'  is all zeros ip address
  EXPECT_STREQ("::", addrstr) << "Setting up in_addr incorrectly";
  // get the port out of localaddr
  unsigned short theport = htons(((sockaddr_in6*)(localaddr->ai_addr))->sin6_port);
  //printf ("the port = %d", theport);
  EXPECT_EQ(portnumber,theport)<< "Setting up in_addr port incorrectly";
  
  //side track, test is_in_addr_equal while we have data to use
  struct sockaddr_storage saddr;
  memset(&saddr, 0, sizeof(struct sockaddr_storage));
  EXPECT_TRUE(is_in_addr_equal(&saddr,&saddr));
  EXPECT_TRUE(is_in_addr_equal( (sockaddr_storage*)(localaddr->ai_addr),
                                (sockaddr_storage*)(localaddr->ai_addr) ) );
  EXPECT_FALSE(is_in_addr_equal( &saddr ,
                                (sockaddr_storage*)(localaddr->ai_addr) ) );

  int port;
  /****** test sipp_bind_socket ********/
  int rc = sipp_bind_socket(ss,(sockaddr_storage*)(localaddr->ai_addr),&port);
  //printf("port = %d\n", port);
  EXPECT_EQ(portnumber, port);
  EXPECT_EQ(0,rc)<<"sipp_bind_socket failed to bind";

  // does the created socket have the right parameters?
  // getsockname only works after connect or bind 
  struct sockaddr_storage addr;
  socklen_t namelen = sizeof (addr);
  EXPECT_EQ(0,getsockname(sock_fd, (sockaddr*) &addr,&namelen)) << "Failed to retreive sockaddr from socket";
  EXPECT_EQ(address_domain, ((sockaddr_in6*)&addr)->sin6_family) << "socket address family doesnt match input to fd_socket";
  //exercise  socket_helper function as well as verify our socket is as expected
  EXPECT_EQ(portnumber, (int) get_in_port((struct sockaddr_storage*)&addr))<<"socket port number should match";
  EXPECT_STREQ("::", socket_to_ip_string((struct sockaddr_storage*)&addr).c_str());
  char dst[INET6_ADDRSTRLEN];
  inet_ntop(addr.ss_family,get_in_addr((struct sockaddr_storage*)&addr), dst, sizeof(dst));
  EXPECT_STREQ("::",dst) <<  "get_in_addr did not return expected address string";
  
  /***** test sipp_close_socket deallocates sipp_socket ****/
  // this also exercises  sipp_socket_invalidate
  sipp_close_socket(ss);
  EXPECT_EQ(0,pollnfds);

  cleanup_sockets();

}

TEST(sockethandler, allocate_notipv6_T_TCP){
  initialize_sockets();
  int  proto = 0;  // all sockets created in sipp just use this default
  int  sipp_transport_type = T_TCP; // T_UDP, T_TCP, T_TLS
  bool ipv6 = false;
  
  int  address_domain = (ipv6 ? AF_INET6 : AF_INET);
  int  socket_type = ((sipp_transport_type == T_UDP) ? SOCK_DGRAM : SOCK_STREAM);
  //printf("socket_type = %d\n",socket_type);
  
  /****test socket_fd correctly creates socket *******/
  SOCKREF sock_fd = socket_fd(ipv6,sipp_transport_type);
  if (sock_fd == INVALID_SOCKET)
#ifdef WIN32
  ASSERT_NE(INVALID_SOCKET, sock_fd) << WSAGetLastError();
#else
  //if (sock_fd == -1)
  {
    perror("failed to create socket");
    ASSERT_NE(-1, sock_fd) << "failed to create socket";
  }
#endif

  EXPECT_EQ(0,pollnfds) << "pollnfds should have zero value before any sockets are allocated";
  /*****test sipp_allocate_socket using socket from sock_fd, verify sipp_socket structure ******/

  struct sipp_socket* ss = sipp_allocate_socket(ipv6, sipp_transport_type, sock_fd, proto);
  ASSERT_TRUE (ss != NULL)       << "failed to allocate sipp_socket structure";
  EXPECT_EQ(1, ss->ss_count);
  EXPECT_EQ(false, ss->ss_ipv6);
  EXPECT_EQ(sipp_transport_type, ss->ss_transport) ;
  EXPECT_EQ(false, ss->ss_control) ;
  EXPECT_EQ(false, ss->ss_call_socket) << "This should not be marked as a call socket";
  EXPECT_EQ(false, ss->ss_changed_dest) << "No destination was changed, should be false";
  EXPECT_EQ(sock_fd, ss->ss_fd) << "sipp_socket does not have socket we passed in";
  EXPECT_EQ(NULL, ss->ss_comp_state);
#ifdef _USE_OPENSSL
  EXPECT_EQ(NULL, ss->ss_ssl) << "No SSL objects should exist";
  EXPECT_EQ(NULL, ss->ss_bio) << "No BIO objet should exist";
#endif
  EXPECT_EQ(false,ss->ss_congested);
  EXPECT_EQ(false,ss->ss_invalid);
  EXPECT_EQ(NULL,ss->ss_in);
  EXPECT_EQ((size_t)0,ss->ss_msglen);
  EXPECT_EQ(NULL,ss->ss_out);

  // check globals are updated
  EXPECT_EQ(1, pollnfds) << "pollnfds should have been incremented by sipp_allocate_sokcet";
  EXPECT_EQ(pollnfds-1,ss->ss_pollidx) << "pollidx should be set to one less than current pollnfds";
  EXPECT_EQ(ss, sockets[ss->ss_pollidx])<< "socket array should have this socket at position ss_pollidx";

  // prepare to call sipp_bind_socket 
  //get local address and bind to the socket
  struct addrinfo hints;
  struct addrinfo *localaddr;
  memset (&hints, 0, sizeof(hints));
  hints.ai_family = address_domain;  
  hints.ai_socktype = socket_type;   
  hints.ai_flags = AI_PASSIVE;  //use my ip  AI_NUMERICHOST
  const char* myport = "6111";
  int portnumber = atoi(myport);
  const char*local_ip = NULL;

  // localaddr to be used for binding
  int rv = getaddrinfo(local_ip, myport , &hints, &localaddr);
  EXPECT_EQ(0, rv) << gai_strerror(rv);
  // get the address out of localaddr result 
  struct in_addr* sinaddr =  &(((sockaddr_in*)(localaddr->ai_addr))->sin_addr);
  char addrstr[INET_ADDRSTRLEN];
  inet_ntop(localaddr->ai_family, sinaddr,addrstr, INET_ADDRSTRLEN);
  //printf ("addr = %s\n", addrstr);
  EXPECT_STREQ("0.0.0.0", addrstr) << "Setting up in_addr incorrectly";
  // get the port out of localaddr
  unsigned short theport = htons(((sockaddr_in*)(localaddr->ai_addr))->sin_port);
  //printf ("the port = %d", theport);
  EXPECT_EQ(portnumber,theport)<< "Setting up in_addr port incorrectly";
  
  //side track, test is_in_addr_equal while we have data to use
  struct sockaddr_storage saddr;
  memset(&saddr, 0, sizeof(struct sockaddr_storage));
  EXPECT_TRUE(is_in_addr_equal(&saddr,&saddr));
  EXPECT_TRUE(is_in_addr_equal( (sockaddr_storage*)(localaddr->ai_addr),
                                (sockaddr_storage*)(localaddr->ai_addr) ) );
  EXPECT_FALSE(is_in_addr_equal( &saddr ,
                                (sockaddr_storage*)(localaddr->ai_addr) ) );

  int port;
  /****** test sipp_bind_socket ********/
  int rc = sipp_bind_socket(ss,(sockaddr_storage*)(localaddr->ai_addr),&port);
  print_if_error(rc);
  EXPECT_EQ(portnumber, port);
  EXPECT_EQ(0,rc)<<"sipp_bind_socket failed to bind";

  // does the created socket have the right parameters?
  // getsockname only works after connect or bind 
  struct sockaddr addr;
  socklen_t namelen = sizeof (addr);
  EXPECT_EQ(0,getsockname(sock_fd, &addr,&namelen)) << "Failed to retreive sockaddr from socket";
  EXPECT_EQ(address_domain, addr.sa_family) << "socket address family doesnt match input to fd_socket";
  //exercise  socket_helper function as well as verify our socket is as expected
  EXPECT_EQ(portnumber, (int) get_in_port((struct sockaddr_storage*)&addr))<<"socket port number should match";
  EXPECT_STREQ("0.0.0.0", socket_to_ip_string((struct sockaddr_storage*)&addr).c_str());
  char dst[INET6_ADDRSTRLEN];
  inet_ntop(addr.sa_family,get_in_addr((struct sockaddr_storage*)&addr), dst, sizeof(dst));
  EXPECT_STREQ("0.0.0.0",dst) <<  "get_in_addr did not return expected address string";

/**** setup remote socket address using loop around *******/
  const char* ipaddr = "127.0.0.1:65432";
  strncpy(remote_host, ipaddr ,255);
  determine_remote_ip();
  EXPECT_EQ(65432,remote_port)<< "failed to set global";
  EXPECT_STREQ("127.0.0.1",remote_ip)<< "failed to set global";

  /***** test sipp_close_socket deallocates sipp_socket ****/
  // this also exercises  sipp_socket_invalidate
  sipp_close_socket(ss);
  //sipp_socket is freed, ss data is no longer be valid, nothing to check
  EXPECT_EQ(0,pollnfds) << "sipp_close_socket should have decremented pollnfds";
  //todo is there a way to verify socket state is closed?

  cleanup_sockets();

}


//sock_fd
//sipp_allocate_socket
//sipp_bind_socket
TEST(sockethandler, allocate_ipv6_T_TCP){
  initialize_sockets();

  //note socket_fd maps
  //                      0          1            2           3 
  //sipp transport_type   T_UDP      T_TCP        T_TLS
  //socket_type           SOCK_DGRAM SOCK_STREAM  none
  //sipp uses DEFAULT_TRANSPORT = T_UDP = 0 
  int  proto = 0;  // all sockets created in sipp just use this default
  int  sipp_transport_type = T_TCP; // T_UDP, T_TCP, T_TLS
  bool ipv6 = true;
  int  address_domain = (ipv6 ? AF_INET6 : AF_INET);
  int  socket_type = ((sipp_transport_type == T_UDP) ? SOCK_DGRAM : SOCK_STREAM);

  // make a socket
  SOCKREF sock_fd = socket_fd(ipv6,sipp_transport_type);
  if (sock_fd == INVALID_SOCKET)
#ifdef WIN32
  ASSERT_NE(INVALID_SOCKET, sock_fd) << WSAGetLastError();
#else
  {
    perror("failed to create socket");
    ASSERT_NE(-1, sock_fd) << "failed to create socket";
  }
#endif

  // fill out sipp_socket structure and associate with socket
  EXPECT_EQ(0,pollnfds) << "pollnfds should have zero value before any sockets are allocated";
  // test sipp_allocate_socket using socket from sock_fd, verify sipp_socket structure
  struct sipp_socket* ss = sipp_allocate_socket(ipv6, sipp_transport_type, sock_fd, proto);
  ASSERT_TRUE (ss != NULL)       << "failed to allocate sipp_socket structure";
  EXPECT_EQ(1, ss->ss_count);
  EXPECT_EQ(true, ss->ss_ipv6);
  EXPECT_EQ(sipp_transport_type, ss->ss_transport) ;
  EXPECT_EQ(false, ss->ss_control) ;
  EXPECT_EQ(false, ss->ss_call_socket) << "This should not be marked as a call socket";
  EXPECT_EQ(false, ss->ss_changed_dest) << "No destination was changed, should be false";
  EXPECT_EQ(sock_fd, ss->ss_fd) << "sipp_socket does not have socket we passed in";
  EXPECT_EQ(NULL, ss->ss_comp_state);
#ifdef _USE_OPENSSL
  // set compiler flag here to match unit under test or sipp_socket structure will be wrong
  EXPECT_EQ(NULL, ss->ss_ssl) << "No SSL objects should exist";
  EXPECT_EQ(NULL, ss->ss_bio) << "No BIO objet should exist";
#endif
  EXPECT_EQ(false,ss->ss_congested);
  EXPECT_EQ(false,ss->ss_invalid);
  EXPECT_EQ(NULL,ss->ss_in);
  EXPECT_EQ((size_t)0,ss->ss_msglen);
  EXPECT_EQ(NULL,ss->ss_out);
  // check that globals are updated as well
  EXPECT_EQ(1, pollnfds) << "pollnfds should have been incremented by sipp_allocate_sokcet";
  EXPECT_EQ(pollnfds-1,ss->ss_pollidx) << "pollidx should be set to one less than current pollnfds";
  EXPECT_EQ(ss, sockets[ss->ss_pollidx])<< "socket array should have this socket at position ss_pollidx";

  //prepare to call sipp_bind_socket 
  //get local address and bind to the socket
  struct addrinfo hints;
  struct addrinfo *localaddr;
  memset (&hints, 0, sizeof(hints));
  hints.ai_family = address_domain;  
  hints.ai_socktype = socket_type;   
  hints.ai_flags = AI_PASSIVE;  //use my ip  AI_NUMERICHOST
  const char* myport = "6111";
  int portnumber = atoi(myport);
  const char*local_ip = NULL;

  // localaddr to be used for binding
  int rv = getaddrinfo(local_ip, myport , &hints, &localaddr);
  EXPECT_EQ(0, rv) << gai_strerror(rv);
  // get the address out of localaddr result 
  struct in6_addr* sinaddr =  &(((sockaddr_in6*)(localaddr->ai_addr))->sin6_addr);
  char addrstr[INET6_ADDRSTRLEN];
  inet_ntop(localaddr->ai_family, sinaddr,addrstr, INET6_ADDRSTRLEN);
  EXPECT_STREQ("::", addrstr) << "Setting up in_addr incorrectly";
  // get the port out of localaddr
  unsigned short theport = htons(((sockaddr_in6*)(localaddr->ai_addr))->sin6_port);
  EXPECT_EQ(portnumber,theport)<< "Setting up in_addr port incorrectly";
  
  //side track, test is_in_addr_equal while we have data to use
  struct sockaddr_storage saddr;
  memset(&saddr, 0, sizeof(struct sockaddr_storage));
  EXPECT_TRUE(is_in_addr_equal(&saddr,&saddr));
  EXPECT_TRUE(is_in_addr_equal( (sockaddr_storage*)(localaddr->ai_addr),
                                (sockaddr_storage*)(localaddr->ai_addr) ) );
  EXPECT_FALSE(is_in_addr_equal( &saddr ,
                                (sockaddr_storage*)(localaddr->ai_addr) ) );

  int port;
  /****** test sipp_bind_socket ********/
  int rc = sipp_bind_socket(ss,(sockaddr_storage*)(localaddr->ai_addr),&port);
  print_if_error(rc);
  EXPECT_EQ(portnumber, port);
  EXPECT_EQ(0,rc)<<"sipp_bind_socket failed to bind";

  // does the created socket have the right parameters?
  // getsockname only works after connect or bind 
  struct sockaddr_storage addr;
  socklen_t namelen = sizeof (addr);
  EXPECT_EQ(0,getsockname(sock_fd, (sockaddr*) &addr,&namelen)) << "Failed to retreive sockaddr from socket";
  EXPECT_EQ(address_domain, ((sockaddr_in6*)&addr)->sin6_family) << "socket address family doesnt match input to fd_socket";
  //exercise  socket_helper function as well as verify our socket is as expected
  EXPECT_EQ(portnumber, (int) get_in_port((struct sockaddr_storage*)&addr))<<"socket port number should match";
  EXPECT_STREQ("::", socket_to_ip_string((struct sockaddr_storage*)&addr).c_str());
  char dst[INET6_ADDRSTRLEN];
  inet_ntop(addr.ss_family,get_in_addr((struct sockaddr_storage*)&addr), dst, sizeof(dst));
  EXPECT_STREQ("::",dst) <<  "get_in_addr did not return expected address string";
  
  sipp_close_socket(ss);
  EXPECT_EQ(0,pollnfds);

  cleanup_sockets();

}





// helper routine used for debugging globals updating
void print_common()
{
  printf ("--------------------");
  printf ("pollnfds = %d\n",pollnfds);
  int i;
  for (i=0;i<SIPP_MAXFDS;i++){
    if (sockets[i] != NULL) {
      printf("i = %d,  count = %d, transport = %d, ipv6 = %s, ss_fd = %d, pollidx = %d\n",
        i, sockets[i]->ss_count, sockets[i]->ss_transport, 
        sockets[i]->ss_ipv6 ? "true":"false", 
        sockets[i]->ss_fd, sockets[i]->ss_pollidx);
    }
  }
}


// all combinations of inputs into new_sipp_socket should all
// update global structures appropriately
// CYGWIN MUST compile with same flags as object under test
// -D_USE_OPENSSL for cygwin in order to get the all the 
// fields in sipp_socket since cygwin compiled the object under test
// with this flag.  ss_ssl, ss_bio will be missing/mismatched if cygwin unit
// test doesnt also have this flag and this test will fail .
TEST(sockethandler_updates_globals, ipv4_or_6_and_all_transport){
  initialize_sockets();
  EXPECT_EQ(0,pollnfds) << "pollnfds should have zero value before any sockets are allocated";

  bool useIPv6 = false;
  int  transport = DEFAULT_TRANSPORT;
  struct sipp_socket* a_socket = new_sipp_socket(useIPv6,transport); 
  EXPECT_EQ(1,pollnfds) << "pollnfds should have 1 socket allocated";
  EXPECT_TRUE(a_socket != NULL) << "failed to allocate first socket";
  EXPECT_EQ(pollnfds-1,a_socket->ss_pollidx) << "sipp_socket->pollidx should be one less than pollnfds";
  EXPECT_EQ(a_socket, sockets[a_socket->ss_pollidx]) << "socket array entry should point to a_socket";
  EXPECT_EQ(a_socket->ss_fd, pollfiles[a_socket->ss_pollidx].fd) << "pollfiles array entry should have fd point to this socket";
  
  transport = T_UDP;
  struct sipp_socket* b_socket = new_sipp_socket(useIPv6,transport);
  EXPECT_EQ(2,pollnfds) << "pollnfds should have 1 socket allocated";
  EXPECT_TRUE(b_socket != NULL) << "failed to allocate second socket";
  EXPECT_EQ( (pollnfds-1),b_socket->ss_pollidx) << "sipp_socket->pollidx should be one less than pollnfds";
  EXPECT_EQ(b_socket, sockets[(b_socket->ss_pollidx)]) << "socket array entry should point to a_socket";
  EXPECT_EQ(b_socket->ss_fd, pollfiles[(b_socket->ss_pollidx)].fd) << "pollfiles array entry should have fd point to this socket";
  
  transport = T_TCP;
  struct sipp_socket* c_socket = new_sipp_socket(useIPv6,transport); 
  EXPECT_EQ(3,pollnfds) << "pollnfds should have 1 socket allocated";
  EXPECT_TRUE(c_socket != NULL) << "failed to allocate third socket";
  EXPECT_EQ(pollnfds-1,c_socket->ss_pollidx) << "sipp_socket->pollidx should be one less than pollnfds";
  EXPECT_EQ(c_socket, sockets[c_socket->ss_pollidx]) << "socket array entry should point to a_socket";
  EXPECT_EQ(c_socket->ss_fd, pollfiles[c_socket->ss_pollidx].fd) << "pollfiles array entry should have fd point to this socket";
  
  useIPv6 = true;
  transport = DEFAULT_TRANSPORT;
  struct sipp_socket* d_socket = new_sipp_socket(useIPv6,transport); 
  EXPECT_EQ(4,pollnfds) << "pollnfds should have 1 socket allocated";
  EXPECT_TRUE(d_socket != NULL) << "failed to allocate first socket";
  EXPECT_EQ(pollnfds-1,d_socket->ss_pollidx) << "sipp_socket->pollidx should be one less than pollnfds";
  EXPECT_EQ(d_socket, sockets[d_socket->ss_pollidx]) << "socket array entry should point to a_socket";
  EXPECT_EQ(d_socket->ss_fd, pollfiles[d_socket->ss_pollidx].fd) << "pollfiles array entry should have fd point to this socket";
  
  transport = T_UDP;
  struct sipp_socket* e_socket = new_sipp_socket(useIPv6,transport); 
  EXPECT_EQ(5,pollnfds) << "pollnfds should have 1 socket allocated";
  EXPECT_TRUE(e_socket != NULL) << "failed to allocate second socket";
  EXPECT_EQ(pollnfds-1,e_socket->ss_pollidx) << "sipp_socket->pollidx should be one less than pollnfds";
  EXPECT_EQ(e_socket, sockets[e_socket->ss_pollidx]) << "socket array entry should point to a_socket";
  EXPECT_EQ(e_socket->ss_fd, pollfiles[e_socket->ss_pollidx].fd) << "pollfiles array entry should have fd point to this socket";
  
  transport = T_TCP;
  struct sipp_socket* f_socket = new_sipp_socket(useIPv6,transport); 
  EXPECT_EQ(6,pollnfds) << "pollnfds should have 1 socket allocated";
  EXPECT_TRUE(f_socket != NULL) << "failed to allocate third socket";
  EXPECT_EQ(pollnfds-1,f_socket->ss_pollidx) << "sipp_socket->pollidx should be one less than pollnfds";
  EXPECT_EQ(f_socket, sockets[f_socket->ss_pollidx]) << "socket array entry should point to a_socket";
  EXPECT_EQ(f_socket->ss_fd, pollfiles[f_socket->ss_pollidx].fd) << "pollfiles array entry should have fd point to this socket";
 
  sipp_close_socket(a_socket);
  EXPECT_EQ(5,pollnfds) << "pollnfds should have deallocated socket";
  EXPECT_EQ(NULL, sockets[pollnfds]) << "the last socket entry should be null after closing socket";

  sipp_close_socket(f_socket);
  EXPECT_EQ(4,pollnfds) << "pollnfds should have deallocated socket";
  EXPECT_EQ(NULL, sockets[pollnfds]) << "the last socket entry should be null after closing socket";

  sipp_close_socket(c_socket);
  EXPECT_EQ(3,pollnfds) << "pollnfds should have deallocated socket";
  EXPECT_EQ(NULL, sockets[pollnfds]) << "the last socket entry should be null after closing socket";

  sipp_close_socket(b_socket);
  EXPECT_EQ(2,pollnfds) << "pollnfds should have deallocated socket";
  EXPECT_EQ(NULL, sockets[pollnfds]) << "the last socket entry should be null after closing socket";

  sipp_close_socket(d_socket);
  EXPECT_EQ(1,pollnfds) << "pollnfds should have deallocated socket";
  EXPECT_EQ(NULL, sockets[pollnfds]) << "the last socket entry should be null after closing socket";

  sipp_close_socket(e_socket);
  EXPECT_EQ(0,pollnfds) << "pollnfds should have deallocated socket";
  EXPECT_EQ(NULL, sockets[pollnfds]) << "the last socket entry should be null after closing socket";

  cleanup_sockets();

}

//Test udp loop around by calling mix of lower level sipp methods 
//  and direct socket api. Verify globals set along the way.
// 
//  determine_remote_ip
//  sock_fd
//  sipp_allocate_socket
//  sipp_bind_socket
//  determine_local_ip
//  write_socket
TEST(sockethandler, udp_ipv4_looparound){
  initialize_sockets();
  //set remote address
  const char* ipaddr = "127.0.0.1:65432";
  strncpy(remote_host, ipaddr ,255);
  determine_remote_ip();
  EXPECT_STREQ("127.0.0.1", remote_host);  // removes port as part of processing
  EXPECT_EQ(65432,remote_port)<< "failed to set global";
  EXPECT_STREQ("127.0.0.1",remote_ip)<< "failed to set global";
  EXPECT_STREQ("127.0.0.1",get_inet_address(&remote_sockaddr)) << "sockethelper routine failed";
  EXPECT_STREQ(ipaddr,socket_to_ip_port_string(&remote_sockaddr).c_str()) << "sockethelper routine failed";
  EXPECT_STREQ("127.0.0.1",socket_to_ip_string(&remote_sockaddr).c_str()) << "sockethelper routine failed";
  EXPECT_EQ(65432,get_in_port(&remote_sockaddr)) << "sockethelper routine failed";

  //create a ipv4 udp socket
  int  proto = 0;  // all sockets created in sipp just use this default
  int  sipp_transport_type = T_UDP; // T_UDP, T_TCP, T_TLS
  bool ipv6 = false;
  int  address_domain = (ipv6 ? AF_INET6 : AF_INET);
  int  socket_type = ((sipp_transport_type == T_UDP) ? SOCK_DGRAM : SOCK_STREAM);
  EXPECT_EQ(SOCK_DGRAM, socket_type);
  SOCKREF sock_fd = socket_fd(ipv6,sipp_transport_type);
  if (sock_fd == INVALID_SOCKET)
#ifdef WIN32
  ASSERT_NE(INVALID_SOCKET, sock_fd) << WSAGetLastError();
#else
  {
    perror("failed to create socket");
    ASSERT_NE(-1, sock_fd) << "failed to create socket";
  }
#endif
  EXPECT_EQ(0,pollnfds) << "pollnfds should have zero value before any sockets are allocated";

  //allocate a sipp_socket using socket from sock_fd
  struct sipp_socket* remote_ss = sipp_allocate_socket(ipv6, sipp_transport_type, sock_fd, proto);
  ASSERT_TRUE (remote_ss != NULL)       << "failed to allocate sipp_socket structure";
  EXPECT_EQ(1, remote_ss->ss_count);
  EXPECT_EQ(false, remote_ss->ss_ipv6);
  EXPECT_EQ(sipp_transport_type, remote_ss->ss_transport) ;
  EXPECT_EQ(false, remote_ss->ss_control) ;
  EXPECT_EQ(false, remote_ss->ss_call_socket) << "This should not be marked as a call socket";
  EXPECT_EQ(false, remote_ss->ss_changed_dest) << "No destination was changed, should be false";
  EXPECT_EQ(sock_fd, remote_ss->ss_fd) << "sipp_socket does not have socket we passed in";
  EXPECT_EQ(NULL, remote_ss->ss_comp_state);
#ifdef _USE_OPENSSL
  EXPECT_EQ(NULL, remote_ss->ss_ssl) << "No SSL objects should exist";
  EXPECT_EQ(NULL, remote_ss->ss_bio) << "No BIO objet should exist";
#endif
  EXPECT_EQ(false,remote_ss->ss_congested);
  EXPECT_EQ(false,remote_ss->ss_invalid);
  EXPECT_EQ(NULL,remote_ss->ss_in);
  EXPECT_EQ((size_t)0,remote_ss->ss_msglen);
  EXPECT_EQ(NULL,remote_ss->ss_out);
  EXPECT_EQ(1, pollnfds) << "pollnfds should have been incremented by sipp_allocate_sokcet";
  EXPECT_EQ(pollnfds-1,remote_ss->ss_pollidx) << "pollidx should be set to one less than current pollnfds";
  EXPECT_EQ(remote_ss, sockets[remote_ss->ss_pollidx])<< "socket array should have this socket at position ss_pollidx";

  //bind socket to remote address
  int remote_port;
  int rc = sipp_bind_socket(remote_ss,(sockaddr_storage*)(&remote_sockaddr),&remote_port);
  print_if_error(rc);
  EXPECT_EQ(65432, remote_port);
  EXPECT_EQ(0,rc)<<"sipp_bind_socket failed to bind";
  struct sockaddr addr;
  socklen_t namelen = sizeof (addr);
  EXPECT_EQ(0,getsockname(sock_fd, &addr,&namelen)) << "Failed to retreive sockaddr from socket";
  EXPECT_EQ(address_domain, addr.sa_family) << "socket address family doesnt match input to fd_socket";
  EXPECT_EQ(65432, (int) get_in_port((struct sockaddr_storage*)&addr))<<"socket remote_port number should match";
  EXPECT_STREQ("127.0.0.1", socket_to_ip_string((struct sockaddr_storage*)&addr).c_str());
  char dst[INET6_ADDRSTRLEN];
  inet_ntop(addr.sa_family,get_in_addr((struct sockaddr_storage*)&addr), dst, sizeof(dst));
  EXPECT_STREQ("127.0.0.1",dst) <<  "get_in_addr did not return expected address string";

  // set local ip
  string local_ip_string = string("127.0.0.1");  //get_local_ip(AF_INET);
  strncpy(local_ip, local_ip_string.c_str(),40);
  determine_local_ip();
  char expected_hostname[80];
  gethostname(expected_hostname,80);
  EXPECT_STREQ(expected_hostname,hostname);
  EXPECT_STREQ(local_ip_string.c_str(),local_ip) << "determin_local_ip should rewrite local_ip back to value or substring of value that was set";
  EXPECT_STREQ(local_ip_string.c_str(), get_inet_address(&local_sockaddr))<<"local_sockaddr should have local hosts ip address";
  EXPECT_STREQ(local_ip_string.c_str(), socket_to_ip_string(&local_sockaddr).c_str()) ;
  EXPECT_EQ(AF_INET, local_sockaddr.ss_family);
  EXPECT_FALSE(local_ip_is_ipv6);
  EXPECT_STREQ(local_ip, local_ip_escaped) <<"ipv4 should have identical values";
  EXPECT_EQ(AF_INET,local_addr_storage->ai_family);
  EXPECT_STREQ(local_ip, get_inet_address((sockaddr_storage*)(local_addr_storage->ai_addr)));

  // open_connections initializes the port in local_sockaddr from user_port
  // which in turn updates local_port by the sipp_bind_socket.  
  // Emulate manually here to set up local_sockaddr global for binding
  user_port = 23456;  //cmd line option -p stored here
  EXPECT_EQ(23456,user_port)<<"failed to initialize local_port";
    if (local_ip_is_ipv6) {
      (_RCAST(struct sockaddr_in6 *, &local_sockaddr))->sin6_port
        = htons((short)user_port);
    } else {
      (_RCAST(struct sockaddr_in *, &local_sockaddr))->sin_port
        = htons((short)user_port);
    }

//create a ipv4 udp socket for local_ip to bind to
  //use same local arg values from above for socket parameters
  SOCKREF local_sock_fd = socket_fd(ipv6,sipp_transport_type);
  if (local_sock_fd == INVALID_SOCKET)
#ifdef WIN32
  ASSERT_NE(INVALID_SOCKET, local_sock_fd) << WSAGetLastError();
#else
  {
    perror("failed to create socket");
    ASSERT_NE(-1, local_sock_fd) << "failed to create socket";
  }
#endif
  EXPECT_EQ(1,pollnfds) << "pollnfds should have zero value before any sockets are allocated";
//allocate a sipp_socket using socket from local_sock_fd
  struct sipp_socket* local_ss = sipp_allocate_socket(ipv6, sipp_transport_type, local_sock_fd, proto);
  ASSERT_TRUE (local_ss != NULL)       << "failed to allocate sipp_socket structure";
  EXPECT_EQ(1, local_ss->ss_count);
  EXPECT_EQ(false, local_ss->ss_ipv6);
  EXPECT_EQ(sipp_transport_type, local_ss->ss_transport) ;
  EXPECT_EQ(false, local_ss->ss_control) ;
  EXPECT_EQ(false, local_ss->ss_call_socket) << "This should not be marked as a call socket";
  EXPECT_EQ(false, local_ss->ss_changed_dest) << "No destination was changed, should be false";
  EXPECT_EQ(local_sock_fd, local_ss->ss_fd) << "sipp_socket does not have socket we passed in";
  EXPECT_EQ(NULL, local_ss->ss_comp_state);
#ifdef _USE_OPENSSL
  EXPECT_EQ(NULL, local_ss->ss_ssl) << "No SSL objects should exist";
  EXPECT_EQ(NULL, local_ss->ss_bio) << "No BIO objet should exist";
#endif
  EXPECT_EQ(false,local_ss->ss_congested);
  EXPECT_EQ(false,local_ss->ss_invalid);
  EXPECT_EQ(NULL,local_ss->ss_in);
  EXPECT_EQ((size_t)0,local_ss->ss_msglen);
  EXPECT_EQ(NULL,local_ss->ss_out);
  EXPECT_EQ(2, pollnfds) << "pollnfds should have been incremented by sipp_allocate_sokcet";
  EXPECT_EQ(pollnfds-1,local_ss->ss_pollidx) << "pollidx should be set to one less than current pollnfds";
  EXPECT_EQ(local_ss, sockets[local_ss->ss_pollidx])<< "socket array should have this socket at position ss_pollidx";

//bind socket to local_ip address
  rc = sipp_bind_socket(local_ss,(sockaddr_storage*)(&local_sockaddr),&local_port);
  print_if_error(rc);
  EXPECT_EQ(0,rc)<<"sipp_bind_socket failed to bind";
  struct sockaddr localaddr;
  socklen_t localnamelen = sizeof (localaddr);
  EXPECT_EQ(0,getsockname(sock_fd, &localaddr,&localnamelen)) << "Failed to retreive sockaddr from socket";
  EXPECT_EQ(address_domain, localaddr.sa_family) << "socket address family doesnt match input to fd_socket";
  EXPECT_STREQ(local_ip, socket_to_ip_string((struct sockaddr_storage*)&localaddr).c_str());
  char localdst[INET6_ADDRSTRLEN];
  inet_ntop(localaddr.sa_family,get_in_addr((struct sockaddr_storage*)&localaddr), localdst, sizeof(localdst));
  EXPECT_STREQ(local_ip,localdst) <<  "get_in_addr did not return expected address string";
  EXPECT_EQ(23456,local_port)<<"failed to initialize local_port";
  //determine_local_ip initializes local_addr_storage explictly with NULL service
  //not user_port as we might guess(wrongly)
  EXPECT_EQ(0, get_in_port((sockaddr_storage*)(local_addr_storage->ai_addr)));


//we now have bound a remote_ss to 127.0.0.1:65432 
//and a local_ss bound 127.0.0.1:23456

  EXPECT_FALSE(local_ss->ss_invalid)<<"cannot write to invalid socket";
  EXPECT_FALSE(local_ss->ss_congested)<<"EWOULDBLOCK";

  // send a message
  char mymsg[] = "drahcir mul saw reven ereh!";
  unsigned int bytes_sent = write_socket(local_ss,mymsg, strlen(mymsg), 0, &remote_sockaddr);
  EXPECT_EQ(strlen(mymsg),bytes_sent)<<"bytes sent is not equal to size of message";
  EXPECT_STREQ("127.0.0.1:23456", fd_to_ipstring(local_ss->ss_fd).c_str())<< "from socket is not set correctly";
  EXPECT_STREQ("127.0.0.1:65432", socket_to_ip_port_string((struct sockaddr_storage*)&remote_sockaddr).c_str())<<"to socket is not set correctly";

  // on remote socket, retrieve the data 
  // empty_socket normally called by pollet_process in sipp.cpp
  // empty_socket in turn invokes recvfrom
  // sipp does not use connect on udp, only on tcp
  
  //emulate portion of empty_socket
  int readsize = local_ss->ss_transport == T_UDP ? SIPP_MAX_MSG_SIZE : tcp_readsize;
  char *buffer;
  buffer = (char *)malloc(readsize);
  struct sockaddr_storage sender;
  socklen_t addrlen = sizeof(sender);
  memset(buffer,0,readsize);
  EXPECT_STREQ("127.0.0.1:65432",fd_to_ipstring(remote_ss->ss_fd).c_str())<<"trying to retrieve data from wrong port";
  int ret;
  if ((ret = recvfrom(remote_ss->ss_fd, 
                buffer, 
                readsize, 
                0, 
                (struct sockaddr*)&sender ,  
                &addrlen)) == -1 ){
      print_if_error(ret);
    }else{
      EXPECT_STREQ(mymsg,buffer) << "received message doesnt match sent message";
    }
  EXPECT_GT(ret,0)<<"recvfrom error";

  sipp_close_socket(remote_ss);
  EXPECT_EQ(1,pollnfds);
  sipp_close_socket(local_ss);
  EXPECT_EQ(0,pollnfds);

  cleanup_sockets();

}

//Test udp loop around by calling sipp methods and verify globals set
//  determine_remote_ip
//  sock_fd
//  sipp_allocate_socket
//  sipp_bind_socket
//  determine_local_ip
//  write_socket
TEST(sockethandler, udp_ipv6_looparound){
  initialize_sockets();
//set remote address
  const char* ipaddr = "[::1]:65432";
  //char* ipaddr = "172.23.6.185:65432";
  strncpy(remote_host, ipaddr ,255);

  determine_remote_ip();
  EXPECT_STREQ("::1", remote_host);  // removes port as part of processing
  EXPECT_EQ(65432,remote_port)<< "failed to set global";
  EXPECT_STREQ("::1",remote_ip)<< "failed to set global";
  EXPECT_STREQ("::1",get_inet_address(&remote_sockaddr)) << "sockethelper routine failed";
  EXPECT_STREQ(ipaddr,socket_to_ip_port_string(&remote_sockaddr).c_str()) << "sockethelper routine failed";
  EXPECT_STREQ("::1",socket_to_ip_string(&remote_sockaddr).c_str()) << "sockethelper routine failed";
  EXPECT_EQ(65432,get_in_port(&remote_sockaddr)) << "sockethelper routine failed";

//create a ipv4 tcp socket
  int  proto = 0;  // all sockets created in sipp just use this default
  int  sipp_transport_type = T_UDP; // T_UDP, T_TCP, T_TLS
  bool ipv6 = true;
  int  address_domain = (ipv6 ? AF_INET6 : AF_INET);
  int  socket_type = ((sipp_transport_type == T_UDP) ? SOCK_DGRAM : SOCK_STREAM);
  EXPECT_EQ(SOCK_DGRAM, socket_type);

  SOCKREF sock_fd = socket_fd(ipv6,sipp_transport_type);
  if (sock_fd == INVALID_SOCKET)
#ifdef WIN32
  ASSERT_NE(INVALID_SOCKET, sock_fd) << WSAGetLastError();
#else
  {
    perror("failed to create socket");
    ASSERT_NE(-1, sock_fd) << "failed to create socket";
  }
#endif
  EXPECT_EQ(0,pollnfds) << "pollnfds should have zero value before any sockets are allocated";

//allocate a sipp_socket using socket from sock_fd
  struct sipp_socket* remote_ss = sipp_allocate_socket(ipv6, sipp_transport_type, sock_fd, proto);
  ASSERT_TRUE (remote_ss != NULL)       << "failed to allocate sipp_socket structure";
  EXPECT_EQ(1, remote_ss->ss_count);
  EXPECT_EQ(true, remote_ss->ss_ipv6);
  EXPECT_EQ(sipp_transport_type, remote_ss->ss_transport) ;
  EXPECT_EQ(false, remote_ss->ss_control) ;
  EXPECT_EQ(false, remote_ss->ss_call_socket) << "This should not be marked as a call socket";
  EXPECT_EQ(false, remote_ss->ss_changed_dest) << "No destination was changed, should be false";
  EXPECT_EQ(sock_fd, remote_ss->ss_fd) << "sipp_socket does not have socket we passed in";
  EXPECT_EQ(NULL, remote_ss->ss_comp_state);
#ifdef _USE_OPENSSL
  EXPECT_EQ(NULL, remote_ss->ss_ssl) << "No SSL objects should exist";
  EXPECT_EQ(NULL, remote_ss->ss_bio) << "No BIO objet should exist";
#endif
  EXPECT_EQ(false,remote_ss->ss_congested);
  EXPECT_EQ(false,remote_ss->ss_invalid);
  EXPECT_EQ(NULL,remote_ss->ss_in);
  EXPECT_EQ((size_t)0,remote_ss->ss_msglen);
  EXPECT_EQ(NULL,remote_ss->ss_out);
  EXPECT_EQ(1, pollnfds) << "pollnfds should have been incremented by sipp_allocate_sokcet";
  EXPECT_EQ(pollnfds-1,remote_ss->ss_pollidx) << "pollidx should be set to one less than current pollnfds";
  EXPECT_EQ(remote_ss, sockets[remote_ss->ss_pollidx])<< "socket array should have this socket at position ss_pollidx";

//bind socket to remote address
  int remote_port;
  int rc = sipp_bind_socket(remote_ss,(sockaddr_storage*)(&remote_sockaddr),&remote_port);
  print_if_error(rc);

  EXPECT_EQ(65432, remote_port);
  EXPECT_EQ(0,rc)<<"sipp_bind_socket failed to bind";
  struct sockaddr_storage addr;
  socklen_t namelen = sizeof (addr);
  EXPECT_EQ(0,getsockname(sock_fd,(struct sockaddr*) &addr,&namelen)) << "Failed to retreive sockaddr from socket";
  print_if_error(rc);

  EXPECT_EQ(address_domain, ((struct sockaddr*)&addr)->sa_family) << "socket address family doesnt match input to fd_socket";
  EXPECT_EQ(65432, (int) get_in_port((struct sockaddr_storage*)&addr))<<"socket remote_port number should match";
  EXPECT_STREQ("::1", socket_to_ip_string((struct sockaddr_storage*)&addr).c_str());
  char dst[INET6_ADDRSTRLEN];
  inet_ntop(addr.ss_family,get_in_addr(&addr), dst, sizeof(dst));
  EXPECT_STREQ("::1",dst) <<  "get_in_addr did not return expected address string";

// set local ip
  string local_ip_string = string("::1");  //get_local_ip(AF_INET);
  strncpy(local_ip, local_ip_string.c_str(),40);

  determine_local_ip();
  char expected_hostname[80];
  gethostname(expected_hostname,80);
  EXPECT_STREQ(expected_hostname,hostname);
  EXPECT_STREQ(local_ip_string.c_str(),local_ip) << "determin_local_ip should rewrite local_ip back to value or substring of value that was set";
  EXPECT_STREQ(local_ip_string.c_str(), get_inet_address(&local_sockaddr))<<"local_sockaddr should have local hosts ip address";
  EXPECT_STREQ(local_ip_string.c_str(), socket_to_ip_string(&local_sockaddr).c_str()) ;
  EXPECT_EQ(AF_INET6, local_sockaddr.ss_family);
  EXPECT_TRUE(local_ip_is_ipv6);
  EXPECT_STREQ("[::1]", local_ip_escaped) <<"ipv6 should have identical values";
  EXPECT_EQ(AF_INET6,local_addr_storage->ai_family);
  EXPECT_STREQ(local_ip, get_inet_address((sockaddr_storage*)(local_addr_storage->ai_addr)));

  // normal sipp operation would initialize from sipp open_connections
  // by calling
      //sipp_bind_socket(main_socket,&local_sockaddr, &local_port)
  // and seeting the port in local_sockaddr manually from user_port
  // which in turn updates local_port by the sipp_bind_socket.  
  // Emulate manually here until open_connetions migrates to sipp_sockethandler 
  user_port = 23456;  //cmd line option -p stored here
  EXPECT_EQ(23456,user_port)<<"failed to initialize local_port";
    if (local_ip_is_ipv6) {
      (_RCAST(struct sockaddr_in6 *, &local_sockaddr))->sin6_port
        = htons((short)user_port);
    } else {
      (_RCAST(struct sockaddr_in *, &local_sockaddr))->sin_port
        = htons((short)user_port);
    }


//create a ipv4 udp socket for local_ip to bind to
  //use same local arg values from above for socket parameters
  SOCKREF local_sock_fd = socket_fd(ipv6,sipp_transport_type);
  if (local_sock_fd == INVALID_SOCKET)
#ifdef WIN32
  ASSERT_NE(INVALID_SOCKET, local_sock_fd) << WSAGetLastError();
#else
  {
    perror("failed to create socket");
    ASSERT_NE(-1, local_sock_fd) << "failed to create socket";
  }
#endif
  EXPECT_EQ(1,pollnfds) << "pollnfds should have zero value before any sockets are allocated";
//allocate a sipp_socket using socket from local_sock_fd
  struct sipp_socket* local_ss = sipp_allocate_socket(ipv6, sipp_transport_type, local_sock_fd, proto);
  ASSERT_TRUE (local_ss != NULL)       << "failed to allocate sipp_socket structure";
  EXPECT_EQ(1, local_ss->ss_count);
  EXPECT_EQ(true, local_ss->ss_ipv6);
  EXPECT_EQ(sipp_transport_type, local_ss->ss_transport) ;
  EXPECT_EQ(false, local_ss->ss_control) ;
  EXPECT_EQ(false, local_ss->ss_call_socket) << "This should not be marked as a call socket";
  EXPECT_EQ(false, local_ss->ss_changed_dest) << "No destination was changed, should be false";
  EXPECT_EQ(local_sock_fd, local_ss->ss_fd) << "sipp_socket does not have socket we passed in";
  EXPECT_EQ(NULL, local_ss->ss_comp_state);
#ifdef _USE_OPENSSL
  EXPECT_EQ(NULL, local_ss->ss_ssl) << "No SSL objects should exist";
  EXPECT_EQ(NULL, local_ss->ss_bio) << "No BIO objet should exist";
#endif
  EXPECT_EQ(false,local_ss->ss_congested);
  EXPECT_EQ(false,local_ss->ss_invalid);
  EXPECT_EQ(NULL,local_ss->ss_in);
  EXPECT_EQ((size_t)0,local_ss->ss_msglen);
  EXPECT_EQ(NULL,local_ss->ss_out);
  EXPECT_EQ(2, pollnfds) << "pollnfds should have been incremented by sipp_allocate_sokcet";
  EXPECT_EQ(pollnfds-1,local_ss->ss_pollidx) << "pollidx should be set to one less than current pollnfds";
  EXPECT_EQ(local_ss, sockets[local_ss->ss_pollidx])<< "socket array should have this socket at position ss_pollidx";

//bind socket to local_ip address

  rc = sipp_bind_socket(local_ss,(sockaddr_storage*)(&local_sockaddr),&local_port);
  print_if_error(rc);
  EXPECT_EQ(0,rc)<<"sipp_bind_socket failed to bind";
  struct sockaddr_storage localaddr;
  socklen_t localnamelen = sizeof (localaddr);
  rc = getsockname(sock_fd, (sockaddr*)&localaddr,&localnamelen);
  EXPECT_EQ(0,rc)<< "Failed to retreive sockaddr from socket";
  print_if_error(rc);

  EXPECT_EQ(address_domain, localaddr.ss_family) << "socket address family doesnt match input to fd_socket";
  //EXPECT_EQ(65432, (int) get_in_port((struct sockaddr_storage*)&addr))<<"socket remote_port number should match";
  EXPECT_STREQ(local_ip, socket_to_ip_string((struct sockaddr_storage*)&localaddr).c_str());
  char localdst[INET6_ADDRSTRLEN];
  inet_ntop(localaddr.ss_family,get_in_addr((struct sockaddr_storage*)&localaddr), localdst, sizeof(localdst));
  EXPECT_STREQ(local_ip,localdst) <<  "get_in_addr did not return expected address string";

  EXPECT_EQ(23456,local_port)<<"failed to initialize local_port";
  //determine_local_ip initializes local_addr_storage explictly with NULL service
  //not user_port as we might guess(wrongly)
  EXPECT_EQ(0, get_in_port((sockaddr_storage*)(local_addr_storage->ai_addr)));


//we now have bound a remote_ss to [::1]:65432 
//and a local_ss bound [::1]:23456

//int write_socket(struct sipp_socket *socket, char *buffer, ssize_t len, int flags, struct sockaddr_storage *dest)
  EXPECT_FALSE(local_ss->ss_invalid)<<"cannot write to invalid socket";
  EXPECT_FALSE(local_ss->ss_congested)<<"EWOULDBLOCK";
  char mymsg[] = "drahcir mul saw reven ereh!";
  unsigned int bytes_sent = write_socket(local_ss,mymsg, strlen(mymsg), 0, &remote_sockaddr);
  EXPECT_EQ(strlen(mymsg),bytes_sent);
  EXPECT_STREQ("[::1]:23456", fd_to_ipstring(local_ss->ss_fd).c_str())<< "from socket is not set correctly";
  EXPECT_STREQ("[::1]:65432", socket_to_ip_port_string((struct sockaddr_storage*)&remote_sockaddr).c_str())<<"to socket is not set correctly";

  // on remote socket, retrieve the data 
  // empty_socket normally called by pollet_process in sipp.cpp
  // empty_socket in turn invokes recvfrom
  // sipp does not use connect on udp, only on tcp
  
  //emulate portion of empty_socket
  int readsize = local_ss->ss_transport == T_UDP ? SIPP_MAX_MSG_SIZE : tcp_readsize;
  char *buffer;
  buffer = (char *)malloc(readsize);
  struct sockaddr_storage sender;
  socklen_t addrlen = sizeof(sender);
  memset(buffer,0,readsize);
  EXPECT_STREQ("[::1]:65432",fd_to_ipstring(remote_ss->ss_fd).c_str())<<"trying to retrieve data from wrong port";
  int ret;
  if ((ret = recvfrom(remote_ss->ss_fd, 
                buffer, 
                readsize, 
                0, 
                (struct sockaddr*)&sender ,  
                &addrlen)) == -1 ){
      print_if_error(ret);
    }else{
      EXPECT_STREQ(mymsg,buffer) << "received message doesnt match sent message";
    }
  EXPECT_GT(ret,0)<<"recvfrom error";

  /* 
  call::send_raw
  call_peer extracted from incoming msg or 
  sock = sockets[num_of_open_socks-1];
  rc = write_socket(sock, msg, len, WS_BUFFER, &call_peer);
  */

  sipp_close_socket(remote_ss);
  EXPECT_EQ(1,pollnfds);
  sipp_close_socket(local_ss);
  EXPECT_EQ(0,pollnfds);

  cleanup_sockets();

}


/**************open_connections********/
// use the highest level sipp socket methods
// verify key globals, validate sent/recv message
// on loop around.

  //Methods under test
  // open_connections
  //   new_sipp_socket
  //    sock_fd
  //    sipp_allocate_socket
  //   sipp_customize_socket
  //   sipp_connect_socket
  // write_socket
  // empty_socket
  // sipp_close_socket

  // sipp_bind_socket  removed to allow dynamic port assignment
  // and eliminate problems with repeated runs of this test

extern int open_connections();
TEST(sockethandler, open_connections_tcp_ipv4){
  initialize_sockets();
  //set up required globals
  local_ip_is_ipv6 = false;
  transport = T_TCP;
  strncpy (remote_host, "127.0.0.1:65432",255);
  strncpy (local_ip, "127.0.0.1",40);
  local_port = 23456;
  buff_size = 65534; 
  
  EXPECT_EQ(0,pollnfds) << "Socket counter should be zero before testing starts";
  determine_remote_and_local_ip();
  EXPECT_EQ(65432, remote_port);
  EXPECT_STREQ("127.0.0.1",remote_host);
  EXPECT_STREQ("127.0.0.1:65432",socket_to_ip_port_string(&remote_sockaddr).c_str());
  
  // some options that affect open_connections that we dont want to exercise here
  twinSippMode = false;
  extendedTwinSippMode = false;
  use_remote_sending_addr = false;
  multisocket = 0; // command line option t1 vs tn
  peripsocket =0;
  sendMode = MODE_SERVER;
  
  // open_connection = socket, setsockopt,bind, listen
  open_connections();
  EXPECT_TRUE(main_socket != NULL);
  EXPECT_EQ(false, main_socket->ss_ipv6);
  EXPECT_EQ(T_TCP, main_socket->ss_transport);
  EXPECT_EQ(1,pollnfds);
  // sipp_customize_socket called by open_connections
  
/*************check effect of sipp_customize_socket *********************/

  // //cygwin always report SO_REUSEADDR false, attempt to explictly set it here to test
  // //works on linux and windows but cygwin never reports true
  //if ((ss_rc=setsockopt(main_socket->ss_fd, SOL_SOCKET, SO_REUSEADDR, SETSOCKOPT_TYPE &socket_option,
   //                sizeof (socket_option))) == -1) {
   //   print_if_error(ss_rc);
   // }
  // //no cygwin errors reported but still value is unexpected
  int rc;
  int socketopt=0x1234;
  socklen_t size_socketopt = sizeof (socketopt);
  if ((rc = getsockopt(main_socket->ss_fd, SOL_SOCKET, SO_REUSEADDR, 
    (char*) &socketopt, &size_socketopt)) == -1){
    EXPECT_NE(-1, rc)<<"failed to retrieve SO_REUSEADDR option";
    print_if_error(rc);
  }else{
    // cygwin always fails here, never getsocketopt for SO_REUSEADDR = 1
#ifndef CYGWIN
    EXPECT_EQ(true, socketopt >= 1);
#endif
  }

  if ((rc = getsockopt(main_socket->ss_fd, SOL_TCP, TCP_NODELAY, 
    (char*) &socketopt, &size_socketopt)) == -1){
    EXPECT_NE(-1, rc)<<"failed to retrieve TCP_NODELAY option";
    print_if_error(rc);
  }else{
    EXPECT_EQ(true, socketopt >= 1);
  }
  struct linger a_linger;
  socklen_t sizeof_a_linger = sizeof (a_linger);
  if ((rc = getsockopt(main_socket->ss_fd, SOL_SOCKET, SO_LINGER, 
     (char*) &a_linger, &sizeof_a_linger)) == -1){
    EXPECT_NE(-1, rc)<<"failed to retrieve SO_LINGER option";
    print_if_error(rc);
  }else{
    EXPECT_EQ(true, a_linger.l_onoff >= 1);
    EXPECT_EQ(true, a_linger.l_linger >= 1);
  }

//  //this works on windows and cygwin but linux is exactly double expected.
//  // open_connections sets SO_RCVBUF and SO_SNDBUF from sipp_customize_socket
//  // try explictly setting it here and see if it changes results
//  // result: only linux getsockopt always reports double requested 
//  int buffsize = buff_size;
//  EXPECT_EQ(65534,buffsize);
//  if(rc = setsockopt(main_socket->ss_fd,
//                SOL_SOCKET,
//                SO_SNDBUF,
//                SETSOCKOPT_TYPE &buffsize,
//                sizeof(buffsize)) == -1 ) {
//    print_if_error(rc);
//  }
//  EXPECT_EQ(65534,buffsize);
//  if(rc = setsockopt(main_socket->ss_fd,
//                SOL_SOCKET,
//                SO_RCVBUF,
//                SETSOCKOPT_TYPE &buffsize,
//                sizeof(buffsize)) == -1 ) {
//    print_if_error(rc);
//  }

  int mybuffer_size;
  socklen_t sizeof_mybuffer_size = sizeof(mybuffer_size);
  if ((rc = getsockopt(main_socket->ss_fd, SOL_SOCKET, SO_RCVBUF, 
     (char*) &mybuffer_size, &sizeof_mybuffer_size)) == -1){
    EXPECT_NE(-1, rc)<<"failed to retrieve SO_RCVBUF option";
    print_if_error(rc);
  }else{
    EXPECT_GE(mybuffer_size,buff_size) << "allocated SO_RCVBUF buffer size is less than requested";
    // note testing shows that linux always reports twice the buffer size that was requested
    // hence test altered to GE
  }
  if ((rc = getsockopt(main_socket->ss_fd, SOL_SOCKET, SO_SNDBUF, 
     (char*) &mybuffer_size, &sizeof_mybuffer_size)) == -1){
    EXPECT_NE(-1, rc)<<"failed to retrieve SO_SNDBUF option";
    print_if_error(rc);
  }else{
    EXPECT_GE(mybuffer_size,buff_size) << "allocated SO_SNDBUF buffer size is less than requested";
    // note testing shows that linux always reports twice the buffer size that was requested
  }

  // get sockaddr associated with main_socket for later use
  sockaddr_storage main_saddr;
  socklen_t size_main_saddr = sizeof(main_saddr);
  getsockname(main_socket->ss_fd, (sockaddr *) &main_saddr, &size_main_saddr);

/**********************emulate remote client connecting to us *************/
  struct sipp_socket* remote_sipp_socket = new_sipp_socket(local_ip_is_ipv6,transport);
  EXPECT_EQ(2,pollnfds);

  //we can explictly bind to a static port but second instance of 
  // test will fail to connect as timewait sitting on our port
  // Remove static port binding

  //rc = sipp_bind_socket(remote_sipp_socket, &remote_sockaddr, &remoteport);
  //EXPECT_TRUE(rc != -1)<<"failed to bind to remote socket";
  //EXPECT_EQ(65432,remoteport);
  // open_connections binds main_socket to local_sockaddr, local_port so we can 
  // use local_sockaddr global as our target to connect to.
  rc = sipp_connect_socket(remote_sipp_socket, &local_sockaddr);
  EXPECT_EQ(0,rc)<< "failed to connect to socket";
  print_if_error(rc);

/********************server side accept connection ****************/
  struct sipp_socket *new_sock = sipp_accept_socket(main_socket,  &remote_sockaddr);
  ASSERT_TRUE(new_sock != NULL) << "failed to accept a socket";
  EXPECT_EQ(3,pollnfds);
  EXPECT_STREQ( "127.0.0.1:23456" ,socket_to_ip_port_string(&local_sockaddr).c_str()) << "source ip port does not match";

/**********************emulate remote client writing to us *************/
  char msg[] = "In a galaxy far far away ...";
  rc = write_socket(remote_sipp_socket, msg, strlen(msg), WS_BUFFER, &local_sockaddr);
  EXPECT_EQ(strlen(msg),(size_t)rc)<<"sent chars doesnt match message size";

/********************** check for server side receipt of mesg **********/
// empty_socket will receive message into buffer and link sipp_socket->ss_in->buf to it
  rc = empty_socket(new_sock);
  EXPECT_EQ((long int)strlen(msg),(long int)rc) << "received messaged length does not match length of msg sent";
  EXPECT_EQ(strlen(msg),new_sock->ss_in->len);
  char output[64];
  memset(output,0,sizeof(output));
  strncpy(output,new_sock->ss_in->buf,new_sock->ss_in->len);
  EXPECT_STREQ(msg,output);

/****************** send response from server to client *************/
  char response[] = "So Say we All!!!";
  rc = write_socket(new_sock, response, strlen(response), WS_BUFFER, &remote_sockaddr);
  EXPECT_EQ((long int)strlen(response),(long int) rc)<< "response chars doesnt match response size";

/****************** check for client side receipt of response *************/
  rc = empty_socket(remote_sipp_socket);
  EXPECT_EQ((long int)strlen(response),(long int)rc) << "received messaged length does not match length of msg sent";
  EXPECT_EQ(strlen(response),remote_sipp_socket->ss_in->len);

  memset(output,0,sizeof(output));
  strncpy(output,remote_sipp_socket->ss_in->buf,remote_sipp_socket->ss_in->len);
  EXPECT_STREQ(response,output);

/***************** clean up ***********************************************/
  sipp_close_socket(main_socket);
  sipp_close_socket(remote_sipp_socket);
  sipp_close_socket(new_sock);

  //make sure we closed all sockets
  EXPECT_EQ(0,pollnfds);

/************reconnect to socket to see if SO_REUSEADDR is actually enabled **/
 
  struct addrinfo hints, *res;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  int rv = getaddrinfo("127.0.0.1","23456",&hints, &res);
  if (rv)
#ifdef WIN32
    wprintf(L"error %s",wsaerrorstr(rv));
#else
    printf("getaddrinfo error: %s\n",gai_strerror(rv));
#endif

  sockaddr_storage res_addr;
  socklen_t size_res_addr = sizeof(res_addr);
  SOCKREF asocket = socket(AF_INET,SOCK_STREAM,0);
  rc = bind(asocket, (sockaddr*)res->ai_addr, res->ai_addrlen);
  EXPECT_EQ(0,rc)<<"failed to bind to newly closed socket";
  print_if_error(rc);
  getsockname(asocket, (sockaddr *) &res_addr, &size_res_addr);
  EXPECT_STREQ("127.0.0.1:23456",socket_to_ip_port_string(&res_addr).c_str())<<"bound to wrong address/port";

  CLOSESOCKET(asocket);
  cleanup_sockets();
};



TEST(sockethandler, open_connections_tcp_ipv6){
  initialize_sockets();
  //set up required globals
  local_ip_is_ipv6 = true;
  transport = T_TCP;
  strncpy (remote_host, "[::1]:65432",255);
  strncpy (local_ip, "::1",40);
  local_port = 23456;
  buff_size = 65534; 
  
  EXPECT_EQ(0,pollnfds) << "Socket counter should be zero before testing starts";
  determine_remote_and_local_ip();
  EXPECT_EQ(65432, remote_port);
  EXPECT_STREQ("::1",remote_host);
  EXPECT_STREQ("[::1]:65432",socket_to_ip_port_string(&remote_sockaddr).c_str());
  
  // some options that affect open_connections that we dont want to exercise here
  twinSippMode = false;
  extendedTwinSippMode = false;
  use_remote_sending_addr = false;
  multisocket = 0; // command line option t1 vs tn
  peripsocket =0;
  sendMode = MODE_SERVER;
  
  // open_connection = socket, setsockopt,bind, listen
  open_connections();
  EXPECT_TRUE(main_socket != NULL);
  EXPECT_EQ(true, main_socket->ss_ipv6);
  EXPECT_EQ(T_TCP, main_socket->ss_transport);
  EXPECT_EQ(1,pollnfds);
  // sipp_customize_socket called by open_connections
  
/*************check effect of sipp_customize_socket *********************/

  // //cygwin always report SO_REUSEADDR false, attempt to explictly set it here to test
  // //works on linux and windows but cygwin never reports true
  //if ((ss_rc=setsockopt(main_socket->ss_fd, SOL_SOCKET, SO_REUSEADDR, SETSOCKOPT_TYPE &socket_option,
   //                sizeof (socket_option))) == -1) {
   //   print_if_error(ss_rc);
   // }
  // //no cygwin errors reported but still value is unexpected
  int rc;
  int socketopt=0x1234;
  socklen_t size_socketopt = sizeof (socketopt);
  if ((rc = getsockopt(main_socket->ss_fd, SOL_SOCKET, SO_REUSEADDR, 
    (char*) &socketopt, &size_socketopt)) == -1){
    EXPECT_NE(-1, rc)<<"failed to retrieve SO_REUSEADDR option";
    print_if_error(rc);
  }else{
    // cygwin always fails here, never getsocketopt for SO_REUSEADDR = 1
#ifndef CYGWIN
    EXPECT_EQ(true, socketopt >= 1);
#endif
  }

  if ((rc = getsockopt(main_socket->ss_fd, SOL_TCP, TCP_NODELAY, 
    (char*) &socketopt, &size_socketopt)) == -1){
    EXPECT_NE(-1, rc)<<"failed to retrieve TCP_NODELAY option";
    print_if_error(rc);
  }else{
    EXPECT_EQ(true, socketopt >= 1);
  }
  struct linger a_linger;
  socklen_t sizeof_a_linger = sizeof (a_linger);
  if ((rc = getsockopt(main_socket->ss_fd, SOL_SOCKET, SO_LINGER, 
     (char*) &a_linger, &sizeof_a_linger)) == -1){
    EXPECT_NE(-1, rc)<<"failed to retrieve SO_LINGER option";
    print_if_error(rc);
  }else{
    EXPECT_EQ(true, a_linger.l_onoff >= 1);
    EXPECT_EQ(true, a_linger.l_linger >= 1);
  }

//  //this works on windows and cygwin but linux is exactly double expected.
//  // open_connections sets SO_RCVBUF and SO_SNDBUF from sipp_customize_socket
//  // try explictly setting it here and see if it changes results
//  // result: only linux getsockopt always reports double requested 
//  int buffsize = buff_size;
//  EXPECT_EQ(65534,buffsize);
//  if(rc = setsockopt(main_socket->ss_fd,
//                SOL_SOCKET,
//                SO_SNDBUF,
//                SETSOCKOPT_TYPE &buffsize,
//                sizeof(buffsize)) == -1 ) {
//    print_if_error(rc);
//  }
//  EXPECT_EQ(65534,buffsize);
//  if(rc = setsockopt(main_socket->ss_fd,
//                SOL_SOCKET,
//                SO_RCVBUF,
//                SETSOCKOPT_TYPE &buffsize,
//                sizeof(buffsize)) == -1 ) {
//    print_if_error(rc);
//  }

  int mybuffer_size;
  socklen_t sizeof_mybuffer_size = sizeof(mybuffer_size);
  if ((rc = getsockopt(main_socket->ss_fd, SOL_SOCKET, SO_RCVBUF, 
     (char*) &mybuffer_size, &sizeof_mybuffer_size)) == -1){
    EXPECT_NE(-1, rc)<<"failed to retrieve SO_RCVBUF option";
    print_if_error(rc);
  }else{
    EXPECT_GE(mybuffer_size,buff_size) << "allocated SO_RCVBUF buffer size is less than requested";
    // note testing shows that linux always reports twice the buffer size that was requested
    // hence test altered to GE
  }
  if ((rc = getsockopt(main_socket->ss_fd, SOL_SOCKET, SO_SNDBUF, 
     (char*) &mybuffer_size, &sizeof_mybuffer_size)) == -1){
    EXPECT_NE(-1, rc)<<"failed to retrieve SO_SNDBUF option";
    print_if_error(rc);
  }else{
    EXPECT_GE(mybuffer_size,buff_size) << "allocated SO_SNDBUF buffer size is less than requested";
    // note testing shows that linux always reports twice the buffer size that was requested
  }

  // get sockaddr associated with main_socket for later use
  sockaddr_storage main_saddr;
  socklen_t size_main_saddr = sizeof(main_saddr);
  getsockname(main_socket->ss_fd, (sockaddr *) &main_saddr, &size_main_saddr);

/**********************emulate remote client connecting to us *************/
  struct sipp_socket* remote_sipp_socket = new_sipp_socket(local_ip_is_ipv6,transport);
  EXPECT_EQ(2,pollnfds);

  //we can explictly bind to a static port but second instance of 
  // test will fail to connect as timewait sitting on our port
  // Remove static port binding

  //rc = sipp_bind_socket(remote_sipp_socket, &remote_sockaddr, &remoteport);
  //EXPECT_TRUE(rc != -1)<<"failed to bind to remote socket";
  //EXPECT_EQ(65432,remoteport);
  // open_connections binds main_socket to local_sockaddr, local_port so we can 
  // use local_sockaddr global as our target to connect to.
  rc = sipp_connect_socket(remote_sipp_socket, &local_sockaddr);
  EXPECT_EQ(0,rc)<< "failed to connect to socket";
  print_if_error(rc);

/********************server side accept connection ****************/
  struct sipp_socket *new_sock = sipp_accept_socket(main_socket,  &remote_sockaddr);
  ASSERT_TRUE(new_sock != NULL) << "failed to accept a socket";
  EXPECT_EQ(3,pollnfds);
  EXPECT_STREQ( "[::1]:23456" ,socket_to_ip_port_string(&local_sockaddr).c_str()) << "source ip port does not match";
  //EXPECT_STREQ( "127.0.0.1:65432" ,socket_to_ip_port_string(&(new_sock->ss_dest)).c_str()) << "destination ip port does not match";

/**********************emulate remote client writing to us *************/
  char msg[] = "In a galaxy far far away ...";
  rc = write_socket(remote_sipp_socket, msg, strlen(msg), WS_BUFFER, &local_sockaddr);
  EXPECT_EQ(strlen(msg),(size_t)rc)<<"sent chars doesnt match message size";

/********************** check for server side receipt of mesg **********/
// empty_socket will receive message into buffer and link sipp_socket->ss_in->buf to it
  rc = empty_socket(new_sock);
  EXPECT_EQ((long int)strlen(msg),(long int)rc) << "received messaged length does not match length of msg sent";
  EXPECT_EQ(strlen(msg),new_sock->ss_in->len);
  char output[64];
  memset(output,0,sizeof(output));
  strncpy(output,new_sock->ss_in->buf,new_sock->ss_in->len);
  EXPECT_STREQ(msg,output);

/****************** send response from server to client *************/
  char response[] = "So Say we All!!!";
  rc = write_socket(new_sock, response, strlen(response), WS_BUFFER, &remote_sockaddr);
  EXPECT_EQ((long int)strlen(response), (long int)rc)<< "response chars doesnt match response size";

/****************** check for client side receipt of response *************/
  rc = empty_socket(remote_sipp_socket);
  EXPECT_EQ((long int)strlen(response),(long int)rc) << "received messaged length does not match length of msg sent";
  EXPECT_EQ(strlen(response),remote_sipp_socket->ss_in->len);

  memset(output,0,sizeof(output));
  strncpy(output,remote_sipp_socket->ss_in->buf,remote_sipp_socket->ss_in->len);
  EXPECT_STREQ(response,output);

/***************** clean up ***********************************************/
  sipp_close_socket(main_socket);
  sipp_close_socket(remote_sipp_socket);
  sipp_close_socket(new_sock);

  //make sure we closed all sockets
  EXPECT_EQ(0,pollnfds);
  cleanup_sockets();
};




TEST(sockethandler, open_connections_udp_ipv6){
  initialize_sockets();
  //set up required globals
  local_ip_is_ipv6 = true;
  transport = T_UDP;
  strncpy (remote_host, "[::1]:65432",255);
  strncpy (local_ip, "::1",40);
  local_port = 23456;
  buff_size = 65534; 
  
  EXPECT_EQ(0,pollnfds) << "Socket counter should be zero before testing starts";
  // from remote_host fills in remote_sockaddr, remote_host, remote_port
  // from local_ip fill in local_host, local_addr_storage, local_sockaddr
  //   open_connections updates local_sockaddr with user_port and
  //   binds main_socket to local_sockaddr
  determine_remote_and_local_ip();
  EXPECT_EQ(65432, remote_port);
  EXPECT_STREQ("::1",remote_host);
  EXPECT_STREQ("[::1]:65432",socket_to_ip_port_string(&remote_sockaddr).c_str());
  
  // some options that affect open_connections that we dont want to exercise here
  twinSippMode = false;
  extendedTwinSippMode = false;
  use_remote_sending_addr = false;
  multisocket = 0; // command line option t1 vs tn
  peripsocket =0;
  sendMode = MODE_SERVER;
  
  // open_connection = socket, setsockopt,bind, listen
  open_connections();
  EXPECT_TRUE(main_socket != NULL);
  EXPECT_EQ(true, main_socket->ss_ipv6);
  EXPECT_EQ(T_UDP, main_socket->ss_transport);
  EXPECT_EQ(1,pollnfds);
  // sipp_customize_socket called by open_connections
  
/*************check effect of sipp_customize_socket *********************/
 
  int rc;
  int mybuffer_size;
  socklen_t sizeof_mybuffer_size = sizeof(mybuffer_size);
  if ((rc = getsockopt(main_socket->ss_fd, SOL_SOCKET, SO_RCVBUF, 
     (char*) &mybuffer_size, &sizeof_mybuffer_size)) == -1){
    EXPECT_NE(-1, rc)<<"failed to retrieve SO_RCVBUF option";
    print_if_error(rc);
  }else{
    EXPECT_GE(mybuffer_size,buff_size) << "allocated SO_RCVBUF buffer size is less than requested";
    // note testing shows that linux always reports twice the buffer size that was requested
    // hence test altered to GE
  }
  if ((rc = getsockopt(main_socket->ss_fd, SOL_SOCKET, SO_SNDBUF, 
     (char*) &mybuffer_size, &sizeof_mybuffer_size)) == -1){
    EXPECT_NE(-1, rc)<<"failed to retrieve SO_SNDBUF option";
    print_if_error(rc);
  }else{
    EXPECT_GE(mybuffer_size,buff_size) << "allocated SO_SNDBUF buffer size is less than requested";
    // note testing shows that linux always reports twice the buffer size that was requested
  }

  // get sockaddr associated with main_socket for later use
  sockaddr_storage main_saddr;
  socklen_t size_main_saddr = sizeof(main_saddr);
  getsockname(main_socket->ss_fd, (sockaddr *) &main_saddr, &size_main_saddr);

/**********************emulate remote client connecting to us *************/
  struct sipp_socket* remote_sipp_socket = new_sipp_socket(local_ip_is_ipv6,transport);
  EXPECT_EQ(2,pollnfds);

  rc = sipp_bind_socket(remote_sipp_socket, &remote_sockaddr,&remote_port);
  EXPECT_EQ(0,rc)<<"failed to bind to remote address port";
  print_if_error(rc);

/********************server side accept connection ****************/
  //open_connections listen(main_socket)
  //new_sock for tcp accept, for udp use main_socket as call socket as well
  struct sipp_socket *new_sock = main_socket;

/**********************emulate remote client writing to us *************/
  char msg[] = "In a galaxy far far away ...";
  rc = write_socket(remote_sipp_socket, msg, strlen(msg), WS_BUFFER, &local_sockaddr);
  EXPECT_EQ((long int)strlen(msg),(long int)rc)<<"sent chars doesnt match message size";

/********************** check for server side receipt of mesg **********/
// empty_socket will receive message into buffer and link sipp_socket->ss_in->buf to it
  rc = empty_socket(new_sock);
  EXPECT_EQ((long int)strlen(msg),(long int)rc) << "received messaged length does not match length of msg sent";
  EXPECT_EQ(strlen(msg),new_sock->ss_in->len);
  char output[64];
  memset(output,0,sizeof(output));
  strncpy(output,new_sock->ss_in->buf,new_sock->ss_in->len);
  EXPECT_STREQ(msg,output);

/****************** send response from server to client *************/
  char response[] = "So Say we All!!!";
  rc = write_socket(new_sock, response, strlen(response), WS_BUFFER, &remote_sockaddr);
  EXPECT_EQ((long int)strlen(response),(long int) rc)<< "response chars doesnt match response size";

/****************** check for client side receipt of response *************/
  rc = empty_socket(remote_sipp_socket);
  EXPECT_EQ((long int)strlen(response),(long int)rc) << "received messaged length does not match length of msg sent";
  EXPECT_EQ(strlen(response),remote_sipp_socket->ss_in->len);

  memset(output,0,sizeof(output));
  strncpy(output,remote_sipp_socket->ss_in->buf,remote_sipp_socket->ss_in->len);
  EXPECT_STREQ(response,output);

/***************** clean up ***********************************************/
  sipp_close_socket(main_socket);
  sipp_close_socket(remote_sipp_socket);
//  sipp_close_socket(new_sock);

  //make sure we closed all sockets
  EXPECT_EQ(0,pollnfds);
  cleanup_sockets();
};


TEST(sockethandler, open_connections_udp_ipv4){
  initialize_sockets();
  //set up required globals
  local_ip_is_ipv6 = false;
  transport = T_UDP;
  strncpy (remote_host, "127.0.0.1:65432",255);
  strncpy (local_ip, "127.0.0.1",40);
  local_port = 23456;
  buff_size = 65534; 
  
  EXPECT_EQ(0,pollnfds) << "Socket counter should be zero before testing starts";
  // from remote_host fills in remote_sockaddr, remote_host, remote_port
  // from local_ip fill in local_host, local_addr_storage, local_sockaddr
  //   open_connections updates local_sockaddr with user_port and
  //   binds main_socket to local_sockaddr
  determine_remote_and_local_ip();
  EXPECT_EQ(65432, remote_port);
  EXPECT_STREQ("127.0.0.1",remote_host);
  EXPECT_STREQ("127.0.0.1:65432",socket_to_ip_port_string(&remote_sockaddr).c_str());
  
  // some options that affect open_connections that we dont want to exercise here
  twinSippMode = false;
  extendedTwinSippMode = false;
  use_remote_sending_addr = false;
  multisocket = 0; // command line option t1 vs tn
  peripsocket =0;
  sendMode = MODE_SERVER;
  
  // open_connection = socket, setsockopt,bind, listen
  open_connections();
  EXPECT_TRUE(main_socket != NULL);
  EXPECT_EQ(false, main_socket->ss_ipv6);
  EXPECT_EQ(T_UDP, main_socket->ss_transport);
  EXPECT_EQ(1,pollnfds);
  // sipp_customize_socket called by open_connections
  
/*************check effect of sipp_customize_socket *********************/
 
  int rc;
  int mybuffer_size;
  socklen_t sizeof_mybuffer_size = sizeof(mybuffer_size);
  if ((rc = getsockopt(main_socket->ss_fd, SOL_SOCKET, SO_RCVBUF, 
     (char*) &mybuffer_size, &sizeof_mybuffer_size)) == -1){
    EXPECT_NE(-1, rc)<<"failed to retrieve SO_RCVBUF option";
    print_if_error(rc);
  }else{
    EXPECT_GE(mybuffer_size,buff_size) << "allocated SO_RCVBUF buffer size is less than requested";
    // note testing shows that linux always reports twice the buffer size that was requested
    // hence test altered to GE
  }
  if ((rc = getsockopt(main_socket->ss_fd, SOL_SOCKET, SO_SNDBUF, 
     (char*) &mybuffer_size, &sizeof_mybuffer_size)) == -1){
    EXPECT_NE(-1, rc)<<"failed to retrieve SO_SNDBUF option";
    print_if_error(rc);
  }else{
    EXPECT_GE(mybuffer_size,buff_size) << "allocated SO_SNDBUF buffer size is less than requested";
    // note testing shows that linux always reports twice the buffer size that was requested
  }

  // get sockaddr associated with main_socket for later use
  sockaddr_storage main_saddr;
  socklen_t size_main_saddr = sizeof(main_saddr);
  getsockname(main_socket->ss_fd, (sockaddr *) &main_saddr, &size_main_saddr);

/**********************emulate remote client connecting to us *************/
  struct sipp_socket* remote_sipp_socket = new_sipp_socket(local_ip_is_ipv6,transport);
  EXPECT_EQ(2,pollnfds);

  rc = sipp_bind_socket(remote_sipp_socket, &remote_sockaddr,&remote_port);
  EXPECT_EQ(0,rc)<<"failed to bind to remote address port";
  print_if_error(rc);

/********************server side accept connection ****************/
  //open_connections listen(main_socket)
  //new_sock for tcp accept, for udp use main_socket as call socket as well
  struct sipp_socket *new_sock = main_socket;

/**********************emulate remote client writing to us *************/
  char msg[] = "In a galaxy far far away ...";
  rc = write_socket(remote_sipp_socket, msg, strlen(msg), WS_BUFFER, &local_sockaddr);
  EXPECT_EQ((long int)strlen(msg),(long int)rc)<<"sent chars doesnt match message size";

/********************** check for server side receipt of mesg **********/
// empty_socket will receive message into buffer and link sipp_socket->ss_in->buf to it
  rc = empty_socket(new_sock);
  EXPECT_EQ((long int)strlen(msg),(long int)rc) << "received messaged length does not match length of msg sent";
  EXPECT_EQ(strlen(msg),new_sock->ss_in->len);
  char output[64];
  memset(output,0,sizeof(output));
  strncpy(output,new_sock->ss_in->buf,new_sock->ss_in->len);
  EXPECT_STREQ(msg,output);

/****************** send response from server to client *************/
  char response[] = "So Say we All!!!";
  rc = write_socket(new_sock, response, strlen(response), WS_BUFFER, &remote_sockaddr);
  EXPECT_EQ((long int)strlen(response),(long int) rc)<< "response chars doesnt match response size";

/****************** check for client side receipt of response *************/
  rc = empty_socket(remote_sipp_socket);
  EXPECT_EQ((long int)strlen(response),(long int)rc) << "received messaged length does not match length of msg sent";
  EXPECT_EQ(strlen(response),remote_sipp_socket->ss_in->len);

  memset(output,0,sizeof(output));
  strncpy(output,remote_sipp_socket->ss_in->buf,remote_sipp_socket->ss_in->len);
  EXPECT_STREQ(response,output);



/***************** clean up ***********************************************/
  sipp_close_socket(main_socket);
  sipp_close_socket(remote_sipp_socket);
//  sipp_close_socket(new_sock);

  //make sure we closed all sockets
  EXPECT_EQ(0,pollnfds);
  cleanup_sockets();
};


/******************************************/
#ifdef WIN32
//get windows socket error string
TEST(sockethandler, error_message){

  initialize_sockets();
  struct sockaddr_storage ss;
  memset(&ss,0,sizeof(ss));
  SOCKREF asocket = socket(AF_INET,SOCK_DGRAM,0);
  int rc = bind(asocket,(sockaddr*)&ss,sizeof(ss));
  EXPECT_EQ(-1,rc) << "binding to uninitialized sockaddr_storage should fail";
  char errorstring[1000];
  wchar_to_char(wsaerrorstr(WSAGetLastError()),errorstring);
  EXPECT_EQ(10047, WSAGetLastError());
  EXPECT_STREQ("An address incompatible with the requested protocol was used.\r\n",errorstring) << "error string doesnt match expected";
  
  SOCKREF bsocket = socket(AF_INET,SOCK_DGRAM,100);
  EXPECT_EQ(10043, WSAGetLastError());
  wchar_to_char(wsaerrorstr(WSAGetLastError()),errorstring);
  EXPECT_STREQ("The requested protocol has not been configured into the system, or no implementation for it exists.\r\n",errorstring);

  char msg[] = "Good Luck Chuck";
  send(asocket,msg,strlen(msg),0);
  EXPECT_EQ(10057, WSAGetLastError());
  wchar_to_char(wsaerrorstr(WSAGetLastError()),errorstring);
  EXPECT_STREQ("A request to send or receive data was disallowed because the socket is not connected and (when sending on a datagram socket using a sendto call) no address was supplied.\r\n",errorstring);

  char buf[1000];
  recv(bsocket,buf,sizeof(buf),0);
  EXPECT_EQ(10038, WSAGetLastError());
  wchar_to_char(wsaerrorstr(WSAGetLastError()),errorstring);
  EXPECT_STREQ("An operation was attempted on something that is not a socket.\r\n",errorstring);

  cleanup_sockets();

}
#endif

// read_error
// write_error, 
extern int write_error(struct sipp_socket *socket, int ret);
TEST(sockethandler, write_error){
  initialize_sockets();
  
  int copy_nb_net_send_errors = nb_net_send_errors;

  char ipaddr[256] = "127.0.0.1:65432";
  strncpy(remote_host, ipaddr ,255);
  determine_remote_ip();
  // fills out remote_host, remote_port, remote_sockaddr
  EXPECT_STREQ(ipaddr,socket_to_ip_port_string(&remote_sockaddr).c_str());
  EXPECT_STREQ("127.0.0.1", remote_host);
  EXPECT_EQ(65432,remote_port);

  strncpy(local_ip, "127.0.0.1",40);
  determine_local_ip();
  // fills out local_addr_storage  hostname[80]  local_ip[40]  local_sockaddr
  // port normally set by open_connections from user_port into local_sockaddr which bind uses.
  user_port = 23456;
  (_RCAST(struct sockaddr_in *, &local_sockaddr))->sin_port = htons((short)user_port);


  // do something to trigger WSAGetLastError or errno
  //EAGAIN or EWOULDBLOCK sets ss_congestion and events |= POLLOUT
  // tcp EPIPE  close, ss_fd=invalid, add sockets_pending_reset - warning or report error
  // udp nb_net_send_errors++

  int ret = 0;  // for sip_tls_error_string, not exercised here
  //set reconnect_allowed to prevent triggering REPORT_ERROR
  reset_number = -1;

  sipp_socket* asocket = new_sipp_socket(false,T_UDP);
  EXPECT_TRUE(asocket->ss_fd != INVALID_SOCKET)<<"failed to allocate socket";
  
  int myport = 0;
  int res = sipp_bind_socket(asocket,&local_sockaddr,&myport);
  print_if_error(res);
  EXPECT_EQ(0,res)<<"bind failed";
  EXPECT_STREQ("127.0.0.1:23456", fd_to_ipstring(asocket->ss_fd).c_str());
  EXPECT_EQ(23456, myport);
  EXPECT_EQ(23456, local_port);
  EXPECT_EQ(23456, user_port);
  EXPECT_STREQ("127.0.0.1:23456", socket_to_ip_port_string(&local_sockaddr).c_str());

  // do something to trigger WSAGetLastError or errno
  //EAGAIN or EWOULDBLOCK sets ss_congestion and events |= POLLOUT
  // tcp EPIPE  close, ss_fd=invalid, add sockets_pending_reset - warning or report error
  // udp nb_net_send_errors++
//  udp  rc = sendto(socket->ss_fd, buffer, len, 0, (struct sockaddr *)dest, SOCK_ADDR_SIZE(dest));
//  tcp  rc = send_nowait(socket->ss_fd, buffer, len, 0);
  char buff[] = "This is a Test,  This is only a Test.  Do not panic......yet";
  int rc = sendto(asocket->ss_fd, buff, strlen(buff), 0, (sockaddr*)&remote_sockaddr, sizeof(local_sockaddr));
  print_if_error(rc);
  rc = write_error(asocket, ret);
  EXPECT_EQ(copy_nb_net_send_errors +1, nb_net_send_errors);
  
  sockaddr_storage null_ss;
  memset (&null_ss,0,sizeof(null_ss));
  rc = sendto(asocket->ss_fd, buff, strlen(buff), 0, (sockaddr*)&remote_sockaddr, sizeof(local_sockaddr));

  print_if_error(rc);
  rc = write_error(asocket, ret);
  EXPECT_EQ(copy_nb_net_send_errors +2, nb_net_send_errors);

  sipp_close_socket(asocket);
  cleanup_sockets();
}





