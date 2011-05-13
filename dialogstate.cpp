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
 *
 *  Author : Edward Estabrook - 13 April 2011
 *           From Polyom Inc.
 *
 */

#include <string.h>
#include <map>

#include "transactionstate.hpp"
#include "dialogstate.hpp"
#include "sipp.hpp"


DialogState::DialogState(unsigned int base_cseq, const string &call_id) : call_id(call_id), 
                         client_cseq(base_cseq), server_cseq(0), peer_tag(0), local_tag(0), 
                         dialog_route_set(0), next_req_url(0), default_txn("") 
{
  client_cseq_method[0] = '\0';
  server_cseq_method[0] = '\0';
  contact_name_and_uri[0] = 0;
  contact_uri[0] = 0;
  to_name_and_uri[0] = 0;
  to_uri[0] = 0;
  from_name_and_uri[0] = 0;
  from_uri[0] = 0;
};

DialogState::~DialogState() 
{ 
  if(peer_tag) free(peer_tag);
  if(local_tag) free(local_tag);

  if(dialog_route_set) free(dialog_route_set);
  if(next_req_url) free(next_req_url);
}

// return the transaction associated with this name or the default transaction if name is empty.
TransactionState &DialogState::get_transaction(const string &name, int msg_index)
{
  if (name.empty())
    return default_txn;
  else {
    Name_Transaction_Map::iterator txn = transactions.find(name);
    if (txn == transactions.end()) {
      ERROR("Message %d is attempting to use transaction '%s' prior to it being started with start_txn (transaction not found).", msg_index, name.c_str()); 
    } 
    else if (txn->second.getBranch().empty()) {
      ERROR("Message %d is attempting to use transaction '%s' but aborting because the branch is empty (an invalid SIP message was associated with the start_txn message).", msg_index, name.c_str()); 
    }
    DEBUG("Found %s", txn->second.trace().c_str());

    return txn->second;
  }
}

// return the transaction associated with this name, creating it if non-existant
TransactionState &DialogState::create_transaction(const string &name)
{
  pair<Name_Transaction_Map::iterator,bool> ret;
  ret = transactions.insert(pair<Name_Transaction_Map::key_type,TransactionState>(Name_Transaction_Map::key_type(name), TransactionState(name)));
  TransactionState &txn = ret.first->second; //transactions[name];
  DEBUG("Transaction %s created", name.c_str());
  return txn;
}

// set last message, from transaction or default if none specified. msg_index is used for error reporting only
void DialogState::setLastReceivedMessage(const string &msg, const string &name, int msg_index){
  DEBUG_IN("name = '%s' Message Length = %d", name.c_str(), msg.length());
  
  // set default transactions's last message
  {
    TransactionState &txn = get_transaction("", msg_index);
    txn.setLastReceivedMessage(msg);
  }

  // set per-transaction message
  if (!name.empty()) {
    TransactionState &txn = get_transaction(name, msg_index);
    txn.setLastReceivedMessage(msg);
    DEBUG("Set last message for transaction %s", name.c_str());
  }
}

// get last message, from transaction or default if none specified. msg_index is used for error reporting only
const string &DialogState::getLastReceivedMessage(const string &name, int msg_index) {
  DEBUG_IN("name = %s", name.c_str());
  TransactionState &txn = get_transaction(name, msg_index);
  return txn.getLastReceivedMessage();
}

