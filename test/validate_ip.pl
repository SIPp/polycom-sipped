#!/usr/bin/perl -w
my $ip = $ARGV[0];
if (!defined($ip)) {
    print "IP not defined\n";
    exit(1);
}
elsif ($ip =~ /^[\d]+\.[\d]+\.[\d]+\.[\d]+$/) {
    print "'$ip' is valid.\n";
    exit(0);
}
else {
    print "'$ip' is NOT valid.\n";
    exit(9);
}
