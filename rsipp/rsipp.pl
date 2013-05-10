#!/usr/bin/perl -w

use strict;
use warnings;
use English;
use Getopt::Long qw(:config pass_through no_ignore_case_always no_auto_abbrev);
use XML::Simple;
use Data::Dumper;
use Socket;
use Net::IP qw (:PROC);
use File::Slurp;

my $rsippDocumentationUrl = "https://twiki.polycom.com/twiki/bin/view/Main/Rsipp";

my $path;
BEGIN
{
    if ( defined( $ENV{'SIPP_SOURCE'} ) )
    {
        $path = "$ENV{'SIPP_SOURCE'}/rsipp";
    }
    elsif ( defined( $ENV{'RSIPP'} ) )
    {
        $path = "$ENV{'RSIPP'}";
    }
    elsif ( defined( $ENV{'SIPP'} ) )
    {
        $path = "$ENV{'SIPP'}";
    }
    else
    {
        die "Environment variable SIPP or RSIPP must be defined.\n"
            . "SIPP should be set to the directory in which sipp-related files are installed (containing sipp.dtd);\n"
            . "alternatively RSIPP should be set to the rsipp directory in which rsipp was installed or checked out.\n"
            . "rsipp.config.xml is searched in many locations, including %APPDATA%\\sipp on Windows, ~ on linux.\n";
    }
}
use lib "$path";


process_command_line_and_run_sipp( extract_parameters_from_config_file( get_config_file_path() ) );

#locating the rsipp.config.xml file
sub get_config_file_path
{
	my $sipp_app_data;
    if ( defined( $ENV{'APPDATA'} ) )
    {
        $sipp_app_data = "$ENV{'APPDATA'}/sipp";
    }

    if ( -e "rsipp.config.xml" )
    {
        return "rsipp.config.xml";
    }
    elsif ( -e "$path/rsipp.config.xml" )
    {
        return "$path/rsipp.config.xml";
    }
    elsif ( -e "~/rsipp.config.xml" )
    {
        return "~/rsipp.config.xml";
    }
    elsif ( -e "$sipp_app_data/rsipp.config.xml" )
    {
        return "$sipp_app_data/rsipp.config.xml";
    }
    else
    {
        die "Could not locate rsipp.config.xml, which is required.\n"
            . "You must copy 'copy_to_rsipp.config.xml' to 'rsipp.config.xml' and edit it before using rsipp.\n"
            . "rsipp.config.xml must be in current, home or %APPDATA%\\sipp directory, or in one of \n"
			. "\$SIPP, \$SIPP_SOURCE/rsipp or \$RSIPP directories.\n"
            . "Please refer to $rsippDocumentationUrl for help.\n";
    }

}

#check that XML file is of a valid format using xmllint
#takes XML file name as parameter
sub check_sipp_file_format_using_xmllint
{
    my $file = shift;

    my $modifiedPath = $path;

    # allows spaces in path name for parameter passing into xmllint
    $modifiedPath =~ s/[ ]/%20/g;

	my $command = "xmllint --path $modifiedPath --postvalid --noout $file --xinclude";
    my $result = system( "xmllint", "--path", $modifiedPath, "--postvalid", "--noout", $file,
                         "--xinclude" );

    # If xmllint cannot be executed, $result = 256 on Windows rather than -1, as on *nix.
    # This is unfortunately equivalent to a failed validation in this context.
    if ( $result == -1 or $result == 256 )
    {
        die "FAILURE attempting to use XMLLint for validation of \"$file\": $!\n"
            . "Please ensure that the xmllint binary is executable and in the System Path.\n"
            . "If you are receiving parser or validation errors, \"$file\"\n"
            . "probably contains syntax errors or is malformed.\n"
			. "\n'$command'\n\n"
            . "Please refer to the XMLLint and SipP documentation.\n";
    }
    elsif ( $result & 127 )
    {
        my $code = ( $result & 127 );
        die "FAILURE attempting to use XMLLint for validation of \"$file\": the xmllint process "
        . "died with signal $code\n"
		. "\n'$command'\n\n";
    }
    elsif ( $result != 0 )
    {
        my $code = ( $result >> 8 );
        die "FAILURE attempting to use XMLLint for validation of \"$file\": the xmllint process "
            . "exited with value $code\n"
			. "Ensure sipp.dtd file is in one of \$SIPP, \$SIPP_SOURCE/rsipp, \$RSIPP directories.\n"
            . "If you are receiving parser or validation errors, \"$file\"\n"
            . "probably contains syntax errors or is malformed.\n"
			. "\n'$command'\n\n"
            . "Please refer to the XMLLint and SipP documentation.\n";
    }
    else
    {
        print "XMLLint validation PASSED for \"$file\"\n";
    }

    return $result;
}

sub is_rsipp_config_file_unedited 
{
    my $xml_file   = shift;
	my $text = read_file($xml_file);
	return $text =~ /REPLACE_WITH_IP_OF_YOUR_NETWORK_INTERFACE/
}

sub prompt_for_ip
{
    my $prompt = shift;
    print "\n$prompt";
    my $ip = <STDIN>;
    if (!defined($ip)) {
        print "Aborted.\n";
        exit(1);
    }
    chomp($ip);
    return $ip;
}

# return the local IP, cannot accept blanks
sub prompt_for_local_ip
{
    print "\n";
    print "---- Local IP ----\n";
    print "SIPp must be configured with the IP address on this computer you wish to use to send and receive packets.\n";
    print "This IP address must be associated with an interface on this computer, and correlates with the -i parameter.\n";
    print "\n";
    print "You may override the default you enter now for a specific run by specifying a -i parameter.\n";
    my $prompt = "Please enter local IP address: ";
    my $local_ip = prompt_for_ip($prompt);
    
    while (not ip_is_ipv4($local_ip)) {
        print "Invalid IP address, please try again.\n";
        $local_ip = prompt_for_ip($prompt);
    }
    return $local_ip;
}

sub prompt_for_remote_ip
{
    print "\n\n";
    print "---- Remote IP ----\n";
    print "You may optionally specify the IP address of a default remote endpoint.\n";
    print "If you generally run scripts against a single device this is helpful.\n";
    print "If you run scripts against a broad range of devices you may leave this blank.\n";
    print "\n";
    print "If you enter a default endpoint now, you may override it for a specific run \n";
    print "by specifying the desired endpoint's IP on the command-line.\n";
    print "\n";
    print "If you do not enter a default you should always specify the remote endpoint's IP each time you run a test.\n";

    my $prompt = "Please enter the default remote endpoint's IP (or leave blank to skip): ";

    my $remote_ip = prompt_for_ip($prompt);
    
    while (($remote_ip ne "") and (not ip_is_ipv4($remote_ip))) {
        print "Invalid IP address, please try again.\n";
        $remote_ip = prompt_for_ip($prompt);
    }
    return $remote_ip;
}

sub create_rsipp_config_file
{
    my ($xml_file, $local_ip, $remote_ip) = @_;
    print "\nWriting $xml_file...\n";
    open my $CONFIGFILE, ">", "$xml_file" or die "Unable to open config file '$xml_file' for writing. Please ensure file is not open in another program and try again.\n";
    print $CONFIGFILE "<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n";
    print $CONFIGFILE "<!DOCTYPE scenario SYSTEM \"sipp.dtd\"[]>\n";
    print $CONFIGFILE "<tools>\n";
    print $CONFIGFILE "  <sipp command=\"sipp\">\n";
    print $CONFIGFILE "    <param name=\"-i\" value=\"$local_ip\" />\n";
    print $CONFIGFILE "    <param name=\"\" value=\"$remote_ip\" />\n" unless $remote_ip eq "";
    print $CONFIGFILE "  </sipp>\n";
    print $CONFIGFILE "</tools>\n";
    close($CONFIGFILE);
	print "Done.\n\n";

}

sub prompt_user_and_create_rsipp_config_file 
{
    my $xml_file   = shift;
    print "Invalid rsipp.config.xml file: SIPp requires configuration.\n\n";
    
    my $local_ip = prompt_for_local_ip();
    my $remote_ip = prompt_for_remote_ip();
    create_rsipp_config_file($xml_file, $local_ip, $remote_ip);
}

#read and extract default settings from XML config file
#takes config file name as parameter
sub extract_parameters_from_config_file
{
    my $xml_file   = shift;
	
	if (is_rsipp_config_file_unedited($xml_file)) {
	    prompt_user_and_create_rsipp_config_file($xml_file);
	}
	
    my $xml_params = " ";
    my $xml        = XML::Simple->new();
    my $data;
    eval { $data = $xml->XMLin( $xml_file, KeyAttr => 'name', ForceArray => 1 ); };
    if ($@)
    {
        die("XML parser failed while trying to retrieve default arguments from rsipp.config.xml. The message was: "
                . $@ );
    }
    my $die_invalid_ip_string = "Malformed address or hostname in configuration file $xml_file\n"
    . "If you have not edited this file since installation, please do so now.\n"
    . "Documentation can be found at $rsippDocumentationUrl\n\n";
    my $command = $data->{sipp}[0]->{command};
    while ( my ( $key, $value ) = each %{ $data->{sipp}[0]->{param} } )
    {
        if ( $key eq '-i' ||      # local machine
             $key eq '' ||        # Device/Endpoint Under test
             $key eq '-mi' ||     # local media IP address
             $key eq '-rsa' ||    # remote sending address
             $key eq '-ci'        # local control IP address
            )
        {                        #validates ip address and/or lookup hostname
            if ( (ip_is_ipv4($value->{value})) ||  
                 (( $value->{value} =~ /^\[(.*)\]$/) && (ip_is_ipv6( $1))) )
            {
              # if valid ipv4 or ipv6 with surrounding [], just use the supplied value
              $xml_params = $key . " " . $value->{value} . " " . $xml_params;
            } elsif    (ip_is_ipv6($value->{value}))
            {
              # valid ipv6 without [], add them. sipp v3.2.21 needs them
              $xml_params = $key . " [" . $value->{value} . "] " . $xml_params;
            } else {          
              usage_and_die(
                     $die_invalid_ip_string . " \tname = \"$key\", value = \"$value->{value}\" \n" );
            }
        }
    }

    return ($command, $xml_params);
}

sub usage_and_die
{
    my $die_string = shift;
    print("Usage:\n"
        . "rsipp.pl (-sf|-sn|-sd) file_or_scenario_name [-validate_only] [options]\n"
        . "     -sf             Runs specified .sipp scenario file\n"
        . "     -sn             Runs a built in scenario from SIPp\n"
        . "     -sd             Dumps the XML of a built in scenario from SIPp to the console\n"
        . "     -validate_only  Validates scenario file with XMLLint, but does not run SIPp. (Only allowed when using -sf)\n"
        . "     -v              Displays version and copyright information from SIPp\n"
        . "     [options]       Any additional options you wish to pass on to SIPp directly\n\n"
        . "     For more information regarding rsipp, see the documentation: $rsippDocumentationUrl\n\n"
    );
    die $die_string;
}

sub process_command_line_and_run_sipp
{
    my $sipp_command = shift;
    my $config_file_params = shift;
    my %command_line_args;
    my @args_list = ('sf=s', # Use scenario from .sipp file
                     'sn=s', # Use scenario built into SIPp
                     'sd=s', # Dump XML of built in scenario to the console
                     'v', # Display SIPp version info
                     'validate_only'); # Validate .sipp file with XMLLint, but do not run SIPp

    GetOptions(\%command_line_args, @args_list);

    my $sipp_args = "";

    # Check if there are any extra arguments to pass to SIPp. Empty quotes in the command line cause Badness.
    if ( scalar(@ARGV) > 0)
    {
        $sipp_args = '"' . join('" "', @ARGV) . '"';
    }

    if ( defined( $command_line_args{v} ) )
    {
        run_sipp($sipp_command, "-v");
    }

    my $scenario;
    my $scenario_type;
    my $scenario_params = "";

    if ( defined( $command_line_args{sf} ) # Use scenario from .sipp file
        && !( $command_line_args{sn} )
        && !( $command_line_args{sd} ) )
    {
        my $sipp_file = $command_line_args{sf};
        die "SIPp scenario file not found: $sipp_file\n" unless ( -e $sipp_file );
        check_sipp_file_format_using_xmllint($sipp_file);
        $scenario = $sipp_file;
        $scenario_type = "-sf";
        $scenario_params = extract_parameters_from_sipp_file($scenario);
    }
    elsif ( defined( $command_line_args{sn} ) # Use scenario built into SIPp
        && !defined( $command_line_args{sf} )
        && !defined( $command_line_args{sd} ) )
    {
        if ( $command_line_args{validate_only} )
        {
            usage_and_die( "Option -validate_only specified with -sn! This option is only allowed with -sf!\n" )
        }
        $scenario = $command_line_args{sn};
        $scenario_type = "-sn";
        usage_and_die(  ) if $scenario eq "";
    }
    elsif ( defined( $command_line_args{sd} ) # Dump XML of built in scenario to the console
        && !defined( $command_line_args{sf} )
        && !defined( $command_line_args{sn} ) )
    {
        if ( $command_line_args{validate_only} )
        {
            usage_and_die( "Option -validate_only specified with -sd! This option is only allowed with -sf!\n" )
        }
        $scenario = $command_line_args{sd};
        $scenario_type = "-sd";
    }
    elsif ( !defined( $command_line_args{sf} )
           && !defined( $command_line_args{sn} )
           && !defined( $command_line_args{sd} ) )
    {
        usage_and_die( "No scenario specified! You must specify one of (-sf|-sn|-sd) with a .sipp file or built in scenario name!\n" );
    }
    else
    {
        usage_and_die( "Multiple scenario options specified! You must specify only one of (-sf|-sn|-sd) with a .sipp file or built in scenario name!\n" );
    }

    if ( !( $command_line_args{validate_only} ) )
    {
        my $run_args = "$config_file_params $scenario_params $sipp_args";
        run_sipp( $sipp_command, $run_args, $scenario, $scenario_type );
    }
}

#open sipp file to extract sipp parameters
#takes scenario filename as a parameter
sub extract_parameters_from_sipp_file
{

    my $scenario_filename = shift;
    my $sipp_params;

    #finding line with parameters
    open my $FILE, "<", $scenario_filename or die $!;
    my @lines = <$FILE>;
    for (@lines)
    {
        if ( $_ =~ /parameters="(.*?)"/ )
        {
            $sipp_params = $1;
        }
    }
    close $FILE;
    return $sipp_params;
}

#takes in your xml and sipp parameters, the scenario name, and the scenario type (-sf|-sn|-sd)
sub run_sipp
{

    my ( $sipp_command, $run_args, $scenario_name, $scenario_type ) = @_;

    my $result;
    if ( scalar( @_ ) == 1 ) # No scenario, i.e. for -v switch
    {
        print("Running: '$sipp_command $run_args'\n");
        $result = system("$sipp_command $run_args");
    }
    else
    {
        print("Running: '$sipp_command $run_args $scenario_type \"$scenario_name\"'\n");
        $result = system("$sipp_command $run_args $scenario_type \"$scenario_name\"");
    }

    if ( $result == -1 )
    {
        die "FAIL: failed to execute: $! \n";
    }
    elsif ( $result & 127 )
    {
        my $code = $result & 127;
        die "FAIL: child died with signal $code\n";
    }
    elsif ( $result != 0 )
    {
        my $code = $result >> 8;
        if ( $code == 1 )
        {
            die "FAIL: the scenario file failed to execute properly.\n"
                . "This was probably caused by an unexpected message, or verification related error.\n";
        }
        elsif ( $code == 95 ) { die "FAIL: test stopped by user\n" }
        elsif ( $code == 96 )
        {
            die "FAIL: test killed by signal, possibly sent by user (e.g. ctrl+c)\n";
        }
        elsif ( $code == 97 ) { die "FAIL: test stopped internally, either due to timeout or stop_now command\n" }
        elsif ( $code == 98 )
        {
            die
                "FAIL: test was terminated before it completed\nPossibly a problem with the remote side of a TCP connection)\n";
        }
        elsif ( $code == 99 )  { print "PASS: No call processed\n" }
        elsif ( $code == 255 ) { die "FAIL: fatal error, see error message for more detail\n" }
        elsif ( $code == 254 ) { die "FAIL: address already in use, cannot bind port\n" }
        elsif ( $code == 253 ) { die "FAIL: system error\n" }
        elsif ( $code == 252 )
        {
            die "FAIL: there was a problem with the arguments passed to SIPp\n";
        }
        else 
        {
            die "FAIL: exited with unexpected value $code\n";
        }
    }
    else
    {
        print "PASS\n";
    }
    return $result;
}

__END__
