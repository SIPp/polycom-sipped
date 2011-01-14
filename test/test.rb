#!/usr/bin/ruby
#
# Copyright (c) 2010, Polycom, Inc.
#
# Author: Edward Estabrook <edward.estabrook@polycom.com>
#


require 'rubygems'
require 'getopt/std'
require 'English'

# require each test case here
require "aa_expires_default.rb"
require "aa_expires_specified.rb"
require "dump_sequence_diagram.rb"
require "include_directory.rb"
require "include_substitution.rb"
require "init_keywords.rb"
require "verify_keywords.rb"


def show_help_message
  puts "Usage: test \n\n";
end

