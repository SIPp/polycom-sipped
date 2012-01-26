#!/usr/bin/ruby
#
# Copyright (c) 2012, Polycom, Inc.
#
# Author: Edward Estabrook <edward.estabrook@polycom.com>
#

require 'test/unit'
require './sipp_test'

class VerifySSL < Test::Unit::TestCase
  
  def test_verify_ssl
	# Verify SSL works back-to-back
    test = SippTest.new("test_verify_ssl", "-sn uac -t ln -mc -force_client_mode", "-sn uas -t l1 -mc")
    assert(test.run(), "Vefify SSL failed (make sure SIPPED environment variable is set)")
  end
  
end
