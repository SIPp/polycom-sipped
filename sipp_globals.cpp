/*
 * sipp_globals.cpp
 *
 *  Created on: Mar 9, 2012
 *      Author: rlum
 */
#include "sipp_globals.hpp"
#include "call.hpp"
//
#include "socketowner.hpp"
#include "variables.hpp"
#ifdef WIN32
#include <winsock2.h>
#else
#include <netdb.h>
#endif
#include <stdio.h>
#include <stddef.h>  // RTCHECK_FULL  MAX_LOCAL_TWIN_SOCKETS
#ifdef _USE_OPENSSL
#include "sslcommon.hpp"
#endif



int                duration                = 0;
double             rate                    = DEFAULT_RATE;
double             rate_scale              = DEFAULT_RATE_SCALE;
int                rate_increase           = 0;
int                rate_max                = 0;
bool               rate_quit               = true;
int                users                   = -1;
int                rate_period_ms          = DEFAULT_RATE_PERIOD_MS;
int                sleeptime               = 0;
unsigned long      defl_recv_timeout       = 0;
unsigned long      defl_send_timeout       = 0;
unsigned long      global_timeout          = 0;
int                transport               = DEFAULT_TRANSPORT;
bool               retrans_enabled         = 1;

//call.hpp defines constants that these initialize to
int                rtcheck                 = RTCHECK_FULL;
int                max_udp_retrans         = UDP_MAX_RETRANS;
int                max_invite_retrans      = UDP_MAX_RETRANS_INVITE_TRANSACTION;
int                max_non_invite_retrans  = UDP_MAX_RETRANS_NON_INVITE_TRANSACTION;

unsigned long      default_behaviors       = DEFAULT_BEHAVIOR_ALL;
unsigned long    deadcall_wait             = DEFAULT_DEADCALL_WAIT;
bool               pause_msg_ign           = 0;
bool               auto_answer             = false;
int                multisocket             = 0;
int                compression             = 0;
int                peripsocket             = 0;
int                peripfield              = 0;
bool               bind_local              = false;
void             * monosocket_comp_state   = 0;
char             * service                 = DEFAULT_SERVICE;
char             * auth_password           = DEFAULT_AUTH_PASSWORD;
unsigned long      report_freq             = DEFAULT_REPORT_FREQ;
unsigned long      report_freq_dumpLog     = DEFAULT_REPORT_FREQ_DUMP_LOG;
bool               periodic_rtd            = false;
const char       * stat_delimiter          = ";";
////////////////////////////////


bool               timeout_exit            = false;
bool               timeout_error           = false;

unsigned long      report_freq_dumpRtt     = DEFAULT_FREQ_DUMP_RTT;

int                max_multi_socket        = DEFAULT_MAX_MULTI_SOCKET;
bool               skip_rlimit             = false;

unsigned int       timer_resolution        = DEFAULT_TIMER_RESOLUTION;
int                max_recv_loops          = MAX_RECV_LOOPS_PER_CYCLE;
int                max_sched_loops         = MAX_SCHED_LOOPS_PER_CYCLE;

unsigned int       global_t2               = DEFAULT_T2_TIMER_VALUE;
unsigned int       auto_answer_expires     = DEFAULT_AUTO_ANSWER_EXPIRES;

char               local_ip[40];
char               local_ip_escaped[42];
bool               local_ip_is_ipv6;
int                local_port              = 0;
char               control_ip[40];
int                control_port            = 0;
int                buff_size               = 65535;
int                tcp_readsize            = 65535;
#ifdef PCAPPLAY
int                hasMedia                = 0;
#endif
bool               rtp_echo_enabled        = 0;
char               media_ip[40];
char               media_ip_escaped[42];
int                user_media_port         = 0;
int                media_port              = 0;
size_t             media_bufsize           = 2048;
bool               media_ip_is_ipv6;
char               remote_ip[40];
char               remote_ip_escaped[42];
int                remote_port             = DEFAULT_PORT;
unsigned int       pid                     = 0;
bool               print_all_responses     = false;
unsigned long      stop_after              = 0xffffffff;
int                quitting                = 0;
bool               q_pressed               = false;
int                interrupt               = 0;
bool               paused                  = false;
int                lose_packets            = 0;
double             global_lost             = 0.0;
char               remote_host[255];
char               twinSippHost[255];
char               twinSippIp[40];
char             * master_name;
char             * slave_number;
int                twinSippPort            = DEFAULT_3PCC_PORT;
bool               twinSippMode            = false;
bool               extendedTwinSippMode    = false;

bool               nostdin                 = false;
bool               backgroundMode          = false;
bool               signalDump              = false;

int                currentScreenToDisplay  = DISPLAY_SCENARIO_SCREEN;
int                currentRepartitionToDisplay  = 1;
unsigned int       base_cseq               = 0;
char             * auth_uri                = 0;
const char       * call_id_string          = "%u-%p@%s";
char             **generic[100];
bool               no_call_id_check        = false;
int                dump_xml                = 0;
int                dump_sequence_diagram   = 0;

bool               force_client_mode       = false;
bool               force_server_mode       = false;

/* TDM map */
bool               use_tdmmap              = false;
unsigned int       tdm_map_a               = 0;
unsigned int       tdm_map_b               = 0;
unsigned int       tdm_map_c               = 0;
unsigned int       tdm_map_x               = 0;
unsigned int       tdm_map_y               = 0;
unsigned int       tdm_map_z               = 0;
unsigned int       tdm_map_h               = 0;
bool               tdm_map[1024];

#ifdef _USE_OPENSSL
BIO               *twinSipp_bio ;
SSL               *twinSipp_ssl ;
char              *tls_cert_name           = DEFAULT_TLS_CERT;
char              *tls_key_name            = DEFAULT_TLS_KEY;
char              *tls_crl_name            = DEFAULT_TLS_CRL;
#endif

// extern field file management
//typedef std::map<string, FileContents *> file_map;
file_map           inFiles;
//typedef std::map<string, str_int_map *> file_index;
char              *ip_file                 = NULL;
char              *default_file            = NULL;

// free user id list
list<int>          freeUsers;
list<int>          retiredUsers;
AllocVariableTable* globalVariables        = NULL;
AllocVariableTable* userVariables          = NULL;
//typedef std::map<int, VariableTable *> int_vt_map;
int_vt_map          userVarMap;

//extern int      new_socket(bool P_use_ipv6, int P_type_socket, int * P_status);
//struct   sipp_socket *new_sipp_socket(bool use_ipv6, int transport);
//struct sipp_socket *new_sipp_call_socket(bool use_ipv6, int transport, bool *existing);
//struct sipp_socket *sipp_accept_socket(struct sipp_socket *accept_socket, struct sockaddr_storage *source=0);
//int  sipp_bind_socket(struct sipp_socket *socket, struct sockaddr_storage *saddr, int *port);
//int  sipp_connect_socket(struct sipp_socket *socket, struct sockaddr_storage *dest);
//int      sipp_reconnect_socket(struct sipp_socket *socket);
//void sipp_customize_socket(struct sipp_socket *socket);
//int      delete_socket(int P_socket);
int                min_socket              = 65535;
int                select_socket           = 0;
bool               socket_close            = true;
bool               test_socket             = true;
bool               maxSocketPresent        = false;


/************************ Statistics **************************/

unsigned long      last_report_calls       = 0;
unsigned long      nb_net_send_errors      = 0;
unsigned long      nb_net_cong             = 0;
unsigned long      nb_net_recv_errors      = 0;
bool               cpu_max                 = false;
bool               outbound_congestion     = false;
int                open_calls_user_setting = 0;
int                resynch_send            = 0;
int                resynch_recv            = 0;
unsigned long      rtp_pckts               = 0;
unsigned long      rtp_bytes               = 0;
volatile unsigned long      rtp_pckts_pcap          = 0;
volatile unsigned long      rtp_bytes_pcap          = 0;
unsigned long      rtp2_pckts              = 0;
unsigned long      rtp2_bytes              = 0;
unsigned long      rtp2_pckts_pcap         = 0;
unsigned long      rtp2_bytes_pcap         = 0;

/************* Rate Control & Contexts variables **************/

int                last_running_calls      = 0;
int                last_woken_calls        = 0;
int                last_paused_calls       = 0;
unsigned int       open_calls_allowed      = 0;
unsigned long      last_report_time        = 0;
unsigned long      last_dump_time          = 0;

/********************** Clock variables ***********************/

unsigned long      clock_tick              = 0;
unsigned long      scheduling_loops        = 0;
unsigned long      last_timer_cycle        = 0;

unsigned long      watchdog_interval       = 400;
unsigned long      watchdog_minor_threshold = 500;
unsigned long      watchdog_minor_maxtriggers = 120;
unsigned long      watchdog_major_threshold = 3000;
unsigned long      watchdog_major_maxtriggers = 10;
unsigned long      watchdog_reset          = 600000;


/********************* dynamic Id ************************* */
int               maxDynamicId            = 12000;  // max value for dynamicId; this value is reached
int               startDynamicId          = 10000;  // offset for first dynamicId  FIXME:in CmdLine
int               stepDynamicId           = 4;      // step of increment for dynamicId

/**
#define GET_TIME(clock)       \
{                             \
  struct timezone tzp;        \
  gettimeofday (clock, &tzp); \
}
**/

/*********************** Global Sockets  **********************/

struct sipp_socket* main_socket            = NULL;
struct sipp_socket* main_remote_socket     = NULL;
struct sipp_socket* tcp_multiplex          = NULL;
int                media_socket            = 0;
int                media_socket_video      = 0;

struct             sockaddr_storage   local_sockaddr;
struct             sockaddr_storage   localTwin_sockaddr;
int                user_port               = 0;
char               hostname[80];
bool               is_ipv6                 = false;

int                reset_number            = 0;
bool               reset_close             = true;
int                reset_sleep             = 1000;
bool               sendbuffer_warn         = false;
/* A list of sockets pending reset. */
set<struct sipp_socket *> sockets_pending_reset;

struct addrinfo *  local_addr_storage;

struct sipp_socket* twinSippSocket         = NULL;
struct sipp_socket* localTwinSippSocket    = NULL;
struct sockaddr_storage twinSipp_sockaddr;

/* 3pcc extended mode */
//typedef struct _T_peer_infos {
//               char                       peer_host[40];
//               int                        peer_port;
//               struct sockaddr_storage    peer_sockaddr;
//               char                       peer_ip[40];
//               struct sipp_socket         *peer_socket ;
//               } T_peer_infos;

//typedef std::map<std::string, char * > peer_addr_map;
peer_addr_map      peer_addrs;
//typedef std::map<std::string, T_peer_infos> peer_map;
peer_map           peers;
//typedef std::map<struct sipp_socket *, std::string > peer_socket_map;
peer_socket_map    peer_sockets;
struct sipp_socket* local_sockets[MAX_LOCAL_TWIN_SOCKETS];
int                local_nb                = 0;
int                peers_connected         = 0;

struct sockaddr_storage remote_sockaddr;
short              use_remote_sending_addr = 0;
struct sockaddr_storage remote_sending_sockaddr;

//enum E_Alter_YesNo
//  {
//    E_ALTER_YES=0,
//    E_ALTER_NO
//  };

/************************** Trace Files ***********************/

FILE *             screenf                 = 0;
FILE *             countf                  = 0;
// extern FILE * timeoutf                  = 0;
bool               useMessagef             = 0;
bool               useCallDebugf           = 0;
bool               useShortMessagef        = 0;
bool               useScreenf              = 0;
bool               useLogf                 = 0;
bool               useDebugf               = 0;
bool               useExecf                = 0;
//extern bool   useTimeoutf                = 0;
bool               dumpInFile              = 0;
bool               dumpInRtt               = 0;
bool               useCountf               = 0;
char *             scenario_file;
char *             slave_cfg_file;

char               screen_last_error[32768];


/******************** Recv Poll Processing *********************/

struct sipp_socket* sockets[SIPP_MAXFDS];


/***************** System Portability Features *****************/

unsigned long long getmicroseconds()
{
  struct timeval LS_system_time;
  unsigned long long VI_micro;
  static unsigned long long VI_micro_base = 0;

  gettimeofday(&LS_system_time, NULL);
  VI_micro = (((unsigned long long) LS_system_time.tv_sec) * 1000000LL) + LS_system_time.tv_usec;
  if (!VI_micro_base) VI_micro_base = VI_micro - 1;
  VI_micro = VI_micro - VI_micro_base;

  clock_tick = (unsigned long) (VI_micro / 1000LL);

  return VI_micro;
}

unsigned long getmilliseconds()
{
  return (unsigned long) (getmicroseconds() / 1000LL);
}

static unsigned char tolower_table[256];

void init_tolower_table()
{
  for (int i = 0; i < 256; i++) {
    tolower_table[i] = tolower(i);
  }
}

/* This is simpler than doing a regular tolower, because there are no branches.
 * We also inline it, so that we don't have function call overheads.
 *
 * An alternative to a table would be to do (c | 0x20), but that only works if
 * we are sure that we are searching for characters (or don't care if they are
 * not characters. */
unsigned char inline mytolower(unsigned char c)
{
  return tolower_table[c];
}

char * strncasestr(char *s, const char *find, size_t n)
{
  char *end = s + n;
  char c, sc;
  size_t len;

  if ((c = *find++) != 0) {
    c = mytolower((unsigned char)c);
    len = strlen(find);
    end -= (len - 1);
    do {
      do {
        if ((sc = *s++) == 0)
          return (NULL);
        if (s >= end)
          return (NULL);
      } while ((char)mytolower((unsigned char)sc) != c);
    } while (strncasecmp(s, find, len) != 0);
    s--;
  }
  return ((char *)s);
}

char * strcasestr2(const char *s, const char *find)
{
  char c, sc;
  size_t len;

  if ((c = *find++) != 0) {
    c = mytolower((unsigned char)c);
    len = strlen(find);
    do {
      do {
        if ((sc = *s++) == 0)
          return (NULL);
      } while ((char)mytolower((unsigned char)sc) != c);
    } while (strncasecmp(s, find, len) != 0);
    s--;
  }
  return ((char *)s);
}

char *jump_over_timestamp(char *src)
{
  char* tmp = src;
  int colonsleft = 4;/* We want to skip the time. */
  while (*tmp && colonsleft) {
    if (*tmp == ':') {
      colonsleft--;
    }
    tmp++;
  }
  while (isspace(*tmp)) {
    tmp++;
  }
  return tmp;
}


int get_decimal_from_hex(char hex)
{
  if (isdigit(hex))
    return hex - '0';
  else
    return tolower(hex) - 'a' + 10;
}


// Socket helper routines moved from sipp.cpp


struct sipp_socket **get_peer_socket(char * peer) {
  struct sipp_socket **peer_socket;
  T_peer_infos infos;
  peer_map::iterator peer_it;
  peer_it = peers.find(peer_map::key_type(peer));
  if(peer_it != peers.end()) {
    infos = peer_it->second;
    peer_socket = &(infos.peer_socket);
    return peer_socket;
  }
  return NULL;
}

char * get_peer_addr(char * peer)
{
  char * addr;
  peer_addr_map::iterator peer_addr_it;
  peer_addr_it = peer_addrs.find(peer_addr_map::key_type(peer));
  if(peer_addr_it != peer_addrs.end()) {
    addr =  peer_addr_it->second;
    return addr;
  }
  return NULL;
}

bool is_a_peer_socket(struct sipp_socket *peer_socket)
{
  peer_socket_map::iterator peer_socket_it;
  peer_socket_it = peer_sockets.find(peer_socket_map::key_type(peer_socket));
  if(peer_socket_it == peer_sockets.end()) {
    return false;
  } else {
    return true;
  }
}

// Hacky stuff 

void stop_all_traces()
{
  message_lfi.fptr = NULL;
  log_lfi.fptr = NULL;
  if(dumpInRtt) dumpInRtt = 0;
  if(dumpInFile) dumpInFile = 0;
}
