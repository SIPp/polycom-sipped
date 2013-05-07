#!/usr/bin/ruby
#
# Copyright (c) 2010, Polycom, Inc.
#
# Author: Edward Estabrook <edward.estabrook@polycom.com>
#

require 'test/unit'
require './sipp_test'

class AaExpiresDefault < Test::Unit::TestCase
  def test_expires
    test = SippTest.new("aa_expires_default", "-sf aa_expires_default.sipp -m 1 -l 1", "-sn uas -aa ")
    assert(test.run())
  end
end
