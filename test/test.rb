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
require "include_directory.rb"

def show_help_message
  puts "Usage: test \n\n";
end

