
#ifndef __WIN32_COMPATIBILITY__
#define __WIN32_COMPATIBILITY__

#ifdef WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #include <process.h>

  // Undo POSIX deprecation of needed routines.
  #define getpid _getpid
  #define strdup _strdup

  #define snprintf _snprintf
  #define ssize_t SSIZE_T


  int usleep(unsigned useconds);

  // Definition of a gettimeofday function 
  // Adopted from http://suacommunity.com/dictionary/gettimeofday-entry.php

  struct timezone
  {
    int  tz_minuteswest; /* minutes W of Greenwich */
    int  tz_dsttime;     /* type of dst correction */
  };
   
  int gettimeofday(struct timeval *tv, struct timezone *tz);

  int strcasecmp(const char *str1, const char *str2);
  int strncasecmp(const char *str1, const char *str2, int n);
  const char *strcasestr(const char *s, const char *pattern);
  char *strcasestr(char *s, const char *pattern);

  char *strndup (char const *s, size_t n);


  // Networking-related

  const char *inet_ntop(int af, const void *src, char *dst, socklen_t cnt);
  int inet_pton(int af, const char *src, void *dst);

  #define uint32_t UINT32
  #define uint16_t SHORT
  #define uint8_t  BYTE

  struct iphdr {
      uint32_t    ihl:4;
      uint32_t    version:4;
      uint8_t     tos;
      uint16_t    tot_len;
      uint16_t    id;
      uint16_t    frag_off;
      uint8_t     ttl;
      uint8_t     protocol;
      uint16_t    check;
      uint32_t    saddr;
      uint32_t    daddr;
  };

  struct udphdr {
      uint16_t    source;
      uint16_t    dest;
      uint16_t    len;
      uint16_t    check;
  };

  #define SETSOCKOPT_TYPE (char *)

  #define SocketError() WSAGetLastError()

  #define SHUT_RDWR SD_BOTH

  // error code mapping for windows
  // copied from http://www.aoc.nrao.edu/php/tjuerges/ALMA/ACE-5.5.2/html/ace/os__errno_8h-source.html
  #define EWOULDBLOCK             WSAEWOULDBLOCK
  #define EINPROGRESS             WSAEINPROGRESS
  #define EALREADY                WSAEALREADY
  #define ENOTSOCK                WSAENOTSOCK
  #define EDESTADDRREQ            WSAEDESTADDRREQ
  #define EMSGSIZE                WSAEMSGSIZE
  #define EPROTOTYPE              WSAEPROTOTYPE
  #define ENOPROTOOPT             WSAENOPROTOOPT
  #define EPROTONOSUPPORT         WSAEPROTONOSUPPORT
  #define ESOCKTNOSUPPORT         WSAESOCKTNOSUPPORT
  #define EOPNOTSUPP              WSAEOPNOTSUPP
  #define EPFNOSUPPORT            WSAEPFNOSUPPORT
  #define EAFNOSUPPORT            WSAEAFNOSUPPORT
  #define EADDRINUSE              WSAEADDRINUSE
  #define EADDRNOTAVAIL           WSAEADDRNOTAVAIL
  #define ENETDOWN                WSAENETDOWN
  #define ENETUNREACH             WSAENETUNREACH
  #define ENETRESET               WSAENETRESET
  #define ECONNABORTED            WSAECONNABORTED
  #define ECONNRESET              WSAECONNRESET
  #define ENOBUFS                 WSAENOBUFS
  #define EISCONN                 WSAEISCONN
  #define ENOTCONN                WSAENOTCONN
  #define ESHUTDOWN               WSAESHUTDOWN
  #define ETOOMANYREFS            WSAETOOMANYREFS
  //#define ETIMEDOUT               WSAETIMEDOUT
  #define ECONNREFUSED            WSAECONNREFUSED
  #define ELOOP                   WSAELOOP
  #define EHOSTDOWN               WSAEHOSTDOWN
  #define EHOSTUNREACH            WSAEHOSTUNREACH
  #define EPROCLIM                WSAEPROCLIM
  #define EUSERS                  WSAEUSERS
  #define EDQUOT                  WSAEDQUOT
  #define ESTALE                  WSAESTALE
  #define EREMOTE                 WSAEREMOTE

#else // Not WIN32
  #define SETSOCKOPT_TYPE (void *)
#endif

#endif // __WIN32_COMPATIBILITY__