#!/usr/bin/perl -w
use Test::Perl::Critic (-severity => "gentle");
use Test::More qw(no_plan);

critic_ok("rsipp.pl");

