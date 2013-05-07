#!/usr/bin/ruby

require 'rubygems'
require 'rbconfig'
require 'win32/process' if Config::CONFIG["host_os"] =~ /mswin|mingw/
require 'sys/proctable' if Config::CONFIG["host_os"] =~ /mswin|mingw/

#kill all sipp processes

@is_windows = Config::CONFIG["host_os"] =~ /mswin|mingw/
@logging = "verbose"

if not @is_windows
  cmd = 'ps -ef  | grep "sipp " |grep -v "grep"'
  result = `#{cmd}`
  result.each{|s|
    # only kill if "sipp " appears in cmd portion of ps - col48 or later
    position = (s =~ /sipp\s/)
    if (position >=48)
        puts s if @logging == "verbose"
        a = s.split()
        pid = a[1].to_i
        if (pid)
          puts "Clearing sipp proceses: sigkill sipp process pid #{pid}" 
          Process.kill("SIGKILL",pid)
        else
          puts "#{pid} is not a number" if @logging == "verbose"
        end 
    end
  }
else 
  puts "Windows: Killing sipp processes " 
  Sys::ProcTable.ps.each { |ps|
    if ps.name.downcase == "sipp.exe"
    puts "killing #{ps.pid}  #{ps.name}"  if @logging == "verbose"
    Process.kill('KILL', ps.pid)
    end
  }
end
