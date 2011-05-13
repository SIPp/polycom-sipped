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
 *  Author : Edward Estabrook - 13 April 2011
 *           From Polyom Inc.
 *
 */
#ifndef __DIALOGSTATE__
#define __DIALOGSTATE__

#include <map>
#include <list>
#include <string>
#include "common.hpp"
#include "transactionstate.hpp"

using namespace std;

/* map transaction names to transactions */
typedef std::map<std::string, TransactionState> Name_Transaction_Map;


// Used by call to store state which must be tracekd on a per-callid basis
class DialogState {

public:
  DialogState(unsigned int base_cseq, const string &call_id="");

  ~DialogState();

  // return the transaction associated with this name
  TransactionState &get_transaction(const string &name, int msg_index);

  // return the transaction associated with this name, creating it if non-existant
  TransactionState &create_transaction(const string &name);

  // set last message, from transaction or default if none specified. msg_index is used for error reporting only
  void setLastReceivedMessage(const string &msg, const string &name, int msg_index=-1);

  // get last message, from transaction or default if none specified. msg_index is used for error reporting only
  const string &getLastReceivedMessage(const string &name, int msg_index=-1);


public:
  string         call_id;

  // cseq value for [cseq] keyword, useful for generating requests; auto-incremented
  unsigned int   client_cseq;

  // cseq_method is the cseq method in the last request this dialog sent
  char           client_cseq_method[MAX_HEADER_LEN];

  // server_* (received_*) - updated on recv of request and used for generating responses in cases where [last_cseq] won't do.
  unsigned int   server_cseq;
  char           server_cseq_method[MAX_HEADER_LEN];

/*
  // Last received message (expected, not optional, and not retransmitted)
  char           * last_recv_msg;
*/

  // remote tag, populated by incoming responses. Referenced by [peer_tag], [remote_tag], [remote_tag_param]
  char           * peer_tag;

  // local tag, populated by incoming responses. Referenced by [local_tag], [local_tag_param]
  char           * local_tag;

  // contact header, populated by incoming responses. Referenced by [contact_uri], [contact_name_and_uri]
  char           contact_name_and_uri[MAX_HEADER_LEN];
  char           contact_uri[MAX_HEADER_LEN];

  // to header, populated by incoming responses. Referenced by [to_uri], [to_name_and_uri]
  char           to_name_and_uri[MAX_HEADER_LEN];
  char           to_uri[MAX_HEADER_LEN];

  // from header, populated by incoming responses. Referenced by [from_uri], [from_name_and_uri]
  char           from_name_and_uri[MAX_HEADER_LEN];
  char           from_uri[MAX_HEADER_LEN];

  // holds the route set 
  char           * dialog_route_set;
  char           * next_req_url; // (contact header)

private:
  // vector containing transactions associated with this dialog, indexed by name, created as encountered
  Name_Transaction_Map transactions;

  // contains transaction-related state for messages that do not specify a specific transaction
  TransactionState default_txn;

};

#endif
