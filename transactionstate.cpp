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
#include <iostream>
#include <sstream>

#include "logging.hpp"
#include "transactionstate.hpp"


TransactionState::TransactionState(const string &name) : name(name), branch(""), lastReceivedMessage(""), 
                                   cseq(0), cseqMethod(""), client(false), lastResponseCode(0), transactionResponseHash(0), ackIndex(0)
{ 
  DEBUG_IN();
}

TransactionState::~TransactionState() 
{ 
  DEBUG_IN();
}


void TransactionState::start(bool pIsClient, const string &pBranch, int pCseq, const string &pCseqMethod) 
{
  client = pIsClient;
  branch = pBranch;
  ackBranch.erase();
  lastReceivedMessage = "";
  cseq = pCseq;
  cseqMethod = pCseqMethod;
  transactionResponseHash = 0;
  ackIndex = 0;
  DEBUG("%s", trace().c_str());
}

string TransactionState::trace() const {
  std::stringstream ss;
  ss << (client ? "Client" : "Server") << " Transaction '" << name << 
                          "': branch:'" << branch << "'; cseq: " << cseq << " " << cseqMethod << 
                          " ; lastResponseCode: " << lastResponseCode << "; transactionResponseHash: " << transactionResponseHash <<
                          " ; ackIndex: " << ackIndex << "; lastReceivedMessage: " << lastReceivedMessage.length();
  return ss.str();
}

void TransactionState::startClient(const string &branch, int cseq, const string &cseq_method) { 
  start(true, branch, cseq, cseq_method);
}

void TransactionState::startServer(const string &branch, int cseq, const string &cseq_method) { 
  start(false, branch, cseq, cseq_method);
}

