#include <winsock2.h>
#include <ws2tcpip.h>

#include <time.h>
#include <windows.h>
#include <iostream>
#include <ctype.h>

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "win32_compatibility.h"

int usleep(unsigned useconds)
{
  Sleep(useconds / 1000);
  return 0;
}


// Definition of a gettimeofday function
// Adopted from http://suacommunity.com/dictionary/gettimeofday-entry.php

using namespace std;
 
#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif
 
 
int gettimeofday(struct timeval *tv, struct timezone *tz)
{
// Define a structure to receive the current Windows filetime
  FILETIME ft;
 
// Initialize the present time to 0 and the timezone to UTC
  unsigned __int64 tmpres = 0;
  static int tzflag = 0;
 
  if (NULL != tv)
  {
    GetSystemTimeAsFileTime(&ft);
 
// The GetSystemTimeAsFileTime returns the number of 100 nanosecond 
// intervals since Jan 1, 1601 in a structure. Copy the high bits to 
// the 64 bit tmpres, shift it left by 32 then or in the low 32 bits.
    tmpres |= ft.dwHighDateTime;
    tmpres <<= 32;
    tmpres |= ft.dwLowDateTime;
 
// Convert to microseconds by dividing by 10
    tmpres /= 10;
 
// The Unix epoch starts on Jan 1 1970.  Need to subtract the difference 
// in seconds from Jan 1 1601.
    tmpres -= DELTA_EPOCH_IN_MICROSECS;
 
// Finally change microseconds to seconds and place in the seconds value. 
// The modulus picks up the microseconds.
    tv->tv_sec = (long)(tmpres / 1000000UL);
    tv->tv_usec = (long)(tmpres % 1000000UL);
  }
 
  if (NULL != tz)
  {
    if (!tzflag)
    {
      _tzset();
      tzflag++;
    }
  
// Adjust for the timezone west of Greenwich
      tz->tz_minuteswest = _timezone / 60;
    tz->tz_dsttime = _daylight;
  }
 
  return 0;
}
    


//
// strcasecmp.cc
//
// strcasecmp: replacement of the strcasecmp functions for architectures that do
//             not have it.
//
// Part of the ht://Dig package   <http://www.htdig.org/>
// Copyright (c) 1999-2004 The ht://Dig Group
// For copyright details, see the file COPYING in your distribution
// or the GNU Library General Public License (LGPL) version 2 or later 
// <http://www.gnu.org/copyleft/lgpl.html>
//
// Adopted from http://www.koders.com/cpp/fidC617D7BA26EC220CB9B5516BCAB6B5F25B0EAC3C.aspx
//

int strcasecmp(const char *str1, const char *str2)
{
    if (!str1 && !str2)
	return 0;
    if (!str1)
	return 1;
    if (!str2)
	return -1;
    while (*str1 &&
	   *str2 &&
	   tolower((unsigned char)*str1) == tolower((unsigned char)*str2))
    {
	str1++;
	str2++;
    }

    return tolower((unsigned char)*str1) - tolower((unsigned char)*str2);
}


//#define tolower(ch)	(isupper(ch) ? (ch) + 'a' - 'A' : (ch))
//*****************************************************************************
//
int strncasecmp(const char *str1, const char *str2, size_t n)
{
  if (!str1 && !str2)
    return 0;
  if (!str1)
    return 1;
  if (!str2)
    return -1;
  if (n < 0)
    return 0;
  while (n &&
    *str1 &&
    *str2 &&
    tolower((unsigned char)*str1) == tolower((unsigned char)*str2))
  {
    str1++;
    str2++;
    n--;
  }

  return n == 0 ? 0 :
    tolower((unsigned char)*str1) - tolower((unsigned char)*str2);
}


// From http://www.google.ca/url?sa=t&rct=j&q=strndup%20visual%20studio&source=web&cd=3&ved=0CCwQFjAC&url=http%3A%2F%2Fgit.savannah.gnu.org%2Fgitweb%2F%3Fp%3Dgnulib.git%3Ba%3Dblob_plain%3Bf%3Dlib%2Fstrndup.c&ei=m2ewTq2WDof3ggey8NnNAQ&usg=AFQjCNHP7pU4FfvAlJBDmHN6buMeIT3YXg&sig2=lvzgjF00zVvkt3Q8g-k6eA&cad=rja
char *strndup (char const *s, size_t n)
{
  size_t len = strnlen (s, n);
  char *newbuf = (char *) malloc (len + 1);

  if (newbuf == NULL)
    return NULL;

  newbuf[len] = '\0';
  return (char *) memcpy (newbuf, s, len);
}


//*****************************************************************************
// char *strcasestr(const char *s, const char *pattern)
//
const char *strcasestr(const char *s, const char *pattern)
{
  size_t length = strlen(pattern);

  while (*s) {
    if (strncasecmp(s, pattern, length) == 0)
      return s;
    s++;
  }
  return 0;
}

char *strcasestr(char *s, const char *pattern)
{ 
  return (char*)strcasestr((const char*)s, pattern);
}


// copied from www.mail-archive.com/users@ipv6.org/msg02107.html
const char *inet_ntop(int af, const void *src, char *dst, socklen_t cnt)
{
        if (af == AF_INET)
        {
                struct sockaddr_in in;
                memset(&in, 0, sizeof(in));
                in.sin_family = AF_INET;
                memcpy(&in.sin_addr, src, sizeof(struct in_addr));
                getnameinfo((struct sockaddr *)&in, sizeof(struct sockaddr_in), dst, cnt, NULL, 0, NI_NUMERICHOST);
                return dst;
        }
        else if (af == AF_INET6)
        {
                struct sockaddr_in6 in;
                memset(&in, 0, sizeof(in));
                in.sin6_family = AF_INET6;
                memcpy(&in.sin6_addr, src, sizeof(struct in_addr6));
                getnameinfo((struct sockaddr *)&in, sizeof(struct sockaddr_in6), dst, cnt, NULL, 0, NI_NUMERICHOST);
                return dst;
        }
        return NULL;
}

int inet_pton(int af, const char *src, void *dst)
{
        struct addrinfo hints, *res, *ressave;

        memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_family = af;

        if (getaddrinfo(src, NULL, &hints, &res) != 0)
        {
                return -1;
        }

        ressave = res;

        while (res)
        {
                memcpy(dst, res->ai_addr, res->ai_addrlen);
                res = res->ai_next;
        }

        freeaddrinfo(ressave);
        return 0;
}

