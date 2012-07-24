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
 *  Author : Guillaume TEISSIER from FTR&D 02/02/2006
 */

#include <stdlib.h>
#ifdef WIN32
# include <WinSock2.h>
# include <ws2tcpip.h>

#else
# include <netinet/in.h>
# include <netinet/udp.h>
# if defined(__HPUX) || defined(__CYGWIN) || defined(__FreeBSD__)
# include <netinet/in_systm.h>
# endif
# include <netinet/ip.h>
# ifndef __CYGWIN
# include <netinet/ip6.h>
# endif
#endif
#include <string.h>
#include "prepare_pcap.hpp"
#include "screen.hpp"
#include "logging.hpp"
//
#include <pcap.h>
#include "win32_compatibility.hpp"

/* We define our own structures for Ethernet Header and IPv6 Header as they are not available on CYGWIN.
 * We only need the fields, which are necessary to determine the type of the next header.
 * we could also define our own structures for UDP and IPv4. We currently use the structures
 * made available by the platform, as we had no problems to get them on all supported platforms.
 */

typedef struct _ether_hdr {
  char dontcare[12];
  u_int16_t ether_type; /* we only need the type, so we can determine, if the next header is IPv4 or IPv6 */
} ether_hdr;

typedef struct _sll_hdr {
  char dontcare[14];
  u_int16_t sll_type; /* we only need the type, so we can determine, if the next header is IPv4 or IPv6 */
} sll_hdr;

typedef struct _radio_hdr {
  char dontcare[58];
  u_int16_t radio_type;
} radio_hdr;

typedef struct _ipv6_hdr {
  char dontcare[6];
  u_int8_t nxt_header; /* we only need the next header, so we can determine, if the next header is UDP or not */
  char dontcare2[33];
} ipv6_hdr;

typedef struct _vlan_hdr {
  u_int16_t vlan_data;
  u_int16_t vlan_type;
} vlan_hdr;

#ifdef __HPUX
int check(u_int16_t *buffer, int len)
{
#else
inline int check(u_int16_t *buffer, int len)
{
#endif
  int sum;
  int i;
  sum = 0;

  for (i=0; i<(len&~1); i+= 2)
    sum += *buffer++;

  if (len & 1) {
    sum += htons( (*(const u_int8_t *)buffer) << 8);
  }
  return sum;
}

#ifdef __HPUX
u_int16_t checksum_carry(int s)
{
#else
inline u_int16_t checksum_carry(int s)
{
#endif
  int s_c = (s >> 16) + (s & 0xffff);
  return (~(s_c + (s_c >> 16)) & 0xffff);
}

char errbuf[PCAP_ERRBUF_SIZE];

/* prepare a pcap file
 */
int prepare_pkts(char *file, pcap_pkts *pkts)
{
  pcap_t *pcap;
  struct pcap_pkthdr *pkthdr = NULL;
  u_char *pktdata = NULL;
  int n_pkts = 0;
  u_long max_length = 0;
  u_int16_t base = 0xffff;
  u_long pktlen;
  pcap_pkt *pkt_index;
  u_int16_t ip_type;
  int frame_size;
  vlan_hdr *vlanhdr;
  struct iphdr *iphdr;
  ipv6_hdr *ip6hdr;
  struct udphdr *udphdr;

  pkts->pkts = NULL;

  pcap = pcap_open_offline(file, errbuf);
  if (!pcap)
    REPORT_ERROR("Can't open PCAP file '%s'. pcap_open_offline returned error: '%s'", file, errbuf);
  int num_of_non_ip_packets = 0;
  int num_of_non_udp_packets = 0;

#if HAVE_PCAP_NEXT_EX
  while (pcap_next_ex (pcap, &pkthdr, (const u_char **) &pktdata) == 1) {
#else
#ifdef __HPUX
  pkthdr = (pcap_pkthdr *) malloc (sizeof (*pkthdr));
#else
  pkthdr = (pcap_pkthdr *) malloc (sizeof (*pkthdr));
#endif
  if (!pkthdr)
    REPORT_ERROR("Can't allocate memory for pcap pkthdr");
  while ((pktdata = (u_char *) pcap_next (pcap, pkthdr)) != NULL) {
#endif
    int link_type = pcap_datalink(pcap);
    if(link_type == DLT_EN10MB) {
      ether_hdr* ethhdr = (ether_hdr *)pktdata;
      ip_type = ethhdr->ether_type;
      frame_size = sizeof(*ethhdr);
    } else if(link_type == DLT_LINUX_SLL) {
      sll_hdr* sllhdr = (sll_hdr *)pktdata;
      ip_type = sllhdr->sll_type;
      frame_size = sizeof(*sllhdr);
    } else if(link_type == DLT_IEEE802_11_RADIO) {
      radio_hdr* radiohdr = (radio_hdr *)pktdata;
      ip_type = radiohdr->radio_type;
      frame_size = sizeof(*radiohdr);
    } else {
      REPORT_ERROR("Unrecognized link layer type");
    }
    if (ntohs(ip_type) == 0x8100 /* VLAN */) {
      int num_of_vlan_headers = 1;
      vlanhdr = (vlan_hdr *)((char *)pktdata + frame_size);
      while (ntohs(vlanhdr->vlan_type) == 0x8100 /* VLAN */) {
        num_of_vlan_headers ++;
        vlanhdr = (vlan_hdr *)((char*)vlanhdr + sizeof(*vlanhdr));
      }
      if(num_of_vlan_headers > 2) {
        WARNING("%d VLAN headers detected. There should be at most 2", num_of_vlan_headers);
      }
      if (ntohs(vlanhdr->vlan_type) != 0x0800 /* IPv4 */
          && ntohs(vlanhdr->vlan_type) != 0x86dd) { /* IPv6 */
        num_of_non_ip_packets ++;
        continue;
      }
      iphdr = (struct iphdr *)((char *)vlanhdr + sizeof(*vlanhdr));
    } else if (ntohs(ip_type) != 0x0800 /* IPv4 */
               && ntohs(ip_type) != 0x86dd) { /* IPv6 */
      num_of_non_ip_packets ++;
      continue;
    } else {
      iphdr = (struct iphdr *)((char *)pktdata + frame_size);
    }
    if (iphdr && iphdr->version == 6) {
      //ipv6
      pktlen = (u_long) pkthdr->len - frame_size - sizeof(*ip6hdr);
      ip6hdr = (ipv6_hdr *)(void *) iphdr;
      if (ip6hdr->nxt_header != IPPROTO_UDP) {
        DEBUG("prepare_pcap.c: Ignoring non UDP packet!\n");
        continue;
      }
      udphdr = (struct udphdr *)((char *)ip6hdr + sizeof(*ip6hdr));
    } else {
      //ipv4
      if (iphdr->protocol != IPPROTO_UDP) {
        num_of_non_udp_packets ++;
        continue;
      }
#if defined(__DARWIN) || defined(__CYGWIN) || defined(__FreeBSD__)
      udphdr = (struct udphdr *)((char *)iphdr + (iphdr->ihl << 2) + 4);
      pktlen = (u_long)(ntohs(udphdr->uh_ulen)) - (sizeof(struct udphdr));
#elif defined ( __HPUX)
      udphdr = (struct udphdr *)((char *)iphdr + (iphdr->ihl << 2));
      pktlen = (u_long) pkthdr->len - sizeof(*ethhdr) - sizeof(*iphdr) - (sizeof(struct udphdr));
#else
      udphdr = (struct udphdr *)((char *)iphdr + (iphdr->ihl << 2));
      pktlen = (u_long)(ntohs(udphdr->len) - (sizeof(struct udphdr)));
#endif
    }
    if (pktlen > PCAP_MAXPACKET) {
      REPORT_ERROR("Packet size is too big or corrupt capture file (%d > %d)! Recompile with bigger PCAP_MAXPACKET in prepare_pcap.h", pktlen, PCAP_MAXPACKET);
    }
    pkts->pkts = (pcap_pkt *) realloc(pkts->pkts, sizeof(*(pkts->pkts)) * (n_pkts + 1));
    if (!pkts->pkts)
      REPORT_ERROR("Can't re-allocate memory for pcap pkt");
    pkt_index = pkts->pkts + n_pkts;
    pkt_index->pktlen = pktlen;
    pkt_index->ts = pkthdr->ts;
    pkt_index->data = (unsigned char *) malloc(pktlen);
    if (!pkt_index->data)
      REPORT_ERROR("Can't allocate memory for pcap pkt data");
    // copy data, skipping over udp header itself
    memcpy(pkt_index->data, (char *)udphdr+(sizeof(struct udphdr)), pktlen);

#if defined(__HPUX) || defined(__DARWIN) || (defined __CYGWIN) || defined(__FreeBSD__)
    udphdr->uh_sum = 0 ;
#else
    udphdr->check = 0;
#endif

    // compute a partial udp checksum
    // not including port that will be changed
    // when sending RTP
#if defined(__HPUX) || defined(__DARWIN) || (defined __CYGWIN) || defined(__FreeBSD__)
    pkt_index->partial_check = check((u_int16_t *) &udphdr->uh_ulen, pktlen - 4) + ntohs(IPPROTO_UDP + pktlen);
#else
    if (pktlen > USHRT_MAX)
      WARNING("pktlen is greater than USHRT_MAX: %d",pktlen);
    pkt_index->partial_check = check((u_int16_t *) &udphdr->len, pktlen - 4) + ntohs(IPPROTO_UDP + (unsigned short)pktlen);
#endif
    if (max_length < pktlen)
      max_length = pktlen;
#if defined(__HPUX) || defined(__DARWIN) || (defined __CYGWIN) || defined(__FreeBSD__)
    if (base > ntohs(udphdr->uh_dport))
      base = ntohs(udphdr->uh_dport);
#else
    if (base > ntohs(udphdr->dest))
      base = ntohs(udphdr->dest);
#endif
    n_pkts++;
  }

  if (num_of_non_ip_packets)
    WARNING("Ignored %d non IP{4,6} packets.\n", num_of_non_ip_packets);
  if (num_of_non_udp_packets)
    WARNING("Ignored %d non UDP packets.\n", num_of_non_udp_packets);

  pkts->max = pkts->pkts + n_pkts;
  pkts->max_length = max_length;
  pkts->base = base;

  //fprintf(stderr, "In pcap %s, npkts %d\nmax pkt length %ld\nbase port %d\n", file, n_pkts, max_length, base);
  pcap_close(pcap);

  return 0;
}

/*
FIXME: this is broken, and never used
void free_pkts(pcap_pkts *pkts) {
  pcap_pkt *pkt_index;
  while (pkt_index < pkts->max) {
    free(pkt_index->data);
  }
  free(pkts->pkts);
}
*/
