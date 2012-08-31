
#ifndef __WIN32_COMPATIBILITY__
#define __WIN32_COMPATIBILITY__

#ifdef WIN32
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <process.h>
#include <io.h>

// Undo POSIX deprecation of needed routines.
#define getpid _getpid
#define strdup _strdup
#define unlink _unlink

#define snprintf _snprintf
#define ssize_t SSIZE_T

int usleep(unsigned useconds);

// Definition of a gettimeofday function
// Adopted from http://suacommunity.com/dictionary/gettimeofday-entry.php

struct timezone {
  int  tz_minuteswest; /* minutes W of Greenwich */
  int  tz_dsttime;     /* type of dst correction */
};

int gettimeofday(struct timeval *tv, struct timezone *tz);

int strcasecmp(const char *str1, const char *str2);
int strncasecmp(const char *str1, const char *str2, size_t n);
const char *strcasestr(const char *s, const char *pattern);
char *strcasestr(char *s, const char *pattern);

char *strndup (char const *s, size_t n);


// Networking-related

#define uint32_t UINT32
#define uint16_t SHORT
#define uint8_t  BYTE

struct iphdr {
  uint32_t    ihl:4;
  uint32_t    version:4;
  uint32_t     tos:8;
  uint32_t    tot_len:16;
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
#define SHUT_RDWR SD_BOTH

void ClearScreen();

#else // Not WIN32
//win32 compat above provides equiv methods to posix version below
#include <sys/wait.h>
#include <netdb.h>      //addrinfo
#include <arpa/inet.h>  //inet_pton
#include <sys/time.h>   //gettimeofday
#include <strings.h>    //strcasecmp strncasecmp
#include <string.h>     //strndup,strcasestr
#include <limits.h>     //PATH_MAX
#include <unistd.h>     // usleep

#define SETSOCKOPT_TYPE (void *)
#define MAX_PATH              PATH_MAX

#endif

#endif // __WIN32_COMPATIBILITY__
