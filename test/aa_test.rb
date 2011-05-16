#!/usr/bin/ruby
#
# Copyright (c) 2010, Polycom, Inc.
#
# Author: Edward Estabrook <edward.estabrook@polycom.com>
#


require 'test/unit'
require 'sipp_test'

class AaTest < Test::Unit::TestCase
  def test_aa
    test = SippTest.new("aa_test", "-sf aa_test_client.sipp -mc -trace_debug -aa", "-sf aa_test_server.sipp -mc -trace_debug")
    assert(test.run())
  end
end

