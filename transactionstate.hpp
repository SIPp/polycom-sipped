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

#ifndef __TRANSACTIONSTATE__
#define __TRANSACTIONSTATE__

#include <string>

using namespace std;

class TransactionState {
public:
  TransactionState(const string &name);

  ~TransactionState();

  string trace() const;

  void startClient(const string &branch, int cseq, const string &cseq_method);
  void startServer(const string &branch, int cseq, const string &cseq_method);

  bool isClientTransaction() const {
    return client;
  }
  bool isServerTransaction() const {
    return !client;
  }

  string getName() const {
    return name;
  }

  string getBranch() const {
    return branch;
  }

  int getCseq() const {
    return cseq;
  }

  string getCseqMethod() const {
    return cseqMethod;
  }

  void setAckBranch(const string &newAckBranch) {
    ackBranch = newAckBranch;
  }
  const string &getAckBranch() const {
    return ackBranch;
  }

  void setLastReceivedMessage(const string &msg) {
    lastReceivedMessage = msg;
  }
  const string &getLastReceivedMessage() const {
    return lastReceivedMessage;
  }

  void setLastResponseCode(int responseCode) {
    lastResponseCode = responseCode;
  }
  int getLastResponseCode() {
    return lastResponseCode;
  }
  bool isLastResponseCode2xx() {
    return ((lastResponseCode >= 200) && (lastResponseCode <= 299));
  }

  void setAckIndex(int newAckIndex) {
    ackIndex = newAckIndex;
  }
  int getAckIndex() {
    return ackIndex;
  }

  void setTransactionHash(unsigned long hash) {
    transactionResponseHash = hash;
  }
  unsigned long getTransactionHash() {
    return transactionResponseHash;
  }


private:
  // transaction name, used only for trace ouput
  string name;

  // branch for transaction. Remains the same even after different branch assocaited with 2xx-ACK sent.
  string branch;

  // branch for ACK transaction
  string ackBranch;

  // true if client (sent) transaction, false if server (received)
  bool   client;

  // cseq & method for this transaction.  May be correlate with client or server CSEQ values depending on 'client'
  int    cseq;
  string cseqMethod;

  // Last received message in this transaction (expected, not optional, and not retransmitted)
  string lastReceivedMessage;

  // stores last response code sent or received in this transaction
  int lastResponseCode;

  // hash of msg as computed in process_incoming() and used for certain retransmissions when not in mc mode.
  unsigned long transactionResponseHash;

  // index of the message that sent most recent ack in this transaction. Non-zero indicates an ACK was sent. Used only to process unexpected messages
  int ackIndex;

  void start(bool pIsClient, const string &pBranch, int pCseq, const string &pCseqMethod);

};

#endif
