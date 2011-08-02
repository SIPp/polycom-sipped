#!/usr/bin/ruby
#
# Copyright (c) 2011, Polycom, Inc.
#
# Author: Daniel Busto <daniel.busto@polycom.com>
#


require 'test/unit'
require './sipp_test'

class Generated < Test::Unit::TestCase
  def test_generated
    #Verify CSeq and branch

    test_generated = SippTest.new("generated", "-sf verify_generated_client.sipp -mc", "-sf verify_generated_server.sipp -mc")

    assert(test_generated.run())
  end
end
