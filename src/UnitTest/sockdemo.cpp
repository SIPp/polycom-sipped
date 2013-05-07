#include "gtest/gtest.h"

#include <stdio.h>
#include <stdlib.h>
#ifndef WIN32
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#else
#include <WinSock2.h>
#include <ws2tcpip.h>
#endif

#include "win32_compatibility.hpp"
#include "sipp_sockethandler.hpp"
#include "socket_helper.hpp"

#include <cmath> //modf

/*
This does not test anything to do with sipp.  Focus is on getting 
basic sockets working on windows vs cygwin vs linux.  This should
highlight any underlying socket api differences on platforms.


NOTE: this test tends to fail on linux if you rerun the test back-to-back since
  we establish tcp loop arounds with specified ports.  The connections 
  are established and then closed but stay connected in a TIME_WAIT state
  which prevents the second iteration from accessing the same ports.
  So far seems to be an issue on linux but not cygwin or win32
todo: look at using dynamically assigned ports instead of fixed ports.

Flow of this test is to use socket routines directly as follows
  sockfd = socket
  bind 127.0.0.1:55432

  r_sockfd = socket
  bind 127.0.0.1:23455
  remote_sockaddr

  sendto(sockfd, msg, strlen(msg),0, remote_sockaddr, remote_addrlen );
  recvfrom(r_sockfd,buf,sizeof(buf),0,    (struct sockaddr*)&from, &fromlen);

  sendto(r_sockfd, bmsg, strlen(bmsg),0, local_sockaddr, local_addrlen );
  recvfrom(sockfd,buf,sizeof(buf),0,    (struct sockaddr*)&from, &fromlen);

Test is done on all combinations of udp/tcp and ipv4/ipv6
*/


int loopit(int argc, char** argv)
{
    SOCKREF newfd;
    SOCKREF mainsock;

    bool verbose = true;
    char ipstr[INET6_ADDRSTRLEN];
    char hostname[256];
    bool is_tcp = false;

    addrinfo hints, *res;
    memset(&hints,0,sizeof (hints));
    hints.ai_flags = AI_PASSIVE;

if (argc==1){
    //noargs, udp ipv4, looparound address
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    strncpy(hostname,"127.0.0.1", sizeof(hostname));
}else if (argc ==2){
    //one arg = address to use
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    strncpy(hostname,argv[1],sizeof(hostname));
}else if (argc ==3){
    //2 args, address, transport SOCK_STREAM=1, SOCK_DGRAM=2
    strncpy(hostname,argv[1],sizeof(hostname));
    hints.ai_socktype = atoi(argv[2]);
    if (atoi(argv[2])==SOCK_STREAM)
      is_tcp = true;
    hints.ai_family = AF_INET;
}else if (argc == 4){
    //3 args, address, transport, Address Family, PF_INET=2, PF_INET6=10
    strncpy(hostname,argv[1],sizeof(hostname));
    hints.ai_socktype = atoi(argv[2]);
    if (atoi(argv[2])==SOCK_STREAM)
      is_tcp = true;
    if (strncmp("AF_INET6",argv[3],strlen("AF_INET6"))==0){
      hints.ai_family = AF_INET6;
    }else{
      hints.ai_family = AF_INET;
    }
}

    int astatus;
    if ((astatus = getaddrinfo(hostname,"0",&hints,&res)) != 0) {
#ifdef WIN32
        char errorstring[1000];
        wchar_to_char(wsaerrorstr(WSAGetLastError()),errorstring);
        printf("%s\n",errorstring);
#else
        printf("getaddrinfo failed : %s\n",gai_strerror(astatus));
#endif
        exit(1);
    }
    addrinfo *p;
    int port;
    for (p=res;p!=NULL;p=p->ai_next){
        void* addr;
        char ipver[24];
        if (p->ai_family == AF_INET){
            struct sockaddr_in *ipv4 = (struct sockaddr_in*)p->ai_addr;
            addr = &(ipv4->sin_addr);
            strncpy(ipver,"IPv4",sizeof (ipver));
            port = htons(ipv4->sin_port);
        }else{
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6*)p->ai_addr;
            addr = &(ipv6->sin6_addr);
            strncpy(ipver,"IPv6",sizeof (ipver));
            port = htons(ipv6->sin6_port);
        }

        inet_ntop(p->ai_family, addr, ipstr, sizeof(ipstr));
        if (verbose)
          printf("local getaddrinfo results %s: %s:%d, protocol %d\n", ipver, ipstr, port, p->ai_protocol);
    }

// get local socket into sockfd, local_sockaddr
    SOCKREF sockfd;
    if ((sockfd = socket(res->ai_family,res->ai_socktype,res->ai_protocol)) == -1)
        perror("Could not allocate local socket:");
    int bstatus;
    if ((bstatus=bind(sockfd, (struct sockaddr*)res->ai_addr, sizeof(sockaddr_storage))) == -1)
        perror("Could not bind to local socket:");

    struct sockaddr local_sockaddr;
    socklen_t local_addrlen = sizeof(local_sockaddr);
    if (getsockname(sockfd,&local_sockaddr, &local_addrlen) != 0){
#ifdef WIN32
      int error = WSAGetLastError();
      printf("local getsockname failed, errorcode %d\n", error);
      wprintf(L"%s",wsaerrorstr(error));
#else
      perror("failed to get socketname from local sockfd;");
#endif
    } else {
      if (verbose)
        printf("local address from socket: %s\n",socket_to_ip_port_string((struct sockaddr_storage*)&local_sockaddr).c_str());
    }

    struct sockaddr remote_sockaddr;
    socklen_t remote_addrlen = sizeof(remote_sockaddr);
    if ((astatus = getaddrinfo(hostname,"0", &hints, &res)) != 0){
        printf("getaddrinfo failed : %s\n", gai_strerror(astatus));
        exit(1);
    } else {
        addrinfo* p = res;
        void* addr;
        char ipver[24];
        if (p->ai_family == AF_INET){
            struct sockaddr_in *ipv4 = (struct sockaddr_in*)p->ai_addr;
            addr = &(ipv4->sin_addr);
            strncpy(ipver,"IPv4",sizeof (ipver));
            port = htons(ipv4->sin_port);
        }else{
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6*)p->ai_addr;
            addr = &(ipv6->sin6_addr);
            strncpy(ipver,"IPv6",sizeof (ipver));
            port = htons(ipv6->sin6_port);
        }
        inet_ntop(p->ai_family, addr, ipstr, sizeof(ipstr));
        if (verbose)
          printf("remote getaddrinfo results %s: %s:%d, protocol %d\n", ipver, ipstr, port, p->ai_protocol);

    }
// get remote socket into r_sockfd, remote_sockaddr
    SOCKREF r_sockfd;
    if ((r_sockfd = socket(res->ai_family,res->ai_socktype,res->ai_protocol)) == -1)
        perror("Could not allocate remote socket:");

    if ((bstatus=bind(r_sockfd, (struct sockaddr*)res->ai_addr, sizeof(sockaddr_storage))) == -1)
        perror("Could not bind to remote socket:");

// verify inputs: local socket (sockfd) and remote socket(r_sockfd), remote_sockaddr, local_sockaddr

    if (getsockname(r_sockfd, &remote_sockaddr, &remote_addrlen) != 0) {
#ifdef WIN32
      int error = WSAGetLastError();
      printf("remote getsockname failed, errorcode %d\n", error);
      wprintf(L"%s",wsaerrorstr(error));
#else
      perror("failed to get socketname from remote r_sockfd;");
#endif
    } else {
      if (verbose)
        printf("remote address from socket: %s\n", socket_to_ip_port_string((struct sockaddr_storage*)&remote_sockaddr).c_str());
    }

    if (verbose) {
      printf("remote_sockaddr: %s\n",socket_to_ip_port_string((sockaddr_storage*)&remote_sockaddr).c_str());
      printf("local_sockaddr: %s\n",socket_to_ip_port_string((sockaddr_storage*)&local_sockaddr).c_str());
    }



    if (is_tcp){
// listen on local
      int lstatus;
      int backlog = 5; // accept upto 5 connections
      if ((lstatus = listen(sockfd, backlog)) !=0){
#ifdef WIN32
        printf("listen on socket failed %s failed\n",
          socket_to_ip_port_string((sockaddr_storage*)local_sockaddr).c_str());
        wprintf(L"%s\n",wsaerrorstr(WSAGetLastError()));
#else
        perror("failed to connect: ");
#endif
      }


// connect from remote to local 
      int cstatus; 
      //connect from remote to local
      if ((cstatus = connect(r_sockfd, &local_sockaddr, local_addrlen) ) != 0){
#ifdef WIN32
        printf("connect to %s failed\n",
          socket_to_ip_port_string((sockaddr_storage*)local_sockaddr).c_str());
        wprintf(L"%s\n",wsaerrorstr(WSAGetLastError()));
#else
        perror("failed to connect: ");
#endif
        EXPECT_EQ(0,cstatus)<< "failed to establish tcp connection";
      }

//accept on local
      struct sockaddr_storage new_connection;
      socklen_t newconn_size = sizeof(new_connection);
      if((newfd = accept(sockfd, (struct sockaddr*)&new_connection,&newconn_size) ) == INVALID_SOCKET){
#ifdef WIN32
        printf("accepting local socket  failed\n");
        wprintf(L"%s\n",wsaerrorstr(WSAGetLastError()));
#else
        perror("failed to accept on local_socket: ");
#endif
      }else{
        if (verbose) printf("accepted connection from %s\n", socket_to_ip_port_string((sockaddr_storage*)&new_connection).c_str());
        //swap names around so std remaining code below will still work 
        //with sockfd for both udp and tcp
        mainsock = sockfd;
        sockfd = newfd;
      }
      //what is address associated with newly assigned sockfd
      struct sockaddr name;
      socklen_t namelen = sizeof(name);
      if (getsockname(sockfd,(struct sockaddr*)&name, &namelen) !=0){
#ifdef WIN32
      int error = WSAGetLastError();
      printf("getsockname failed, errorcode %d\n", error);
      wprintf(L"%s",wsaerrorstr(error));
#else
      perror("failed to get socketname from r_sockfd;");
#endif
    }else{
      if(verbose)
        printf("newly accepted call socket: %s\n",socket_to_ip_port_string((struct sockaddr_storage*)&name).c_str());
    }


    }


    const char* msg = "drahcir mul saw reven ereh!";
    int sent = sendto(sockfd, msg, (int)strlen(msg),0, &remote_sockaddr, remote_addrlen );
    if (sent&&verbose)
        printf("sent from sockfd to remote_sockaddr %d letters\n", sent);

    const char* bmsg = "eht rewsna ym drneirf si gniwolb ni eht dniw";
    int sent_to_local = sendto(r_sockfd, bmsg, (int)strlen(bmsg),0, &local_sockaddr, local_addrlen );
    if (sent&&verbose)
        printf("sent from r_sockfd to local_sockaddr %d letters\n", sent_to_local);


    struct sockaddr_storage from;
#ifdef WIN32
    socklen_t fromlen = sizeof(from);    
#else
    socklen_t fromlen = sizeof(from);
#endif
    char buf[4096];
    memset(buf,0,sizeof(buf));
    int received;
    if ((received = recvfrom(r_sockfd,buf,sizeof(buf),0,
          (struct sockaddr*)&from, &fromlen)) == -1){
      printf("rec error, ret = %d\n", received);
#ifdef WIN32
      printf("ErrorCode = %d\n",WSAGetLastError());
      char errorstring[1000];
      wchar_to_char(wsaerrorstr(WSAGetLastError()),errorstring);
      printf("%s\n",errorstring);
#else
      perror("recfrom failed:");
#endif
    }else{
      if (verbose)
        printf ("received %d : %s\n",received, buf);
    }
    EXPECT_STREQ(msg,buf)<<"did not receive what was sent from sockfd to remote_sockaddr";

    void* addr;
    //int port;
    if (received){
      if(verbose)
        printf ("received %d letters : %s\n", received, buf);
      if (from.ss_family == AF_INET){
          addr = (void*)&(((struct sockaddr_in*)&from)->sin_addr);
          //addr = &(ipv4->sin_addr);
          port = htons( ((struct sockaddr_in*)&from)->sin_port);
      }else{
          addr = &((struct sockaddr_in6*)&from)->sin6_addr;
          port = htons( ((struct sockaddr_in6*)&from)->sin6_port);
      }
      if (!is_tcp){
        //info from recfrom is not updated on tcp - only udp 
        char astr[4096];
        memset(astr,0,sizeof(astr));
        inet_ntop(from.ss_family,addr ,astr, sizeof(ipstr));
        if (verbose)
          printf("sender : %s:%d\n", astr, port); 
        EXPECT_STREQ(socket_to_ip_string((sockaddr_storage*)&local_sockaddr).c_str(),astr)<<"sender should be remote address";
      }

    }


    int localreceived;
    if ((localreceived = recvfrom(sockfd,buf,sizeof(buf),
        0,(struct sockaddr*)&from, &fromlen)) == -1){
      printf("rec error, localreceived = %d\n", localreceived);
#ifdef WIN32
      printf("ErrorCode = %d\n",WSAGetLastError());
      char errorstring[1000];
      wchar_to_char(wsaerrorstr(WSAGetLastError()),errorstring);
      printf("%s\n",errorstring);
#else
      perror("recfrom failed:");
#endif
    }else{
      if(verbose)
        printf ("received %d : %s\n",localreceived, buf);
    }
    EXPECT_STREQ(bmsg,buf)<<"did not receive what was sent from sockfd to remote_sockaddr";


    //void* addr;
    //int port;
    if (localreceived){
        if (verbose)
          printf ("received %d letters : %s\n", localreceived, buf);
        if (from.ss_family == AF_INET){
            addr = (void*)&(((struct sockaddr_in*)&from)->sin_addr);
            //addr = &(ipv4->sin_addr);
            port = htons( ((struct sockaddr_in*)&from)->sin_port);
        }else{
            addr = &((struct sockaddr_in6*)&from)->sin6_addr;
            port = htons( ((struct sockaddr_in6*)&from)->sin6_port);
        }
        if (!is_tcp){
          char astr[4096];
          memset(astr,0,sizeof(astr));
          inet_ntop(from.ss_family,addr ,astr, sizeof(ipstr));
          if (verbose) printf("sender : %s:%d\n", astr, port); 
          EXPECT_STREQ(socket_to_ip_string((sockaddr_storage*)&remote_sockaddr).c_str(),astr)<<"sender should be remote address";
        }
    }


    CLOSESOCKET(sockfd);
    CLOSESOCKET(r_sockfd);
    CLOSESOCKET(mainsock);
  return received;

}
    


TEST(sockdemo, ipv4_udp){
  char *argv[32];
  int argc = 0;
  char arg1[] = "ipv4_udp";
  argv[argc++] = arg1;
  //strncpy(argv[argc++],"ipv4_udp",32);
  int rc;
  initialize_sockets();
  rc = loopit(argc, argv);
  cleanup_sockets();
  EXPECT_EQ(27, rc)<<"did not receive sent message on remote socket";
  
}

TEST(sockdemo, ipv6_udp){
  char *argv[32];
  int argc = 0;
  char arg0[] = "ipv6_udp";
  argv[argc++] = arg0;
  char arg1[] = "::1";
  argv[argc++] = arg1;
  char arg2[] = "2"; // SOCK_DGRAM
  argv[argc++] = arg2;
  char arg3[] = "AF_INET6"; 
  argv[argc++] = arg3;

  int rc;
  initialize_sockets();
  rc = loopit(argc, argv);
  cleanup_sockets();
  EXPECT_EQ(27, rc)<<"did not receive sent message on remote socket";
  
}


TEST(sockdemo, ipv4_tcp){
  char *argv[32];
  int argc = 0;
  char arg0[] = "ipv4_tcp";
  argv[argc++] = arg0;
  char arg1[] = "127.0.0.1";
  argv[argc++] = arg1;
  char arg2[] = "1"; // SOCK_STREAM
  argv[argc++] = arg2;
  char arg3[] = "AF_INET"; 
  argv[argc++] = arg3;

  int rc;
  initialize_sockets();
  rc = loopit(argc, argv);
  cleanup_sockets();
  EXPECT_EQ(27, rc)<<"did not receive sent message on remote socket";
  
}


TEST(sockdemo, ipv6_tcp){
  char *argv[32];
  int argc = 0;
  char arg0[] = "ipv6_tcp";
  argv[argc++] = arg0;
  char arg1[] = "::1";
  argv[argc++] = arg1;
  char arg2[] = "1"; // SOCK_STREAM
  argv[argc++] = arg2;
  char arg3[] = "AF_INET6"; 
  argv[argc++] = arg3;

  int rc;
  initialize_sockets();
  rc = loopit(argc, argv);
  cleanup_sockets();
  EXPECT_EQ(27, rc)<<"did not receive sent message on remote socket";

  
}


TEST(sockdemo, polldemo){
  initialize_sockets();
  SOCKREF s1, s2,serv;
  int rv;
  char buf1[256], buf3out[256];
  struct pollfd ufds[5];


  sockaddr_storage ss;
  ss.ss_family = AF_INET;
  inet_pton(AF_INET, "127.0.0.1", &(((sockaddr_in*)&ss)->sin_addr));
  ((sockaddr_in*)&ss)->sin_port = htons(65432);
  EXPECT_STREQ("127.0.0.1:65432",socket_to_ip_port_string(&ss).c_str());

  serv = socket(PF_INET, SOCK_STREAM,0);
  if (serv == INVALID_SOCKET)
#ifdef WIN32
    ASSERT_NE(INVALID_SOCKET, serv) << WSAGetLastError();
#else
  {
    perror("failed to create socket");
    ASSERT_NE(INVALID_SOCKET,serv);
  }
#endif


  int rc = bind(serv, (struct sockaddr*)&ss, sizeof(ss));
  print_if_error(rc);
  EXPECT_EQ(0,rc)<<"failed to bind server";
  rc = listen(serv,10);
  print_if_error(rc);
  EXPECT_EQ(0,rc)<<"server failed to listen";


  s1 = socket(PF_INET, SOCK_STREAM, 0);
  s2 = socket(PF_INET, SOCK_STREAM, 0);

  rc = connect(s1, (sockaddr*)&ss, sizeof(ss));
  print_if_error(rc);
  EXPECT_EQ(0,rc)<<"failed to connect s1";
  rc = connect(s2, (sockaddr*)&ss, sizeof(ss));
  print_if_error(rc);
  EXPECT_EQ(0,rc)<<"failed to connect s2";

  struct sockaddr_storage addr1;
  socklen_t  sizeof_addr1 = sizeof (addr1);
  SOCKREF call_1 = accept(serv,(sockaddr*)&addr1, &sizeof_addr1);
  if (call_1 == INVALID_SOCKET)
#ifdef WIN32
    ASSERT_NE(INVALID_SOCKET, serv) << WSAGetLastError();
#else
  {
    perror("failed to create socket");
    ASSERT_NE(INVALID_SOCKET,serv);
  }
#endif

  struct sockaddr_storage addr2;
  socklen_t sizeof_addr2 = sizeof (addr2);
  SOCKREF call_2 = accept(serv, (sockaddr*)&addr2, &sizeof_addr2);
  if (call_1 == INVALID_SOCKET)
#ifdef WIN32
    ASSERT_NE(INVALID_SOCKET, serv) << WSAGetLastError();
#else
  {
    perror("failed to create socket");
    ASSERT_NE(INVALID_SOCKET,serv);
  }
#endif

  // set up the array of file descriptors.
  //
  // in this example, we want to know when there's normal or out-of-band
  // data ready to be recv()'d...

  // WSAPoll chokes on POLLERR as input event, it will use POLLERR as a revent
  // as required though so this should make no functional difference if we exclude 
  // POLLERR from events.

  ufds[0].fd = s1;
  ufds[0].events =  POLLIN | POLLOUT ;

  ufds[1].fd = s2;
  ufds[1].events = POLLIN | POLLOUT; // check for just normal data

  ufds[2].fd = serv;
  ufds[2].events = POLLIN ;

  ufds[3].fd = call_1;
  ufds[3].events = POLLIN | POLLOUT;

  ufds[4].fd = call_2;
  ufds[4].events = POLLIN | POLLOUT;

  int loops = 40;
  int timeout = 100; // milliseconds
  int recv_count = 0;
  int send_count = 0;

  while(loops){
    rv = POLL(ufds, 5,timeout);
    EXPECT_GT(rv,0) << "polling failed to retieve an POLLIN or POLLOUT event";
    if (rv == -1) {
      //perror("poll"); // error occurred in poll()
      print_if_error(rv);
    } else if (rv == 0) {
      printf("Timeout occurred!  No data after %d milliseconds.\n",timeout);
    } else {

      int j;
      for (j=0;j<5;j++){
        if (ufds[j].revents & POLLOUT){
          sprintf(buf3out,"Hello there");
          rc = send(ufds[j].fd, buf3out, sizeof buf3out, 0);
          print_if_error(rc);
          send_count++;
        }
        if (ufds[j].revents & POLLIN){
          rc = recv(ufds[j].fd, buf1, sizeof buf1, 0); 
          print_if_error(rc);
          //printf("%d received: %s\n", j, buf1);
          EXPECT_STREQ("Hello there", buf1) << "received string doesnt match what was sent";
          recv_count++;
        }
      }
    }
    loops--;
  }


  int i;
  for (i=0;i<5;i++){
    CLOSESOCKET(ufds[i].fd);
  }
  EXPECT_GE(recv_count, 100);
  EXPECT_GE(send_count, recv_count);

  cleanup_sockets();

}

