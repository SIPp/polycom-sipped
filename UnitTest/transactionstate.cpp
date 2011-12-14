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
 *  Author : Edward Estabrook - 13 December 2011
 *           From Polyom Inc.
 *
 */


#include "gtest/gtest.h"
#include "transactionstate.hpp"
#include <string>

const string name("Name");


TEST(TransactionState, Constructor){
  TransactionState t(name);

  EXPECT_EQ(t.getName(), name);
}

TEST(TransactionState, startClient){
  TransactionState t(name);
  t.startClient("aBranch", 2, "METHOD");

  EXPECT_TRUE(t.isClientTransaction());
  EXPECT_FALSE(t.isServerTransaction());
  EXPECT_EQ(t.getBranch(), "aBranch");
  EXPECT_EQ(t.getCseq(), 2);
  EXPECT_EQ(t.getCseqMethod(), "METHOD");
}


TEST(TransactionState, startServer){
  TransactionState t(name);
  t.startServer("aBranch", 2, "METHOD");

  EXPECT_FALSE(t.isClientTransaction());
  EXPECT_TRUE(t.isServerTransaction());
  EXPECT_EQ(t.getBranch(), "aBranch");
  EXPECT_EQ(t.getCseq(), 2);
  EXPECT_EQ(t.getCseqMethod(), "METHOD");
}

TEST(TransactionState, ackBranch){
  TransactionState t(name);
  EXPECT_EQ(t.getAckBranch(), "");

  t.setAckBranch("anAckBranch");
  EXPECT_EQ(t.getAckBranch(), "anAckBranch");
}

TEST(TransactionState, lastReceivedMessage){
  TransactionState t(name);
  EXPECT_EQ(t.getLastReceivedMessage(), "");

  t.setLastReceivedMessage("aMessage");
  EXPECT_EQ(t.getLastReceivedMessage(), "aMessage");
}

TEST(TransactionState, lastResponseCode){
  TransactionState t(name);
  EXPECT_EQ(t.getLastResponseCode(), 0);

  t.setLastResponseCode(123);
  EXPECT_EQ(t.getLastResponseCode(), 123);
  EXPECT_FALSE(t.isLastResponseCode2xx());

  t.setLastResponseCode(200);
  EXPECT_EQ(t.getLastResponseCode(), 200);
  EXPECT_TRUE(t.isLastResponseCode2xx());

  t.setLastResponseCode(299);
  EXPECT_EQ(t.getLastResponseCode(), 299);
  EXPECT_TRUE(t.isLastResponseCode2xx());

  t.setLastResponseCode(300);
  EXPECT_EQ(t.getLastResponseCode(), 300);
  EXPECT_FALSE(t.isLastResponseCode2xx());
}

TEST(TransactionState, ackIndex){
  TransactionState t(name);
  EXPECT_EQ(t.getAckIndex(), 0);

  t.setAckIndex(123);
  EXPECT_EQ(t.getAckIndex(), 123);
}

TEST(TransactionState, transactionHash){
  TransactionState t(name);
  EXPECT_EQ(t.getTransactionHash(), 0);

  t.setTransactionHash(123);
  EXPECT_EQ(t.getTransactionHash(), 123);
}

// Not tested:
// string trace() const;

