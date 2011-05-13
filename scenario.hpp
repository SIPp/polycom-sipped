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
 *           Olivier Jacques
 *           From Hewlett Packard Company.
 *           Peter Higginson
 *           JPeG
 *           Guillaume TEISSIER from FTR&D
*/

#ifndef __SCENARIO__
#define __SCENARIO__

#include <map>
#include <sys/socket.h>
#include <string.h>
#include "actions.hpp"
#include "variables.hpp"
#include "message.hpp"
#include "stat.hpp"


#define MSG_TYPE_SENDCMD   0
#define MSG_TYPE_RECVCMD   1

#define MSG_TYPE_SEND      2
#define MSG_TYPE_RECV      3
#define MSG_TYPE_PAUSE     4
#define MSG_TYPE_NOP       5

#define MODE_CLIENT        0
#define MODE_SERVER        1

#define MODE_3PCC_NONE		0
#define MODE_3PCC_CONTROLLER_A  2
#define MODE_3PCC_CONTROLLER_B  3   
#define MODE_3PCC_A_PASSIVE     4

/* 3pcc extended mode*/
#define MODE_MASTER             5 
#define MODE_MASTER_PASSIVE     6
#define MODE_SLAVE              7

#define OPTIONAL_TRUE      1
#define OPTIONAL_FALSE     0
#define OPTIONAL_GLOBAL    2

class message {

public:
  /* If this is a pause */
  CSample        *pause_distribution;
  int		 pause_variable;
  /* This string is used for the display screen. */
  char		 *pause_desc;
  /* Is this a final pause, intended for catching retransmissions? */
  bool		timewait;

  /* Number of sessions in a pause */
  int            sessions; 

  /* should collect route set? */
  bool           bShouldRecordRoutes;

#ifdef _USE_OPENSSL
  /* should collect authentication info? */
  bool           bShouldAuthenticate;
#endif

  /* If this is a send */
  SendingMessage *send_scheme;
  unsigned int   retrans_delay;
  /* The receive/send timeout. */
  unsigned int   timeout;

 /* 3pcc extended mode: if this is a sendCmd */
  char         * peer_dest;

 /* 3pcc extended mode: if this is a recvCmd */
  char         * peer_src;

  /* If this is a recv */
  int   	       recv_response;
  char         * recv_request;
  int            optional;
  bool           advance_state;
  int            regexp_match;
  regex_t      * regexp_compile;

  /* Anyway */
  int            start_rtd;
  int            stop_rtd;
  bool           repeat_rtd;
  int            counter;
  double         lost;
  int            crlf;
  bool           hide;
  char         * display_str;
  int            next;
  char         * nextLabel;
  int            test;
  int            condexec;
  bool           condexec_inverse;
  int            chance;/* 0=always, RAND_MAX+1=never (test rand() >= chance) */
  int            on_timeout;
  char         * onTimeoutLabel;

  /* user can specify messages as belonging to a particular dialog via the dialog="n" attribute */
  /* default to -1, if set, state (cseq, call-id, to, from, etc) will be based only on messages with same id set */
	int            dialog_number; 

  /* Statistics */
  unsigned long   nb_sent;
  unsigned long   nb_recv;
  unsigned long   nb_sent_retrans;
  unsigned long   nb_recv_retrans;
  unsigned long   nb_timeout;
  unsigned long   nb_unexp;
  unsigned long   nb_lost;

  CActions*       M_actions;

  int             M_type;

  SendingMessage *M_sendCmdData;
  unsigned long   M_nbCmdSent;
  unsigned long   M_nbCmdRecv;

  typedef enum {
      ContentLengthNoPresent = 0,
      ContentLengthValueZero,
      ContentLengthValueNoZero
  }ContentLengthFlag;
  
  ContentLengthFlag   content_length_flag ;

  // *ALL* possible methods are appended to this string for matching when a response is received.
  char           *recv_response_for_cseq_method_list; 


/*
  // A non-zero value in *_txn indicates that the corresponding *_txn attribute was specified for this message. 
  // The value is the index into call::transactions and scenario::transactions, as returned via get_txn_index(name)
  int            start_txn;     // non-zero means store state associated with message as it is sent or received
  int            use_txn;       // non-zero means verify state on recv and use saved state for keywords on send
  int            response_txn;  // non-zero means received response packet
  int            ack_txn;
*/
  int            index;
  const char *         desc;
  message(int index, const char *desc);
  ~message();

  bool isSend() { return M_type == MSG_TYPE_SEND; }
  bool isRecv() { return M_type == MSG_TYPE_RECV; }
  bool isResponse() { return isSend() ? send_scheme->isResponse() : (recv_response != 0); }
  bool isRequest() { return !isResponse(); }
  bool isCancel() { return isRequest() && (isSend() ? send_scheme->isCancel() : !strcmp(recv_request, "CANCEL")); }
  bool isAck() { return isRequest() && (isSend() ? send_scheme->isAck() : !strcmp(recv_request, "ACK")); }

  void startTransaction(const string &name) { start_txn = true; txn_name = checkTransactionName(name); }
  void useTransaction(const string &name) { start_txn = false; txn_name = checkTransactionName(name); };

  string getTransactionName() { return txn_name; }
  bool isStartTxn()  { return start_txn; }
  bool isUseTxn()    { return !start_txn && !txn_name.empty(); }


private:
  bool           start_txn;          // true means store state associated with message as it is sent or received
  string         txn_name;           // transaction name (empty if none defined). Note that non-empty txn_name does not imply use_txn as it may also by start_txn.

  // return txnName if valid name, abort with error if not
  const string &checkTransactionName(const string &txnName);

};

typedef std::vector<message *> msgvec;

/*
Gone: transactions are now only tracked on a per-dialog basis within call

struct txnControlInfo {
  char *name;        // name as passed to the start_txn, ack_txn or response_txn parameter
  bool isInvite;

  // counters for how many times get_txn_index() is called for with corresponding flag set.
  // used by validate_txn_usage() to ensure transactions start and finish as required.
  int acks;      // counts number of acked transactions (sent only)
  int started;   // counts number of started transactions (sent or received)
  int responses; // count number of received responses in this transaction (received only)
};
typedef std::vector<txnControlInfo> txnvec;
*/

class scenario {
public:
  scenario(char * filename, int deflt, int dumpxml);
  ~scenario();

  void runInit();

	/* One message for each command in XML file (ie <send>, <recv>, <nop>, etc*/
  msgvec messages;
  msgvec initmessages;
  char *name;
  int duration;

/* Gone: transactions are now only tracked on a per-dialog basis within call
  // stores transaction name and tracks number of times get_txn_index() is called on it.
  txnvec transactions;
*/
  int unexpected_jump;
  int retaddr;
  int pausedaddr;

  void computeSippMode();

  int get_var(const char *varName, const char *what);
  int get_counter(const char *varName, const char *what);
  int get_rtd(const char *ptr, bool start);
  int find_var(const char *varName, const char *what);

  CStat *stats;
  AllocVariableTable *allocVars;

private:

  /* The mapping of labels to IDs. */
  str_int_map labelMap;
  str_int_map initLabelMap;

/* Gone: transactions are now only tracked on a per-dialog basis within call
  /* map transaction names to index in call::transactions & scenario::transactions vector/array * /
  str_int_map txnMap;
*/

  bool found_timewait;

  void getBookKeeping(message *message);
  void getCommonAttributes(message *message);
  void getActionForThisMessage(message *message);
  void parseAction(CActions *actions, int dialog_number);
  void handle_arithmetic(CAction *tmpAction, const char *what);
  void handle_rhs(CAction *tmpAction, const char *what);
  void checkOptionalRecv(char *elem, unsigned int scenario_file_cursor);

  void apply_labels(msgvec v, str_int_map labels);
  void validate_variable_usage();
//  void validate_txn_usage(); Gone: transactions are now only tracked on a per-dialog basis within call



/* Gone: transactions are now only tracked on a per-dialog basis within call
  // Return the index to txnName, creating it if it does not already exist.
  int get_txn_index(const char *txnName, const char *what, bool start, bool isInvite, bool isAck);
*/

  int xp_get_var(const char *name, const char *what);
  int xp_get_var(const char *name, const char *what, int defval);

  bool hidedefault;
  bool last_recv_optional;
};

/* There are external variable containing the current scenario */
extern scenario      *main_scenario;
extern scenario      *ooc_scenario;
extern scenario      *display_scenario;
extern int           creationMode;
extern int           sendMode;
extern int           thirdPartyMode;

extern message::ContentLengthFlag  content_length_flag;

void load_scenario(char * filename, 
                   int    deflt);

/* 3pcc extended mode */
void parse_slave_cfg();

void getActionForThisMessage();
CSample *parse_distribution(bool oldstyle);
int  createIntegerTable(char          * P_listeStr, 
                        unsigned int ** listeInteger, 
                        int           * sizeOfList);

int  isWellFormed(char * P_listeStr, 
                  int  * nombre);

/* String table functions. */
int createStringTable(char * inputString, char *** stringList, int *sizeOfList);
void freeStringTable(char ** stringList, int sizeOfList);



int find_scenario(const char *scenario);
extern char * default_scenario[11];

/* Useful utility functions for parsing integers, etc. */
long get_long(const char *ptr, const char *what);
unsigned long long get_long_long(const char *ptr, const char *what);
long get_time(const char *ptr, const char *what, int multiplier);
double get_double(const char *ptr, const char *what);
bool get_bool(const char *ptr, const char *what);
int time_string(double ms, char *res, int reslen);
int get_var(const char *varName, const char *what);

extern int get_cr_number(const char *msg);

#endif
