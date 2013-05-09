#!/usr/bin/env perl 
use strict;
use warnings;

my $version_number;
my $sipp_version = `sipp -v`;
if ($sipp_version =~ /SIPp(ed)? v?([0-9]+\.[0-9]+(\.[0-9]+)?)\s*[A-Za-z0-9\-]+, vers/) {
    $version_number = $2;
    print "$version_number\n";
}else{
    die "Unable to extract version number from sipp";
}


 
