#!/usr/bin/ruby
#
# Copyright (c) 2010, Polycom, Inc.
#
# Author: Edward Estabrook <edward.estabrook@polycom.com>
#


require 'test/unit'
require './sipp_test'

class VerifyURIEncoding < Test::Unit::TestCase
  def test_uri_encoding
    #check all charecters are encoded properly (except ';', '[' and ']' which cause errors for SIPp)
    test = SippTest.new("verify_uri_encoding", "-sf verify_uri_encoding_client.sipp -mc", "-sf verify_uri_encoding_server.sipp -mc")
    assert(test.run())
  end
end

