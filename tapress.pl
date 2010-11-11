#!/usr/bin/perl -w
# Create a user agent object
use LWP::UserAgent;
use Time::HiRes qw(sleep);
use strict;

# Declare the subroutines
sub post_request($$);
sub get_request($);
sub request($$$);
sub dialNumber($$);
# define version number
my $version = "0.1";

# main program start

my $DELAY = 0.1;

#the phone's IP addresses is the first argument
my $phone_ip = shift @ARGV || die "Please enter caller IP";

# define the request string
my $TA_PRESS = "http://$phone_ip/TA/keyPress";


#treat remaining arguments as keys to press
foreach my $request (@ARGV)
{
    if ($request =~ m/^([0-9]|\.)+$/)
    {
    	print "Dialing the number $request...\n";
        dialNumber($TA_PRESS, $request);
    }
    else
    {
    	print "Pressing $request \n";
    	post_request($TA_PRESS, $request);
    }
}

my $number = $ARGV[0] || die "Please enter number to dial";


###########################
# Subroutines
###########################

# POST a http request and dump the results on terminal
sub post_request($$)
{
    return request(1, $_[0], $_[1]);
}

# GET a http request and dump the results on terminal
sub get_request($)
{
    return request(0, $_[0], 0);
}

# send a http request and dump the results on terminal
sub request($$$)
{
    my $ua = new LWP::UserAgent;
    $ua->agent("AgentName/0.1 " . $ua->agent);

    # Create a request
    my $req;
    if ($_[0] == 0)
    {
        $req = new HTTP::Request GET => $_[1];
    }
    else
    {
        $req = new HTTP::Request POST => $_[1];
        $req->content_type('application/x-www-form-urlencoded');
        $req->content("value=$_[2]");
    }

    # Pass request to the user agent and get a response back
    my $res = $ua->request($req);

    # Check the outcome of the response
    if ($res->is_success)
    {
        print $res->content;
    }
    else
    {
        print "Error sending request: Check that automation is enabled on phone and that IP address is correct ($_[1] $_[2] responded " . $res->content . ".\n";
    }
    
    return length($res->content) > 0 ? $res->content : "";
} 

sub dialNumber($$)
{
    my @keys = split(//, $_[1]);

    foreach my $key (@keys)
    {
        if ($key =~ m/\./ixm)
        {
            $key = "Star";
        }

        pressKey($_[0],$key);
        sleep $DELAY;
    }
}

sub pressKey
{
    post_request($_[0], "Dialpad".$_[1]);
}