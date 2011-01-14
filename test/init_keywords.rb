#!/usr/bin/ruby
#
# Copyright (c) 2010, Polycom, Inc.
#
# Author: Edward Estabrook <edward.estabrook@polycom.com>
#

require 'test/unit'
require 'sipp_test'

class Exec < Test::Unit::TestCase
  
  def test_exec_verify_pass
	# verify [local_ip] and [remote_ip] have IPs in <init> section
    test = SippTest.new("init_keywords", "-sf init_keywords.sipp -mc", "-sn uas -aa ")
    assert(test.run())
  end
  
end
