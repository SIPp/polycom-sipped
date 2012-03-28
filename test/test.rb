#!/usr/bin/ruby
#
# Copyright (c) 2010, Polycom, Inc.
#
# Author: Edward Estabrook <edward.estabrook@polycom.com>
#


require 'rubygems'
require 'getopt/std'
require 'English'
# delete prev set of logs and console captures
def cleandir
    Dir.glob('*.log').each do |f|
        File.delete(f)
        #puts "Deleted #{f}"
    end
    Dir.glob('*.out').each do |f|
        File.delete(f)
        #puts "Deleted #{f}"
    end 
end
cleandir()

# require each test case here
require "./aa_expires_default.rb"
require "./aa_expires_specified.rb"
require "./dump_sequence_diagram.rb"
require "./exec.rb"
require "./include_directory.rb"
require "./include_substitution.rb"
require "./init_keywords.rb"
require "./short_headers.rb"
require "./verify_aa_represses_unexpected_message_error.rb"
require "./verify_cooked_packets.rb"
require "./verify_encoding.rb"
require "./verify_error_codes.rb"
require "./verify_error_messages.rb"
require "./verify_generated.rb"
require "./verify_keywords.rb"
require "./verify_keywords_scenarios.rb"
require "./verify_manual_call-ids.rb"
require "./verify_pcap_play.rb"
require "./verify_ssl.rb"
require "./verify_strcmp.rb"
require "./verify_test_action.rb"
require "./verify_transaction_keywords.rb"
require "./zero_content_length_header_not_last_scenarios.rb"
require "./verify_whereami.rb"

def show_help_message
  puts "Usage: test \n\n";
end


