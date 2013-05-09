#!/usr/bin/env perl 
use strict;
use warnings;

#   argument (if any) will be used to form part of dest filename 
if ($#ARGV<0){
  $ARGV[0] = "";
}

my $filename = "SIPp" . $ARGV[0] . ".msi";
die "$filename does not exist." unless (-e $filename);

my $sipp_version_screen = `sipp -v`;
if ($sipp_version_screen =~ m\SIPp(ed)? (v.*), version \) {
    my $version = lc($2);
    $version =~ s\ \_\g;  # replace all spaces in output name
    my $targetname = "SIPp" . $ARGV[0] . "_" . $version . ".msi"; 
    print "Copying '$filename' to '$targetname'.\n";
    system("copy /Y $filename \"$targetname\"");   #can preserve spaces in output name
    
    print "\nInstaller Creation Complete: '$targetname' created.\n\n";
    }
else {
    die "ERROR: SIPp version screen did not contain version information\n";
}
