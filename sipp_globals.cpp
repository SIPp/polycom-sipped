/*
/*
 * sipp_globals.cpp
 *
 *  Created on: Mar 9, 2012
 *      Author: sipped
 */
#include "sipp_globals.hpp"
#include "call.hpp"
//rtcheck = RTCHECK_FULL;const def in call.cpp
/** has to be after defined constants above because call.cpp needs us
 * to provide defines and we need call.hpp to get his contstants
 * int                rtcheck                 = RTCHECK_FULL;  in sipp_globals.cpp
 *
 *
In file included from listener.hpp:33,
                 from call.hpp:55,
                 from sipp_globals.hpp:18,
                 from sipp_globals.cpp:9:
sipp.hpp:464: error: ‘MAX_LOCAL_TWIN_SOCKETS’ was not declared in this scope
 *
 * MAX_LOCAL_TWIN_SOCKETS is defined above but
 */
// #include "call.hpp" //rtcheck = const def in call.cpp

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

//unsigned long getmilliseconds();
//unsigned long long getmicroseconds();

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
unsigned long      rtp_pckts_pcap          = 0;
unsigned long      rtp_bytes_pcap          = 0;
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


//#define GET_TIME(clock)       \
//{                             \
//  struct timezone tzp;        \
//  gettimeofday (clock, &tzp); \
//}

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

//void rotate_errorf();
//int rotatef(struct logfile_info *lfi);
//void log_off(struct logfile_info *lfi);

/* Screen/Statistics Printing Functions. */
//void print_statistics(int last);
//void print_count_file(FILE *f, int header);


/********************* Mini-Parser Routines *******************/

//int get_method(char *msg);
//char * get_tag_from_to(char *msg);
//char * get_tag_from_from(char *msg);
//unsigned long int get_cseq_value(const char *msg);
//unsigned long get_reply_code(const char *msg);

/********************** Network Interfaces ********************/

//int send_message(int s, void ** comp_state, char * msg);
//#ifdef _USE_OPENSSL
//int send_message_tls(SSL *s, void ** comp_state, char * msg);
//#endif

/* Socket Buffer Management. */
//#define NO_COPY 0
//#define DO_COPY 1
//struct socketbuf *alloc_socketbuf(char *buffer, size_t size, int copy);
//void free_socketbuf(struct socketbuf *socketbuf);

/* These buffers lets us read past the end of the message, and then split it if
 * required.  This eliminates the need for reading a message octet by octet and
 * performing a second read for the content length. */
//struct socketbuf {
//  char *buf;
//  size_t len;
//  size_t offset;
//  struct sockaddr_storage addr;
//  struct socketbuf *next;
//};

/* This is an abstraction of a socket, which provides buffers for input and
 * output. */
//struct sipp_socket {
//  int  ss_count; /* How many users are there of this socket? */
//
//  int ss_transport; /* T_TCP, T_UDP, or T_TLS. */
//  bool ss_ipv6;
//  bool ss_control; /* Is this a control socket? */
//  bool ss_call_socket; /* Is this a call socket? */
//  bool ss_changed_dest; /* Has the destination changed from default. */
//
//  int ss_fd;  /* The underlying file descriptor for this socket. */
//  void *ss_comp_state; /* The compression state. */
//#ifdef _USE_OPENSSL
//  SSL *ss_ssl;  /* The underlying SSL descriptor for this socket. */
//  BIO *ss_bio;  /* The underlying BIO descriptor for this socket. */
//#endif
//  struct sockaddr_storage ss_remote_sockaddr; /* Who we are talking to. */
//  struct sockaddr_storage ss_dest; /* Who we are talking to. */
//
//
//  int ss_pollidx; /* The index of this socket in our poll structures. */
//  bool ss_congested; /* Is this socket congested? */
//  bool ss_invalid; /* Has this socket been closed remotely? */
//
//  struct socketbuf *ss_in; /* Buffered input. */
//  size_t ss_msglen; /* Is there a complete SIP message waiting, and if so how big? */
//  struct socketbuf *ss_out; /* Buffered output. */
//};

/* Write data to a socket. */
//int write_socket(struct sipp_socket *socket, char *buffer, ssize_t len, int flags, struct sockaddr_storage *dest);
/* Mark a socket as "bad". */
//void sipp_socket_invalidate(struct sipp_socket *socket);
/* Actually free the socket. */
//void sipp_close_socket(struct sipp_socket *socket);

//#define WS_EAGAIN 1 /* Return EAGAIN if there is no room for writing the message. */
//#define WS_BUFFER 2 /* Buffer the message if there is no room for writing the message. */


//#if defined (__hpux) || defined (__alpha) && !defined (__FreeBSD__)
//#define sipp_socklen_t  int
//#else
//#define sipp_socklen_t  socklen_t
//#endif

//#define SOCK_ADDR_SIZE(a) \
//  (((a)->ss_family == AF_INET) ? sizeof(struct sockaddr_in) \
//                               : sizeof(struct sockaddr_in6))
//
//#if defined(__cplusplus) && defined (__hpux)
//#define _RCAST(type, val) (reinterpret_cast<type> (val))
//#else
//#define _RCAST(type, val) ((type)(val))
//#endif

/********************* Utilities functions  *******************/

//char *strcasestr2 ( const char *__haystack, const char *__needle);
//char *get_peer_addr(char *);
//int get_decimal_from_hex(char hex);
//
//bool reconnect_allowed();
//void reset_connection(struct sipp_socket *);
//void close_calls(struct sipp_socket *);
//int close_connections();
//int open_connections();
//void timeout_alarm(int);
//
//int determine_remote_and_local_ip();
//
//char *jump_over_timestamp(char *src);

/* extended 3PCC mode */
//struct sipp_socket **get_peer_socket(char *);
//bool is_a_peer_socket(struct sipp_socket *);
//bool is_a_local_socket(struct sipp_socket *);
//void connect_to_peer (char *, int , sockaddr_storage *, char *, struct sipp_socket **);
//void connect_to_all_peers ();
//void connect_local_twin_socket(char *);
//void close_peer_sockets();
//void close_local_sockets();
//void free_peer_addr_map();

/******************** Recv Poll Processing *********************/

struct sipp_socket* sockets[SIPP_MAXFDS];

// all defines in hpp
// all pure struct defintions in hpp
// all sipp.cpp global function decl in hpp
// only mem alloc in this file


