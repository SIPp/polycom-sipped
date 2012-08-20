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
 *           From Hewlett Packard Company.
 *       Charles P. Wright from IBM Research
 *           Andy Aicken
 */

#ifndef __CALL__
#define __CALL__

#include <map>
#include <set>

#ifdef WIN32
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <process.h>
#include <pthread.h>
#else
#include <sys/socket.h>
#ifdef __CYGWIN
#include <stdio.h>
#include <process.h>
#else
#include <spawn.h>
#endif
#endif

#ifdef _USE_OPENSSL
#include "sslcommon.hpp"
#endif
#ifdef PCAPPLAY
#include "send_packets.hpp"
#endif

#include "dialogstate.hpp"
#include "listener.hpp"
#include "scenario.hpp"
#include "socketowner.hpp"
#include "task.hpp"
// directly referenced but by covered by above
#include "actions.hpp"
#include "message.hpp"
#include "stat.hpp"
#include "transactionstate.hpp"
#include "variables.hpp"

#define MAX_SUB_MESSAGE_LENGTH  2049
#define SIP_TRANSACTION_TIMEOUT 32000

#define MAXIMUM_NUMBER_OF_RTP_MEDIA_THREADS 20

/* For keeping track of short and long form header */
#define MAX_HEADER_NAME_LEN  18
#define NUM_OF_SHORT_FORM_HEADERS 10

/* to ensure the buffer for encoding is as long as the longest possible encoded message */
#define ENCODE_LEN_PER_CHAR 3

/* the arguments to spawn are cmd.exe or sh and /C or -c respectively, the command you want to run, and lastly a NULL character */
#define NUM_ARGS_FOR_SPAWN 4

#ifdef __HPUX
extern int createAuthHeader(char * user, char * password, char * method, char * uri, char * msgbody, char * auth, char * aka_OP, char * aka_AMF, char * aka_K, char * result);
#else
extern int createAuthHeader(char * user, char * password, char * method, char * uri, char * msgbody, char * auth, char * aka_OP, char * aka_AMF, char * aka_K, char * result);
int verifyAuthHeader(char * user, char * password, char * method, char * auth);
#endif



typedef std::map<int, DialogState*> perDialogStateMap;

class call : virtual public task, virtual public listener, public virtual socketowner {
public:
  // These are wrappers for various circumstances, (private) init does the real work.
  call(char *p_id, bool use_ipv6, int userId, struct sockaddr_storage *dest);
  call(char *p_id, struct sipp_socket *socket, struct sockaddr_storage *dest);
  static call *add_call(int userId, bool ipv6, struct sockaddr_storage *dest);
  call(scenario * call_scenario, struct sipp_socket *socket, struct sockaddr_storage *dest, const char * p_id, int userId, bool ipv6, bool isAutomatic, bool isInitCall);

  virtual ~call();

  virtual bool process_incoming(char * msg, struct sockaddr_storage *src = NULL, struct sipp_socket *socket = NULL);
  virtual bool  process_twinSippCom(char * msg);

  virtual bool run();
  /* Terminate this call, depending on action results and timewait. */
  virtual void terminate(CStat::E_Action reason);
  virtual void tcpClose();

  /* When should this call wake up? */
  virtual unsigned int wake();
  virtual bool  abortCall(bool writeLog); // call aborted with BYE or CANCEL
  virtual void abort();

  /* Dump call info to error log. */
  virtual void dump();

  /* Automatic */
  enum T_AutoMode {
    E_AM_DEFAULT,
    E_AM_UNEXP_BYE,
    E_AM_UNEXP_CANCEL,
    E_AM_PING,
    E_AM_AA,
    E_AM_AA_REGISTER,
    E_AM_OOCALL,
  };

  // set last message for default, per-dialog(if dialog_number) and (if msg_index specified) per-transaction structures.
  void setLastMsg(const string &msg, int dialog_number=-1, int message_index=-1);

  bool  automaticResponseMode(T_AutoMode P_case, char* P_recv);

private:
  /* This is the core constructor function. */
  void init(scenario * call_scenario, struct sipp_socket *socket, struct sockaddr_storage *dest, const char * p_id, int userId, bool ipv6, bool isAutomatic, bool isInitCall);
  /* This this call for initialization? */
  bool initCall;

  /* compute call_id based on specified % formatted string */
  static void compute_id(char *call_id, int length);

  struct sockaddr_storage call_peer;

  scenario *call_scenario;
  unsigned int   number;

  bool loose_message_sequence;
  unsigned int get_last_insequence_received_message(int search_from_msg_index);



public:
  static   int   maxDynamicId;    // max value for dynamicId; this value is reached !
  static   int   startDynamicId;  // offset for first dynamicId  FIXME:in CmdLine
  static   int   stepDynamicId;   // step of increment for dynamicId
  static   int   dynamicId;       // a counter for general use, incrementing  by  stepDynamicId starting at startDynamicId  wrapping at maxDynamicId  GLOBALY
private:


  unsigned int   tdm_map_number;

  int   msg_index;
  int   zombie;

  /* store state associated with most recent messages */
  DialogState          *last_dialog_state;
  /* store state by dialog (call="n" parameter) */
  /* call-id verification is performed */
  perDialogStateMap    per_dialog_state;

  /* Last message sent from scenario step (retransmitions do not
   * change this index. Only message sent from the scenario
   * are kept in this index.) */
  int             last_send_index;
  char          * last_send_msg;
  int             last_send_len;

  /* How long until sending this message times out. */
  unsigned int    send_timeout;

  /* Last received message (expected,  not optional, and not
   * retransmitted) and the associated hash. Stills setted until a new
   * scenario steps sends a message */
  unsigned long    last_recv_hash;
  int              last_recv_index;
  typedef std::pair<unsigned long, int> hash_msgindex_pair;
  std::set<hash_msgindex_pair> recvhash_msgindex_pairs;

  /* Recv message characteristics when we sent a valid message
   *  (scneario, no retrans) just after a valid reception. This was
   * a cause relationship, so the next time this cookie will be recvd,
   * we will retransmit the same message we sent this time */
  unsigned long  recv_retrans_hash;
  int   recv_retrans_recv_index;
  int   recv_retrans_send_index;
  unsigned int   recv_timeout;

#ifdef PCAPPLAY
  int hasMediaInformation;
  pthread_t* media_threads[MAXIMUM_NUMBER_OF_RTP_MEDIA_THREADS];
  int number_of_active_rtp_threads;
  //play_args_t play_args_a;
  //play_args_t play_args_v;
  // the changes:
  vector<play_args_t> play_args_audio;
  vector<play_args_t> play_args_video;
  vector<play_args_t> play_args_application;
  void set_audio_from_port(int port, unsigned int index=1);
  void set_video_from_port(int port, unsigned int index=1);
  void set_application_from_port(int port, unsigned int index=1);

 // void set_audio_from_port(int port);
 // void set_video_from_port(int port);
#endif


#ifdef _USE_OPENSSL
  /* holds the auth header and if the challenge was 401 or 407 */
  char         * dialog_authentication;
  int            dialog_challenge_type;
#endif

  unsigned int   next_retrans;
  int      nb_retrans;
  unsigned int   nb_last_delay;

  unsigned int   paused_until;

  unsigned long  start_time;
  unsigned long long *start_time_rtd;
  bool           *rtd_done;

  struct sipp_socket *call_remote_socket;
  int            call_port;

  void         * comp_state;

  int            deleted;

  bool           call_established; // == true when the call is established
  // ie ACK received or sent
  // => init to false
  bool           ack_is_pending;   // == true if an ACK is pending
  // Needed to avoid abortCall sending a
  // CANCEL instead of BYE in some extreme
  // cases for 3PCC scenario.
  // => init to false

  /* Call Variable Table */
  VariableTable *M_callVariableTable;

  /* result of execute action */
  enum T_ActionResult {
    E_AR_NO_ERROR = 0,
    E_AR_REGEXP_DOESNT_MATCH,
    E_AR_REGEXP_SHOULDNT_MATCH,
    E_AR_STOP_CALL,
    E_AR_CONNECT_FAILED,
    E_AR_HDR_NOT_FOUND
  };

  /* Store the last action result to allow  */
  /* call to continue and mark it as failed */
  T_ActionResult last_action_result;

  /* rc == true means call not deleted by processing */
  void formatNextReqUrl (char* next_req_url);
  void computeRouteSetAndRemoteTargetUri (char* rrList, char* contact, bool bRequestIncoming, DialogState *ds);
  bool matches_scenario(unsigned int index, int reply_code, char * request, char * responsecseqmethod, char *branch, string &call_id, char *reason);

  bool executeMessage(message *curmsg);
  T_ActionResult executeAction(char * msg, message *message);
  void  extractSubMessage(char * msg, char * matchingString, char* result, bool case_indep,
                          int occurrence, bool headers);
  bool  rejectCall();
  double get_rhs(CAction *currentAction);


// *** methods that operate on default or specfied msg_index ***

  // Return true if use_txn specified for currently indexed message
  bool use_txn(int index=-1);

  string txn_name(int index=-1);

  // returns TransactionState appropriate for the indexed message (defaulting to msg_index if none specified).
  // Also checks that transaction has been used and aborts with an error if it has not been.
//  TransactionState &get_txn(int index=-1);

//  void setLastReceivedMessage(const string &msg, int index=-1); is named setLastMsg ; maybe need to move it!
//  const string &getLastReceivedMessage(int index=-1) const;

  // return last message regardless of dialog or transaction
  const string &getDefaultLastReceivedMessage();

  void verifyIsServerTransaction(TransactionState &txn, const string &wrongKeyword, const string &correctKeyword);
  void verifyIsClientTransaction(TransactionState &txn, const string &wrongKeyword, const string &correctKeyword);

  // P_index use for message index in scenario and ctrl of CRLF
  // P_index = -2 No ctrl of CRLF
  // P_index = -1 Add crlf to end of message
  char* createSendingMessage(SendingMessage *src, int P_index, int *msgLen=NULL);
  char* createSendingMessage(char * src, int P_index, bool skip_sanity = false);
  char* createSendingMessage(SendingMessage *src, int P_index, char *msg_buffer, int buflen, int *msgLen=NULL);

  // method for the management of unexpected messages
  bool  checkInternalCmd(char* cmd);  // check of specific internal command
  // received from the twin socket
  // used for example to cancel the call
  // of the third party
  bool  check_peer_src(char* msg,
                       int search_index);    // 3pcc extended mode:check if
  // the twin message received
  // comes from the expected sender
  void   sendBuffer(char *buf, int len = 0);     // send a message out of a scenario
  // execution

  T_AutoMode  checkAutomaticResponseMode(char * P_recv);

  int   sendCmdMessage(message *curmsg); // 3PCC

  int   sendCmdBuffer(char* cmd); // for 3PCC, send a command out of a
  // scenario execution

  static void readInputFileContents(const char* fileName);
  static void dumpFileContents(void);

  void getFieldFromInputFile(const char* fileName, int field, SendingMessage *line, char*& dest);

  /* Associate a user with this call. */
  void setUser(int userId);

  /* Is this call just around for final retransmissions. */
  bool timewait;

  /* rc == true means call not deleted by processing */
  bool next();
  bool process_unexpected(char * msg, const char *reason);
  void do_bookkeeping(message *curmsg);

  void  extract_cseq_method (char* responseCseq, char* msg);
  string extract_cseq_method (char* msg);

  void  extract_branch (char* branch, const char* msg);  // copy Via's branch attribute from msg into branch
  string extract_branch (const char* msg);

  int   extract_name_and_uri (char* uri, char* name_and_uri, char* msg, const char *name);

  int   send_raw(char * msg, int index, int len);
  char * send_scene(int index, int *send_status, int *msgLen);
  bool   connect_socket_if_needed();

  char * get_header_field_code(char * msg, char * code);
  char * get_last_header(const char * name, const char *msg, bool valueOnly);

  /* only return payload of the header (not the 'header:' bit) */
  char * get_header_content(const char *message, const char * name);
  char * get_header_content(char* message, const char * name);

  /* If content is true, we only return the header's contents. */
  char * get_header(const char* message, const char * name, bool content);
  char * get_header(char* message, const char * name, bool content);
  const char * swap_long_and_short_form_header(const char* name);
  char * get_first_line(char* message);
  string get_last_request_uri(const char *last_recv_msg);
  unsigned long hash(char * msg);
  int get_dialog_fr_callid(string callid);

//headers with short forms.
  enum T_compactHeaders {
    E_CALL_ID_SHORT_FORM,
    E_CONTACT_SHORT_FORM,
    E_CONTENT_ENCODING_SHORT_FORM,
    E_CONTENT_LENGTH_SHORT_FORM,
    E_CONTENT_TYPE_SHORT_FORM,
    E_FROM_SHORT_FORM,
    E_SUBJECT_SHORT_FORM,
    E_SUPPORTED_SHORT_FORM,
    E_TO_SHORT_FORM,
    E_VIA_SHORT_FORM,
    E_CALL_ID_LONG_FORM,
    E_CONTACT_LONG_FORM,
    E_CONTENT_ENCODING_LONG_FORM,
    E_CONTENT_LENGTH_LONG_FORM,
    E_CONTENT_TYPE_LONG_FORM,
    E_FROM_LONG_FORM,
    E_SUBJECT_LONG_FORM,
    E_SUPPORTED_LONG_FORM,
    E_TO_LONG_FORM,
    E_VIA_LONG_FORM,
  };

  typedef std::map <std::string, int> file_line_map;
  file_line_map *m_lineNumber;
  int    userId;

  bool   use_ipv6;

  void   get_remote_media_addr(char * message);

  bool lost(int index);

  void computeStat (CStat::E_Action P_action);
  void computeStat (CStat::E_Action P_action, unsigned long P_value);
  void computeStat (CStat::E_Action P_action, unsigned long P_value, int which);


  void queue_up(char *msg);
  char *queued_msg;


#ifdef _USE_OPENSSL
  SSL_CTX   *m_ctx_ssl ;
  BIO       *m_bio     ;
#endif

  int _callDebug(const char *fmt, ...);
  char *debugBuffer;
  int debugLength;

  // DialogState helper routines

  // Returns dialog state associated with dialog_number.
  // If no state exists for dialog_number, a new entry is created.
  DialogState *get_dialogState(int dialog_number);

  // Releases all memory allocated to store dialog state
  void free_dialogState();

};


/* Default Message Functions. */
void init_default_messages();
void free_default_messages();
SendingMessage *get_default_message(const char *which);
void set_default_message(const char *which, char *message);


enum T_encoding {
  E_ENCODING_NONE,
  E_ENCODING_URI
};

const char *encode_as_needed(const char *str, struct MessageComponent *comp);
void encode(struct MessageComponent *comp, const char *src, char *dest);
void uri_encode(const char *src, char *dest);
bool is_reserved_char(char c);
int countArguments(char* args);
void setArguments(char* args, char** argv);

string get_set_of_problematic_optional_messages(scenario * call_scenario);
string remove_ipv6_brackets_if_present(char* ip);


#endif
