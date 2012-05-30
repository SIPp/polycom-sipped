/*
 * send_packets.c: from tcpreplay tools by Aaron Turner 
 * http://tcpreplay.sourceforge.net/
 * send_packets.c is under BSD license (see below)
 * SIPp is under GPL license
 *
 *
 * Copyright (c) 2001-2004 Aaron Turner.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the names of the copyright owners nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*Map linux structure fields to BSD ones*/
#ifdef __LINUX
#define __BSD_SOURCE
#define _BSD_SOURCE
#define __FAVOR_BSD
#endif /*__LINUX*/

#ifdef WIN32
# pragma warning (disable: 4003; disable: 4996)
# include <winsock2.h>
# include <ws2tcpip.h>

#else
# include <unistd.h>
# include <netinet/udp.h>
# if defined(__DARWIN) || defined(__CYGWIN) || defined(__FreeBSD__)
#  include <netinet/in.h>
# endif
# ifndef __CYGWIN
#  include <netinet/ip6.h>
# endif
# include <arpa/inet.h>
# include <errno.h>
#endif

#include <string.h>
#include <pthread.h>
#include "logging.hpp"
#include "send_packets.hpp"
#include "screen.hpp"
//
#include "sipp_globals.hpp"
#include "prepare_pcap.hpp"
#include <fcntl.h>
#include <pcap.h>
#include <stdlib.h>
#include "win32_compatibility.hpp"
#include <stdio.h>
#include <sys/types.h>





inline void
timerdiv (struct timeval *tvp, float div)
{
  double interval;

  if (div == 0 || div == 1)
    return;

  interval = ((double) tvp->tv_sec * 1000000 + tvp->tv_usec) / (double) div;
  tvp->tv_sec = interval / (int) 1000000;
  tvp->tv_usec = interval - (tvp->tv_sec * 1000000);
}

/*
 * converts a float to a timeval structure
 */
inline void
float2timer (float time, struct timeval *tvp)
{
  float n;

  n = time;

  tvp->tv_sec = n;

  n -= tvp->tv_sec;
  tvp->tv_usec = n * 100000;
}

/* buffer should be "file_name" */
int parse_play_args (char *buffer, pcap_pkts *pkts)
{
  pkts->file = strdup (buffer);
  prepare_pkts(pkts->file, pkts);
  return 1;
}


/*Safe threaded version*/
void do_sleep (struct timeval *, struct timeval *, struct timeval *, struct timeval *);
void send_packets_cleanup(void *arg)
{
  int sock = (int) ((long) arg);

  // Close send socket
  close(sock);
}


int
send_packets (play_args_t * play_args)
{
  int ret, sock;
  pcap_pkt *pkt_index, *pkt_max;
  struct timeval didsleep = { 0, 0 };
  struct timeval start = { 0, 0 };
  struct timeval last = { 0, 0 };
  pcap_pkts *pkts = play_args->pcap;
  int allow_ports_to_change = 0; /* may be exposed at later date */
  struct sockaddr_storage *to, *from, to_struct, from_struct;
  struct sockaddr_in6 to6, from6;
  char buffer[PCAP_MAXPACKET];
  int result = 0;

  if(allow_ports_to_change) {
    // For now, never possible.
    to = &play_args -> to;
    from = &play_args -> from;
  } else {
    to_struct = play_args -> to;
    from_struct = play_args -> from;
    to = &to_struct;
    from = &from_struct;
  }

  DEBUG_IN();

  if (media_ip_is_ipv6) {
    sock = socket(PF_INET6, SOCK_DGRAM, 0);
    if (sock < 0) {
      REPORT_ERROR_NO("Can't create RTP socket (need to run as root/Administrator and/or lower your Windows 7 User Account Settings?)");
    }
    DEBUG("IPv6 from_port = %hu, from_addr = 0x%x, to_port = %hu, to_addr = 0x%x", 
      ntohs(((struct sockaddr_in6 *)(void *) from )->sin6_port), 
      ((struct sockaddr_in6 *)(void *) from )->sin6_addr.s6_addr, 
      ntohs(((struct sockaddr_in6 *)(void *) to )->sin6_port), 
      ((struct sockaddr_in6 *)(void *) to )->sin6_addr.s6_addr);
  }
  else {
    sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
      REPORT_ERROR_NO("Can't create RTP socket (need to run as root/Administrator and/or lower your Windows 7 User Account Settings?)");
    }
    DEBUG("IPv4 from_port = %hu, from_addr = 0x%x, to_port = %hu, to_addr = 0x%x", 
      ntohs(((struct sockaddr_in *)(void *) from )->sin_port), 
      ((struct sockaddr_in *)(void *) from )->sin_addr.s_addr, 
      ntohs(((struct sockaddr_in *)(void *) to )->sin_port), 
      ((struct sockaddr_in *)(void *) to )->sin_addr.s_addr);
  }
  int sock_opt = 1;
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, SETSOCKOPT_TYPE &sock_opt, sizeof (sock_opt)) == -1) {
    REPORT_ERROR_NO("setsockopt(sock, SO_REUSEADDR) failed");
  }
	
  if (!media_ip_is_ipv6) {
    if (bind(sock, (struct sockaddr *) from, sizeof(struct sockaddr_in)) < 0){
      char ip[INET_ADDRSTRLEN];

      inet_ntop(AF_INET, &(((struct sockaddr_in *) from)->sin_addr), ip, INET_ADDRSTRLEN);
      REPORT_ERROR_NO("Could not bind RTP traffic socket to %s:%hu", ip, ntohs(((struct sockaddr_in *) from)->sin_port));
    }
  }
  else {
    if(bind(sock, (struct sockaddr *) from, sizeof(struct sockaddr_in6)) < 0){
      char ip[INET6_ADDRSTRLEN];

      inet_ntop(AF_INET6, &(((struct sockaddr_in6 *) from)->sin6_addr), ip, INET_ADDRSTRLEN);
      REPORT_ERROR_NO("Could not bind socket to send RTP traffic %s:%hu", ip, ntohs(((struct sockaddr_in6 *)from )->sin6_port));
    }
  }

#ifndef MSG_DONTWAIT
# ifdef WIN32
  int iMode = 1;
  ioctlsocket(sock, FIONBIO, (u_long FAR*) &iMode);
# else
  int fd_flags;
  fd_flags = fcntl(sock, F_GETFL , NULL);
  fd_flags |= O_NONBLOCK;
  fcntl(sock, F_SETFL , fd_flags);
# endif
#endif

  pkt_index = pkts->pkts;
  pkt_max = pkts->max;
	
  if (media_ip_is_ipv6) {
    memset(&to6, 0, sizeof(to6));
    memset(&from6, 0, sizeof(from6));
    to6.sin6_family = AF_INET6;
    from6.sin6_family = AF_INET6;
    memcpy(&(to6.sin6_addr.s6_addr), &(((struct sockaddr_in6 *)(void *) to)->sin6_addr.s6_addr), sizeof(to6.sin6_addr.s6_addr));
    memcpy(&(from6.sin6_addr.s6_addr), &(((struct sockaddr_in6 *)(void *) from)->sin6_addr.s6_addr), sizeof(from6.sin6_addr.s6_addr));
    memcpy(&(to6.sin6_port), &(((struct sockaddr_in6 *)(void *) to)->sin6_port), sizeof(to6.sin6_port));
    memcpy(&(from6.sin6_port), &(((struct sockaddr_in6 *)(void *) from)->sin6_port), sizeof(from6.sin6_port));
  }


  /* Ensure the sender socket is closed when the thread exits - this
   * allows the thread to be cancelled cleanly.
   */
  pthread_cleanup_push(send_packets_cleanup, ((void *) sock));

  while (pkt_index < pkt_max) {
    memcpy(buffer,  pkt_index->data, pkt_index->pktlen);

    do_sleep ((struct timeval *) &pkt_index->ts, &last, &didsleep, &start);

#ifdef MSG_DONTWAIT
    if (!media_ip_is_ipv6) {
      ret = sendto(sock, buffer, pkt_index->pktlen, MSG_DONTWAIT, (struct sockaddr *)(void *) to, sizeof(struct sockaddr_in));
    }
    else {
      ret = sendto(sock, buffer, pkt_index->pktlen, MSG_DONTWAIT, (struct sockaddr *)(void *) &to6, sizeof(struct sockaddr_in6));
    }
#else
    if (!media_ip_is_ipv6) {
      ret = sendto(sock, buffer, pkt_index->pktlen, 0, (struct sockaddr *)(void *) to, sizeof(struct sockaddr_in));
    }
    else {
      ret = sendto(sock, buffer, pkt_index->pktlen, 0, (struct sockaddr *)(void *) &to6, sizeof(struct sockaddr_in6));
    }
#endif
    if (ret < 0) {
      WARNING("send_packets.c: sendto failed with error: %s (sock = %d)", strerror(errno), sock);
      result = -1;
      break; // Can not return directly because of pthread_cleanup_push/pop's use of { and }
    }

    rtp_pckts_pcap++;
    rtp_bytes_pcap += pkt_index->pktlen;
    memcpy (&last, &(pkt_index->ts), sizeof (struct timeval));
    pkt_index++;
  }

  /* Closing the socket is handled by pthread_cleanup_push()/pthread_cleanup_pop() */
  pthread_cleanup_pop(1);
  DEBUG_OUT();
  return result;
}

/*
 * Given the timestamp on the current packet and the last packet sent,
 * calculate the appropriate amount of time to sleep and do so.
 */
void do_sleep (struct timeval *time, struct timeval *last, struct timeval *didsleep, struct timeval *start)
{
  struct timeval nap, now, delta;

  if (gettimeofday (&now, NULL) < 0)
    {
      WARNING("Error gettimeofday: %s\n", strerror (errno));
    }

  /* First time through for this file */
  if (!timerisset (last))
    {
      *start = now;
      timerclear (&delta);
      timerclear (didsleep);
    }
  else
    {
      timersub (&now, start, &delta);
    }

  if (timerisset (last) && timercmp (time, last, >))
    {
      timersub (time, last, &nap);
    }
  else
    {
      /* 
       * Don't sleep if this is our first packet, or if the
       * this packet appears to have been sent before the 
       * last packet.
       */
      timerclear (&nap);
    }

  timeradd (didsleep, &nap, didsleep);

  if (timercmp (didsleep, &delta, >))
    {
      timersub (didsleep, &delta, &nap);

#ifdef WIN32
      usleep(nap.tv_sec*1000000+nap.tv_usec);
#else
      struct timespec sleep;
      sleep.tv_sec = nap.tv_sec;
      sleep.tv_nsec = nap.tv_usec * 1000;	/* convert us to ns */
      while ((nanosleep (&sleep, &sleep) == -1) && (errno == -EINTR));
#endif
    }
}


