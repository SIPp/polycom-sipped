#!/usr/bin/ruby
#
# Copyright (c) 2010, Polycom, Inc.
#
# Author: Edward Estabrook <edward.estabrook@polycom.com>
#


require 'test/unit'
require './sipp_test'

class AaExpiresSpecified < Test::Unit::TestCase
  def test_keywords
    # Verify [local_tag_param], [remote_tag], [contact_uri], [contact_name_and_uri], [to_uri], [to_name_and_uri], [from_uri], [from_name_and_uri], [next_url]
    test = SippTest.new("test_keywords", "-sf verify_keywords_client.sipp -mc", "-sf verify_keywords_server.sipp -mc")
    assert(test.run())
  end
end
