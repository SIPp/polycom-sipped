#!/usr/bin/ruby
#
# Copyright (c) 2010, Polycom, Inc.
#
# Author: Edward Estabrook <edward.estabrook@polycom.com>
#


require 'test/unit'
require 'sipp_test'

class TransactionKeywords < Test::Unit::TestCase
  def test_keywords
    # Verify [cseq_method], [cseq], [client_cseq_method], [client_cseq], 
	# [server_cseq_method], [server_cseq], [received_cseq_method], [received_cseq], 
	# [last_cseq_number], [last_branch], [last_Request_URI], [last_message]
    test = SippTest.new("verify_transaction_keywords", "-sf verify_transaction_keywords_client.sipp -mc", "-sf verify_transaction_keywords_server.sipp -mc")
    assert(test.run())
  end
end

