#include "sipp_sockethandler.hpp"

#include "logging.hpp" // DEBUG
//note we can eliminate dependency on screen by throw runtime_error("text for report_error");
#include "screen.hpp" // REPORT_ERROR
#include "stat.hpp"     //CStat
#include <assert.h>
#include "socket_helper.hpp"

#ifdef WIN32
#include <Winsock2.h>
#include <errno.h>  // EPIPE
#else
#include <sys/poll.h> //pollfd
#include <sys/types.h> //EPIPE
#include <poll.h> // POLLOUT
#include "comp.hpp"  //comp_compress, COMP_OK
#include <unistd.h> //F_GETFL
#include <fcntl.h>
#include <netinet/tcp.h>  //TCP_NODELAY
#endif

POLLREF        pollfiles[SIPP_MAXFDS]; 
int            errorcode;

#ifdef WIN32
WSADATA WSStartData;
#endif

#ifdef _USE_OPENSSL
SSL_CTX  *sip_trp_ssl_ctx = NULL; /* For SSL cserver context */
SSL_CTX  *sip_trp_ssl_ctx_client = NULL; /* For SSL cserver context */
SSL_CTX  *twinSipp_sip_trp_ssl_ctx_client = NULL; /* For SSL cserver context */
#endif

void initialize_sockets()
{
#ifdef WIN32
  int rc = WSAStartup(MAKEWORD (2,0),&WSStartData);
  DEBUG("WSAStartData.szDescription = %s\n",WSStartData.szDescription);
  DEBUG("WSAStartData.szSystemStatus = %s\n", WSStartData.szSystemStatus);
  DEBUG("WSAStartData.iMaxSockets = %d\n", WSStartData.iMaxSockets);
  switch (rc){
    case 0:
      //all ok
      break;
    case WSASYSNOTREADY:
      printf("WSASYSNOTREADY: The underlying network subsystem is not ready for network communication.\n");
      break;
    case WSAVERNOTSUPPORTED:
      printf("WSAVERNOTSUPPORTED: The version of Windows Sockets support requested is not provided by this particular Sockets implementation\n");
      break;
    case WSAEINPROGRESS:
      printf("WSAEINPROGRESS: A blocking Windows Sockets 1.1 operation is in progress\n");
      break;
    case WSAEPROCLIM:
      printf("WSAEPROCLIM: A limit on the number of tasks supported by the Windows Sockets implementation has been reached.\n");
      break;
    case WSAEFAULT:
      printf("WSAEFAULT: The lpWSAData parameter is not a valid pointer.\n");
      break;
    default:
      printf("WSAStartUp error code %d\n", rc);
      break;
  }
#endif
}

void cleanup_sockets()
{
#ifdef WIN32
    char running[] ="Running";
    if (strncmp(running,WSStartData.szSystemStatus, strlen(running))==0){
      DEBUG("WSA is running, cleaning up");
      WSACleanup();
    }else{
      DEBUG("WSA is '%s' %d, no need to clean up",WSStartData.szSystemStatus,strlen(WSStartData.szSystemStatus));
    }
#endif
}

void sipp_socket_invalidate(struct sipp_socket *socket)
{
  int pollidx;

  DEBUGIN();
  if (socket->ss_invalid) {
    return;
  }

#ifdef _USE_OPENSSL
  if (SSL *ssl = socket->ss_ssl) {
    SSL_set_shutdown(ssl, SSL_SENT_SHUTDOWN|SSL_RECEIVED_SHUTDOWN);
    SSL_free(ssl);
  }
#endif

  shutdown(socket->ss_fd, SHUT_RDWR);
  CLOSESOCKET(socket->ss_fd);
  socket->ss_fd = INVALID_SOCKET;

  if((pollidx = socket->ss_pollidx) >= pollnfds) {
    REPORT_ERROR("Pollset error: index %d is greater than number of fds %d!", pollidx, pollnfds);
  }

  socket->ss_invalid = true;
  socket->ss_pollidx = -1;

  /* Adds call sockets in the array */
  assert(pollnfds > 0);

  pollnfds--;   

  pollfiles[pollidx] = pollfiles[pollnfds];
  sockets[pollidx] = sockets[pollnfds];
  sockets[pollidx]->ss_pollidx = pollidx;
  sockets[pollnfds] = NULL;

  if (socket->ss_msglen) {
    pending_messages--;
  }
  DEBUGOUT();
}

void sipp_close_socket (struct sipp_socket *socket)
{
  int count = --socket->ss_count;

  if (count > 0) {
    return;
  }

  sipp_socket_invalidate(socket);
  free(socket);
  socket = NULL;  
}




/* Free a poll buffer. */
void free_socketbuf(struct socketbuf *socketbuf)
{
  free(socketbuf->buf);
  free(socketbuf);
}

#ifdef _USE_OPENSSL
/****** SSL error handling                         *************/
const char *sip_tls_error_string(SSL *ssl, int size)
{
  int err;
  err=SSL_get_error(ssl, size);
  switch(err) {
  case SSL_ERROR_NONE:
    return "No error";
  case SSL_ERROR_ZERO_RETURN:
    return "SSL_read returned SSL_ERROR_ZERO_RETURN (the TLS/SSL connection has been closed)";
  case SSL_ERROR_WANT_WRITE:
    return "SSL_read returned SSL_ERROR_WANT_WRITE";
  case SSL_ERROR_WANT_READ:
    return "SSL_read returned SSL_ERROR_WANT_READ";
  case SSL_ERROR_WANT_CONNECT:
    return "SSL_read returned SSL_ERROR_WANT_CONNECT";
  case SSL_ERROR_WANT_ACCEPT:
    return "SSL_read returned SSL_ERROR_WANT_ACCEPT";
  case SSL_ERROR_WANT_X509_LOOKUP:
    return "SSL_read returned SSL_ERROR_WANT_X509_LOOKUP";
  case SSL_ERROR_SYSCALL:
    if(size<0) { /* not EOF */
      return strerror(ERRORNUMBER);
    } else { /* EOF */
      return "SSL socket closed on SSL_read";
    }
  case SSL_ERROR_SSL:
    return "SSL_read returned SSL_ERROR_SSL. One possibility is that the underlying TCP connection was unexpectedly closed.";
  }
  return "Unknown SSL Error.";
}

#endif

/* This socket is congestion, mark its as such and add it to the poll files. */
int enter_congestion(struct sipp_socket *socket, int again)
{
  socket->ss_congested = true;

  TRACE_MSG("Problem %s on socket  %d and poll_idx  is %d \n",
            again == EWOULDBLOCK ? "EWOULDBLOCK" : "EAGAIN",
            socket->ss_fd, socket->ss_pollidx);

  pollfiles[socket->ss_pollidx].events |= POLLOUT;
  nb_net_cong++;
  return -1;
}

//static 
int write_error(struct sipp_socket *socket, int ret)
{
  DEBUGIN();
#ifdef WIN32
  ERRORNUMBER = WSAGetLastError();
  wchar_t *error_msg = wsaerrorstr(ERRORNUMBER);
  char errorstring[1000];
  const char *errstring = wchar_to_char(error_msg,errorstring);
#else
  const char *errstring = strerror(ERRORNUMBER);
#endif

#ifndef EAGAIN
  int again = (ERRORNUMBER == EWOULDBLOCK) ? ERRORNUMBER : 0;
#else
  int again = ((ERRORNUMBER == EAGAIN) || (ERRORNUMBER == EWOULDBLOCK)) ? ERRORNUMBER : 0;

  /* Scrub away EAGAIN from the rest of the code. */
  if (ERRORNUMBER == EAGAIN) {
    ERRORNUMBER = EWOULDBLOCK;
  }
#endif

  if(again) {
    return enter_congestion(socket, again);
  }

  if (socket->ss_transport == T_TCP && ERRORNUMBER == EPIPE) {
    nb_net_send_errors++;
    CLOSESOCKET(socket->ss_fd);
    socket->ss_fd = INVALID_SOCKET;
    sockets_pending_reset.insert(socket);
    if (reconnect_allowed()) {
      WARNING("Broken pipe on TCP connection, remote peer "
              "probably closed the socket");
    } else {
      REPORT_ERROR("Broken pipe on TCP connection, remote peer "
                   "probably closed the socket");
    }
    return -1;
  }

#ifdef _USE_OPENSSL
  if (socket->ss_transport == T_TLS) {
    errstring = sip_tls_error_string(socket->ss_ssl, ret);
  }
#endif

  WARNING("Unable to send %s message: %s", TRANSPORT_TO_STRING(socket->ss_transport), errstring);
  nb_net_send_errors++;
  DEBUGOUT();
  return -1;
}







/* Allocate a socket buffer. */
struct socketbuf *alloc_socketbuf(char *buffer, size_t size, int copy, struct sockaddr_storage *dest) {
  struct socketbuf *socketbuf;

  socketbuf = (struct socketbuf *)malloc(sizeof(struct socketbuf));
  if (!socketbuf) {
    REPORT_ERROR("Could not allocate socket buffer!\n");
  }
  memset(socketbuf, 0, sizeof(struct socketbuf));
  if (copy) {
    socketbuf->buf = (char *)malloc(size);
    if (!socketbuf->buf) {
      REPORT_ERROR("Could not allocate socket buffer data!\n");
    }
    memcpy(socketbuf->buf, buffer, size);
  } else {
    socketbuf->buf = buffer;
  }
  socketbuf->len = size;
  socketbuf->offset = 0;
  if (dest) {
    memcpy(&socketbuf->addr, dest, SOCK_ADDR_SIZE(dest));
  }
  socketbuf->next = NULL;

  return socketbuf;
}




void buffer_write(struct sipp_socket *socket, char *buffer, size_t len, struct sockaddr_storage *dest)
{
  struct socketbuf *buf = socket->ss_out;

  DEBUGIN();

  if (!buf) {
    socket->ss_out = alloc_socketbuf(buffer, len, DO_COPY, dest);
    TRACE_MSG("Added first buffered message to socket %d\n", socket->ss_fd);
    return;
  }

  while(buf->next) {
    buf = buf->next;
  }

  buf->next = alloc_socketbuf(buffer, len, DO_COPY, dest);
  TRACE_MSG("Appended buffered message to socket %d\n", socket->ss_fd);
  DEBUGOUT();
}


int send_nowait(SOCKREF s, const void *msg, int len, int flags)
{
#if defined(MSG_DONTWAIT) && !defined(__SUNOS)
  return send(s, msg, len, flags | MSG_DONTWAIT);
#else

# ifdef WIN32
  int iMode = 1; //set to nonblocking
  ioctlsocket(s, FIONBIO, (u_long FAR*) &iMode);
# else
  int fd_flags = fcntl(s, F_GETFL , NULL);
  int initial_fd_flags;

  initial_fd_flags = fd_flags;
  //  fd_flags &= ~O_ACCMODE; // Remove the access mode from the value
  fd_flags |= O_NONBLOCK;
  fcntl(s, F_SETFL , fd_flags);
# endif

  int rc = send(s, (const char *)msg, len, flags);

# ifdef WIN32
  iMode = 0; // set back to blocking (default)
  ioctlsocket(s, FIONBIO, (u_long FAR*) &iMode);
# else
  fcntl(s, F_SETFL , initial_fd_flags);
# endif

  return rc;
#endif
}

#ifdef _USE_OPENSSL

int send_nowait_tls(SSL *ssl, const void *msg, int len, int flags)
{

  int rc;
  SOCKREF fd;
  if ( (fd = SSL_get_fd(ssl)) == INVALID_SOCKET ) {
    return (-1);
  }

#ifdef WIN32
  unsigned long int iMode = 1;  // 0=blocking, 1=nonblocking
  rc = ioctlsocket(fd, FIONBIO, (u_long FAR*) &iMode);
  if (rc != 0){
    ERRORNUMBER = WSAGetLastError();
    wchar_t *error_msg = wsaerrorstr(ERRORNUMBER);
    char errorstring[1000];
    const char *errstring = wchar_to_char(error_msg,errorstring);
    WARNING("Failed to set tls socket mode:%s",errstring);
  }
#else
  int initial_fd_flags;
  // get the flags, add NONBLOCK, set flags, send, reset flags.
  int fd_flags;
  fd_flags = fcntl(fd, F_GETFL , NULL);
  initial_fd_flags = fd_flags;
  fd_flags |= O_NONBLOCK;
  fcntl(fd, F_SETFL , fd_flags);
#endif

  rc = SSL_write(ssl,msg,len);
  if ( rc <= 0 ) {
    return(rc);
  }

#ifdef WIN32
  //restore prior state (default is blocking)
  iMode=0;
  ioctlsocket(fd, FIONBIO, (u_long FAR*) &iMode);
#else
  fcntl(fd, F_SETFL , initial_fd_flags);
#endif
  return rc;
}
#endif

ssize_t socket_write_primitive(struct sipp_socket *socket, char *buffer, size_t len, struct sockaddr_storage *dest)
{
  ssize_t rc;
  DEBUG_IN("this method actually performs send_to() call");

  /* Refuse to write to invalid sockets. */
  if (socket->ss_invalid) {
    WARNING("Returning EPIPE on invalid socket: %p (%d)\n", socket, socket->ss_fd);
    ERRORNUMBER = EPIPE;
    return -1;
  }

  /* Always check congestion before sending. */
  if (socket->ss_congested) {
    DEBUG("socket->ss_congested so returning EWOULDBLOCK");
    ERRORNUMBER = EWOULDBLOCK;
    return -1;
  }

  switch(socket->ss_transport) {
  case T_TLS:
#ifdef _USE_OPENSSL
    rc = send_nowait_tls(socket->ss_ssl, buffer, len, 0);
#else
    ERRORNUMBER = EOPNOTSUPP;
    rc = -1;
#endif
    break;
  case T_TCP:
    rc = send_nowait(socket->ss_fd, buffer, len, 0);
    break;
  case T_UDP:
    if(compression) {
#ifndef WIN32
      static char comp_msg[SIPP_MAX_MSG_SIZE];
      strcpy(comp_msg, buffer);
      if(comp_compress(&socket->ss_comp_state,
                       comp_msg,
                       (unsigned int *) &len) != COMP_OK) {
        REPORT_ERROR("Compression pluggin error");
      }
      buffer = (char *)comp_msg;

      TRACE_MSG("---\nCompressed message len: %d\n", len);
#else
      REPORT_ERROR("Cannot Compress.  Compression is not enabled in this build");
#endif
    }

    DEBUG("sendto(%d, buffer, %d, 0, %s:%hu [&=%p], %d)", socket->ss_fd, len, inet_ntoa( ((struct sockaddr_in*)dest)->sin_addr ), ntohs(((struct sockaddr_in*)dest)->sin_port), dest, SOCK_ADDR_SIZE(dest));
    rc = sendto(socket->ss_fd, buffer, len, 0, (struct sockaddr *)dest, SOCK_ADDR_SIZE(dest));

    break;
  default:
    REPORT_ERROR("Internal error, unknown transport type %d\n", socket->ss_transport);
  }

  DEBUG_OUT("return %d", rc);
  return rc;
}





/* Flush any output buffers for this socket. */
int flush_socket(struct sipp_socket *socket)
{
  struct socketbuf *buf;
  ssize_t ret;

  DEBUGIN();
  while ((buf = socket->ss_out)) {
    ssize_t size = buf->len - buf->offset;
    ret = socket_write_primitive(socket, buf->buf + buf->offset, size, &buf->addr);
    TRACE_MSG("Wrote %d of %d bytes in an output buffer.", ret, size);
    if (ret == size) {
      /* Everything is great, throw away this buffer. */
      socket->ss_out = buf->next;
      free_socketbuf(buf);
    } else if (ret <= 0) {
      /* Handle connection closes and errors. */
      return write_error(socket, ret);
    } else {
      /* We have written more of the partial buffer. */
      buf->offset += ret;
      ERRORNUMBER = EWOULDBLOCK;
      enter_congestion(socket, EWOULDBLOCK);
      return -1;
    }
  }

  DEBUGOUT();
  return 0;
}


/* Write data to a socket. */
int write_socket(struct sipp_socket *socket, char *buffer, ssize_t len, int flags, struct sockaddr_storage *dest)
{
  int rc;
  DEBUGIN();
  if ( socket == NULL ) {
    //FIX coredump when trying to send data but no master yet ... ( for example after unexpected mesdsage)
    return 0;
  }

  if (socket->ss_out) {
    rc = flush_socket(socket);
    TRACE_MSG("Attempted socket flush returned %d\r\n", rc);
    if (rc < 0) {
      if ((ERRORNUMBER == EWOULDBLOCK) && (flags & WS_BUFFER)) {
        buffer_write(socket, buffer, len, dest);
        return len;
      } else {
        return rc;
      }
    }
  }

  rc = socket_write_primitive(socket, buffer, len, dest);

  if (rc == len) {
    /* Everything is great. */
    if (useMessagef == 1) {
      struct timeval currentTime;
      GET_TIME (&currentTime);
      TRACE_MSG("----------------------------------------------- %s\n"
                "%s %smessage sent (%d bytes):\n\n%.*s\n",
                CStat::formatTime(&currentTime, true),
                TRANSPORT_TO_STRING(socket->ss_transport),
                socket->ss_control ? "control " : "",
                len, len, buffer);
    }
  } else if (rc <= 0) {
    DEBUG("else if (rc <= 0) : Entered");
    if ((ERRORNUMBER == EWOULDBLOCK) && (flags & WS_BUFFER)) {
      buffer_write(socket, buffer, len, dest);
      enter_congestion(socket, ERRORNUMBER);
      return len;
    }
    if (useMessagef == 1) {
      struct timeval currentTime;
      GET_TIME (&currentTime);
      TRACE_MSG("----------------------------------------------- %s\n"
                "Error sending %s message:\n\n%.*s\n",
                CStat::formatTime(&currentTime, true),
                TRANSPORT_TO_STRING(socket->ss_transport),
                len, buffer);
    }
    return write_error(socket, ERRORNUMBER);
  } else {
    /* We have a truncated message, which must be handled internally to the write function. */
    if (useMessagef == 1) {
      struct timeval currentTime;
      GET_TIME (&currentTime);
      TRACE_MSG("----------------------------------------------- %s\n"
                "Truncation sending %s message (%d of %d sent):\n\n%.*s\n",
                CStat::formatTime(&currentTime, true),
                TRANSPORT_TO_STRING(socket->ss_transport),
                rc, len, len, buffer);
    }
    buffer_write(socket, buffer + rc, len - rc, dest);
    enter_congestion(socket, ERRORNUMBER);
  }

  DEBUG_OUT("return %d", rc);
  return rc;
}

int sipp_do_connect_socket(struct sipp_socket *socket)
{
  int ret;
  //We only connect our socket if using a connection-based protocol, or only one call is allowed.
  assert(socket->ss_transport == T_TCP || socket->ss_transport == T_TLS || no_call_id_check );

  ERRORNUMBER = 0;
  DEBUG("Calling connect()");
  ret = connect(socket->ss_fd, (struct sockaddr *)&socket->ss_dest, SOCK_ADDR_SIZE(&socket->ss_dest));
  if (ret < 0) {
    DEBUG("sockfd = %d, AF=%d dest = %s", socket->ss_fd,
      socket->ss_dest.ss_family,
      socket_to_ip_port_string((struct sockaddr_storage *)&socket->ss_dest).c_str());
#ifdef WIN32
    int error = WSAGetLastError();
    wchar_t *error_msg = wsaerrorstr(error);
    char errorstring[1000];
    const char *errstring = wchar_to_char(error_msg,errorstring);
    WARNING("connect error: %s\n", errstring);
#else
    WARNING("connect error: %s\n", strerror(errno));
#endif

    return ret;
  }

  if (socket->ss_transport == T_TLS) {
#ifdef _USE_OPENSSL
    int err;
    DEBUG("Calling SSL_connect()");
    if ((err = SSL_connect(socket->ss_ssl)) < 0) {
      REPORT_ERROR("Error in SSL connection: %s\n", sip_tls_error_string(socket->ss_ssl, err));
    }
#else
    REPORT_ERROR("You need to compile SIPp with TLS support");
#endif
  }

  return 0;
}






int sipp_connect_socket(struct sipp_socket *socket, struct sockaddr_storage *dest)
{
  memcpy(&socket->ss_dest, dest, SOCK_ADDR_SIZE(dest));
  return sipp_do_connect_socket(socket);
}




void sipp_customize_socket(struct sipp_socket *socket)
{
  unsigned int buffsize = buff_size;
  DEBUGIN();

  /* Allows fast TCP reuse of the socket */
  if (socket->ss_transport == T_TCP || socket->ss_transport == T_TLS ) {
    int sock_opt = 1;

    if (setsockopt(socket->ss_fd, SOL_SOCKET, SO_REUSEADDR, SETSOCKOPT_TYPE &sock_opt,
                   sizeof (sock_opt)) == -1) {
      REPORT_ERROR_NO("setsockopt(SO_REUSEADDR) failed");
    }


    if (setsockopt(socket->ss_fd, SOL_TCP, TCP_NODELAY, SETSOCKOPT_TYPE &sock_opt,
                   sizeof (sock_opt)) == -1) {
      {
        REPORT_ERROR_NO("setsockopt(TCP_NODELAY) failed");
      }
    }

    {
      struct linger linger;

      linger.l_onoff = 1;
      linger.l_linger = 1;
      if (setsockopt (socket->ss_fd, SOL_SOCKET, SO_LINGER,
                      SETSOCKOPT_TYPE &linger, sizeof (linger)) < 0) {
        REPORT_ERROR_NO("Unable to set SO_LINGER option");
      }
    }
  }

  /* Increase buffer sizes for this sockets */
  if(setsockopt(socket->ss_fd,
                SOL_SOCKET,
                SO_SNDBUF,
                SETSOCKOPT_TYPE &buffsize,
                sizeof(buffsize))) {
    REPORT_ERROR_NO("Unable to set socket sndbuf");
  }

  buffsize = buff_size;
  if(setsockopt(socket->ss_fd,
                SOL_SOCKET,
                SO_RCVBUF,
                SETSOCKOPT_TYPE &buffsize,
                sizeof(buffsize))) {
    REPORT_ERROR_NO("Unable to set socket rcvbuf");
  }
  DEBUGOUT();
}



int sipp_bind_socket(struct sipp_socket *socket, struct sockaddr_storage *saddr, int *port)
{
  int ret;
  int len;
  char ip_and_port[INET6_ADDRSTRLEN+10];
  DEBUGIN();

  const char* res;
  if (socket->ss_ipv6) {
    len = sizeof(struct sockaddr_in6);
    char ip[INET6_ADDRSTRLEN];
    res = inet_ntop(AF_INET6, &(((struct sockaddr_in6 *) saddr)->sin6_addr), ip, INET_ADDRSTRLEN);
    if (res!=0) sprintf(ip_and_port, "%s:%hu", ip, ntohs(((struct sockaddr_in6 *)saddr )->sin6_port));
  } else {
    len = sizeof(struct sockaddr_in);
    char ip[INET_ADDRSTRLEN];
    res = inet_ntop(AF_INET, &(((struct sockaddr_in *) saddr)->sin_addr), ip, INET_ADDRSTRLEN);
    if (res!=0) sprintf(ip_and_port, "%s:%hu", ip, ntohs(((struct sockaddr_in *) saddr)->sin_port));
  }

  if(res==0){
#ifdef WIN32
      ERRORNUMBER = WSAGetLastError();
      wchar_t *error_msg = wsaerrorstr(ERRORNUMBER);
      char errorstring[1000];
      const char *errstring = wchar_to_char(error_msg,errorstring);
      WARNING("inet_ntop failed for AF = %d, error: %s", saddr->ss_family, errorstring);
#else
    perror("inet_ntop");
#endif
  }

  if((ret = bind(socket->ss_fd, (sockaddr *)saddr, len))) {
#ifdef WIN32
    ERRORNUMBER = WSAGetLastError();
    wchar_t *error_msg = wsaerrorstr(ERRORNUMBER);
    char errorstring[1000];
    const char *errstring = wchar_to_char(error_msg,errorstring);
    DEBUG("Could not bind socket to %s (%d: %s)", ip_and_port, ERRORNUMBER, errstring);
#else
    DEBUG("Could not bind socket to %s (%d: %s)", ip_and_port, ERRORNUMBER, strerror(ERRORNUMBER));
#endif
    return ret;
  }
  DEBUG("Bound socket %d to %s", socket->ss_fd, ip_and_port);

  if (!port) {
    return 0;
  }

  if ((ret = getsockname(socket->ss_fd, (sockaddr *)saddr, (sipp_socklen_t *) &len))) {
    return ret;
  }

  if (socket->ss_ipv6) {
    *port = ntohs((short)((_RCAST(struct sockaddr_in6 *, saddr))->sin6_port));
  } else {
    *port = ntohs((short)((_RCAST(struct sockaddr_in *, saddr))->sin_port));
  }

  DEBUG_OUT("bound to %s\n", ip_and_port);
  return 0;
}

struct sipp_socket *sipp_allocate_socket(bool use_ipv6, int transport, SOCKREF fd, int accepting) {
  struct sipp_socket *ret = NULL;
  DEBUGIN();

  ret = (struct sipp_socket *)malloc(sizeof(struct sipp_socket));
  if (!ret) {
    REPORT_ERROR("Could not allocate a sipp_socket structure.");
  }
  memset(ret, 0, sizeof(struct sipp_socket));


  ret->ss_transport = transport;
  ret->ss_control = false;
  ret->ss_ipv6 = use_ipv6;
  ret->ss_fd = fd;
  ret->ss_comp_state = NULL;
  ret->ss_count = 1;
  ret->ss_changed_dest = false;

  /* Initialize all sockets with our destination address. */
  memcpy(&ret->ss_remote_sockaddr, &remote_sockaddr, sizeof(ret->ss_remote_sockaddr));

#ifdef _USE_OPENSSL
  ret->ss_ssl = NULL;

  if ( transport == T_TLS ) {
    DEBUG("Performing SSL socket initialization");
    if ((ret->ss_bio = BIO_new_socket(fd,BIO_NOCLOSE)) == NULL) {
      REPORT_ERROR("Unable to create BIO object:Problem with BIO_new_socket()\n");
    }

    if (!(ret->ss_ssl = SSL_new(accepting ? sip_trp_ssl_ctx : sip_trp_ssl_ctx_client))) {
      REPORT_ERROR("Unable to create SSL object : Problem with SSL_new() \n");
    }

    SSL_set_bio(ret->ss_ssl,ret->ss_bio,ret->ss_bio);
  }
#endif

  ret->ss_in = NULL;
  ret->ss_out = NULL;
  ret->ss_msglen = 0;
  ret->ss_congested = false;
  ret->ss_invalid = false;

  /* Store this socket in the tables. */
  ret->ss_pollidx = pollnfds++;
  sockets[ret->ss_pollidx] = ret;
  pollfiles[ret->ss_pollidx].fd      = ret->ss_fd;
#ifdef WIN32
  // WSAPoll chokes on POLLERR as input event, it will use POLLERR as a revent
  // as required though so this should make no functional difference
  pollfiles[ret->ss_pollidx].events  = POLLIN;
#else
  pollfiles[ret->ss_pollidx].events  = POLLIN | POLLERR;
#endif
  pollfiles[ret->ss_pollidx].revents = 0;


  DEBUG_OUT("return code %d", ret);
  return ret;
}

struct sipp_socket *sipp_allocate_socket(bool use_ipv6, int transport, SOCKREF fd) {
  return sipp_allocate_socket(use_ipv6, transport, fd, 0);
}


SOCKREF socket_fd(bool use_ipv6, int transport)
{
  int socket_type;
  SOCKREF fd;

  switch(transport) {
  case T_UDP:
    socket_type = SOCK_DGRAM;
    break;
  case T_TLS:
#ifndef _USE_OPENSSL
    REPORT_ERROR("You do not have TLS support enabled!\n");
#endif
  case T_TCP:
    socket_type = SOCK_STREAM;
    break;
  }

  if((fd = socket(use_ipv6 ? AF_INET6 : AF_INET, socket_type, 0))== INVALID_SOCKET) {
    REPORT_ERROR("Unable to get a %s socket (3)", TRANSPORT_TO_STRING(transport));
  }

  DEBUG_OUT("socket fd = %d", fd);
  return fd;
}



struct sipp_socket *new_sipp_socket(bool use_ipv6, int transport) {
  DEBUGIN();

  struct sipp_socket *ret;
  SOCKREF fd = socket_fd(use_ipv6, transport);

#if defined(__SUNOS)
  if (fd < 256) {
    int newfd = fcntl(fd, F_DUPFD, 256);
    if (newfd <= 0) {
      // Typically, (24)(Too many open files) is the error here
      WARNING("Unable to get a different %s socket, errno=%d(%s)",
              TRANSPORT_TO_STRING(transport), ERRORNUMBER, strerror(ERRORNUMBER));

      // Keep the original socket fd.
      newfd = fd;
    } else {
      CLOSESOCKET(fd);
    }
    fd = newfd;
  }
#endif

  ret  = sipp_allocate_socket(use_ipv6, transport, fd);
  if (!ret) {
    CLOSESOCKET(fd);
    REPORT_ERROR("Could not allocate new socket structure!");
  }
  DEBUGOUT();
  return ret;
}





int sipp_reconnect_socket(struct sipp_socket *socket)
{
  DEBUGIN();
  assert(socket->ss_fd == INVALID_SOCKET);

  socket->ss_fd = socket_fd(socket->ss_ipv6, socket->ss_transport);
  if (socket->ss_fd == INVALID_SOCKET) {
    REPORT_ERROR_NO("Could not obtain new socket: ");
  }

  if (socket->ss_invalid) {
#ifdef _USE_OPENSSL
    socket->ss_ssl = NULL;

    if ( transport == T_TLS ) {
      if ((socket->ss_bio = BIO_new_socket(socket->ss_fd,BIO_NOCLOSE)) == NULL) {
        REPORT_ERROR("Unable to create BIO object:Problem with BIO_new_socket()\n");
      }

      if (!(socket->ss_ssl = SSL_new(sip_trp_ssl_ctx_client))) {
        REPORT_ERROR("Unable to create SSL object : Problem with SSL_new() \n");
      }

      SSL_set_bio(socket->ss_ssl,socket->ss_bio,socket->ss_bio);
    }
#endif

    /* Store this socket in the tables. */
    socket->ss_pollidx = pollnfds++;

    sockets[socket->ss_pollidx] = socket;
    pollfiles[socket->ss_pollidx].fd      = socket->ss_fd;
#ifdef WIN32
  // WSAPoll chokes on POLLERR as input event, it will use POLLERR as a revent
  // as required though so this should make no functional difference
    pollfiles[socket->ss_pollidx].events  = POLLIN ;
#else
    pollfiles[socket->ss_pollidx].events  = POLLIN | POLLERR;
#endif
    pollfiles[socket->ss_pollidx].revents = 0;
    socket->ss_invalid = false;
  }

  DEBUG_OUT("about to call 'return sipp_do_connect_socket(socket)'");
  return sipp_do_connect_socket(socket);
}




struct sipp_socket *sipp_accept_socket(struct sipp_socket *accept_socket, struct sockaddr_storage *source) {
  DEBUGIN();
  struct sipp_socket *ret;
  struct sockaddr_storage remote_sockaddr;
  int fd;
  sipp_socklen_t addrlen = sizeof(remote_sockaddr);

  DEBUG("Calling accept()");
  if((fd = accept(accept_socket->ss_fd, (struct sockaddr *)&remote_sockaddr, &addrlen))== -1) {
#ifdef WIN32
    ERRORNUMBER = WSAGetLastError();
    wchar_t *error_msg = wsaerrorstr(ERRORNUMBER);
    char errorstring[1000];
    const char *errstring = wchar_to_char(error_msg,errorstring);
    REPORT_ERROR("Unable to accept on a %s socket: %s", TRANSPORT_TO_STRING(transport), errorstring);
#else
    REPORT_ERROR("Unable to accept on a %s socket: %s", TRANSPORT_TO_STRING(transport), strerror(ERRORNUMBER));
#endif
  }
  DEBUG("accept() returned fd %d from %s", fd, socket_to_ip_port_string(&remote_sockaddr).c_str());

#if defined(__SUNOS)
  if (fd < 256) {
    int newfd = fcntl(fd, F_DUPFD, 256);
    if (newfd <= 0) {
      // Typically, (24)(Too many open files) is the error here
      WARNING("Unable to get a different %s socket, errno=%d(%s)",
              TRANSPORT_TO_STRING(transport), ERRORNUMBER, strerror(ERRORNUMBER));

      // Keep the original socket fd.
      newfd = fd;
    } else {
      CLOSESOCKET(fd);
    }
    fd = newfd;
  }
#endif

  // Verify it's from the right spot if source specified
  if (source) {
    if (!is_in_addr_equal(&remote_sockaddr, source)) {
      WARNING("Closing new TCP connection from %s as it does not match specified remote host %s",
              socket_to_ip_port_string(&remote_sockaddr).c_str(), socket_to_ip_string(source).c_str());
      CLOSESOCKET(fd);
      return 0;
    }
  }

  ret  = sipp_allocate_socket(accept_socket->ss_ipv6, accept_socket->ss_transport, fd, 1);
  if (!ret) {
    CLOSESOCKET(fd);
    REPORT_ERROR_NO("Could not allocate new socket!");
  }

  memcpy(&ret->ss_remote_sockaddr, &remote_sockaddr, sizeof(ret->ss_remote_sockaddr));
  /* We should connect back to the address which connected to us if we
   * experience a TCP failure. */
  memcpy(&ret->ss_dest, &remote_sockaddr, sizeof(ret->ss_remote_sockaddr));

  if (ret->ss_transport == T_TLS) {
#ifdef _USE_OPENSSL
    int err;
    DEBUG("Calling SSL_accept()");
    if ((err = SSL_accept(ret->ss_ssl)) < 0) {
      REPORT_ERROR("Error in SSL_accept: %s\n", sip_tls_error_string(accept_socket->ss_ssl, err));
    }
#else
    REPORT_ERROR("You need to compile SIPp with TLS support");
#endif
  }

  DEBUGOUT();
  return ret;
}


void determine_remote_ip()
{
  if(!strlen(remote_host)) {
    memset(&remote_sockaddr, 0, sizeof( remote_sockaddr ));
    // remote_host option required for client, optional for server.
    if((sendMode != MODE_SERVER)) {
      REPORT_ERROR("Missing remote host parameter. This scenario requires it.  \nCommon reasons are that the first message is a sent by SIPp or that a NOP statement precedes the <recv> and you specified the -mc option");
    }
  } else {
    int temp_remote_port;
    get_host_and_port(remote_host, remote_host, &temp_remote_port);
    if (temp_remote_port != 0) {
      remote_port = temp_remote_port;
    }

    /* Resolving the remote IP */
    {
      struct addrinfo   hints;
      struct addrinfo * local_addr;

      printf ("Resolving remote host '%s'... ", remote_host);

      memset((char*)&hints, 0, sizeof(hints));
      hints.ai_flags  = AI_PASSIVE;
      hints.ai_family = PF_UNSPEC;

      /* FIXME: add DNS SRV support using liburli? */
      if (getaddrinfo(remote_host,
                      NULL,
                      &hints,
                      &local_addr) != 0) {
        REPORT_ERROR("Unknown remote host '%s'.\n"
                     "Use 'sipp -h' for details", remote_host);
      }

      memset(&remote_sockaddr, 0, sizeof( remote_sockaddr ));
      memcpy(&remote_sockaddr,
             local_addr->ai_addr,
             SOCK_ADDR_SIZE(
               _RCAST(struct sockaddr_storage *,local_addr->ai_addr)));

      freeaddrinfo(local_addr);

      strcpy(remote_ip, get_inet_address(&remote_sockaddr));
      if (remote_sockaddr.ss_family == AF_INET) {
        (_RCAST(struct sockaddr_in *, &remote_sockaddr))->sin_port =
          htons((short)remote_port);
        strcpy(remote_ip_escaped, remote_ip);
      } else {
        (_RCAST(struct sockaddr_in6 *, &remote_sockaddr))->sin6_port =
          htons((short)remote_port);
        sprintf(remote_ip_escaped, "[%s]", remote_ip);
      }
      printf("Done.\n");
    }
  }

} // determine_remote_ip

void determine_local_ip()
{
  int rc;
  if((rc=gethostname(hostname,64)) != 0) {
#ifdef WIN32
    int errornumber = WSAGetLastError();
    if (rc==SOCKET_ERROR){
      wprintf(L"%s\n",wsaerrorstr(errornumber));
    }
    REPORT_ERROR("Can't get local hostname in 'gethostname(hostname,64)': error number %d",errornumber);
#else
    REPORT_ERROR_NO("Can't get local hostname in 'gethostname(hostname,64)'");
#endif
  }

  {
    char            * local_host = NULL;
    struct addrinfo * local_addr;
    struct addrinfo   hints;

    if (!strlen(local_ip)) {
      local_host = (char *)hostname;
    } else {
      local_host = (char *)local_ip;
    }

    memset((char*)&hints, 0, sizeof(hints));
    hints.ai_flags  = AI_PASSIVE;
    hints.ai_family = PF_UNSPEC;

    /* Resolving local IP */
    int rv;
    if ((rv=getaddrinfo(local_host, NULL, &hints, &local_addr)) != 0) {
#ifdef WIN32
      int error = WSAGetLastError();
      wchar_t *error_msg = wsaerrorstr(error);
      char errorstring[1000];
      const char *errstring = wchar_to_char(error_msg,errorstring);
      REPORT_ERROR("Can't get local IP address in getaddrinfo, local_host='%s', local_ip='%s', error: %s",
                   local_host,
                   local_ip,
                   errstring);
#else
      REPORT_ERROR("Can't get local IP address in getaddrinfo, local_host='%s', local_ip='%s', error: %s",
                   local_host,
                   local_ip,
                   gai_strerror(rv));
#endif
    }
    // store local addr info for rsa option
    getaddrinfo(local_host, NULL, &hints, &local_addr_storage);

    memset(&local_sockaddr,0,sizeof(struct sockaddr_storage));
    local_sockaddr.ss_family = local_addr->ai_addr->sa_family;

    if (!strlen(local_ip)) {
      strcpy(local_ip,
             get_inet_address(
               _RCAST(struct sockaddr_storage *, local_addr->ai_addr)));
    } else {
      memcpy(&local_sockaddr,
             local_addr->ai_addr,
             SOCK_ADDR_SIZE(
               _RCAST(struct sockaddr_storage *,local_addr->ai_addr)));
    }
    freeaddrinfo(local_addr);
    if (local_sockaddr.ss_family == AF_INET6) {
      local_ip_is_ipv6 = true;
      sprintf(local_ip_escaped, "[%s]", local_ip);
    } else {
      local_ip_is_ipv6 = false;
      strcpy(local_ip_escaped, local_ip);
    }
  }

  //i2=[local_ip2] is to be used for media streaming as an alternate 'from address'
  //  we will not be using as sip traffic port. Initialize here along
  //  with local_ip2_escapted and local_ip2_is_ipv6
  struct addrinfo   hints;
  struct addrinfo * local_addr;
  if (strlen(local_ip2)!=0){
    memset((char*)&hints, 0, sizeof(hints));
    hints.ai_flags  = AI_PASSIVE;
    hints.ai_family = PF_UNSPEC;
     /* Resolving local IP2 */
    if (getaddrinfo(local_ip2, NULL, &hints, &local_addr) != 0) {
      REPORT_ERROR("Can't get local_ip2 address in getaddrinfo, local_ip='%s'",
                   local_ip2);
    }
    if (local_addr->ai_addr->sa_family==AF_INET6) {
      local_ip2_is_ipv6 = true;
      sprintf(local_ip2_escaped, "[%s]", local_ip2);
    }else{
      local_ip2_is_ipv6 = false;
      strcpy(local_ip2_escaped, local_ip2);
    }
    freeaddrinfo(local_addr);
  }

} // determine_local_ip

void determine_remote_and_local_ip()
{
  determine_remote_ip();
  determine_local_ip();
} // determine_remote_and_local_ip



void connect_to_peer(char *peer_host, int peer_port, struct sockaddr_storage *peer_sockaddr, char *peer_ip, struct sipp_socket **peer_socket)
{

  /* Resolving the  peer IP */
  printf("Resolving peer address : %s...\n",peer_host);
  struct addrinfo   hints;
  struct addrinfo * local_addr;
  memset((char*)&hints, 0, sizeof(hints));
  hints.ai_flags  = AI_PASSIVE;
  hints.ai_family = PF_UNSPEC;
  is_ipv6 = false;
  /* Resolving twin IP */
  if (getaddrinfo(peer_host,
                  NULL,
                  &hints,
                  &local_addr) != 0) {

    REPORT_ERROR("Unknown peer host '%s'.\n"
                 "Use 'sipp -h' for details", peer_host);
  }

  memcpy(peer_sockaddr,
         local_addr->ai_addr,
         SOCK_ADDR_SIZE(
           _RCAST(struct sockaddr_storage *,local_addr->ai_addr)));

  freeaddrinfo(local_addr);

  if (peer_sockaddr->ss_family == AF_INET) {
    (_RCAST(struct sockaddr_in *,peer_sockaddr))->sin_port =
      htons((short)peer_port);
  } else {
    (_RCAST(struct sockaddr_in6 *,peer_sockaddr))->sin6_port =
      htons((short)peer_port);
    is_ipv6 = true;
  }
  strcpy(peer_ip, get_inet_address(peer_sockaddr));
  // by happy coincidence T_TCP=1=SOCK_STREAM
  if((*peer_socket = new_sipp_socket(is_ipv6, T_TCP)) == NULL) {
    REPORT_ERROR_NO("Unable to get a twin sipp TCP socket");
  }

  /* Mark this as a control socket. */
  (*peer_socket)->ss_control = 1;

  if(sipp_connect_socket(*peer_socket, peer_sockaddr)) {
    if(ERRORNUMBER == EINVAL) {
      /* This occurs sometime on HPUX but is not a true INVAL */
      REPORT_ERROR_NO("Unable to connect a twin sipp TCP socket\n "
                      ", remote peer error.\n"
                      "Use 'sipp -h' for details");
    } else {
      REPORT_ERROR_NO("Unable to connect a twin sipp socket "
                      "\n"
                      "Use 'sipp -h' for details");
    }
  }

  sipp_customize_socket(*peer_socket);
}





void connect_to_all_peers()
{
  peer_map::iterator peer_it;
  T_peer_infos infos;
  for (peer_it = peers.begin(); peer_it != peers.end(); peer_it++) {
    infos = peer_it->second;
    get_host_and_port(infos.peer_host, infos.peer_host, &infos.peer_port);
    connect_to_peer(infos.peer_host, infos.peer_port,&(infos.peer_sockaddr), infos.peer_ip, &(infos.peer_socket));
    peer_sockets[infos.peer_socket] = peer_it->first;
    peers[std::string(peer_it->first)] = infos;
  }
  peers_connected = 1;
}


void connect_local_twin_socket(char * twinSippHost)
{
  /* Resolving the listener IP */
  printf("Resolving listener address : %s...\n", twinSippHost);
  struct addrinfo   hints;
  struct addrinfo * local_addr;
  memset((char*)&hints, 0, sizeof(hints));
  hints.ai_flags  = AI_PASSIVE;
  hints.ai_family = PF_UNSPEC;
  is_ipv6 = false;

  /* Resolving twin IP */
  if (getaddrinfo(twinSippHost,
                  NULL,
                  &hints,
                  &local_addr) != 0) {
    REPORT_ERROR("Unknown twin host '%s'.\n"
                 "Use 'sipp -h' for details", twinSippHost);
  }
  memcpy(&twinSipp_sockaddr,
         local_addr->ai_addr,
         SOCK_ADDR_SIZE(
           _RCAST(struct sockaddr_storage *,local_addr->ai_addr)));

  if (twinSipp_sockaddr.ss_family == AF_INET) {
    (_RCAST(struct sockaddr_in *,&twinSipp_sockaddr))->sin_port =
      htons((short)twinSippPort);
  } else {
    (_RCAST(struct sockaddr_in6 *,&twinSipp_sockaddr))->sin6_port =
      htons((short)twinSippPort);
    is_ipv6 = true;
  }
  strcpy(twinSippIp, get_inet_address(&twinSipp_sockaddr));

  if((localTwinSippSocket = new_sipp_socket(is_ipv6, T_TCP)) == NULL) {
    REPORT_ERROR_NO("Unable to get a listener TCP socket ");
  }

  memset(&localTwin_sockaddr, 0, sizeof(struct sockaddr_storage));
  if (!is_ipv6) {
    localTwin_sockaddr.ss_family = AF_INET;
    (_RCAST(struct sockaddr_in *,&localTwin_sockaddr))->sin_port =
      htons((short)twinSippPort);
  } else {
    localTwin_sockaddr.ss_family = AF_INET6;
    (_RCAST(struct sockaddr_in6 *,&localTwin_sockaddr))->sin6_port =
      htons((short)twinSippPort);
  }

  // add socket option to allow the use of it without the TCP timeout
  // This allows to re-start the controller B (or slave) without timeout after its exit
  int reuse = 1;
  setsockopt(localTwinSippSocket->ss_fd,SOL_SOCKET,SO_REUSEADDR,SETSOCKOPT_TYPE &reuse,sizeof(reuse));
  sipp_customize_socket(localTwinSippSocket);

  if(sipp_bind_socket(localTwinSippSocket, &localTwin_sockaddr, 0)) {
    REPORT_ERROR_NO("Unable to bind twin sipp socket ");
  }

  if(listen(localTwinSippSocket->ss_fd, 100)) {
    REPORT_ERROR_NO("Unable to listen twin sipp socket in ");
  }
}





int open_connections()
{
  DEBUGIN();
  int status=0;
  local_port = 0;


  /* Creating and binding the local socket */
  if ((main_socket = new_sipp_socket(local_ip_is_ipv6, transport)) == NULL) {
    REPORT_ERROR_NO("Unable to get the local socket");
  }

  sipp_customize_socket(main_socket);

  /* Trying to bind local port */
  char peripaddr[256];
  if(!user_port) {
    unsigned short l_port;
    for(l_port = DEFAULT_PORT;
        l_port < (DEFAULT_PORT + 60);
        l_port++) {

      // Bind socket to local_ip
      if (bind_local || peripsocket) {
        struct addrinfo * local_addr;
        struct addrinfo   hints;
        memset((char*)&hints, 0, sizeof(hints));
        hints.ai_flags  = AI_PASSIVE;
        hints.ai_family = PF_UNSPEC;

        if (peripsocket) {
          // On some machines it fails to bind to the self computed local
          // IP address.
          // For the socket per IP mode, bind the main socket to the
          // first IP address specified in the inject file.
          inFiles[ip_file]->getField(0, peripfield, peripaddr, sizeof(peripaddr));
          if (getaddrinfo(peripaddr,
                          NULL,
                          &hints,
                          &local_addr) != 0) {
            REPORT_ERROR("Unknown host '%s'.\n"
                         "Use 'sipp -h' for details", peripaddr);
          }
        } else {
          if (getaddrinfo(local_ip,
                          NULL,
                          &hints,
                          &local_addr) != 0) {
            REPORT_ERROR("Unknown host '%s'.\n"
                         "Use 'sipp -h' for details", peripaddr);
          }
        }
        memcpy(&local_sockaddr,
               local_addr->ai_addr,
               SOCK_ADDR_SIZE(
                 _RCAST(struct sockaddr_storage *, local_addr->ai_addr)));
        freeaddrinfo(local_addr);
      }
      if (local_ip_is_ipv6) {
        (_RCAST(struct sockaddr_in6 *, &local_sockaddr))->sin6_port
          = htons((short)l_port);
      } else {
        (_RCAST(struct sockaddr_in *, &local_sockaddr))->sin_port
          = htons((short)l_port);
      }
      if(sipp_bind_socket(main_socket, &local_sockaddr, &local_port) == 0) {
        break;
      }
      DEBUG("main socket %d, bound to AF = %d, %s", main_socket->ss_fd, 
        local_sockaddr.ss_family,
        socket_to_ip_port_string((sockaddr_storage*)(&local_sockaddr)).c_str());
    }
  }

  if(!local_port) {
    /* Not already binded, use user_port of 0 to let
     * the system choose a port. */

    if (bind_local || peripsocket) {
      struct addrinfo * local_addr;
      struct addrinfo   hints;
      memset((char*)&hints, 0, sizeof(hints));
      hints.ai_flags  = AI_PASSIVE;
      hints.ai_family = PF_UNSPEC;

      if (peripsocket) {
        // On some machines it fails to bind to the self computed local
        // IP address.
        // For the socket per IP mode, bind the main socket to the
        // first IP address specified in the inject file.
        inFiles[ip_file]->getField(0, peripfield, peripaddr, sizeof(peripaddr));
        if (getaddrinfo(peripaddr,
                        NULL,
                        &hints,
                        &local_addr) != 0) {
          REPORT_ERROR("Unknown host '%s'.\n"
                       "Use 'sipp -h' for details", peripaddr);
        }
      } else {
        if (getaddrinfo(local_ip,
                        NULL,
                        &hints,
                        &local_addr) != 0) {
          REPORT_ERROR("Unknown host '%s'.\n"
                       "Use 'sipp -h' for details", peripaddr);
        }
      }
      memcpy(&local_sockaddr,
             local_addr->ai_addr,
             SOCK_ADDR_SIZE(
               _RCAST(struct sockaddr_storage *, local_addr->ai_addr)));
      freeaddrinfo(local_addr);
    }

    if (local_ip_is_ipv6) {
      (_RCAST(struct sockaddr_in6 *, &local_sockaddr))->sin6_port
        = htons((short)user_port);
    } else {
      (_RCAST(struct sockaddr_in *, &local_sockaddr))->sin_port
        = htons((short)user_port);
    }
    if(sipp_bind_socket(main_socket, &local_sockaddr, &local_port)) {
      if (local_ip_is_ipv6) {
        REPORT_ERROR_NO("Unable to bind main socket to IPv6 address, port %d. This may be caused by an incorrectly specified local IP ", user_port);
      } else {
        REPORT_ERROR_NO("Unable to bind main socket to %s:%d. Make sure that the local IP (as specified with -i) is actually an IP address on your computer", inet_ntoa(((struct sockaddr_in *) &local_sockaddr)->sin_addr), user_port);
      }
    }
    DEBUG("main socket %d, bound to AF = %d, %s", main_socket->ss_fd, 
        local_sockaddr.ss_family,
        socket_to_ip_port_string((sockaddr_storage*)(&local_sockaddr)).c_str());
  }

  if (peripsocket) {
    // Add the main socket to the socket per subscriber map
    map_perip_fd[peripaddr] = main_socket;
  }

  // Create additional server sockets when running in socket per
  // IP address mode.
  if (peripsocket && sendMode == MODE_SERVER) {
    struct sockaddr_storage server_sockaddr;
    struct addrinfo * local_addr;
    struct addrinfo   hints;
    memset((char*)&hints, 0, sizeof(hints));
    hints.ai_flags  = AI_PASSIVE;
    hints.ai_family = PF_UNSPEC;

    char peripaddr[256];
    struct sipp_socket *sock;
    unsigned int lines = inFiles[ip_file]->numLines();
    for (unsigned int i = 0; i < lines; i++) {
      inFiles[ip_file]->getField(i, peripfield, peripaddr, sizeof(peripaddr));
      map<string, struct sipp_socket *>::iterator j;
      j = map_perip_fd.find(peripaddr);

      if (j == map_perip_fd.end()) {
        if((sock = new_sipp_socket(is_ipv6, transport)) == NULL) {
          REPORT_ERROR_NO("Unable to get server socket");
        }

        if (getaddrinfo(peripaddr,
                        NULL,
                        &hints,
                        &local_addr) != 0) {
          REPORT_ERROR("Unknown remote host '%s'.\n"
                       "Use 'sipp -h' for details", peripaddr);
        }

        memcpy(&server_sockaddr,
               local_addr->ai_addr,
               SOCK_ADDR_SIZE(
                 _RCAST(struct sockaddr_storage *, local_addr->ai_addr)));
        freeaddrinfo(local_addr);

        if (is_ipv6) {
          (_RCAST(struct sockaddr_in6 *, &server_sockaddr))->sin6_port
            = htons((short)local_port);
        } else {
          (_RCAST(struct sockaddr_in *, &server_sockaddr))->sin_port
            = htons((short)local_port);
        }

        sipp_customize_socket(sock);
        if(sipp_bind_socket(sock, &server_sockaddr, NULL)) {
          REPORT_ERROR_NO("Unable to bind server socket");
        }
        DEBUG("perip socket %d, bound to AF = %d, %s", sock->ss_fd, 
          server_sockaddr.ss_family,
          socket_to_ip_port_string((sockaddr_storage*)(&server_sockaddr)).c_str());
        map_perip_fd[peripaddr] = sock;
      }
    }
  }

  if((!multisocket) && (transport == T_TCP || transport == T_TLS) &&
      (sendMode != MODE_SERVER)) {
    DEBUG("Single-socket mode, TCP or TLS, and sendMode == MODE_CLIENT: creating tcp_multiplex socket");
    if((tcp_multiplex = new_sipp_socket(local_ip_is_ipv6, transport)) == NULL) {
      REPORT_ERROR_NO("Unable to get a TCP socket");
    }
    // on a host with multiple ip addresses, we want to originate from ip address 
    // specified by -i in command line (local_ip) so bind to the local address/port before 
    // connecting to remote host 
    sockaddr_storage tcp_ss;
    memcpy(&tcp_ss,&local_sockaddr, sizeof(sockaddr_storage));
    // set the port to zero - allow system to choose any random port
    int tcp_port = 0;
    if (tcp_ss.ss_family==AF_INET6){
      ((sockaddr_in6*)(&tcp_ss))->sin6_port=htons(tcp_port);
    }else{
      ((sockaddr_in*)(&tcp_ss))->sin_port=htons(tcp_port);
    }
    DEBUG("binding tcp client to %s", socket_to_ip_port_string(&tcp_ss).c_str());
    if(sipp_bind_socket(tcp_multiplex, &tcp_ss,(int*) &tcp_port) ) {
       REPORT_ERROR_NO("Unable to BIND a TCP socket, %s",
         socket_to_ip_port_string(&tcp_ss).c_str());
    }


    /* OJA FIXME: is it correct? */
    if (use_remote_sending_addr) {
      remote_sockaddr = remote_sending_sockaddr ;
    }

    if(sipp_connect_socket(tcp_multiplex, &remote_sockaddr)) {
      if(reset_number >0) {
        WARNING("Failed to reconnect\n");
        sipp_close_socket(main_socket);
        reset_number--;
        return 1;
      } else {
        if(ERRORNUMBER == EINVAL) {
          /* This occurs sometime on HPUX but is not a true INVAL */
          REPORT_ERROR_NO("Unable to connect a TCP socket, remote peer error.\n"
                          "Use 'sipp -h' for details");
        } else {
          REPORT_ERROR_NO("Unable to connect a TCP socket.\n"
                          "Use 'sipp -h' for details");
        }
      }
    }

    sipp_customize_socket(tcp_multiplex);
  }


  if(transport == T_TCP || transport == T_TLS) {
    DEBUG("Listening on main_socket (fd = %d)", main_socket->ss_fd);
    if(listen(main_socket->ss_fd, 100)) {
      REPORT_ERROR_NO("Unable to listen main socket");
    }
  }

  /* Trying to connect to Twin Sipp in 3PCC mode */
  if(twinSippMode) {
    if(thirdPartyMode == MODE_3PCC_CONTROLLER_A || thirdPartyMode == MODE_3PCC_A_PASSIVE) {
      connect_to_peer(twinSippHost, twinSippPort, &twinSipp_sockaddr, twinSippIp, &twinSippSocket);
    } else if(thirdPartyMode == MODE_3PCC_CONTROLLER_B) {
      connect_local_twin_socket(twinSippHost);
    } else {
      REPORT_ERROR("TwinSipp Mode enabled but thirdPartyMode is different "
                   "from 3PCC_CONTROLLER_B and 3PCC_CONTROLLER_A\n");
    }
  } else if (extendedTwinSippMode) {
    if (thirdPartyMode == MODE_MASTER || thirdPartyMode == MODE_MASTER_PASSIVE) {
      char *temp_peer_addr = get_peer_addr(master_name);
      if (temp_peer_addr == NULL) {
        REPORT_ERROR("get_peer_addr: Peer %s not found\n", master_name);
      }
      strcpy(twinSippHost, temp_peer_addr);
      get_host_and_port(twinSippHost, twinSippHost, &twinSippPort);
      connect_local_twin_socket(twinSippHost);
      connect_to_all_peers();
    } else if(thirdPartyMode == MODE_SLAVE) {
      char *temp_peer_addr = get_peer_addr(slave_number);
      if (temp_peer_addr == NULL) {
        REPORT_ERROR("get_peer_addr: Peer %s not found\n", slave_number);
      }
      strcpy(twinSippHost, temp_peer_addr);
      get_host_and_port(twinSippHost, twinSippHost, &twinSippPort);
      connect_local_twin_socket(twinSippHost);
    } else {
      REPORT_ERROR("extendedTwinSipp Mode enabled but thirdPartyMode is different "
                   "from MASTER and SLAVE\n");
    }
  }

  //casting remote_sockaddr as int* and derefrencing to get first byte. If it is null, no IP has been specified.
  if(*(int*)&remote_sockaddr && no_call_id_check && main_socket->ss_transport == T_UDP) {
    DEBUG("Connecting (limiting) UDP main_socket (fd = %d) to remote address");
    if(sipp_connect_socket(main_socket, &remote_sockaddr)) REPORT_ERROR("Could not connect socket to remote address. Check to make sure the remote IP is valid.");
  }

  DEBUGOUT();
  return status;
} // open_connections

void buffer_read(struct sipp_socket *socket, struct socketbuf *newbuf)
{
  struct socketbuf *buf = socket->ss_in;
  struct socketbuf *prev = buf;

  if (!buf) {
    socket->ss_in = newbuf;
    return;
  }

  while(buf->next) {
    prev = buf;
    buf = buf->next;
  }

  prev->next = newbuf;
}


int empty_socket(struct sipp_socket *socket)
{
  int readsize = socket->ss_transport == T_UDP ? SIPP_MAX_MSG_SIZE : tcp_readsize;
  struct socketbuf *socketbuf;
  char *buffer;
  int ret;
  DEBUGIN();
  /* Where should we start sending packets to, ideally we should begin to parse
   * the Via, Contact, and Route headers.  But for now SIPp always sends to the
   * host specified on the command line; or for UAS mode to the address that
   * sent the last message. */
  sipp_socklen_t addrlen = sizeof(struct sockaddr_storage);

  buffer = (char *)malloc(readsize);
  if (!buffer) {
    REPORT_ERROR("Could not allocate memory for read!");
  }
  socketbuf = alloc_socketbuf(buffer, readsize, NO_COPY, NULL);

  switch(socket->ss_transport) {
  case T_TCP:
  case T_UDP:
    ret = recvfrom(socket->ss_fd, buffer, readsize, 0, (struct sockaddr *)&socketbuf->addr,  &addrlen);
    break;
  case T_TLS:
#ifdef _USE_OPENSSL
    ret = SSL_read(socket->ss_ssl, buffer, readsize);
    /* XXX: Check for clean shutdown. */
#else
    REPORT_ERROR("TLS support is not enabled!");
#endif
    break;
  }
  if (ret <= 0) {
    free_socketbuf(socketbuf);
    return ret;
  }

  socketbuf->len = ret;

  buffer_read(socket, socketbuf);

  /* Do we have a complete SIP message? */
  if (!socket->ss_msglen) {
    if (int msg_len = check_for_message(socket)) {
      socket->ss_msglen = msg_len;
      pending_messages++;
    }
  }

  DEBUG_OUT("return %d", ret);
  return ret;
}


void merge_socketbufs(struct socketbuf *socketbuf)
{
  struct socketbuf *next = socketbuf->next;
  int newsize;
  char *newbuf;

  if (!next) {
    return;
  }

  if (next->offset) {
    REPORT_ERROR("Internal error: can not merge a socketbuf with a non-zero offset.");
  }

  if (socketbuf->offset) {
    memmove(socketbuf->buf, socketbuf->buf + socketbuf->offset, socketbuf->len - socketbuf->offset);
    socketbuf->len -= socketbuf->offset;
    socketbuf->offset = 0;
  }

  newsize = socketbuf->len + next->len;

  newbuf = (char *)realloc(socketbuf->buf, newsize);
  if (!newbuf) {
    REPORT_ERROR("Could not allocate memory to merge socket buffers!");
  }
  memcpy(newbuf + socketbuf->len, next->buf, next->len);
  socketbuf->buf = newbuf;
  socketbuf->len = newsize;
  socketbuf->next = next->next;
  free_socketbuf(next);
}



int check_for_message(struct sipp_socket *socket)
{
  struct socketbuf *socketbuf = socket->ss_in;
  int state = socket->ss_control ? CFM_CONTROL : CFM_NORMAL;
  const char *l;

  if (!socketbuf)
    return 0;

  if (socket->ss_transport == T_UDP) {
    return socketbuf->len;
  }

  int len = 0;

  while (socketbuf->offset + len < socketbuf->len) {
    char c = socketbuf->buf[socketbuf->offset + len];

    switch(state) {
    case CFM_CONTROL:
      /* For CMD Message the escape char is the end of message */
      if (c == 27) {
        return len + 1; /* The plus one includes the control character. */
      }
      break;
    case CFM_NORMAL:
      if (c == '\r') {
        state = CFM_CR;
      }
      break;
    case CFM_CR:
      if (c == '\n') {
        state = CFM_CRLF;
      } else {
        state = CFM_NORMAL;
      }
      break;
    case CFM_CRLF:
      if (c == '\r') {
        state = CFM_CRLFCR;
      } else {
        state = CFM_NORMAL;
      }
      break;
    case CFM_CRLFCR:
      if (c == '\n') {
        state = CFM_CRLFCRLF;
      } else {
        state = CFM_NORMAL;
      }
      break;
    }

    /* Head off failing because the buffer does not contain the whole header. */
    if (socketbuf->offset + len == socketbuf->len - 1) {
      merge_socketbufs(socketbuf);
    }

    if (state == CFM_CRLFCRLF) {
      break;
    }

    len++;
  }

  /* We did not find the end-of-header marker. */
  if (state != CFM_CRLFCRLF) {
    return 0;
  }

  /* Find the content-length header. */
  const char *content_length_const = "\r\nContent-Length:";
  const char *content_length_short_const = "\r\nl:";
  if ((l = strncasestr(socketbuf->buf + socketbuf->offset, content_length_const, len))) {
    l += strlen(content_length_const);
  } else if ((l = strncasestr(socketbuf->buf + socketbuf->offset, content_length_short_const, len))) {
    l += strlen(content_length_short_const);
  } else {
    /* There is no header, so the content-length is zero. */
    return len + 1;
  }

  /* Skip spaces. */
  while(isspace(*l)) {
    if (*l == '\r' || *l == '\n') {
      /* We ran into an end-of-line, so there is no content-length. */
      return len + 1;
    }
    l++;
  }

  /* Do the integer conversion, we only allow '\r' or spaces after the integer. */
  char *endptr;
  int content_length = strtol(l, &endptr, 10);
  if (*endptr != '\r' && !isspace(*endptr)) {
    content_length = 0;
  }

  /* Now that we know how large this message is, we make sure we have the whole thing. */
  do {
    /* It is in this buffer. */
    if (socketbuf->offset + len + content_length < socketbuf->len) {
      return len + content_length + 1;
    }
    if (socketbuf->next == NULL) {
      /* There is no buffer to merge, so we fail. */
      return 0;
    }
    /* We merge ourself with the next buffer. */
    merge_socketbufs(socketbuf);
  } while (1);
}


/*********** for error reporting ***************/
#ifdef WIN32
wchar_t* wsaerrorstr(int errnumber){
  DWORD dwFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
  LPCVOID lpSource = NULL;
  DWORD dwMessageId = errnumber;  // WSAGetLastError();
  DWORD dwLanguageId = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
  LPWSTR lpBuffer=NULL;
  DWORD nSize = 0;
  va_list *Args = NULL;

  DWORD buffsize  = FormatMessage( dwFlags, lpSource, dwMessageId, dwLanguageId,
          (LPWSTR)&lpBuffer, nSize, Args);

  if (buffsize == 0){
    wchar_t* tmp = (wchar_t*)malloc(256);
    wcsncpy (tmp,L"error code: ",sizeof(tmp));
    wchar_t* num = (wchar_t*) malloc(256);
    int size = _snwprintf(num,sizeof (num),L"%d",errnumber);
    wcsncat(tmp,num,sizeof(tmp));
    return tmp;
  }else{
    return  lpBuffer;
  }
}

char*  wchar_to_char(wchar_t* orig,char* nstring){
  size_t origsize = wcslen(orig)+1;
  //const size_t newsize = 1000;
  size_t convertedChars = 0;
  //char nstring[newsize];
  wcstombs_s(&convertedChars, nstring, origsize, orig, _TRUNCATE);
  //strcat_s(nstring, " (char *)");
  return nstring;
}

#endif


void print_if_error(int rc){
  if (rc == -1){
#ifdef WIN32
    int errorno = WSAGetLastError();
    wprintf(L"%s\n",wsaerrorstr(errorno));
#else
    perror("Error:");
#endif
  }
}


/*************************************************/





/*
  moving pollset_process from sipp.cpp to here will pull in the following

    sipMsgCheck       
    handle_ctrl_socket
    process_message   
      listner
      scenario
      CStat

    which in turn starts pulling in many other non socket routines.

*/







