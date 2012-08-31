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
 *  Author : Richard GAYRAUD - 04 Nov 2003
 *           Marc LAMBERTON
 *           Olivier JACQUES
 *           Herve PELLAN
 *           David MANSUTTI
 *           Francois-Xavier Kowalski
 *           Gerard Lyonnaz
 *           Francois Draperi (for dynamic_id)
 *           From Hewlett Packard Company.
 *           F. Tarek Rogers
 *           Peter Higginson
 *           Vincent Luba
 *           Shriram Natarajan
 *           Guillaume Teissier from FTR&D
 *           Clement Chen
 *           Wolfgang Beck
 *           Charles P Wright from IBM Research
 *           Martin Van Leeuwen
 *           Andy Aicken
 *       Michael Hirschbichler
 */


#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef WIN32
#include <time.h>
#include <pthread.h>
#include <conio.h>
#else
#include <netinet/tcp.h>
#include <sys/poll.h>
#include <sys/resource.h>
#include <dlfcn.h>
#include <errno.h>
#endif
#include "win32_compatibility.hpp"




#ifdef __SUNOS
#include <stdarg.h>
#endif

#if defined(__HPUX) || defined(__SUNOS)
#include <alloca.h>
#endif

#include "assert.h"
#include <string.h>
#ifdef WIN32
# include <io.h>
# include <process.h>
#include <time.h>  // for clock usage in global timer
#else
#include "comp.hpp"
#endif

#include "call.hpp"
#include "logging.hpp"
#include "opentask.hpp"
#include "reporttask.hpp"
#include "screen.hpp"
#include "sipp_globals.hpp"
#include "sipp.hpp"
#include "watchdog.hpp"
#include "socket_helper.hpp"
#include "sipp_sockethandler.hpp"
#include <stdexcept>

#ifndef __CYGWIN
#ifndef FD_SETSIZE
#define FD_SETSIZE 65000
#endif
#else
#ifndef FD_SETSIZE
#define FD_SETSIZE 1024
#endif
#endif

#ifdef WIN32
#pragma warning (disable: 4003; disable: 4996)
#endif

#ifdef _USE_OPENSSL

enum ssl_init_status {
  SSL_INIT_NORMAL, /* 0   Normal completion    */
  SSL_INIT_ERROR   /* 1   Unspecified error    */
};

#define CALL_BACK_USER_DATA "ksgr"

int passwd_call_back_routine(char  *buf , int size , int flag, void *passwd)
{
  strncpy(buf, (char *)(passwd), size);
  buf[size - 1] = '\0';
  return(strlen(buf));
}
#endif

struct sipp_socket *ctrl_socket = NULL;  // ctrl_socket is network socket for remote control of SIPp
struct sipp_socket *stdin_socket = NULL; // stdin_socket treats stdin as socket for use in SIPp poll loop

void set_sipp_version_string(){
  memset(sipp_version,0,SIPPVERSSIZE);
  sprintf(sipp_version,"SIPped v3.2.67"
#ifdef WIN32
               "-W32"
#endif
#ifdef _USE_OPENSSL
               "-TLS"
#endif
#ifdef PCAPPLAY
               "-PCAP"
#endif
               ", version %s, built %s, %s.",
               CREATESTRING($Revision$), __DATE__, __TIME__);
}


/* These could be local to main, but for the option processing table. */
static int argiFileName;

/***************** Option Handling Table *****************/
struct sipp_option {
  const char *option;
  const char *help;
  int type;
  void *data;
  /* Pass 0: Help and other options that should exit immediately. */
  /* Pass 1: All other options. */
  /* Pass 2: Scenario parsing. */
  int pass;
};

#define SIPP_OPTION_HELP          1
#define SIPP_OPTION_INT           2
#define SIPP_OPTION_SETFLAG       3
#define SIPP_OPTION_UNSETFLAG     4
#define SIPP_OPTION_STRING        5
#define SIPP_OPTION_ARGI          6
#define SIPP_OPTION_TIME_SEC      7
#define SIPP_OPTION_FLOAT         8
#define SIPP_OPTION_BOOL          10
#define SIPP_OPTION_VERSION       11
#define SIPP_OPTION_TRANSPORT     12
#define SIPP_OPTION_NEED_SSL      13
#define SIPP_OPTION_IP            14
#define SIPP_OPTION_MAX_SOCKET    15
#define SIPP_OPTION_CSEQ          16
#define SIPP_OPTION_SCENARIO      17
#define SIPP_OPTION_RSA           18
#define SIPP_OPTION_LIMIT         19
#define SIPP_OPTION_USERS         20
#define SIPP_OPTION_KEY           21
#define SIPP_OPTION_3PCC          22
#define SIPP_OPTION_TDMMAP        23
#define SIPP_OPTION_TIME_MS       24
#define SIPP_OPTION_SLAVE_CFG     25
#define SIPP_OPTION_3PCC_EXTENDED 26
#define SIPP_OPTION_INPUT_FILE    27
#define SIPP_OPTION_TIME_MS_LONG  28
#define SIPP_OPTION_LONG          29
#define SIPP_OPTION_LONG_LONG     30
#define SIPP_OPTION_DEFAULTS      31
#define SIPP_OPTION_OOC_SCENARIO  32
#define SIPP_OPTION_INDEX_FILE    33
#define SIPP_OPTION_VAR           34
#define SIPP_OPTION_RTCHECK       35
#define SIPP_OPTION_LFNAME        36
#define SIPP_OPTION_LFOVERWRITE   37
#define SIPP_OPTION_PLUGIN        38
#define SIPP_OPTION_NO_CALL_ID_CHECK 39
#define SIPP_OPTION_IP2            40

#define MAX_CMD_CHARS 256

/* Put Each option, its help text, and type in this table. */
struct sipp_option options_table[] = {

  {"aa", "Enable automatic 200 OK answer for INFO, UPDATE, REGISTER and NOTIFY messages.", SIPP_OPTION_SETFLAG, &auto_answer, 1},
  {"aa_expires", "Expires value used in auto responses that require one (currently only REGISTER response). Default unit is seconds. Default value is 3600.", SIPP_OPTION_INT, &auto_answer_expires, 1},

#ifdef _USE_OPENSSL
  {
    "auth_uri", "Force the value of the URI for authentication.\n"
    "By default, the URI is composed of remote_ip:remote_port.", SIPP_OPTION_STRING, &auth_uri, 1
  },
#else
  {"auth_uri", NULL, SIPP_OPTION_NEED_SSL, NULL, 1},
#endif

  {"base_cseq", "Start value of [cseq] for each call.", SIPP_OPTION_CSEQ, NULL, 1},
  {"bg", "Launch SIPp in background mode.", SIPP_OPTION_SETFLAG, &backgroundMode, 1},
  {"bind_local", "Bind socket to local IP address, i.e. the local IP address is used as the source IP address.  If SIPp runs in server mode  it will only listen on the local IP address instead of all IP addresses.", SIPP_OPTION_SETFLAG, &bind_local, 1},
  {"buff_size", "Set the send and receive buffer size.", SIPP_OPTION_INT, &buff_size, 1},

  {"calldebug_file", "Set the name of the call debug file.", SIPP_OPTION_LFNAME, &calldebug_lfi, 1},
  {"calldebug_overwrite", "Overwrite the call debug file (default true).", SIPP_OPTION_LFOVERWRITE, &calldebug_lfi, 1},

  {"cid_str", "Call ID string (default %u-%p@%s).  %u=call_number, %s=ip_address, %p=process_number, %%=% (in any order).", SIPP_OPTION_STRING, &call_id_string, 1},
  {"ci", "Set the local control IP address", SIPP_OPTION_IP, control_ip, 1},
  {"cp", "Set the local control port number. Default is 8888.", SIPP_OPTION_INT, &control_port, 1},

  {"d", "Controls the length of calls. More precisely, this controls the duration of 'pause' instructions in the scenario, if they do not have a 'milliseconds' section. Default value is 0 and default unit is milliseconds.", SIPP_OPTION_TIME_MS, &duration, 1},
  {"deadcall_wait", "How long the Call-ID and final status of calls should be kept to improve message and error logs (default unit is ms). Default is 33 seconds, unless -mc is specified in which case it is 0.", SIPP_OPTION_TIME_MS, &deadcall_wait, 1},
  {
    "default_behaviors", "Set the default behaviors that SIPp will use.  Possbile values are:\n"
    "- all\tUse all default behaviors\n"
    "- none\tUse no default behaviors\n"
    "- bye\tSend byes for aborted calls\n"
    "- abortunexp\tAbort calls on unexpected messages\n"
    "- pingreply\tReply to ping requests\n"
    "If a behavior is prefaced with a -, then it is turned off.  Example: all,-bye\n",
    SIPP_OPTION_DEFAULTS, &default_behaviors, 1
  },
  {"dump_xml", "Dump expanded XML to screen. Useful for debugging includes", SIPP_OPTION_SETFLAG, &dump_xml, 1},
  {"dump_sequence_diagram", "Dump sequence diagram.", SIPP_OPTION_SETFLAG, &dump_sequence_diagram, 1},
  {"dsd", "Dump sequence diagram.", SIPP_OPTION_SETFLAG, &dump_sequence_diagram, 1},
  {"debug_file", "Set the name of the call debug file.", SIPP_OPTION_LFNAME, &debug_lfi, 1},
  {"debug_overwrite", "Overwrite the call debug file (default true).", SIPP_OPTION_LFOVERWRITE, &debug_lfi, 1},

  {"error_file", "Set the name of the error log file.", SIPP_OPTION_LFNAME, &error_lfi, 1},
  {"error_overwrite", "Overwrite the error log file (default true).", SIPP_OPTION_LFOVERWRITE, &error_lfi, 1},
  {"exec_file", "Set the name of the exec log file.", SIPP_OPTION_LFNAME, &exec_lfi, 1},
  {"exec_overwrite", "Overwrite the call debug file (default true).", SIPP_OPTION_LFOVERWRITE, &exec_lfi, 1},

  {"f", "Set the statistics report frequency on screen. Default is 1 and default unit is seconds.", SIPP_OPTION_TIME_SEC, &report_freq, 1},
  {"fd", "Set the statistics dump log report frequency. Default is 60 and default unit is seconds.", SIPP_OPTION_TIME_SEC, &report_freq_dumpLog, 1},
  {"force_server_mode", "Set creation/send mode to server regardless of the contents of the scenario. Defaults to false, unless using mc and TLS transport is enabled.", SIPP_OPTION_SETFLAG, &force_server_mode, 1},
  {"force_client_mode", "Set creation/send mode to client regardless of the contents of the scenario. Defaults to false.", SIPP_OPTION_SETFLAG, &force_client_mode, 1},

  {"h", NULL, SIPP_OPTION_HELP, NULL, 0},
  {"help", NULL, SIPP_OPTION_HELP, NULL, 0},

  {"i", "Set the local IP address for 'Contact:','Via:', and 'From:' headers. Default is primary host IP address.\n", SIPP_OPTION_IP, local_ip, 1},
  {"i2", "Set the secondary local IP address \n", SIPP_OPTION_IP, local_ip2, 1},
  {
    "inf", "Inject values from an external CSV file during calls into the scenarios.\n"
    "First line of this file say whether the data is to be read in sequence (SEQUENTIAL), random (RANDOM), or user (USER) order.\n"
    "Each line corresponds to one call and has one or more ';' delimited data fields. Those fields can be referred as [field0], [field1], ... in the xml scenario file.  Several CSV files can be used simultaneously (syntax: -inf f1.csv -inf f2.csv ...)", SIPP_OPTION_INPUT_FILE, NULL, 1
  },
  {"infindex", "file field\nCreate an index of file using field.  For example -inf users.csv -infindex users.csv 0 creates an index on the first key.", SIPP_OPTION_INDEX_FILE, NULL, 1 },

  {
    "ip_field", "Set which field from the injection file contains the IP address from which the client will send its messages.\n"
    "If this option is omitted and the '-t ui' option is present, then field 0 is assumed.\n"
    "Use this option together with '-t ui'", SIPP_OPTION_INT, &peripfield, 1
  },


  {
    "l", "Set the maximum number of simultaneous calls. Once this limit is reached, traffic is decreased until the number of open calls goes down. Default:\n"
    "  (3 * call_duration (s) * rate).", SIPP_OPTION_LIMIT, NULL, 1
  },

  {"log_file", "Set the name of the log actions log file.", SIPP_OPTION_LFNAME, &log_lfi, 1},
  {"log_overwrite", "Overwrite the log actions log file (default true).", SIPP_OPTION_LFOVERWRITE, &log_lfi, 1},

  {"lost", "Set the number of packets to lose by default (scenario specifications override this value).", SIPP_OPTION_FLOAT, &global_lost, 1},
  {"rtcheck", "Select the retransmisison detection method: full (default) or loose.", SIPP_OPTION_RTCHECK, &rtcheck, 1},
  {"m", "Stop the test and exit when 'calls' calls are processed", SIPP_OPTION_LONG, &stop_after, 1},
  {"mi", "Set the local media IP address (default: local primary host IP address)", SIPP_OPTION_IP, media_ip, 1},
  {"master","3pcc extended mode: indicates the master number", SIPP_OPTION_3PCC_EXTENDED, &master_name, 1},
  {"max_recv_loops", "Set the maximum number of messages received read per cycle. Increase this value for high traffic level.  The default value is 1000.", SIPP_OPTION_INT, &max_recv_loops, 1},
  {"max_sched_loops", "Set the maximum number of calsl run per event loop. Increase this value for high traffic level.  The default value is 1000.", SIPP_OPTION_INT, &max_sched_loops, 1},
  {"max_reconnect", "Set the the maximum number of reconnection.", SIPP_OPTION_INT, &reset_number, 1},
  {"max_retrans", "Maximum number of UDP retransmissions before call ends on timeout.  Default is 5 for INVITE transactions and 7 for others.", SIPP_OPTION_INT, &max_udp_retrans, 1},
  {"max_invite_retrans", "Maximum number of UDP retransmissions for invite transactions before call ends on timeout.", SIPP_OPTION_INT, &max_invite_retrans, 1},
  {"max_non_invite_retrans", "Maximum number of UDP retransmissions for non-invite transactions before call ends on timeout.", SIPP_OPTION_INT, &max_non_invite_retrans, 1},
  {"max_log_size", "What is the limit for error and message log file sizes.", SIPP_OPTION_LONG_LONG, &max_log_size, 1},
  {"max_socket", "Set the max number of sockets to open simultaneously. This option is significant if you use one socket per call. Once this limit is reached, traffic is distributed over the sockets already opened. Default value is 50000", SIPP_OPTION_MAX_SOCKET, NULL, 1},

  {"mb", "Set the RTP echo buffer size (default: 2048).", SIPP_OPTION_INT, &media_bufsize, 1},
  {"message_file", "Set the name of the message log file.", SIPP_OPTION_LFNAME, &message_lfi, 1},
  {"message_overwrite", "Overwrite the message log file (default true).", SIPP_OPTION_LFOVERWRITE, &message_lfi, 1},
  {"mp", "Set the local RTP echo port number. Default is 6000.", SIPP_OPTION_INT, &user_media_port, 1},

  {
    "nd", "No Default. Disable all default behavior of SIPp which are the following:\n"
    "- On UDP retransmission timeout, abort the call by sending a BYE or a CANCEL\n"
    "- On receive timeout with no ontimeout attribute, abort the call by sending a BYE or a CANCEL\n"
    "- On unexpected BYE send a 200 OK and close the call\n"
    "- On unexpected CANCEL send a 200 OK and close the call\n"
    "- On unexpected PING send a 200 OK and continue the call\n"
    "- On any other unexpected message, abort the call by sending a BYE or a CANCEL\n",
    SIPP_OPTION_UNSETFLAG, &default_behaviors, 1
  },
  {
    "mc", "Enable multiple-dialog support by directing all messages to one scenario regardless of call-id.\n"
    "Only 1 concurrent call is possible, stop calls (-m) defaults to 1.",
    SIPP_OPTION_NO_CALL_ID_CHECK, NULL, 1
  },
  {"nr", "Disable retransmission in UDP mode. Retransmissions are enabled by default unless -mc options is used.", SIPP_OPTION_UNSETFLAG, &retrans_enabled, 1},
  {"yr", "Enable retransmission in UDP mode. Retransmissions are enabled by default unless -mc options is used.", SIPP_OPTION_SETFLAG, &retrans_enabled, 1},
  {"ar", "Absorb retransmission in UDP mode, no corresponding outgoing retransmission are triggered. absorb retransmissions are disabled by default unless -ar options is used.", SIPP_OPTION_SETFLAG, &absorb_retrans, 1},

  {"nostdin", "Disable stdin.\n", SIPP_OPTION_SETFLAG, &nostdin, 1},

  {"p", "Set the local port number.  Default is a random free port chosen by the system.", SIPP_OPTION_INT, &user_port, 1},
  {"pause_msg_ign", "Ignore the messages received during a pause defined in the scenario ", SIPP_OPTION_SETFLAG, &pause_msg_ign, 1},
  {"periodic_rtd", "Reset response time partition counters each logging interval.", SIPP_OPTION_SETFLAG, &periodic_rtd, 1},
  {"plugin", "Load a plugin.", SIPP_OPTION_PLUGIN, NULL, 1},

  {
    "r", "Set the call rate (in calls per seconds).  This value can be"
    "changed during test by pressing '+','_','*' or '/'. Default is 10.\n"
    "pressing '+' key to increase call rate by 1 * rate_scale,\n"
    "pressing '-' key to decrease call rate by 1 * rate_scale,\n"
    "pressing '*' key to increase call rate by 10 * rate_scale,\n"
    "pressing '/' key to decrease call rate by 10 * rate_scale.\n"
    "If the -rp option is used, the call rate is calculated with the period in ms given by the user.", SIPP_OPTION_FLOAT, &rate, 1
  },
  {
    "rp", "Specify the rate period for the call rate.  Default is 1 second and default unit is milliseconds.  This allows you to have n calls every m milliseconds (by using -r n -rp m).\n"
    "Example: -r 7 -rp 2000 ==> 7 calls every 2 seconds.\n         -r 10 -rp 5s => 10 calls every 5 seconds.", SIPP_OPTION_TIME_MS, &rate_period_ms, 1
  },
  {"rate_scale", "Control the units for the '+', '-', '*', and '/' keys.", SIPP_OPTION_FLOAT, &rate_scale, 1},
  {
    "rate_increase", "Specify the rate increase every -fd units (default is seconds).  This allows you to increase the load for each independent logging period.\n"
    "Example: -rate_increase 10 -fd 10s\n"
    "  ==> increase calls by 10 every 10 seconds.", SIPP_OPTION_INT, &rate_increase, 1
  },
  {
    "rate_max", "If -rate_increase is set, then quit after the rate reaches this value.\n"
    "Example: -rate_increase 10 -rate_max 100\n"
    "  ==> increase calls by 10 until 100 cps is hit.", SIPP_OPTION_INT, &rate_max, 1
  },
  {"no_rate_quit", "If -rate_increase is set, do not quit after the rate reaches -rate_max.", SIPP_OPTION_UNSETFLAG, &rate_quit, 1},
  {"recv_timeout", "Global receive timeout. Default unit is milliseconds. If the expected message is not received, the call times out and is aborted.", SIPP_OPTION_TIME_MS_LONG, &defl_recv_timeout, 1},
  {"send_timeout", "Global send timeout. Default unit is milliseconds. If a message is not sent (due to congestion), the call times out and is aborted.", SIPP_OPTION_TIME_MS_LONG, &defl_send_timeout, 1},
  {"sleep", "How long to sleep for at startup. Default unit is seconds.", SIPP_OPTION_TIME_SEC, &sleeptime, 1},
  {"reconnect_close", "Should calls be closed on reconnect?", SIPP_OPTION_BOOL, &reset_close, 1},
  {"reconnect_sleep", "How long (in milliseconds) to sleep between the close and reconnect?", SIPP_OPTION_TIME_MS, &reset_sleep, 1},
  {"ringbuffer_files", "How many error/message files should be kept after rotation?", SIPP_OPTION_INT, &ringbuffer_files, 1},
  {"ringbuffer_size", "How large should error/message files be before they get rotated?", SIPP_OPTION_LONG_LONG, &ringbuffer_size, 1},
  {"rsa", "Set the remote sending address to host:port for sending the messages.", SIPP_OPTION_RSA, NULL, 1},
  {
    "rtp_echo", "Enable RTP echo. RTP/UDP packets received on port defined by -mp are echoed to their sender.\n"
    "RTP/UDP packets coming on this port + 2 are also echoed to their sender (used for sound and video echo).",
    SIPP_OPTION_SETFLAG, &rtp_echo_enabled, 1
  },
  {
    "rtt_freq", "freq is mandatory. Dump response times every freq calls in the log file defined by -trace_rtt. Default value is 200.",
    SIPP_OPTION_LONG, &report_freq_dumpRtt, 1
  },
  {"s", "Set the username part of the resquest URI. Default is 'service'.", SIPP_OPTION_STRING, &service, 1},
  {"sd", "Dumps a default scenario (embeded in the sipp executable)", SIPP_OPTION_SCENARIO, NULL, 0},
  {"sf", "Loads an alternate xml scenario file.  To learn more about XML scenario syntax, use the -sd option to dump embedded scenarios. They contain all the necessary help.", SIPP_OPTION_SCENARIO, NULL, 2},
  {"shortmessage_file", "Set the name of the short message log file.", SIPP_OPTION_LFNAME, &shortmessage_lfi, 1},
  {"shortmessage_overwrite", "Overwrite the short message log file (default true).", SIPP_OPTION_LFOVERWRITE, &shortmessage_lfi, 1},
  {"oocsf", "Load out-of-call scenario.", SIPP_OPTION_OOC_SCENARIO, NULL, 2},
  {"oocsn", "Load out-of-call scenario.", SIPP_OPTION_OOC_SCENARIO, NULL, 2},
  {"skip_rlimit", "Do not perform rlimit tuning of file descriptor limits.  Default: false.", SIPP_OPTION_SETFLAG, &skip_rlimit, 1},
  {"slave", "3pcc extended mode: indicates the slave number", SIPP_OPTION_3PCC_EXTENDED, &slave_number, 1},
  {"slave_cfg", "3pcc extended mode: indicates the file where the master and slave addresses are stored", SIPP_OPTION_SLAVE_CFG, NULL, 1},
  {
    "sn", "Use a default scenario (embedded in the sipp executable). If this option is omitted, the Standard SipStone UAC scenario is loaded.\n"
    "Available values in this version:\n\n"
    "- 'uac'      : Standard SipStone UAC (default).\n"
    "- 'uas'      : Simple UAS responder.\n"
    "- 'regexp'   : Standard SipStone UAC - with regexp and variables.\n"
    "- 'branchc'  : Branching and conditional branching in scenarios - client.\n"
    "- 'branchs'  : Branching and conditional branching in scenarios - server.\n\n"
    "Default 3pcc scenarios (see -3pcc option):\n\n"
    "- '3pcc-C-A' : Controller A side (must be started after all other 3pcc scenarios)\n"
    "- '3pcc-C-B' : Controller B side.\n"
    "- '3pcc-A'   : A side.\n"
    "- '3pcc-B'   : B side.\n", SIPP_OPTION_SCENARIO, NULL, 2
  },

  {"stat_delimiter", "Set the delimiter for the statistics file", SIPP_OPTION_STRING, &stat_delimiter, 1},
  {"stf", "Set the file name to use to dump statistics", SIPP_OPTION_ARGI, &argiFileName, 1},

  {
    "t", "Set the transport mode:\n"
    "- u1: UDP with one socket (default),\n"
    "- un: UDP with one socket per call,\n"
    "- ui: UDP with one socket per IP address The IP addresses must be defined in the injection file.\n"
    "- t1: TCP with one socket,\n"
    "- tn: TCP with one socket per call,\n"
    "- l1: TLS with one socket,\n"
    "- ln: TLS with one socket per call,\n"
    "- c1: u1 + compression (only if compression plugin loaded),DISABLED\n"
    "- cn: un + compression (only if compression plugin loaded).  This plugin is not provided with sipp.DISABLED\n"
    , SIPP_OPTION_TRANSPORT, NULL, 1
  },

  {"timeout", "Global timeout. Default unit is seconds.  If this option is set, SIPp quits after nb units (-timeout 20s quits after 20 seconds).", SIPP_OPTION_TIME_SEC, &global_timeout, 1},
  {"timeout_error", "SIPp fails if the global timeout is reached is set (-timeout option required).", SIPP_OPTION_SETFLAG, &timeout_error, 1},
  {
    "timer_resol", "Set the timer resolution. Default unit is milliseconds.  This option has an impact on timers precision."
    "Small values allow more precise scheduling but impacts CPU usage."
    "If the compression is on, the value is set to 50ms. The default value is 10ms.", SIPP_OPTION_TIME_MS, &timer_resolution, 1
  },

  {"T2", "Global T2-timer in milli seconds", SIPP_OPTION_TIME_MS, &global_t2, 1},



  {"sendbuffer_warn", "Produce warnings instead of errors on SendBuffer failures.", SIPP_OPTION_BOOL, &sendbuffer_warn, 1},

  {"trace_msg", "Displays sent and received SIP messages in <scenario file name>_<pid>_messages.log", SIPP_OPTION_SETFLAG, &useMessagef, 1},
  {"trace_shortmsg", "Displays sent and received SIP messages as CSV in <scenario file name>_<pid>_shortmessages.log", SIPP_OPTION_SETFLAG, &useShortMessagef, 1},
  {"trace_screen", "Dump statistic screens in the <scenario_name>_<pid>_screens.log file when quitting SIPp. Useful to get a final status report in background mode (-bg option).", SIPP_OPTION_SETFLAG, &useScreenf, 1},
  {"trace_err", "Trace all unexpected messages in <scenario file name>_<pid>_errors.log.", SIPP_OPTION_SETFLAG, &print_all_responses, 1},
//  {"trace_timeout", "Displays call ids for calls with timeouts in <scenario file name>_<pid>_timeout.log", SIPP_OPTION_SETFLAG, &useTimeoutf, 1},
  {"trace_debug", "Dumps debugging information about SIPp execution and ALL other messages to <scenario_name>_<pid>_DEBUG.log file.", SIPP_OPTION_SETFLAG, &useDebugf, 1},
  {"trace_exec", "Redirects output from <exec> commands to <scenario_name>_<pid>_EXEC.log file.", SIPP_OPTION_SETFLAG, &useExecf, 1},
  {"trace_calldebug", "Dumps debugging information about aborted calls to <scenario_name>_<pid>_calldebug.log file.", SIPP_OPTION_SETFLAG, &useCallDebugf, 1},
  {"trace_stat", "Dumps all statistics in <scenario_name>_<pid>.csv file. Use the '-h stat' option for a detailed description of the statistics file content.", SIPP_OPTION_SETFLAG, &dumpInFile, 1},
  {"trace_counts", "Dumps individual message counts in a CSV file.", SIPP_OPTION_SETFLAG, &useCountf, 1},
  {"trace_rtt", "Allow tracing of all response times in <scenario file name>_<pid>_rtt.csv.", SIPP_OPTION_SETFLAG, &dumpInRtt, 1},
  {"trace_logs", "Allow tracing of <log> actions in <scenario file name>_<pid>_logs.log.", SIPP_OPTION_SETFLAG, &useLogf, 1},

  {"users", "Instead of starting calls at a fixed rate, begin 'users' calls at startup, and keep the number of calls constant.", SIPP_OPTION_USERS, NULL, 1},

  {"v", "Display version and copyright information.", SIPP_OPTION_VERSION, NULL, 0},

  {"watchdog_interval", "Set gap between watchdog timer firings.  Default is 400, unless -mc option is used (in which case watchdogs are disabled by default).", SIPP_OPTION_TIME_MS, &watchdog_interval, 1},
  {"watchdog_reset", "If the watchdog timer has not fired in more than this time period, then reset the max triggers counters.  Default is 10 minutes.", SIPP_OPTION_TIME_MS, &watchdog_reset, 1},
  {"watchdog_minor_threshold", "If it has been longer than this period between watchdog executions count a minor trip.  Default is 500.", SIPP_OPTION_TIME_MS, &watchdog_minor_threshold, 1},
  {"watchdog_major_threshold", "If it has been longer than this period between watchdog executions count a major trip.  Default is 3000.", SIPP_OPTION_TIME_MS, &watchdog_major_threshold, 1},
  {"watchdog_major_maxtriggers", "How many times the major watchdog timer can be tripped before the test is terminated.  Default is 10.", SIPP_OPTION_INT, &watchdog_major_maxtriggers, 1},
  {"watchdog_minor_maxtriggers", "How many times the minor watchdog timer can be tripped before the test is terminated.  Default is 120.", SIPP_OPTION_INT, &watchdog_minor_maxtriggers, 1},

#ifdef _USE_OPENSSL
  {"ap", "Set the password for authentication challenges. Default is 'password", SIPP_OPTION_STRING, &auth_password, 1},
  {"tls_cert", "Set the name for TLS Certificate file. Default is 'cacert.pem", SIPP_OPTION_STRING, &tls_cert_name, 1},
  {"tls_key", "Set the name for TLS Private Key file. Default is 'cakey.pem'", SIPP_OPTION_STRING, &tls_key_name, 1},
  {"tls_crl", "Set the name for Certificate Revocation List file. If not specified, X509 CRL is not activated.", SIPP_OPTION_STRING, &tls_crl_name, 1},
#else
  {"ap", NULL, SIPP_OPTION_NEED_SSL, NULL, 1},
  {"tls_cert", NULL, SIPP_OPTION_NEED_SSL, NULL, 1},
  {"tls_key", NULL, SIPP_OPTION_NEED_SSL, NULL, 1},
  {"tls_crl", NULL, SIPP_OPTION_NEED_SSL, NULL, 1},
#endif
  {
    "3pcc", "Launch the tool in 3pcc mode (\"Third Party call control\"). The passed ip address is depending on the 3PCC role.\n"
    "- When the first twin command is 'sendCmd' then this is the address of the remote twin socket.  SIPp will try to connect to this address:port to send the twin command (This instance must be started after all other 3PCC scenarii).\n"
    "    Example: 3PCC-C-A scenario.\n"
    "- When the first twin command is 'recvCmd' then this is the address of the local twin socket. SIPp will open this address:port to listen for twin command.\n"
    "    Example: 3PCC-C-B scenario.", SIPP_OPTION_3PCC, NULL, 1
  },
  {
    "tdmmap", "Generate and handle a table of TDM circuits.\n"
    "A circuit must be available for the call to be placed.\n"
    "Format: -tdmmap {0-3}{99}{5-8}{1-31}", SIPP_OPTION_TDMMAP, NULL, 1
  },
  {"key", "keyword value\nSet the generic parameter named \"keyword\" to \"value\".", SIPP_OPTION_KEY, NULL, 1},
  {"set", "variable value\nSet the global variable parameter named \"variable\" to \"value\".", SIPP_OPTION_VAR, NULL, 3},
  {"dynamicStart", "variable value\nSet the start offset of dynamic_id varaiable",  SIPP_OPTION_INT, &startDynamicId, 1},
  {"dynamicMax",   "variable value\nSet the maximum of dynamic_id variable     ",   SIPP_OPTION_INT, &maxDynamicId,   1},
  {"dynamicStep",  "variable value\nSet the increment of dynamic_id variable",      SIPP_OPTION_INT, &stepDynamicId,  1}
};



struct sipp_option *find_option(const char *option) {
  int i;
  int max = sizeof(options_table)/sizeof(options_table[0]);

  /* Allow options to start with '-' or '--' */
  if (option[0] != '-') {
    return NULL;
  }
  option++;
  if (option[0] == '-') {
    option++;
  }

  for (i = 0; i < max; i++) {
    if (!strcmp(options_table[i].option, option)) {
      return &(options_table[i]);
    }
  }

  return NULL;
};

/***************** System Portability Features *****************/

bool file_exists(const string &filename)
{
  if (FILE * file = fopen(filename.c_str(), "r")) {
    fclose(file);
    return true;
  }
  return false;
}


string prepend_environment_if_needed(const string &name, const string &message=string(""))
{
  if (file_exists(name)) {
    return name;
  } else if (getenv("SIPPED")) {
    string fullname = string(getenv("SIPPED")) + string("/") + name;
    if (file_exists(fullname)) {
      return fullname;
    }
  }
  REPORT_ERROR("Error opening %s file '%s': File not found in current directory or in directory specified by SIPPED environment variable", message.c_str(), name.c_str());
  return ""; // Never executes
}


#ifdef _USE_OPENSSL
///****** SSL error handling                         *************/
// moved to sipp_sockethandler to satisfy write_error flush_socket 

/****** Certificate Verification Callback FACILITY *************/
int sip_tls_verify_callback(int ok , X509_STORE_CTX *store)
{
  char data[512];

  if (!ok) {
    X509 *cert = X509_STORE_CTX_get_current_cert(store);

    X509_NAME_oneline(X509_get_issuer_name(cert),
                      data,512);
    WARNING("TLS verification error for issuer: '%s'", data);
    X509_NAME_oneline(X509_get_subject_name(cert),
                      data,512);
    WARNING("TLS verification error for subject: '%s'", data);
  }
  return ok;
}

/***********  Load the CRL's into SSL_CTX **********************/
int sip_tls_load_crls( SSL_CTX *ctx , char *crlfile)
{
  X509_STORE          *store;
  X509_LOOKUP         *lookup;

  /*  Get the X509_STORE from SSL context */
  if (!(store = SSL_CTX_get_cert_store(ctx))) {
    return (-1);
  }

  /* Add lookup file to X509_STORE */
  if (!(lookup = X509_STORE_add_lookup(store,X509_LOOKUP_file()))) {
    return (-1);
  }

  /* Add the CRLS to the lookpup object */
  if (X509_load_crl_file(lookup, prepend_environment_if_needed(crlfile, "CRL").c_str(), X509_FILETYPE_PEM) != 1) {
    return (-1);
  }

  /* Set the flags of the store so that CRLS's are consulted */
#if OPENSSL_VERSION_NUMBER >= 0x00907000L
  X509_STORE_set_flags( store,X509_V_FLAG_CRL_CHECK | X509_V_FLAG_CRL_CHECK_ALL);
#else
#warning This version of OpenSSL (<0.9.7) cannot handle CRL files in capath
  REPORT_ERROR("This version of OpenSSL (<0.9.7) cannot handle CRL files in capath");
#endif

  return (1);
}

/************* Prepare the SSL context ************************/
static ssl_init_status FI_init_ssl_context (void)
{
  sip_trp_ssl_ctx = SSL_CTX_new( TLSv1_method() );
  if ( sip_trp_ssl_ctx == NULL ) {
    REPORT_ERROR("FI_init_ssl_context: SSL_CTX_new with TLSv1_method failed");
    return SSL_INIT_ERROR;
  }

  sip_trp_ssl_ctx_client = SSL_CTX_new( TLSv1_method() );
  if ( sip_trp_ssl_ctx_client == NULL) {
    REPORT_ERROR("FI_init_ssl_context: SSL_CTX_new with TLSv1_method failed");
    return SSL_INIT_ERROR;
  }

  /*  Load the trusted CA's */
  SSL_CTX_load_verify_locations(sip_trp_ssl_ctx, tls_cert_name, NULL);
  SSL_CTX_load_verify_locations(sip_trp_ssl_ctx_client, tls_cert_name, NULL);

  /*  CRL load from application specified only if specified on the command line */
  if (strlen(tls_crl_name) != 0) {
    if(sip_tls_load_crls(sip_trp_ssl_ctx,tls_crl_name) == -1) {
      REPORT_ERROR("FI_init_ssl_context: Unable to load CRL file (%s)", tls_crl_name);
      return SSL_INIT_ERROR;
    }

    if(sip_tls_load_crls(sip_trp_ssl_ctx_client,tls_crl_name) == -1) {
      REPORT_ERROR("FI_init_ssl_context: Unable to load CRL (client) file (%s)", tls_crl_name);
      return SSL_INIT_ERROR;
    }
    /* The following call forces to process the certificates with the */
    /* initialised SSL_CTX                                            */
    SSL_CTX_set_verify(sip_trp_ssl_ctx,
                       SSL_VERIFY_PEER |
                       SSL_VERIFY_FAIL_IF_NO_PEER_CERT,
                       sip_tls_verify_callback);

    SSL_CTX_set_verify(sip_trp_ssl_ctx_client,
                       SSL_VERIFY_PEER |
                       SSL_VERIFY_FAIL_IF_NO_PEER_CERT,
                       sip_tls_verify_callback);
  }

  /* Selection Cipher suits - load the application specified ciphers */
  SSL_CTX_set_default_passwd_cb_userdata(sip_trp_ssl_ctx,
                                         (void *)CALL_BACK_USER_DATA );
  SSL_CTX_set_default_passwd_cb_userdata(sip_trp_ssl_ctx_client,
                                         (void *)CALL_BACK_USER_DATA );
  SSL_CTX_set_default_passwd_cb( sip_trp_ssl_ctx,
                                 passwd_call_back_routine );
  SSL_CTX_set_default_passwd_cb( sip_trp_ssl_ctx_client,
                                 passwd_call_back_routine );

  if ( SSL_CTX_use_certificate_file(sip_trp_ssl_ctx, prepend_environment_if_needed(tls_cert_name, "TLS Cert").c_str(), SSL_FILETYPE_PEM ) != 1) {
    REPORT_ERROR("FI_init_ssl_context: SSL_CTX_use_certificate_file failed");
    return SSL_INIT_ERROR;
  }

  if ( SSL_CTX_use_certificate_file(sip_trp_ssl_ctx_client, prepend_environment_if_needed(tls_cert_name, "TLS Cert").c_str(), SSL_FILETYPE_PEM ) != 1) {
    REPORT_ERROR("FI_init_ssl_context: SSL_CTX_use_certificate_file (client) failed");
    return SSL_INIT_ERROR;
  }
  if ( SSL_CTX_use_PrivateKey_file(sip_trp_ssl_ctx, prepend_environment_if_needed(tls_key_name, "TLS key").c_str(), SSL_FILETYPE_PEM ) != 1) {
    REPORT_ERROR("FI_init_ssl_context: SSL_CTX_use_PrivateKey_file failed");
    return SSL_INIT_ERROR;
  }

  if ( SSL_CTX_use_PrivateKey_file(sip_trp_ssl_ctx_client, prepend_environment_if_needed(tls_key_name, "TLS key").c_str(), SSL_FILETYPE_PEM ) != 1) {
    REPORT_ERROR("FI_init_ssl_context: SSL_CTX_use_PrivateKey_file (client) failed");
    return SSL_INIT_ERROR;
  }

  return SSL_INIT_NORMAL;
}


#endif





/***************** Check of the message received ***************/

bool sipMsgCheck (const char *P_msg, int P_msgSize, struct sipp_socket *socket)
{
  const char C_sipHeader[] = "SIP/2.0" ;

  if (socket == twinSippSocket || socket == localTwinSippSocket ||
      is_a_peer_socket(socket) || is_a_local_socket(socket))
    return true;

  if (strstr(P_msg, C_sipHeader) !=  NULL) {
    return true ;
  }

  return false ;
}

/************** Statistics display & User control *************/
// number of display functions moved to sipp_sockethandler.cpp
/* Function to dump all available screens in a file */
void print_screens(void)
{
  int oldScreen = currentScreenToDisplay;
  int oldRepartition = currentRepartitionToDisplay;

  currentScreenToDisplay = DISPLAY_SCENARIO_SCREEN;
  print_header_line(   screenf, 0);
  if (print_stats_in_file( screenf, 0) == SCENARIO_NOT_IMPLEMENTED)
    REPORT_ERROR("Scenario command not implemented in display\n");
  if (print_bottom_line(   screenf, 0)==INTERNAL_ERROR)
    REPORT_ERROR("Internal error: creationMode=%d, thirdPartyMode=%d", creationMode, thirdPartyMode);

  currentScreenToDisplay = DISPLAY_STAT_SCREEN;
  print_header_line(   screenf, 0);
  display_scenario->stats->displayStat(screenf);
  if (print_bottom_line(   screenf, 0)==INTERNAL_ERROR)
    REPORT_ERROR("Internal error: creationMode=%d, thirdPartyMode=%d", creationMode, thirdPartyMode);

  currentScreenToDisplay = DISPLAY_REPARTITION_SCREEN;
  print_header_line(   screenf, 0);
  display_scenario->stats->displayRepartition(screenf);
  if (print_bottom_line(   screenf, 0)==INTERNAL_ERROR)
    REPORT_ERROR("Internal error: creationMode=%d, thirdPartyMode=%d", creationMode, thirdPartyMode);

  currentScreenToDisplay = DISPLAY_SECONDARY_REPARTITION_SCREEN;
  for (currentRepartitionToDisplay = 2; currentRepartitionToDisplay <= display_scenario->stats->nRtds(); currentRepartitionToDisplay++) {
    print_header_line(   screenf, 0);
    display_scenario->stats->displayRtdRepartition(screenf, currentRepartitionToDisplay);
    if(print_bottom_line(   screenf, 0)==INTERNAL_ERROR)
      REPORT_ERROR("Internal error: creationMode=%d, thirdPartyMode=%d", creationMode, thirdPartyMode);
  }

  currentScreenToDisplay = oldScreen;
  currentRepartitionToDisplay = oldRepartition;
}



void sipp_sigusr1(int /* not used */)
{
  /* Smooth exit: do not place any new calls and exit */
  quitting+=10;
}

void sipp_sigusr2(int /* not used */)
{
  if (!signalDump) {
    signalDump = true ;
  }
}

bool process_key(int c)
{
  switch (c) {
  case '1':
    currentScreenToDisplay = DISPLAY_SCENARIO_SCREEN;
    if (print_statistics(0)==INTERNAL_ERROR)
      REPORT_ERROR("Internal error: creationMode=%d, thirdPartyMode=%d", creationMode, thirdPartyMode);
    break;

  case '2':
    currentScreenToDisplay = DISPLAY_STAT_SCREEN;
    if (print_statistics(0)==INTERNAL_ERROR)
      REPORT_ERROR("Internal error: creationMode=%d, thirdPartyMode=%d", creationMode, thirdPartyMode);
    break;

  case '3':
    currentScreenToDisplay = DISPLAY_REPARTITION_SCREEN;
    if (print_statistics(0)==INTERNAL_ERROR)
      REPORT_ERROR("Internal error: creationMode=%d, thirdPartyMode=%d", creationMode, thirdPartyMode);
    break;

  case '4':
    currentScreenToDisplay = DISPLAY_VARIABLE_SCREEN;
    if (print_statistics(0)==INTERNAL_ERROR)
      REPORT_ERROR("Internal error: creationMode=%d, thirdPartyMode=%d", creationMode, thirdPartyMode);
    break;

  case '5':
    if (use_tdmmap) {
      currentScreenToDisplay = DISPLAY_TDM_MAP_SCREEN;
      if (print_statistics(0)==INTERNAL_ERROR)
        REPORT_ERROR("Internal error: creationMode=%d, thirdPartyMode=%d", creationMode, thirdPartyMode);
    }
    break;

    /* Screens 6, 7, 8, 9  are for the extra RTD repartitions. */
  case '6':
  case '7':
  case '8':
  case '9':
    currentScreenToDisplay = DISPLAY_SECONDARY_REPARTITION_SCREEN;
    currentRepartitionToDisplay = (c - '6') + 2;
    if (print_statistics(0)==INTERNAL_ERROR)
      REPORT_ERROR("Internal error: creationMode=%d, thirdPartyMode=%d", creationMode, thirdPartyMode);
    break;

  case '+':
    if (users >= 0) {
      opentask::set_users((int)(users + 1 * rate_scale));
    } else {
      opentask::set_rate(rate + 1 * rate_scale);
    }
    if (print_statistics(0)==INTERNAL_ERROR)
      REPORT_ERROR("Internal error: creationMode=%d, thirdPartyMode=%d", creationMode, thirdPartyMode);
    break;

  case '-':
    if (users >= 0) {
      opentask::set_users((int)(users - 1 * rate_scale));
    } else {
      opentask::set_rate(rate - 1 * rate_scale);
    }
    if (print_statistics(0)==INTERNAL_ERROR)
      REPORT_ERROR("Internal error: creationMode=%d, thirdPartyMode=%d", creationMode, thirdPartyMode);
    break;

  case '*':
    if (users >= 0) {
      opentask::set_users((int)(users + 10 * rate_scale));
    } else {
      opentask::set_rate(rate + 10 * rate_scale);
    }
    if (print_statistics(0)==INTERNAL_ERROR)
      REPORT_ERROR("Internal error: creationMode=%d, thirdPartyMode=%d", creationMode, thirdPartyMode);
    break;

  case '/':
    if (users >= 0) {
      opentask::set_users((int)(users - 10 * rate_scale));
    } else {
      opentask::set_rate(rate - 10 * rate_scale);
    }
    if (print_statistics(0)==INTERNAL_ERROR)
      REPORT_ERROR("Internal error: creationMode=%d, thirdPartyMode=%d", creationMode, thirdPartyMode);
    break;

  case 'p':
    if(paused) {
      opentask::set_paused(false);
    } else {
      opentask::set_paused(true);
    }
    if (print_statistics(0)==INTERNAL_ERROR)
      REPORT_ERROR("Internal error: creationMode=%d, thirdPartyMode=%d", creationMode, thirdPartyMode);
    break;

  case 's':
    if (screenf) {
      print_screens();
    }
    break;

  case 'q':
    quitting+=10;
    q_pressed = true;
    DEBUG("q pressed. quitting = %d", quitting);
    if (print_statistics(0)==INTERNAL_ERROR)
      REPORT_ERROR("Internal error: creationMode=%d, thirdPartyMode=%d", creationMode, thirdPartyMode);
    break;

  case 'Q':
    /* We are going to break, so we never have a chance to press q twice. */
    quitting+=20;
    q_pressed = true;
    DEBUG("Q pressed. quitting = %d", quitting);
    if (print_statistics(0)==INTERNAL_ERROR)
      REPORT_ERROR("Internal error: creationMode=%d, thirdPartyMode=%d", creationMode, thirdPartyMode);
    break;
  }
  return false;
}

void trim(char *s)
{
  char *p = s;
  while(isspace(*p)) {
    p++;
  }
  int l = strlen(p);
  for (int i = l - 1; i >= 0 && isspace(p[i]); i--) {
    p[i] = '\0';
  }
  memmove(s, p, l + 1);
}

void process_set(char *what)
{
  char *rest = strchr(what, ' ');
  if (rest) {
    *rest++ = '\0';
    trim(rest);
  } else {
    WARNING("The set command requires two arguments (attribute and value)");
    return;
  }

  if (!strcmp(what, "rate")) {
    char *end;
    double drest = strtod(rest, &end);

    if (users >= 0) {
      WARNING("Rates can not be set in a user-based benchmark.");
    } else if (*end) {
      WARNING("Invalid rate value: \"%s\"", rest);
    } else {
      opentask::set_rate(drest);
    }
  } else if (!strcmp(what, "rate-scale")) {
    char *end;
    double drest = strtod(rest, &end);
    if (*end) {
      WARNING("Invalid rate-scale value: \"%s\"", rest);
    } else {
      rate_scale = drest;
    }
  } else if (!strcmp(what, "users")) {
    char *end;
    int urest = strtol(rest, &end, 0);

    if (users < 0) {
      WARNING("Users can not be changed at run time for a rate-based benchmark.");
    } else if (*end) {
      WARNING("Invalid users value: \"%s\"", rest);
    } else if (urest < 0) {
      WARNING("Invalid users value: \"%s\"", rest);
    } else {
      opentask::set_users(urest);
    }
  } else if (!strcmp(what, "limit")) {
    char *end;
    unsigned long lrest = strtoul(rest, &end, 0);
    if (users >= 0) {
      WARNING("Can not set call limit for a user-based benchmark.");
    } else if (*end) {
      WARNING("Invalid limit value: \"%s\"", rest);
    } else {
      open_calls_allowed = lrest;
      open_calls_user_setting = 1;
    }
  } else if (!strcmp(what, "display")) {
    if (!strcmp(rest, "main")) {
      display_scenario = main_scenario;
      display_scenario_stats = main_scenario->stats;
    } else if (!strcmp(rest, "ooc")) {
      display_scenario = ooc_scenario;
      display_scenario_stats = ooc_scenario->stats;
    } else {
      WARNING("Unknown display scenario: %s", rest);
    }
  } else if (!strcmp(what, "hide")) {
    if (!strcmp(rest, "true")) {
      do_hide = true;
    } else if (!strcmp(rest, "false")) {
      do_hide = false;
    } else {
      WARNING("Invalid bool: %s", rest);
    }
  } else if (!strcmp(what, "index")) {
    if (!strcmp(rest, "true")) {
      show_index = true;
    } else if (!strcmp(rest, "false")) {
      show_index = false;
    } else {
      WARNING("Invalid bool: %s", rest);
    }
  } else {
    WARNING("Unknown set attribute: %s", what);
  }
}



void process_trace(char *what)
{
  bool on = false;
  char *rest = strchr(what, ' ');
  if (rest) {
    *rest++ = '\0';
    trim(rest);
  } else {
    WARNING("The trace command requires two arguments (log and [on|off])");
    return;
  }

  if (!strcmp(rest, "on")) {
    on = true;
  } else if (!strcmp(rest, "off")) {
    on = false;
  } else if (!strcmp(rest, "true")) {
    on = true;
  } else if (!strcmp(rest, "false")) {
    on = false;
  } else {
    WARNING("The trace command's second argument must be on or off.");
    return;
  }

  if (!strcmp(what, "error")) {
    if (on == !!print_all_responses) {
      return;
    }
    if (on) {
      print_all_responses = 1;
    } else {
      print_all_responses = 0;
      log_off(&error_lfi);
    }
  } else if (!strcmp(what, "logs")) {
    if (on == !!log_lfi.fptr) {
      return;
    }
    if (on) {
      useLogf = 1;
      rotate_logfile();
    } else {
      useLogf = 0;
      log_off(&log_lfi);
    }
  } else if (!strcmp(what, "messages")) {
    if (on == !!message_lfi.fptr) {
      return;
    }
    if (on) {
      useMessagef = 1;
      rotate_logfile();
    } else {
      useMessagef = 0;
      log_off(&message_lfi);
    }
  } else if (!strcmp(what, "shortmessages")) {
    if (on == !!shortmessage_lfi.fptr) {
      return;
    }

    if (on) {
      useShortMessagef = 1;
      rotate_shortmessagef();
    } else {
      useShortMessagef = 0;
      log_off(&shortmessage_lfi);
    }
  } else {
    WARNING("Unknown log file: %s", what);
  }
}

void process_dump(char *what)
{
  if (!what) {
    WARNING("Must specify dump type of 'tasks' or variables'.");
    return;
  }
  if (!strcmp(what, "tasks")) {
    dump_tasks();
  } else if (!strcmp(what, "variables")) {
    display_scenario->allocVars->dump();
  } else {
    WARNING("Unknown dump type: %s. Must specify dump type of 'tasks' or variables'", what);
  }
}

void process_reset(char *what)
{
  if (!strcmp(what, "stats")) {
    main_scenario->stats->computeStat(CStat::E_RESET_C_COUNTERS);
  } else {
    WARNING("Unknown reset type: %s", what);
  }
}

bool process_command(char *command)
{
  trim(command);

  char *rest = strchr(command, ' ');
  if (rest) {
    *rest++ = '\0';
    trim(rest);
  }

  if (!strcmp(command, "set")) {
    process_set(rest);
  } else if (!strcmp(command, "trace")) {
    process_trace(rest);
  } else if (!strcmp(command, "dump")) {
    process_dump(rest);
  } else if (!strcmp(command, "reset")) {
    process_reset(rest);
  } else {
    WARNING("Unrecognized command: \"%s\"", command);
  }

  return false;
}


int handle_ctrl_socket()
{
  unsigned char bufrcv [SIPP_MAX_MSG_SIZE];

  int ret = recv(ctrl_socket->ss_fd,(char *) &bufrcv,sizeof(bufrcv) - 1,0);
  if (ret <= 0) {
    return ret;
  }

  if (bufrcv[0] == 'c') {
    /* No 'c', but we need one for '\0'. */
    char *command = (char *)malloc(ret);
    if (!command) {
      REPORT_ERROR("Out of memory allocated command buffer.");
    }
    memcpy(command, bufrcv + 1, ret - 1);
    command[ret - 1] = '\0';
    process_command(command);
    free(command);
  } else {
    process_key(bufrcv[0]);
  }
  return 0;
}

void setup_ctrl_socket()
{
  int port, firstport;
  int try_counter = 60;
  struct sockaddr_storage ctl_sa;

  SOCKREF sock = socket(AF_INET,SOCK_DGRAM,0);
  if (sock == -1) {
    REPORT_ERROR_NO("Unable to create remote control socket!");
  }

  if (control_port) {
    port = control_port;
    /* If the user specified the control port, then we must assume they know
     * what they want, and should not cycle. */
    try_counter = 1;
  } else {
    /* Allow 60 control sockets on the same system */
    /* (several SIPp instances)                   */
    port = DEFAULT_CTRL_SOCKET_PORT;
  }
  firstport = port;

  memset(&ctl_sa,0,sizeof(struct sockaddr_storage));
  if (control_ip[0]) {
    struct addrinfo hints;
    struct addrinfo *addrinfo;

    memset((char*)&hints, 0, sizeof(hints));
    hints.ai_flags  = AI_PASSIVE;
    hints.ai_family = PF_UNSPEC;

    if (getaddrinfo(control_ip, NULL, &hints, &addrinfo) != 0) {
      REPORT_ERROR("Unknown control address '%s'.\n"
                   "Use 'sipp -h' for details", control_ip);
    }

    memcpy(&ctl_sa, addrinfo->ai_addr, SOCK_ADDR_SIZE(_RCAST(struct sockaddr_storage *,addrinfo->ai_addr)));
    freeaddrinfo(addrinfo);
  } else {
    ((struct sockaddr_in *)&ctl_sa)->sin_family = AF_INET;
    ((struct sockaddr_in *)&ctl_sa)->sin_addr.s_addr = INADDR_ANY;
  }

  while (try_counter) {
    ((struct sockaddr_in *)&ctl_sa)->sin_port = htons(port);
    if (!bind(sock,(struct sockaddr *)&ctl_sa,sizeof(struct sockaddr_in))) {
      /* Bind successful */
      break;
    }
    try_counter--;
    port++;
  }

  if (try_counter == 0) {

    if (control_port) {
#ifdef WIN32
      ERRORNUMBER = WSAGetLastError();
#endif
      REPORT_ERROR_NO("Unable to bind remote control socket to UDP port %d",
                      control_port);
    } else {
#ifdef WIN32
      ERRORNUMBER = WSAGetLastError();
      wchar_t *error_msg = wsaerrorstr(ERRORNUMBER);
      char errorstring[1000];
      const char *errstring = wchar_to_char(error_msg,errorstring);
      WARNING("Unable to bind remote control socket (tried UDP ports %d-%d): %s",
              firstport, port - 1, errstring);

#else
      WARNING("Unable to bind remote control socket (tried UDP ports %d-%d): %s",
              firstport, port - 1, strerror(ERRORNUMBER));
#endif
    }
    return;
  }

  ctrl_socket = sipp_allocate_socket(0, T_UDP, sock, 0);
  if (!ctrl_socket) {
    REPORT_ERROR_NO("Could not setup control socket!\n");
  }
}

void setup_stdin_socket()
{
// NOTE: On Win32 we handle standard in with a special loop so don't need this.
#ifndef WIN32
  fcntl(fileno(stdin), F_SETFL, fcntl(fileno(stdin), F_GETFL) | O_NONBLOCK);
    stdin_socket = sipp_allocate_socket(0, T_UDP, fileno(stdin), 0);
  if (!stdin_socket) {
    REPORT_ERROR_NO("Could not setup keyboard (stdin) socket!\n");
  }
#endif

}

void handle_stdin_socket()
{
  int c;
  int chars = 0;

  if (feof(stdin)) {
    sipp_close_socket(stdin_socket);
    stdin_socket = NULL;
    return;
  }

  while (((c = screen_readkey()) != -1)) {
    chars++;
    if (command_mode) {
      if (c == '\n') {
        bool quit = process_command(command_buffer);
        if (quit) {
          return;
        }
        command_buffer[0] = '\0';
        command_mode = 0;
        printf(SIPP_ENDL);
      } else if (c == KEY_BACKSPACE_SIPP || c == KEY_DC_SIPP) {
        // this code is hopeless broken, but we don't care.
        int command_len = strlen(command_buffer);
        if (command_len > 0) {
          command_buffer[command_len--] = '\0';
        }
      } else {
        int command_len = strlen(command_buffer);
        char * new_command_buffer = (char *)realloc(command_buffer, command_len + 2);
        if (!new_command_buffer) {
          REPORT_ERROR("Unable to allocate command buffer of size %d", command_len + 2);
        }
        command_buffer = new_command_buffer;
        command_buffer[command_len++] = c;
        command_buffer[command_len] = '\0';
        putchar(c);
        fflush(stdout);
      }
    } else if (c == 'c') {
      command_mode = 1;
      char * new_command_buffer = (char *)realloc(command_buffer, 1);
      if (!new_command_buffer) {
        REPORT_ERROR("Unable to allocate command buffer of size %d", 1);
      }
      command_buffer = new_command_buffer;
      command_buffer[0] = '\0';
      printf("Command: ");
      fflush(stdout);
    } else {
      process_key(c);
    }
  }
  if (chars == 0) {
    /* We did not read any characters, even though we should have. */
    sipp_close_socket(stdin_socket);
    stdin_socket = NULL;
  }
}

//
//
///*************************** Mini SIP parser ***************************/
// some functions moved to sipp_sockethandler


char * get_incoming_header_content(char* message, char * name)
{
  /* non reentrant. consider accepting char buffer as param */
  static char last_header[MAX_HEADER_LEN * 10];
  char * src, *dest, *ptr;

  /* returns empty string in case of error */
  memset(last_header, 0, sizeof(last_header));

  if((!message) || (!strlen(message))) {
    return last_header;
  }

  src = message;
  dest = last_header;

  /* for safety's sake */
  if (NULL == name || NULL == strrchr(name, ':')) {
    return last_header;
  }

  while((src = strstr(src, name))) {

    /* just want the header's content */
    src += strlen(name);

    ptr = strchr(src, '\n');

    /* Multiline headers always begin with a tab or a space
     * on the subsequent lines */
    while((ptr) &&
          ((*(ptr+1) == ' ' ) ||
           (*(ptr+1) == '\t')    )) {
      ptr = strchr(ptr + 1, '\n');
    }

    if(ptr) {
      *ptr = 0;
    }
    // Add "," when several headers are present
    if (dest != last_header) {
      dest += sprintf(dest, ",");
    }
    dest += sprintf(dest, "%s", src);
    if(ptr) {
      *ptr = '\n';
    }

    src++;
  }

  if(dest == last_header) {
    return last_header;
  }

  *(dest--) = 0;

  /* Remove trailing whitespaces, tabs, and CRs */
  while ((dest > last_header) &&
         ((*dest == ' ') || (*dest == '\r')|| (*dest == '\t'))) {
    *(dest--) = 0;
  }

  /* remove enclosed CRs in multilines */
  while((ptr = strchr(last_header, '\r'))) {
    /* Use strlen(ptr) to include trailing zero */
    memmove(ptr, ptr+1, strlen(ptr));
  }

  return last_header;
}

char * get_incoming_first_line(char * message)
{
  /* non reentrant. consider accepting char buffer as param */
  static char last_header[MAX_HEADER_LEN * 10];
  char * src, *dest;

  /* returns empty string in case of error */
  memset(last_header, 0, sizeof(last_header));

  if((!message) || (!strlen(message))) {
    return last_header;
  }

  src = message;
  dest = last_header;

  int i=0;
  while (*src) {
    if((*src=='\n')||(*src=='\r')) {
      break;
    } else {
      last_header[i]=*src;
    }
    i++;
    src++;
  }

  return last_header;
}



/*************************** I/O functions ***************************/



//this isnt used anywhere??
size_t decompress_if_needed(int sock, char *buff,  size_t len, void **st)
{
#ifndef WIN32
  DEBUGIN();
  if(compression && len) {
    if (useMessagef == 1) {
      struct timeval currentTime;
      GET_TIME (&currentTime);
      TRACE_MSG("----------------------------------------------- %s\n"
                "Compressed message received, header :\n"
                "0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x "
                "0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
                CStat::formatTime(&currentTime, true),
                buff[0] , buff[1] , buff[2] , buff[3],
                buff[4] , buff[5] , buff[6] , buff[7],
                buff[8] , buff[9] , buff[10], buff[11],
                buff[12], buff[13], buff[14], buff[15]);
    }

    int rc = comp_uncompress(st,
                             buff,
                             (unsigned int *) &len);

    switch(rc) {
    case COMP_OK:
      TRACE_MSG("Compressed message decompressed properly.\n");
      break;

    case COMP_REPLY:
      TRACE_MSG("Compressed message KO, sending a reply (resynch).\n");
      sendto(sock,
             buff,
             len,
             0,
             (sockaddr *)(void *)&remote_sockaddr,
             SOCK_ADDR_SIZE(&remote_sockaddr));
      resynch_send++;
      return 0;

    case COMP_DISCARD:
      TRACE_MSG("Compressed message discarded by pluggin.\n");
      resynch_recv++;
      return 0;

    default:
    case COMP_KO:
      REPORT_ERROR("Compression pluggin error");
      return 0;
    }
  }
  DEBUGOUT();
#else
  REPORT_ERROR("Cannot Decompress. Compression is not enabled in this build");
#endif
  return len;
}


void close_peer_sockets()
{
  peer_map::iterator peer_it;
  T_peer_infos infos;

  for(peer_it = peers.begin(); peer_it != peers.end(); peer_it++) {
    infos = peer_it->second;
    sipp_close_socket(infos.peer_socket);
    infos.peer_socket = NULL ;
    peers[std::string(peer_it->first)] = infos;
  }

  peers_connected = 0;
}


void close_local_sockets()
{
  for (int i = 0; i< local_nb; i++) {
    sipp_close_socket(local_sockets[i]);
    local_sockets[i] = NULL;
  }
}


void free_peer_addr_map()
{
  peer_addr_map::iterator peer_addr_it;
  for (peer_addr_it = peer_addrs.begin(); peer_addr_it != peer_addrs.end(); peer_addr_it++) {
    free(peer_addr_it->second);
  }
}

static int read_error(struct sipp_socket *socket, int ret)
{
#ifdef WIN32
  ERRORNUMBER = WSAGetLastError();
  wchar_t *error_msg = wsaerrorstr(ERRORNUMBER);
  char errorstring[1000];
  const char *errstring = wchar_to_char(error_msg,errorstring);
#else
  const char *errstring = strerror(ERRORNUMBER);
#endif
#ifdef _USE_OPENSSL
  if (socket->ss_transport == T_TLS) {
    errstring = sip_tls_error_string(socket->ss_ssl, ret);
  }
#endif

  assert(ret <= 0);

#ifdef EAGAIN
  /* Scrub away EAGAIN from the rest of the code. */
  if (ERRORNUMBER == EAGAIN) {
    ERRORNUMBER = EWOULDBLOCK;
  }
#endif

  /* We have only non-blocking reads, so this should not occur. */
  if (ret < 0) {
    assert(ERRORNUMBER != EAGAIN);
  }

  if (socket->ss_transport == T_TCP || socket->ss_transport == T_TLS) {
    if (ret == 0) {
      /* The remote side closed the connection. */
      if(socket->ss_control) {
        if(localTwinSippSocket) sipp_close_socket(localTwinSippSocket);
        if (extendedTwinSippMode) {
          close_peer_sockets();
          close_local_sockets();
          free_peer_addr_map();
          WARNING("One of the twin instances has ended -> exiting");
          quitting += 20;
        } else if(twinSippMode) {
          if(twinSippSocket) sipp_close_socket(twinSippSocket);
          if(thirdPartyMode == MODE_3PCC_CONTROLLER_B) {
            WARNING("3PCC controller A has ended -> exiting");
            quitting += 20;
          } else {
            quitting = 1;
          }
        }
      } else {
        /* The socket was closed "cleanly", but we may have calls that need to
         * be destroyed.  Also, if these calls are not complete, and attempt to
         * send again we may "ressurect" the socket by reconnecting it.*/
        sipp_socket_invalidate(socket);
        if (reset_close) {
          WARNING("Read Error: TCP connection closed remotely. Ending call. Use options '-t tn -reconnect_close false ' to allow remote disconnect in a call");
          close_calls(socket);
        }
      }
      return 0;
    }

    CLOSESOCKET(socket->ss_fd);
    socket->ss_fd = -1;
    sockets_pending_reset.insert(socket);

    nb_net_recv_errors++;
    if (reconnect_allowed()) {
      WARNING("Error on TCP connection, remote peer probably closed the socket: %s", errstring);
    } else {
      REPORT_ERROR("Error on TCP connection, remote peer probably closed the socket: %s", errstring);
    }
    return -1;
  }

  WARNING("Unable to receive %s message: %s", TRANSPORT_TO_STRING(socket->ss_transport), errstring);
  nb_net_recv_errors++;
  return -1;
}





/****************************** Network Interface *******************/



static ssize_t read_message(struct sipp_socket *socket, char *buf, size_t len, struct sockaddr_storage *src)
{
  size_t avail;
  DEBUGIN();

  if (!socket->ss_msglen)
    return 0;
  if (socket->ss_msglen > len)
    REPORT_ERROR("There is a message waiting in sockfd(%d) that is bigger (%d bytes) than the read size.",
                 socket->ss_fd, socket->ss_msglen);

  len = socket->ss_msglen;

  avail = socket->ss_in->len - socket->ss_in->offset;
  if (avail > len) {
    avail = len;
  }

  memcpy(buf, socket->ss_in->buf + socket->ss_in->offset, avail);
  memcpy(src, &socket->ss_in->addr, SOCK_ADDR_SIZE(&socket->ss_in->addr));
  DEBUG("socket->ss_in->addr assigned %s:%hu", inet_ntoa( ((struct sockaddr_in*)&(socket->ss_in->addr))->sin_addr ), ntohs(((struct sockaddr_in*)&(socket->ss_in->addr))->sin_port) );

  /* Update our buffer and return value. */
  buf[avail] = '\0';
  /* For CMD Message the escape char is the end of message */
  if((socket->ss_control) && buf[avail-1] == 27 ) buf[avail-1] = '\0';

  socket->ss_in->offset += avail;

  /* Have we emptied the buffer? */
  if (socket->ss_in->offset == socket->ss_in->len) {
    struct socketbuf *next = socket->ss_in->next;
    free_socketbuf(socket->ss_in);
    socket->ss_in = next;
  }

  if (int msg_len = check_for_message(socket)) {
    socket->ss_msglen = msg_len;
  } else {
    socket->ss_msglen = 0;
    pending_messages--;
  }

  if (useMessagef == 1) {
    struct timeval currentTime;
    GET_TIME (&currentTime);
    TRACE_MSG("----------------------------------------------- %s\n"
              "%s %smessage received [%d] bytes :\n\n%s\n",
              CStat::formatTime(&currentTime, true),
              TRANSPORT_TO_STRING(socket->ss_transport),
              socket->ss_control ? "control " : "",
              avail, buf);
  }

  DEBUGOUT();
  return avail;
}



void process_message(struct sipp_socket *socket, char *msg, ssize_t msg_size, struct sockaddr_storage *src)
{
  DEBUGIN();
  if(msg_size <= 0) {
    return;
  }
  if (sipMsgCheck(msg, msg_size, socket) == false) {
    WARNING("non SIP message discarded");
    return;
  }

  char *call_id = get_call_id(msg);
  if (call_id[0] == '\0') {
    WARNING("SIP message without Call-ID discarded '%s'", msg);
    return;
  }
  listener *listener_ptr = get_listener(no_call_id_check == false ? call_id : NULL);

  if (useShortMessagef == 1) {
    struct timeval currentTime;
    GET_TIME (&currentTime);
    TRACE_SHORTMSG("%s\tR\t%s\tCSeq:%s\t%s\n",
                   CStat::formatTime(&currentTime),call_id, get_incoming_header_content(msg,"CSeq:"), get_incoming_first_line(msg));
  }

  if(!listener_ptr) {
    DEBUG("get_listener() returned 0 (so new call must be created)");
    if(thirdPartyMode == MODE_3PCC_CONTROLLER_B || thirdPartyMode == MODE_3PCC_A_PASSIVE
        || thirdPartyMode == MODE_MASTER_PASSIVE || thirdPartyMode == MODE_SLAVE) {
      // Adding a new OUTGOING call !
      DEBUG("Adding a new OUTGOING 3PCC call");
      main_scenario->stats->computeStat(CStat::E_CREATE_OUTGOING_CALL);
      call *new_ptr = new call(call_id, is_ipv6, 0, use_remote_sending_addr ? &remote_sending_sockaddr : &remote_sockaddr);
      if (!new_ptr) {
        REPORT_ERROR("Out of memory allocating a call!");
      }

      outbound_congestion = false;
      if((socket != main_socket) &&
          (socket != tcp_multiplex) &&
          (socket != localTwinSippSocket) &&
          (socket != twinSippSocket) &&
          (!is_a_local_socket(socket))) {
        new_ptr->associate_socket(socket);
        socket->ss_count++;
      } else {
        /* We need to hook this call up to a real *call* socket. */
        if (!multisocket) {
          switch(transport) {
          case T_UDP:
            new_ptr->associate_socket(main_socket);
            main_socket->ss_count++;
            break;
          case T_TCP:
          case T_TLS:
            new_ptr->associate_socket(tcp_multiplex);
            tcp_multiplex->ss_count++;
            break;
          }
        }
      }
      listener_ptr = new_ptr;
    } else if(creationMode == MODE_SERVER) {
      if (quitting >= 1) {
        CStat::globalStat(CStat::E_OUT_OF_CALL_MSGS);
        TRACE_MSG("Discarded message for new calls while quitting\n");
        return;
      }

      // Adding a new INCOMING call !
      main_scenario->stats->computeStat(CStat::E_CREATE_INCOMING_CALL);
      listener_ptr = new call(call_id, socket, use_remote_sending_addr ? &remote_sending_sockaddr : src);
      if (!listener_ptr) {
        REPORT_ERROR("Out of memory allocating a call!");
      }
    } else { // mode is CLIENT [mode != from SERVER, 3PCC, MODE_SLAVE, MODE_MASTER]
      // This is a message that is not relating to any known call
      if (auto_answer == true) {
        // If auto answer mode, try to answer the incoming message
        // with automaticResponseMode
        // call is discarded before exiting the block
        if(!get_reply_code(msg)) {
          ooc_scenario->stats->computeStat(CStat::E_CREATE_INCOMING_CALL);
          /* This should have the real address that the message came from. */
          call *call_ptr = new call(ooc_scenario, socket, use_remote_sending_addr ? &remote_sending_sockaddr : src, call_id, 0 /* no user. */, socket->ss_ipv6, true, false);
          if (!call_ptr) {
            REPORT_ERROR("Out of memory allocating a call!");
          }
          CStat::globalStat(CStat::E_AUTO_ANSWERED);
          call_ptr->process_incoming(msg, src);
        } else {
          /* We received a response not relating to any known call */
          /* Do nothing, even if in auto answer mode */
          CStat::globalStat(CStat::E_OUT_OF_CALL_MSGS);
        }
      } else {
        CStat::globalStat(CStat::E_OUT_OF_CALL_MSGS);
        WARNING("Discarding message which can't be mapped to a known SIPp call:\n%s", msg);
      }
    }
  }


  /* If the call was not created above, we just drop this message. */
  if (!listener_ptr) {
    DEBUG("Call not created above so dropping message");
    return;
  }

  if((socket == localTwinSippSocket) || (socket == twinSippSocket) || (is_a_local_socket(socket))) {
    listener_ptr -> process_twinSippCom(msg);
  } else {
    listener_ptr -> process_incoming(msg, src, socket);
  }
  DEBUGOUT();
} // process_message

void pollset_process(int wait)
{
  int rs; /* Number of times to execute recv().
       For TCP with 1 socket per call:
           no. of events returned by poll
       For UDP and TCP with 1 global socket:
           recv_count is a flag that stays up as
           long as there's data to read */

  int loops = max_recv_loops;

  /* What index should we try reading from? */
  static int read_index;

  if (read_index >= pollnfds) {
    read_index = 0;
  }

  /* We need to process any messages that we have left over. */
  while (pending_messages && (loops > 0)) {
    getmilliseconds();
    if (sockets[read_index]->ss_msglen) {
      struct sockaddr_storage src;
      char msg[SIPP_MAX_MSG_SIZE];
      ssize_t len = read_message(sockets[read_index], msg, sizeof(msg), &src);
      if (len > 0) {
        process_message(sockets[read_index], msg, len, &src);
      } else {
        assert(0);
      }
      loops--;
    }
    read_index = (read_index + 1) % pollnfds;
  }

  /* Don't read more data if we still have some left over. */
  if (pending_messages) {
    return;
  }

  /* Get socket events. */
  rs = POLL(pollfiles, pollnfds, wait ? 1 : 0);
  print_if_error(rs);  
#ifdef WIN32
  ERRORNUMBER = WSAGetLastError();
#endif
  //todo possible collision err.h defines EINTR 4
  //   we def EINTR as WSAEINTR which is 10004L
  if((rs < 0) && (ERRORNUMBER == EINTR)) {
    return;
  }

  /* We need to flush all sockets and pull data into all of our buffers. */
  for(int poll_idx = 0; rs > 0 && poll_idx < pollnfds; poll_idx++) {
    struct sipp_socket *sock = sockets[poll_idx];
    int events = 0;
    int ret = 0;

    assert(sock);

    if(pollfiles[poll_idx].revents & POLLOUT) {
      /* We can flush this socket. */
      TRACE_MSG("Exit problem event on socket %d \n", sock->ss_fd);
      pollfiles[poll_idx].events &= ~POLLOUT;
      sock->ss_congested = false;

      flush_socket(sock);
      events++;
    }

    if(pollfiles[poll_idx].revents & POLLIN) {
      /* We can empty this socket. */
      if ((transport == T_TCP || transport == T_TLS) && sock == main_socket) {
        // accept connection (limiting to remote_host if one was specified and we're using no_call_id_check)
        struct sipp_socket *new_sock = sipp_accept_socket(sock, (strlen(remote_host) && no_call_id_check) ? &remote_sockaddr : 0);
        DEBUG("Allocated new socket remote %s, dest %s ",
              socket_to_ip_string(&(new_sock->ss_remote_sockaddr)).c_str(),
              socket_to_ip_string(&(new_sock->ss_dest)).c_str()  );
      } else if (sock == ctrl_socket) {
        handle_ctrl_socket();
      } else if (sock == stdin_socket) {
        handle_stdin_socket();
      } else if (sock == localTwinSippSocket) {
        if (thirdPartyMode == MODE_3PCC_CONTROLLER_B) {
          twinSippSocket = sipp_accept_socket(sock);
          if (!twinSippMode) {
            REPORT_ERROR_NO("Accepting new TCP connection on Twin SIPp Socket.\n");
          }
          twinSippSocket->ss_control = 1;
        } else {
          /*3pcc extended mode: open a local socket
            which will be used for reading the infos sent by this remote
            twin sipp instance (slave or master) */
          if(local_nb == MAX_LOCAL_TWIN_SOCKETS) {
            REPORT_ERROR("Max number of twin instances reached\n");
          }

          struct sipp_socket *localSocket = sipp_accept_socket(sock);
          localSocket->ss_control = 1;
          local_sockets[local_nb] = localSocket;
          local_nb++;
          if(!peers_connected) {
            connect_to_all_peers();
          }
        }
      } else {
        // ret = return value from recvfrom
        // 0 = orderly shutdown
        // -1 = error
        if ((ret = empty_socket(sock)) <= 0) {
          ret = read_error(sock, ret);
          if (ret == 0) {
            /* If read_error() then the poll_idx now belongs
             * to the newest/last socket added to the sockets[].
             * Need to re-do the same poll_idx for the "new" socket. */
            poll_idx--;
            events++;
            rs--;
            continue;
          }
        }
      }
      events++;
    }

    pollfiles[poll_idx].revents = 0;
    if (events) {
      rs--;
    }
  }

  if (read_index >= pollnfds) {
    read_index = 0;
  }

  /* We need to process any new messages that we read. */
  while (pending_messages && (loops > 0)) {
    getmilliseconds();

    if (sockets[read_index]->ss_msglen) {
      char msg[SIPP_MAX_MSG_SIZE];
      struct sockaddr_storage src;
      ssize_t len;
      len = read_message(sockets[read_index], msg, sizeof(msg), &src);
      if (len > 0) {
        process_message(sockets[read_index], msg, len, &src);
      } else {
        assert(0);
      }
      loops--;
    }
    read_index = (read_index + 1) % pollnfds;
  }

  cpu_max = loops <= 0;
}

void timeout_alarm(int param)
{
  if (timeout_error) {
    REPORT_ERROR("%s timed out after '%.3lf' seconds", scenario_file, ((double)clock_tick / 1000LL));
  }
  quitting = 1;
  timeout_exit = true;
}

/* Send loop & trafic generation*/
/* This is called directoy by main() and runs until SIPp quits */

void traffic_thread()
{
  DEBUGIN();
  /* create the file */
  char         L_file_name [MAX_PATH];
  sprintf (L_file_name, "%s_%d_screen.log", scenario_file, getpid());

  getmilliseconds();

#ifdef WIN32
  clock_t targettime = clock()+(global_timeout);
  // check for expiry in traffic loop
#else
  /* Arm the global timer if needed */
  if (global_timeout > 0) {
    signal(SIGALRM, timeout_alarm);
    alarm(global_timeout / 1000);
  }
#endif



  // Dump (to create file on disk) and showing screen at the beginning even if
  // the report period is not reached
  stattask::report();
  screentask::report(false);

#ifdef WIN32
  // report task uses these values to refresh the screen and maintain command echo
  command_mode = 0;
  unsigned int cmd_char_count = 0;
#endif

  while(1) {
    scheduling_loops++;
    getmilliseconds();

#ifdef WIN32
  // check if global timer is set and if it is expired.
    if ((global_timeout > 0)&&(clock() > targettime)) {
      timeout_alarm(0);
    }

  // check for console input, no blocking calls since this is traffic loop
    int ch;
    if(kbhit()){
      ch = getch();
      if (command_mode){
        putchar(ch);
        switch (ch){
          case 0x1b:  //esc 
          case 0x3:   //ctl c
            free(command_buffer);
            command_buffer = NULL;
            command_mode = 0;
            cmd_char_count = 0;
            break;
          case 0x8:   //del
            if (cmd_char_count > 0){
              command_buffer[cmd_char_count-1]=0;
              cmd_char_count--;
            }
            break;
          case 0xd:   //return
            command_buffer[cmd_char_count] = 0;
            process_command(command_buffer);
            free(command_buffer);
            command_buffer = NULL;
            command_mode = 0;
            cmd_char_count = 0;
            break;
          default:
            if (cmd_char_count<MAX_CMD_CHARS-1)
              command_buffer[cmd_char_count++] = (char)ch;
            else
              WARNING("command buffer full:..sure you know what you're doing?");
            break;
        }
      }else{
        if (ch == (int) 'c'){
          command_mode = 1;
          printf("Command: ");
          command_buffer = (char*)calloc(MAX_CMD_CHARS,1);
        }else
          process_key(ch);
      }
    }
#endif

    if (signalDump) {
      /* Screen dumping in a file */
      if (screenf) {
        print_screens();
      } else {
        /* If the -trace_screen option has not been set, */
        /* create the file at this occasion              */
        screenf = fopen(L_file_name, "a");
        if (!screenf) {
          WARNING("Unable to create '%s'", L_file_name);
        }
        print_screens();
        fclose(screenf);
        screenf = 0;
      }

      if(dumpInRtt) {
        main_scenario->stats->dumpDataRtt ();
      }

      signalDump = false ;
    }

    while (sockets_pending_reset.begin() != sockets_pending_reset.end()) {
      reset_connection(*(sockets_pending_reset.begin()));
      sockets_pending_reset.erase(sockets_pending_reset.begin());
    }

    if (((main_scenario->stats->GetStat(CStat::CPT_C_IncomingCallCreated) + main_scenario->stats->GetStat(CStat::CPT_C_OutgoingCallCreated)) >= stop_after) && !quitting) {
      DEBUG("Setting quitting to 1, since Incoming Calls: %d plus Outgoing Calls: %d is greater than stop_after: %d", main_scenario->stats->GetStat(CStat::CPT_C_IncomingCallCreated), main_scenario->stats->GetStat(CStat::CPT_C_OutgoingCallCreated), stop_after);
      quitting = 1;
    }
    if (quitting) {
      if (quitting > 11) {
        /* Force exit: abort all calls */
        abort_all_tasks();
      }
      /* Quitting and no more openned calls, close all */
      if(!main_scenario->stats->GetStat(CStat::CPT_C_CurrentCall)) {
        /* We can have calls that do not count towards our open-call count (e.g., dead calls). */
        abort_all_tasks();

        for (int i = 0; i < pollnfds; i++) {
          sipp_close_socket(sockets[i]);
        }

        screentask::report(true);
        stattask::report();
        if (screenf) {
          print_screens();
        }
        if (q_pressed) {
          WARNING("Ending test because q was pressed by user");
          screen_exit(EXIT_TEST_MANUALLY_STOPPED);
        }
        screen_exit(EXIT_TEST_RES_UNKNOWN);
      }
    }

    getmilliseconds();

    /* Schedule all pending calls and process their timers */
    task_list *running_tasks;
    if((clock_tick - last_timer_cycle) > timer_resolution) {

      /* Just for the count. */
      running_tasks = get_running_tasks();
      last_running_calls = running_tasks->size();

      /* If we have expired paused calls, move them to the run queue. */
      last_woken_calls += expire_paused_tasks();

      last_paused_calls = paused_tasks_count();

      last_timer_cycle = clock_tick;
    }

    /* We should never get so busy with running calls that we can't process some messages. */
    int loops = max_sched_loops;

    /* Now we process calls that are on the run queue. */
    running_tasks = get_running_tasks();

    /* Workaround hpux problem with iterators. Deleting the
     * current object when iterating breaks the iterator and
     * leads to iterate again on the destroyed (deleted)
     * object. Thus, we have to wait one step befere actual
     * deletion of the object*/
    task * last = NULL;

    task_list::iterator iter;
    for(iter = running_tasks->begin(); iter != running_tasks->end(); iter++) {
      if(last) {
        last -> run();
        if (sockets_pending_reset.begin() != sockets_pending_reset.end()) {
          last = NULL;
          break;
        }
      }
      last = *iter;
      if (--loops <= 0) {
        break;
      }
    }
    if(last) {
      last -> run();
    }
    while (sockets_pending_reset.begin() != sockets_pending_reset.end()) {
      reset_connection(*(sockets_pending_reset.begin()));
      sockets_pending_reset.erase(sockets_pending_reset.begin());
    }

    /* Update the clock. */
    getmilliseconds();
    /* Receive incoming messages */
    pollset_process(running_tasks->size() == 0);
  }
  DEBUGOUT();
}

/*************** RTP ECHO THREAD ***********************/
/* param is a pointer to RTP socket */


void rtp_echo_thread (void * param)
{
  char *msg = (char*)alloca(media_bufsize);
  size_t nr, ns;
  sipp_socklen_t len;
  struct sockaddr_storage remote_rtp_addr;

#ifndef WIN32
  //windows doesnt support signals, no need to block them
  int                   rc;
  sigset_t              mask;
  sigfillset(&mask); /* Mask all allowed signals */
  rc = pthread_sigmask(SIG_BLOCK, &mask, NULL);
#endif

  for (;;) {
    len = sizeof(remote_rtp_addr);
    nr = recvfrom(*(int *)param,
                  msg,
                  media_bufsize, 0,
                  (sockaddr *)(void *) &remote_rtp_addr,
                  &len);
#ifdef WIN32
    ERRORNUMBER = WSAGetLastError();
#endif
    if (((long)nr) < 0) {
      WARNING("%s %i",
              "Error on RTP echo reception - stopping echo - errno=",
              ERRORNUMBER);
      return;
    }
    ns = sendto(*(int *)param, msg, nr,
                0, (sockaddr *)(void *) &remote_rtp_addr,
                len);
#ifdef WIN32
    ERRORNUMBER = WSAGetLastError();
#endif
    if (ns != nr) {
      WARNING("%s %i",
              "Error on RTP echo transmission - stopping echo - errno=",
              ERRORNUMBER);
      return;
    }

    if (*(int *)param==media_socket) {
      rtp_pckts++;
      rtp_bytes += ns;
    } else {
      /* packets on the second RTP stream */
      rtp2_pckts++;
      rtp2_bytes += ns;
    }
  }
}

/* Wrap the help text. */
char *wrap(const char *in, int offset, int size)
{
  int pos = 0;
  int i, j;
  int l = strlen(in);
  int alloced = l + 1;
  char *out = (char *)malloc(alloced);
  int indent = 0;

  if (!out) {
    REPORT_ERROR_NO("malloc");
  }

  for (i = j = 0; i < l; i++) {
    out[j++] = in[i];
    if (in[i] == '\n') {
      char * new_out = (char *)realloc(out, alloced += offset);
      if (!new_out) {
        REPORT_ERROR_NO("Unable to allocate memory in wrap().");
      }
      out = new_out;
      pos = 0;
      for (int k = 0; k < offset; k++) {
        out[j++] = ' ';
      }
      if (indent) {
        indent = 0;
      }
    }
    if (in[i] == '-' && i > 0 && in[i - 1] == '\n') {
      indent = 1;
    }
    if (++pos > size) {
      int k;
      for (k = j - 1; k > 0 && !isspace(out[k]); k--);
      int useoffset = offset;

      if (indent) {
        useoffset += 2;
      }

      if (k == 0 || out[k] == '\n') {
        pos = 0;
        out[j++] = '\n';
        char * new_out = (char *)realloc(out, alloced += useoffset);
        if (!new_out) {
          REPORT_ERROR_NO("Unable to allocate memory in wrap().");
        }
        out = new_out;
        for (k = 0; k < useoffset; k++) {
          out[j++] = ' ';
        }
      } else {
        int m;
        int move_back = 0;

        //printf("Before wrapping (pos = %d, k = %d, j = %d):\n%-*s%s\n", pos, k, j, offset, "", out);

        out[k] = '\n';
        pos = j - k;
        // move_back is used to step back in the in and out buffers when a
        // word is longer than useoffset.
        if (i > (k + useoffset)) {
          move_back = i - (k + useoffset);
          i -= move_back;
        }
        k++;
        char * new_out = (char *)realloc(out, alloced += useoffset);
        if (!new_out) {
          REPORT_ERROR_NO("Unable to allocate memory in wrap().");
        }
        out = new_out;
        for (m = 0; m < useoffset; m++) {
          if (k + useoffset + m < alloced) {
            out[k + useoffset + m] = out[k + m];
          }
          out[k + m] = ' ';
        }
        j += useoffset - move_back;
        //printf("After wrapping (pos = %d, k = %d):\n%-*s%s\n", pos, k, offset, "", out);
      }
    }
  }
  out[j] = '\0';

  return out;
}

/* Help screen */
void help()
{
  int i, max;

  printf
  ("\n"
   "Usage:\n"
   "\n"
   "  sipp remote_host[:remote_port] [options]\n"
   "\n"
   "  Available options:\n"
   "\n");

  /* We automatically generate the help messages based on the options array.
   * This should hopefully encourage people to write help text when they
   * introduce a new option and keep the code a bit cleaner. */
  max = sizeof(options_table)/sizeof(options_table[0]);
  for (i = 0; i < max; i++) {
    char *formatted;
    if (!options_table[i].help) {
      continue;
    }
    formatted = wrap(options_table[i].help, 22, 57);
    printf("   -%-16s: %s\n\n", options_table[i].option, formatted);
    free(formatted);
  }

  printf
  (
#ifdef WIN32
    "Signal handling:\n"
    "\n"
    "   SIPp can be controlled using posix signals. The following signals\n"
    "   are handled:\n"
    "   USR1: Similar to press 'q' keyboard key. It triggers a soft exit\n"
    "         of SIPp. No more new calls are placed and all ongoing calls\n"
    "         are finished before SIPp exits.\n"
    "         Example: kill -SIGUSR1 732\n"
    "   USR2: Triggers a dump of all statistics screens in\n"
    "         <scenario_name>_<pid>_screens.log file. Especially useful \n"
    "         in background mode to know what the current status is.\n"
    "         Example: kill -SIGUSR2 732\n"
#endif
        "\n"
    "Exit code:\n"
    "\n"
    "   Upon exit (on fatal error or when the number of asked calls (-m\n"
    "   option) is reached, sipp exits with one of the following exit\n"
    "   code:\n"
    "    0: All calls were successful\n"
    "    1: At least one call failed\n"
    "   97: exit on internal command. Calls may have been processed\n"
    "   99: Normal exit without calls processed\n"
    "   -1: Fatal error\n"
    "   -2: Fatal error binding a socket\n"
    "\n"
    "\n"
    "Example:\n"
    "\n"
    "   Run sipp with embedded server (uas) scenario:\n"
    "     ./sipp -sn uas\n"
    "   On the same host, run sipp with embedded client (uac) scenario\n"
    "     ./sipp -sn uac 127.0.0.1\n"
    "\n");
}


void help_stats()
{
  printf(
    "\n"
    "  The  -trace_stat option dumps all statistics in the\n"
    "  <scenario_name.csv> file. The dump starts with one header\n"
    "  line with all counters. All following lines are 'snapshots' of \n"
    "  statistics counter given the statistics report frequency\n"
    "  (-fd option). This file can be easily imported in any\n"
    "  spreadsheet application, like Excel.\n"
    "\n"
    "  In counter names, (P) means 'Periodic' - since last\n"
    "  statistic row and (C) means 'Cumulated' - since sipp was\n"
    "  started.\n"
    "\n"
    "  Available statistics are:\n"
    "\n"
    "  - StartTime: \n"
    "    Date and time when the test has started.\n"
    "\n"
    "  - LastResetTime:\n"
    "    Date and time when periodic counters where last reseted.\n"
    "\n"
    "  - CurrentTime:\n"
    "    Date and time of the statistic row.\n"
    "\n"
    "  - ElapsedTime:\n"
    "    Elapsed time.\n"
    "\n"
    "  - CallRate:\n"
    "    Call rate (calls per seconds).\n"
    "\n"
    "  - IncomingCall:\n"
    "    Number of incoming calls.\n"
    "\n"
    "  - OutgoingCall:\n"
    "    Number of outgoing calls.\n"
    "\n"
    "  - TotalCallCreated:\n"
    "    Number of calls created.\n"
    "\n"
    "  - CurrentCall:\n"
    "    Number of calls currently ongoing.\n"
    "\n"
    "  - SuccessfulCall:\n"
    "    Number of successful calls.\n"
    "\n"
    "  - FailedCall:\n"
    "    Number of failed calls (all reasons).\n"
    "\n"
    "  - FailedCannotSendMessage:\n"
    "    Number of failed calls because Sipp cannot send the\n"
    "    message (transport issue).\n"
    "\n"
    "  - FailedMaxUDPRetrans:\n"
    "    Number of failed calls because the maximum number of\n"
    "    UDP retransmission attempts has been reached.\n"
    "\n"
    "  - FailedUnexpectedMessage:\n"
    "    Number of failed calls because the SIP message received\n"
    "    is not expected in the scenario.\n"
    "\n"
    "  - FailedCallRejected:\n"
    "    Number of failed calls because of Sipp internal error.\n"
    "    (a scenario sync command is not recognized or a scenario\n"
    "    action failed or a scenario variable assignment failed).\n"
    "\n"
    "  - FailedCmdNotSent:\n"
    "    Number of failed calls because of inter-Sipp\n"
    "    communication error (a scenario sync command failed to\n"
    "    be sent).\n"
    "\n"
    "  - FailedRegexpDoesntMatch:\n"
    "    Number of failed calls because of regexp that doesn't\n"
    "    match (there might be several regexp that don't match\n"
    "    during the call but the counter is increased only by\n"
    "    one).\n"
    "\n"
    "  - FailedRegexpShouldntMatch:\n"
    "    Number of failed calls because of regexp that shouldn't\n"
    "    match (there might be several regexp that shouldn't match\n"
    "    during the call but the counter is increased only by\n"
    "    one).\n"
    "\n"
    "  - FailedRegexpHdrNotFound:\n"
    "    Number of failed calls because of regexp with hdr    \n"
    "    option but no matching header found.\n"
    "\n"
    "  - OutOfCallMsgs:\n"
    "    Number of SIP messages received that cannot be associated\n"
    "    to an existing call.\n"
    "\n"
    "  - AutoAnswered:\n"
    "    Number of unexpected specific messages received for new Call-ID.\n"
    "    The message has been automatically answered by a 200 OK\n"
    "    Currently, implemented for 'PING' message only.\n"
    "\n");
}

/************* exit handler *****************/

void print_last_stats()
{
  interrupt = 1;
  // print last current screen
  if (print_statistics(1)==INTERNAL_ERROR)
      REPORT_ERROR("Internal error: creationMode=%d, thirdPartyMode=%d", creationMode, thirdPartyMode);
  // and print statistics screen
  currentScreenToDisplay = DISPLAY_STAT_SCREEN;
  if (print_statistics(1)==INTERNAL_ERROR)
      REPORT_ERROR("Internal error: creationMode=%d, thirdPartyMode=%d", creationMode, thirdPartyMode);
  if (main_scenario) {
    stattask::report();
  }
}




void freeInFiles()
{
  for (file_map::iterator file_it = inFiles.begin(); file_it != inFiles.end(); file_it++) {
    delete file_it->second;
  }
}

void freeUserVarMap()
{
  for (int_vt_map::iterator vt_it = userVarMap.begin(); vt_it != userVarMap.end(); vt_it++) {
    vt_it->second->putTable();
    userVarMap[vt_it->first] = NULL;
  }
}

void releaseGlobalAllocations()
{
  cleanup_sockets();
  delete main_scenario;
  delete ooc_scenario;
  free_default_messages();
  freeInFiles();
  freeUserVarMap();
  delete globalVariables;

}


/* Main */
int main(int argc, char *argv[])
{
  int                  argi = 0;
  struct sockaddr_storage   media_sockaddr;
  pthread_t            pthread2_id,  pthread3_id;
  int                  L_maxSocketPresent = 0;
  unsigned int         generic_count = 0;
  bool                 slave_masterSet = false;

  generic[0] = NULL;

  /* At least one argument is needed */
  if(argc < 2) {
    help();
    exit(EXIT_OTHER);
  }
#ifndef WIN32
  {
    /* Ignore the SIGPIPE signal */
    struct sigaction action_pipe;
    memset(&action_pipe, 0, sizeof(action_pipe));
    action_pipe.sa_handler=SIG_IGN;
    sigaction(SIGPIPE, &action_pipe, NULL);

    /* The Window Size change Signal is also useless, and causes failures. */
# ifdef SIGWINCH
    sigaction(SIGWINCH, &action_pipe, NULL);
# endif

    /* sig usr1 management */
    struct sigaction action_usr1;
    memset(&action_usr1, 0, sizeof(action_usr1));
    action_usr1.sa_handler = sipp_sigusr1;
    sigaction(SIGUSR1, &action_usr1, NULL);

    /* sig usr2 management */
    struct sigaction action_usr2;
    memset(&action_usr2, 0, sizeof(action_usr2));
    action_usr2.sa_handler = sipp_sigusr2;
    sigaction(SIGUSR2, &action_usr2, NULL);
  }
#else
  // no windows signals 
#endif // ifndef WIN32
  screen_set_exename((char *)"sipp");
  set_sipp_version_string();

  pid = getpid();
  memset(local_ip, 0, 40);
  memset(local_ip2, 0, 40);
  memset(media_ip,0, 40);
  memset(control_ip,0, 40);
  memset(media_ip_escaped,0, 42);

  /* Load compression pluggin if available */
#ifndef WIN32
  comp_load();
#else
  // compression disabled on win32
#endif
  /* Initialize the tolower table. */
  init_tolower_table();

  /* Initialize our global variable structure. */
  globalVariables = new AllocVariableTable(NULL);
  userVariables = new AllocVariableTable(globalVariables);

  /* Command line parsing */
  /* Verify that value associated with argument exists (is not another argument) */
#define REQUIRE_ARG() if((++argi) >= argc || (argv[argi])[0] == '-') { REPORT_ERROR("Missing argument for param '%s'.\n" \
             "Use 'sipp -h' for details",  argv[argi - 1]); }
#define CHECK_PASS() if (option->pass != pass) { break; }

  int default_scenario_to_use = -2; // -1 indicates non-default scenario ; -2 indicates none specified

  for (int pass = 0; pass <= 3; pass++) {
    for(argi = 1; argi < argc; argi++) {
      struct sipp_option *option = find_option(argv[argi]);
      if (!option) {
        if((argv[argi])[0] != '-') {
          strcpy(remote_host, argv[argi]);
          continue;
        }
        help();
        REPORT_ERROR("Invalid argument: '%s'.\n"
                     "Use 'sipp -h' for details", argv[argi]);
      }

      switch(option->type) {
      case SIPP_OPTION_HELP:
        if(((argi+1) < argc) && (!strcmp(argv[argi+1], "stat"))) {
          help_stats();
        } else {
          help();
        }
        exit(EXIT_OTHER);
      case SIPP_OPTION_VERSION:
        printf("\n%s\n\n",sipp_version);
        printf
        (" This program is free software; you can redistribute it and/or\n"
         " modify it under the terms of the GNU General Public License as\n"
         " published by the Free Software Foundation; either version 2 of\n"
         " the License, or (at your option) any later version.\n"
         "\n"
         " This program is distributed in the hope that it will be useful,\n"
         " but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
         " MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
         " GNU General Public License for more details.\n"
         "\n"
         " You should have received a copy of the GNU General Public\n"
         " License along with this program; if not, write to the\n"
         " Free Software Foundation, Inc.,\n"
         " 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA\n"
         "\n"
         " Author: see source files.\n\n");
        exit(EXIT_OTHER);
      case SIPP_OPTION_INT:
        REQUIRE_ARG();
        CHECK_PASS();
        *((int *)option->data) = get_long(argv[argi], argv[argi-1]);
        break;
      case SIPP_OPTION_LONG:
        REQUIRE_ARG();
        CHECK_PASS();
        *((long *)option->data) = get_long(argv[argi], argv[argi-1]);
        break;
      case SIPP_OPTION_LONG_LONG:
        REQUIRE_ARG();
        CHECK_PASS();
        *((unsigned long long *)option->data) = get_long_long(argv[argi], argv[argi-1]);
        break;
      case SIPP_OPTION_TIME_SEC:
        REQUIRE_ARG();
        CHECK_PASS();
        *((long *)option->data) = get_time(argv[argi], argv[argi-1], 1000);
        break;
      case SIPP_OPTION_TIME_MS:
        REQUIRE_ARG();
        CHECK_PASS();
        *((int *)option->data) = get_time(argv[argi], argv[argi-1], 1);
        break;
      case SIPP_OPTION_TIME_MS_LONG:
        REQUIRE_ARG();
        CHECK_PASS();
        *((long *)option->data) = get_time(argv[argi], argv[argi-1], 1);
        break;
      case SIPP_OPTION_BOOL:
        REQUIRE_ARG();
        CHECK_PASS();
        *((bool *)option->data) = get_bool(argv[argi], argv[argi-1]);
        break;
      case SIPP_OPTION_FLOAT:
        REQUIRE_ARG();
        CHECK_PASS();
        *((double *)option->data) = get_double(argv[argi], argv[argi-1]);
        break;
      case SIPP_OPTION_STRING:
        REQUIRE_ARG();
        CHECK_PASS();
        *((char **)option->data) = argv[argi];
        break;
      case SIPP_OPTION_ARGI:
        REQUIRE_ARG();
        CHECK_PASS();
        *((int *)option->data) = argi;
        break;
      case SIPP_OPTION_INPUT_FILE: {
        REQUIRE_ARG();
        CHECK_PASS();
        FileContents *data = new FileContents(argv[argi]);
        char *name = argv[argi];
        if (strrchr(name, '/')) {
          name = strrchr(name, '/') + 1;
        } else if (strrchr(name, '\\')) {
          name = strrchr(name, '\\') + 1;
        }
        assert(name);
        inFiles[name] = data;
        /* By default, the first file is used for IP address input. */
        if (!ip_file) {
          ip_file = name;
        }
        if (!default_file) {
          default_file = name;
        }
      }
      break;
      case SIPP_OPTION_INDEX_FILE:
        REQUIRE_ARG();
        REQUIRE_ARG();
        CHECK_PASS();
        {
          char *fileName = argv[argi - 1];
          char *endptr;
          int field;

          if (inFiles.find(fileName) == inFiles.end()) {
            REPORT_ERROR("Could not find file for -infindex: %s", argv[argi - 1]);
          }

          field = strtoul(argv[argi], &endptr, 0);
          if (*endptr) {
            REPORT_ERROR("Invalid field specification for -infindex: %s", argv[argi]);
          }

          inFiles[fileName]->index(field);
        }
        break;
      case SIPP_OPTION_SETFLAG:
        CHECK_PASS();
        *((bool *)option->data) = true;
        break;
      case SIPP_OPTION_UNSETFLAG:
        CHECK_PASS();
        *((bool *)option->data) = false;
        break;
      case SIPP_OPTION_TRANSPORT:
        REQUIRE_ARG();
        CHECK_PASS();

        if (strlen(argv[argi]) != 2) {
          REPORT_ERROR("Invalid argument for -t param : '%s'.\n"
                       "Use 'sipp -h' for details",  argv[argi]);
        }

        switch(argv[argi][0]) {
        case 'u':
          transport = T_UDP;
          break;
        case 't':
          transport = T_TCP;
          break;
        case 'l':
#ifdef _USE_OPENSSL
          transport = T_TLS;
          if ( init_OpenSSL() != 1) {
            printf("OpenSSL Initialization problem\n");
            exit ( -1);
          }
          if(!user_port) {
            user_port = 5061;
          }
          if(remote_port == DEFAULT_PORT) {
            remote_port = 5061;
          }
#else
          REPORT_ERROR("To use a TLS transport you must compile SIPp with OpenSSL");
#endif
          break;
        case 'c':
#ifndef WIN32
          if(strlen(comp_error)) {
            REPORT_ERROR("No " COMP_PLUGGIN " pluggin available:\n%s", comp_error);
          }
          transport = T_UDP;
          compression = 1;
#else
          REPORT_ERROR("Compression is not supported in this build of sipp");
#endif
        }
        switch(argv[argi][1]) {
        case '1':
          multisocket = 0;
          peripsocket = 0;
          break;
        case 'n':
          multisocket = 1;
          peripsocket = 0;
          break;
        case 'i':
          multisocket = 1;
          peripsocket = 1;
          socket_close = false;
          break;
        }

        if (peripsocket && transport != T_UDP) {
          REPORT_ERROR("You can only use a perip socket with UDP!\n");
        }
        break;
      case SIPP_OPTION_NEED_SSL:
        CHECK_PASS();
        REPORT_ERROR("OpenSSL is required for the %s option.", argv[argi]);
        break;
      case SIPP_OPTION_MAX_SOCKET:
        REQUIRE_ARG();
        CHECK_PASS();
        max_multi_socket = get_long(argv[argi], argv[argi - 1]);
        maxSocketPresent = true ;
        break;
      case SIPP_OPTION_CSEQ:
        REQUIRE_ARG();
        CHECK_PASS();
        base_cseq = get_long(argv[argi], argv[argi - 1]);
        base_cseq--;
        break;
      case SIPP_OPTION_IP: {
        int dummy_port;
        char *ptr = (char *)option->data;
        REQUIRE_ARG();
        CHECK_PASS();

        strcpy(ptr, argv[argi]);
        get_host_and_port(ptr, ptr, &dummy_port);
      }
      break;
      case SIPP_OPTION_LIMIT:
        REQUIRE_ARG();
        CHECK_PASS();
        if (users >= 0) {
          REPORT_ERROR("Can not set open call limit (-l) when -users is specified.");
        }
        if (no_call_id_check) {
          REPORT_ERROR("Can not set open call limit (-l) when -mc is specified.");
        }
        open_calls_allowed = get_long(argv[argi], argv[argi - 1]);
        open_calls_user_setting = 1;
        break;
      case SIPP_OPTION_NO_CALL_ID_CHECK:
        CHECK_PASS();
        no_call_id_check = true;
        open_calls_allowed = 1;
        open_calls_user_setting = 1;
        retrans_enabled = 0;
        watchdog_interval = 0;
        deadcall_wait = 0;
        absorb_retrans = 1;
        // default is to stop after 1 call if value not changed on command line.
        if (stop_after == 0xffffffff) {
          DEBUG("Setting stop_after to one since it is %u", stop_after);
          stop_after = 1;
        }
        break;
      case SIPP_OPTION_USERS:
        REQUIRE_ARG();
        CHECK_PASS();
        users = open_calls_allowed = get_long(argv[argi], argv[argi - 1]);
        open_calls_user_setting = 1;
        break;
      case SIPP_OPTION_KEY:
        REQUIRE_ARG();
        REQUIRE_ARG();
        CHECK_PASS();

        if (generic_count+1 >= sizeof(generic)/sizeof(generic[0])) {
          REPORT_ERROR("Too many generic parameters %d",generic_count+1);
        }
        generic[generic_count++] = &argv[argi - 1];
        generic[generic_count] = NULL;
        break;
      case SIPP_OPTION_VAR:
        REQUIRE_ARG();
        REQUIRE_ARG();
        CHECK_PASS();

        {
          int varId = globalVariables->find(argv[argi  - 1], false);
          if (varId == -1) {
            globalVariables->dump();
            REPORT_ERROR("Can not set the global variable %s, because it does not exist.", argv[argi - 1]);
          }
          globalVariables->getVar(varId)->setString(strdup(argv[argi]));
        }
        break;
      case SIPP_OPTION_3PCC:
        if(slave_masterSet) {
          REPORT_ERROR("-3PCC option is not compatible with -master and -slave options\n");
        }
        if(extendedTwinSippMode) {
          REPORT_ERROR("-3pcc and -slave_cfg options are not compatible\n");
        }
        REQUIRE_ARG();
        CHECK_PASS();
        twinSippMode = true;
        strcpy(twinSippHost, argv[argi]);
        get_host_and_port(twinSippHost, twinSippHost, &twinSippPort);
        break;
      case SIPP_OPTION_SCENARIO:
        REQUIRE_ARG();
        CHECK_PASS();
        if (!strcmp(argv[argi - 1], "-sf")) {
          scenario_file = new char [strlen(argv[argi])+1] ;
          sprintf(scenario_file,"%s", argv[argi]);
          set_logging_scenario_file_name(scenario_file);
          if (useLogf == 1) {
            rotate_logfile();
          }
          default_scenario_to_use = -1;
        } else if (!strcmp(argv[argi - 1], "-sn")) {
          int i = find_scenario(argv[argi]);

          scenario_file = new char [strlen(argv[argi])+1] ;
          sprintf(scenario_file,"%s", argv[argi]);
          set_logging_scenario_file_name(scenario_file);
          default_scenario_to_use = i;
        } else if (!strcmp(argv[argi - 1], "-sd")) {
          int i = find_scenario(argv[argi]);
          fprintf(stdout, "%s", default_scenario[i]);
          exit(EXIT_OTHER);
        } else {
          REPORT_ERROR("Internal error, I don't recognize %s as a scenario option\n", argv[argi] - 1);
        }
        break;
      case SIPP_OPTION_OOC_SCENARIO:
        REQUIRE_ARG();
        CHECK_PASS();
        if (!strcmp(argv[argi - 1], "-oocsf")) {
          ooc_scenario = new scenario(argv[argi], 0, 0);
        } else if (!strcmp(argv[argi - 1], "-oocsn")) {
          int i = find_scenario(argv[argi]);
          ooc_scenario = new scenario(0, i, 0);
        } else {
          REPORT_ERROR("Internal error, I don't recognize %s as a scenario option\n", argv[argi] - 1);
        }
        break;
      case SIPP_OPTION_SLAVE_CFG:
        REQUIRE_ARG();
        CHECK_PASS();
        if(twinSippMode) {
          REPORT_ERROR("-slave_cfg and -3pcc options are not compatible\n");
        }
        extendedTwinSippMode = true;
        slave_cfg_file = new char [strlen(argv[argi])+1] ;
        sprintf(slave_cfg_file,"%s", argv[argi]);
        parse_slave_cfg();
        break;
      case SIPP_OPTION_3PCC_EXTENDED:
        REQUIRE_ARG();
        CHECK_PASS();
        if(slave_masterSet) {
          REPORT_ERROR("-slave and -master options are not compatible\n");
        }
        if(twinSippMode) {
          REPORT_ERROR("-master and -slave options are not compatible with -3PCC option\n");
        }
        *((char **)option->data) = argv[argi];
        slave_masterSet = true;
        break;
      case SIPP_OPTION_RSA: {
        REQUIRE_ARG();
        CHECK_PASS();
        char *remote_s_address ;
        int   remote_s_p = DEFAULT_PORT;
        int   temp_remote_s_p;

        temp_remote_s_p = 0;
        remote_s_address = argv[argi] ;
        get_host_and_port(remote_s_address, remote_s_address, &temp_remote_s_p);
        if (temp_remote_s_p != 0) {
          remote_s_p = temp_remote_s_p;
        }
        struct addrinfo   hints;
        struct addrinfo * local_addr;

        printf("Resolving remote sending address %s...\n", remote_s_address);

        memset((char*)&hints, 0, sizeof(hints));
        hints.ai_flags  = AI_PASSIVE;
        hints.ai_family = PF_UNSPEC;

        /* FIXME: add DNS SRV support using liburli? */
        if (getaddrinfo(remote_s_address,
                        NULL,
                        &hints,
                        &local_addr) != 0) {
          REPORT_ERROR("Unknown remote host '%s'.\n"
                       "Use 'sipp -h' for details", remote_s_address);
        }

        memcpy(&remote_sending_sockaddr,
               local_addr->ai_addr,
               SOCK_ADDR_SIZE(
                 _RCAST(struct sockaddr_storage *, local_addr->ai_addr)));

        if (remote_sending_sockaddr.ss_family == AF_INET) {
          (_RCAST(struct sockaddr_in *, &remote_sending_sockaddr))->sin_port =
            htons((short)remote_s_p);
        } else {
          (_RCAST(struct sockaddr_in6 *, &remote_sending_sockaddr))->sin6_port =
            htons((short)remote_s_p);
        }
        use_remote_sending_addr = 1 ;

        freeaddrinfo(local_addr);
        break;
      }
      case SIPP_OPTION_RTCHECK:
        REQUIRE_ARG();
        CHECK_PASS();
        if (!strcmp(argv[argi], "full")) {
          *((int *)option->data) = RTCHECK_FULL;
        } else if (!strcmp(argv[argi], "loose")) {
          *((int *)option->data) = RTCHECK_LOOSE;
        } else {
          REPORT_ERROR("Unknown retransmission detection method: %s\n", argv[argi]);
        }
        break;
      case SIPP_OPTION_TDMMAP: {
        REQUIRE_ARG();
        CHECK_PASS();
        int i1, i2, i3, i4, i5, i6, i7;

        if (sscanf(argv[argi], "{%d-%d}{%d}{%d-%d}{%d-%d}", &i1, &i2, &i3, &i4, &i5, &i6, &i7) == 7) {
          use_tdmmap = true;
          tdm_map_a = i2 - i1;
          tdm_map_x = i1;
          tdm_map_h = i3;
          tdm_map_b = i5 - i4;
          tdm_map_y = i4;
          tdm_map_c = i7 - i6;
          tdm_map_z = i6;
        } else {
          REPORT_ERROR("Parameter -tdmmap must be of form {%%d-%%d}{%%d}{%%d-%%d}{%%d-%%d}");
        }
        break;
      }
      case SIPP_OPTION_DEFAULTS: {
        unsigned long *ptr = (unsigned long *)option->data;
        char *token;

        REQUIRE_ARG();
        CHECK_PASS();

        *ptr = 0;

        token = argv[argi];
        while ((token = strtok(token, ","))) {
          if (!strcmp(token, "none")) {
            *ptr = 0;
          } else {
            unsigned long mask = 0;
            int mode = 1;
            char *p = token;
            if (token[0] == '+') {
              mode = 1;
              p++;
            } else if (token[0] == '-') {
              mode = -1;
              p++;
            }
            if (!strcmp(p, "all")) {
              mask = DEFAULT_BEHAVIOR_ALL;
            } else if (!strcmp(p, "bye")) {
              mask = DEFAULT_BEHAVIOR_BYE;
            } else if (!strcmp(p, "abortunexp")) {
              mask = DEFAULT_BEHAVIOR_ABORTUNEXP;
            } else if (!strcmp(p, "pingreply")) {
              mask = DEFAULT_BEHAVIOR_PINGREPLY;
            } else {
              REPORT_ERROR("Unknown default behavior: '%s'\n", token);
            }
            switch(mode) {
            case 0:
              *ptr = mask;
              break;
            case 1:
              *ptr |= mask;
              break;
            case -1:
              *ptr &= ~mask;
              break;
            default:
              assert(0);
            }
          }
          token = NULL;
        }
        break;
      }
      case SIPP_OPTION_LFNAME:
        REQUIRE_ARG();
        CHECK_PASS();
        ((struct logfile_info*)option->data)->fixedname = true;
        strcpy(((struct logfile_info*)option->data)->file_name, argv[argi]);
        break;
      case SIPP_OPTION_LFOVERWRITE:
        REQUIRE_ARG();
        CHECK_PASS();
        ((struct logfile_info*)option->data)->fixedname = true;
        ((struct logfile_info*)option->data)->overwrite = get_bool(argv[argi], argv[argi-1]);
        break;
#ifndef WIN32
      case SIPP_OPTION_PLUGIN: {
        void *handle;
        char *error;
        int (*init)();
        int ret;

        REQUIRE_ARG();
        CHECK_PASS();

        handle = dlopen(argv[argi], RTLD_NOW);
        if (!handle) {
          REPORT_ERROR("Could not open plugin %s: %s", argv[argi], dlerror());
        }

        init = (int (*)())dlsym(handle, "init");
        if((error = (char *) dlerror())) {
          REPORT_ERROR("Could not locate init function in %s: %s", argv[argi], dlerror());
        }

        ret = init();
        if (ret != 0) {
          REPORT_ERROR("Plugin %s initialization failed.", argv[argi]);
        }
      }
      break;
#endif
      default:
        REPORT_ERROR("Internal error: I don't recognize the option type for %s\n", argv[argi]);
      }
    }
  }

  if((extendedTwinSippMode && !slave_masterSet) || (!extendedTwinSippMode && slave_masterSet)) {
    REPORT_ERROR("-slave_cfg option must be used with -slave or -master option\n");
  }

  if (peripsocket) {
    if (!ip_file) {
      REPORT_ERROR("You must use the -inf option when using -t ui.\n"
                   "Use 'sipp -h' for details");
    }
  }

  if (ringbuffer_size && max_log_size) {
    REPORT_ERROR("Ring Buffer options and maximum log size are mutually exclusive.");
  }

  if (global_lost) {
    lose_packets = 1;
  }

  if (default_scenario_to_use == -2) {
    REPORT_ERROR("You must specify a scenario (for example with -sf or -sn parameters)");
  }

  /* trace file setting */
  if (scenario_file == NULL) {
    scenario_file = new char [ 5 ] ;
    sprintf(scenario_file, "%s", "sipp");
    set_logging_scenario_file_name(scenario_file);
  }


#ifdef _USE_OPENSSL
  if ((transport == T_TLS) && (FI_init_ssl_context() != SSL_INIT_NORMAL)) {
    REPORT_ERROR("FI_init_ssl_context() failed");
  }
#endif

  if (!dump_xml && !dump_sequence_diagram)
    screen_init(print_last_stats, releaseGlobalAllocations);

// OPENING FILES HERE

  if (useDebugf) {
    rotate_debugf();

    // enable all other logging to ensure we get everything.
    useMessagef = 1;
    useCallDebugf = 1;
    useLogf = 1;
    print_all_responses = 1;
    useScreenf = 1;
  }

  if (useMessagef == 1) {
    rotate_messagef();
  }

  if (useShortMessagef == 1) {
    rotate_shortmessagef();
  }

  if (useCallDebugf) {
    rotate_calldebugf();
  }

  // Handled each time in TRACE_EXEC, but important as this establishes the file name
  if (useExecf) {
    rotate_execf();
  }


  if (useScreenf == 1) {
    char L_file_name [MAX_PATH];
    sprintf (L_file_name, "%s_%d_screen.log", scenario_file, getpid());
    screenf = fopen(L_file_name, "w");
    if(!screenf) {
      REPORT_ERROR("Unable to create '%s'", L_file_name);
    }
    setvbuf(screenf, (char *)NULL, _IONBF, 0);
  }

  //check if no_call_id_check is enabled with call limit 1
  //this feature can run just with 1 active call
  if (no_call_id_check == true && open_calls_allowed > 1) {
    REPORT_ERROR("-mc is only allowed with -l 1, meaning just one call can be active.");
  }

  // TODO: finish the -trace_timeout option implementation

  /* if (useTimeoutf == 1) {
     char L_file_name [MAX_PATH];
     sprintf (L_file_name, "%s_%d_timeout.log", scenario_file, getpid());
     timeoutf = fopen(L_file_name, "w");
     if(!timeoutf) {
       REPORT_ERROR("Unable to create '%s'", L_file_name);
     }
   } */

  if (useCountf == 1) {
    char L_file_name [MAX_PATH];
    sprintf (L_file_name, "%s_%d_counts.csv", scenario_file, getpid());
    countf = fopen(L_file_name, "w");
    if(!countf) {
      REPORT_ERROR("Unable to create '%s'", L_file_name);
    }
    print_count_file(countf, 1);
  }

  if (dumpInRtt == 1) {
    main_scenario->stats->initRtt((char*)scenario_file, (char*)".csv",
                                  report_freq_dumpRtt);
  }

  if ((maxSocketPresent) && (max_multi_socket > FD_SETSIZE) ) {
    L_maxSocketPresent = 1;
  }

#ifndef WIN32
  // Initialization:  boost open file limit to the max (AgM) [non-Windows only]

  if (!skip_rlimit) {
    struct rlimit rlimit;

    if (getrlimit (RLIMIT_NOFILE, &rlimit) < 0) {
      REPORT_ERROR_NO("getrlimit error");
    }

    if (rlimit.rlim_max >
# ifndef __CYGWIN
        ((L_maxSocketPresent) ?  (unsigned int)max_multi_socket : FD_SETSIZE)
# else
        FD_SETSIZE
# endif
       ) {
      if (!no_call_id_check) {
        WARNING("Warning: open file limit > FD_SETSIZE; "
                "limiting max. # of open files to FD_SETSIZE = %d\n",
                FD_SETSIZE);
      }
      rlimit.rlim_max =
# ifndef __CYGWIN
        (L_maxSocketPresent) ?  (unsigned int)max_multi_socket+min_socket : FD_SETSIZE ;
# else

        FD_SETSIZE;
# endif
    }

    rlimit.rlim_cur = rlimit.rlim_max;
    if (setrlimit (RLIMIT_NOFILE, &rlimit) < 0) {
      REPORT_ERROR("Unable to increase the open file limit to FD_SETSIZE = %d", FD_SETSIZE);
    }
  }
#endif // ifndef WIN32
  DEBUG("%s", sipp_version);
  DEBUG("Configuration complete, initializing...");

  if (scenario_file) {
    if(default_scenario_to_use != -1) {
      main_scenario = new scenario(0, default_scenario_to_use, dump_xml);
      main_scenario->stats->setFileName(scenario_file, (char*)".csv");
    } else {
      main_scenario = new scenario(scenario_file, 0, dump_xml);
      main_scenario->stats->setFileName(scenario_file, (char*)".csv");
    }
  }

  /* Load default scenario in case nothing was loaded */
  if(!main_scenario) {
    main_scenario = new scenario(0, 0, dump_xml);
    main_scenario->stats->setFileName((char*)"uac", (char*)".csv");
    scenario_file = new char [5];
    DEBUG("Creating scenario_file as not certain this was correct before");
    sprintf(scenario_file,"uac");
    set_logging_scenario_file_name(scenario_file);
  }
  if(!ooc_scenario) {
    ooc_scenario = new scenario(0, find_scenario("ooc_default"), 0);
    ooc_scenario->stats->setFileName((char*)"ooc_default", (char*)".csv");
  }
  display_scenario = main_scenario;
  display_scenario_stats = main_scenario->stats;

  if (dump_sequence_diagram){
    if (print_stats_in_file(stdout, 0, 1)==SCENARIO_NOT_IMPLEMENTED)
      REPORT_ERROR("Scenario command not implemented in display\n");
  }
  if (dump_sequence_diagram || dump_xml)
    exit(0);

  init_default_messages();
  for (int i = 1; i <= users; i++) {
    freeUsers.push_back(i);
    userVarMap[i] = new VariableTable(userVariables);
  }

  if(argiFileName) {
    main_scenario->stats->setFileName(argv[argiFileName]);
  }

  // setup option form cmd line
  call::maxDynamicId   = maxDynamicId;
  call::startDynamicId = startDynamicId;
  call::dynamicId      = startDynamicId;
  call::stepDynamicId  = stepDynamicId;

  /* In which mode the tool is launched ? */
  main_scenario->computeSippMode();
  
  initialize_sockets();
  /* initialize [remote_ip] and [local_ip] keywords for use in <init> section */
  determine_remote_and_local_ip();

  /* Now Initialize the scenarios. */
  main_scenario->runInit();
  ooc_scenario->runInit();


  /* checking if we need to launch the tool in background mode */
  if(backgroundMode == true) {
#ifdef WIN32
    REPORT_ERROR("Background mode not supported on Win32.\n");
#else
    DEBUG("Entering background mode (forking)");
    pid_t l_pid;
    switch(l_pid = fork()) {
    case -1:
      // error when forking !
      REPORT_ERROR_NO("Forking error");
      exit(EXIT_SYSTEM_ERROR);
    case 0:
      // child process - poursuing the execution
      // close all of our file descriptors
    {
      int nullfd = open("/dev/null", O_RDWR);

      dup2(nullfd, fileno(stdin));
      dup2(nullfd, fileno(stdout));
      dup2(nullfd, fileno(stderr));
  // fileclose not CLOSESOCKET
      close(nullfd);
    }
    break;
    default:
      // parent process - killing the parent - the child get the parent pid
      printf("Background mode - PID=[%d]\n", l_pid);
      exit(EXIT_OTHER);
    }
#endif
  }

  usleep(sleeptime * 1000);

  /* Create the statistics reporting task. */
  stattask::initialize();
  /* Create the screen update task. */
  screentask::initialize();
  /* Create a watchdog task. */
  if (watchdog_interval) {
    DEBUG("Watchdog timer intialized.");
    new watchdog(watchdog_interval, watchdog_reset, watchdog_major_threshold, watchdog_major_maxtriggers, watchdog_minor_threshold, watchdog_minor_maxtriggers);
  }

  /* Setting the rate and its dependant params (open_calls_allowed) */
  /* If we are a client, then create the task to open new calls. */
  if (creationMode == MODE_CLIENT) {
    opentask::initialize();
    opentask::set_rate(rate);
  }

  open_connections();

  /* Defaults for media sockets */
  if (media_ip[0] == '\0') {
    strcpy(media_ip, local_ip);
  }
  if (media_ip_escaped[0] == '\0') {
    strcpy(media_ip_escaped, local_ip);
  }
  if (local_ip_is_ipv6) {
    media_ip_is_ipv6 = true;
  } else {
    media_ip_is_ipv6 = false;
  }

  /* retrieve RTP local addr */
  struct addrinfo   hints;
  struct addrinfo * local_addr;

  memset((char*)&hints, 0, sizeof(hints));
  hints.ai_flags  = AI_PASSIVE;
  hints.ai_family = PF_UNSPEC;

  /* Resolving local IP */
  if (getaddrinfo(media_ip,
                  NULL,
                  &hints,
                  &local_addr) != 0) {
    REPORT_ERROR("Unknown RTP address '%s'.\n"
                 "Use 'sipp -h' for details", media_ip);
  }

  memset(&media_sockaddr,0,sizeof(struct sockaddr_storage));
  media_sockaddr.ss_family = local_addr->ai_addr->sa_family;

  memcpy(&media_sockaddr,
         local_addr->ai_addr,
         SOCK_ADDR_SIZE(
           _RCAST(struct sockaddr_storage *,local_addr->ai_addr)));
  freeaddrinfo(local_addr);

  media_port = user_media_port ? user_media_port : DEFAULT_MEDIA_PORT;

  /*  ICMP msgs from os running sipp will occur if sipp is being sent rtp and no port is open                  */
  int create_media_socket = rtp_echo_enabled;
  if (!create_media_socket) {
    if (media_sockaddr.ss_family == AF_INET) {
      (_RCAST(struct sockaddr_in *,&media_sockaddr))->sin_port = htons((short)media_port);
    } else {
      (_RCAST(struct sockaddr_in6 *,&media_sockaddr))->sin6_port = htons((short)media_port);
      media_ip_is_ipv6 = true;
    }
  } else {
    if((media_socket = socket(media_ip_is_ipv6 ? AF_INET6 : AF_INET,
                              SOCK_DGRAM, 0)) == -1) {
      char msg[512];
      sprintf(msg, "Unable to get the audio RTP socket (IP=%s, port=%d)", media_ip, media_port);
      REPORT_ERROR_NO(msg);
    }
    int sock_opt = 1;
    if (setsockopt(media_socket, SOL_SOCKET, SO_REUSEADDR, SETSOCKOPT_TYPE &sock_opt, sizeof (sock_opt)) == -1) {
      REPORT_ERROR_NO("setsockopt(media_socket, SO_REUSEADDR) failed");
    }

    /* create a second socket for video */
    if((media_socket_video = socket(media_ip_is_ipv6 ? AF_INET6 : AF_INET,
                                    SOCK_DGRAM, 0)) == -1) {
      char msg[512];
      sprintf(msg, "Unable to get the video RTP socket (IP=%s, port=%d)", media_ip, media_port+2);
      REPORT_ERROR_NO(msg);
    }
    if (setsockopt(media_socket_video, SOL_SOCKET, SO_REUSEADDR, SETSOCKOPT_TYPE &sock_opt, sizeof (sock_opt)) == -1) {
      REPORT_ERROR_NO("setsockopt(media_socket_video, SO_REUSEADDR) failed");
    }

    int try_counter;
    int max_tries = user_media_port ? 1 : 100;
    for (try_counter = 0; try_counter < max_tries; try_counter++) {

      if (media_sockaddr.ss_family == AF_INET) {
        (_RCAST(struct sockaddr_in *,&media_sockaddr))->sin_port =
          htons((short)media_port);
      } else {
        (_RCAST(struct sockaddr_in6 *,&media_sockaddr))->sin6_port =
          htons((short)media_port);
        media_ip_is_ipv6 = true;
      }
      strcpy(media_ip_escaped, media_ip);

      if(bind(media_socket,
              (sockaddr *)(void *)&media_sockaddr,
              SOCK_ADDR_SIZE(&media_sockaddr)) == 0) {
        break;
      }
      DEBUG("Unable to bind to audio port %hu, try %d of %d", media_port, try_counter, max_tries);

      media_port++;
    }

    if (try_counter >= max_tries) {
      char msg[512];
      sprintf(msg, "Unable to bind audio RTP socket (IP=%s, port=%d)", media_ip, media_port);
      REPORT_ERROR_NO(msg);
    }
    DEBUG("Bound audio socket %s:%hu, after %d attempts", media_ip, media_port, try_counter);

    /*---------------------------------------------------------
       Bind the second socket to media_port+2
       (+1 is reserved for RTCP)
    ----------------------------------------------------------*/

    if (media_sockaddr.ss_family == AF_INET) {
      (_RCAST(struct sockaddr_in *,&media_sockaddr))->sin_port =
        htons((short)media_port+2);
      strcpy(media_ip_escaped, media_ip);
    } else {
      (_RCAST(struct sockaddr_in6 *,&media_sockaddr))->sin6_port =
        htons((short)media_port+2);
      media_ip_is_ipv6 = true;
      strcpy(media_ip_escaped, media_ip);
    }

    if(bind(media_socket_video,
            (sockaddr *)(void *)&media_sockaddr,
            SOCK_ADDR_SIZE(&media_sockaddr))) {
      char msg[512];
      sprintf(msg, "Unable to bind video RTP socket (IP=%s, port=%d)", media_ip, media_port+2);
      REPORT_ERROR_NO(msg);
    }
    DEBUG("Bound video socket %s:%hu", media_ip, media_port+2);
    /* Second socket bound */
  }

  /* Creating the remote control socket thread */
  setup_ctrl_socket();
  if (!nostdin) {
    setup_stdin_socket();
  }

  if ((media_socket > 0) && (rtp_echo_enabled)) {
    if (pthread_create
        (&pthread2_id,
         NULL,
         (void *(*)(void *)) rtp_echo_thread,
         (void*)&media_socket)
        == -1) {
      REPORT_ERROR_NO("Unable to create RTP echo thread");
    }
  }


  /* Creating second RTP echo thread for video */
  if ((media_socket_video > 0) && (rtp_echo_enabled)) {
    DEBUG("RTP echo enabled: starting rtp_echo_thread");
    if (pthread_create
        (&pthread3_id,
         NULL,
         (void *(*)(void *)) rtp_echo_thread,
         (void*)&media_socket_video)
        == -1) {
      REPORT_ERROR_NO("Unable to create second RTP echo thread");
    }
  }

  // this allows us to
  //  throw runtime_error("text for report_error");
  // if we include <stdexcept> in throwing file. This can help
  // us remove dependancy on screen just to call REPORT_ERROR
  try {
    traffic_thread();
  }catch (exception& e)
  { 
    cleanup_sockets();
    REPORT_ERROR(e.what());
  }
  // no finally clause in c++
  cleanup_sockets(); 

  if (scenario_file != NULL) {
    delete [] scenario_file ;
    scenario_file = NULL ;
  }

}

void reset_connection(struct sipp_socket *socket)
{
  DEBUGIN();
  if (!reconnect_allowed()) {
    REPORT_ERROR_NO("Max number of reconnections reached");
  }

  if (reset_number != -1) {
    reset_number--;
  }

  if (reset_close) {
    WARNING("Closing calls, because of TCP reset or close!");
    close_calls(socket);
  }

  /* Sleep for some period of time before the reconnection. */
  usleep(1000 * reset_sleep);

  if (sipp_reconnect_socket(socket) < 0) {
    WARNING_NO("Could not reconnect TCP socket");
    close_calls(socket);
  } else {
    WARNING("Socket required a reconnection.");
  }
  DEBUGOUT();
}

/* Close just those calls for a given socket (e.g., if the remote end closes
 * the connection. */
void close_calls(struct sipp_socket *socket)
{
  DEBUGIN();
  owner_list *owners = get_owners_for_socket(socket);
  owner_list::iterator owner_it;
  socketowner *owner_ptr = NULL;

  for (owner_it = owners->begin(); owner_it != owners->end(); owner_it++) {
    owner_ptr = *owner_it;
    if(owner_ptr) {
      owner_ptr->tcpClose();
    }
  }

  delete owners;
  DEBUGOUT();
}








bool is_a_local_socket(struct sipp_socket *s)
{
  for (int i = 0; i< local_nb + 1; i++) {
    if(local_sockets[i] == s) return true;
  }
  return (false);
}



