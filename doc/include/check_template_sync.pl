#!/usr/bin/perl -w

use strict;

my @oldfunctions;
my @newfunctions;

start_parse($ARGV[0], $ARGV[1]);

sub start_parse {
	my $markup = shift;
	my $template = shift;
	
	open(TMPL, "< $template");
	while (<TMPL>) {
		my $line = $_;
		if ($line =~ /^(\s*)%%(\w+)\s*$/) {
			my $func = $2;
			my $tabs = $1;
			push(@oldfunctions, $func);
		}
	}
	close(TMPL);
	
	open(MARKUP, "< $markup");
	while (<MARKUP>) {
		if (/^NAME (.+)\s*$/) {
			push(@newfunctions, $1);
		}
	}
	close(MARKUP);
	
	# compute intersection
	my (%seen, @only);
	@seen{@oldfunctions} = ();
	foreach my $item (@newfunctions) {
		push(@only, $item) unless exists $seen{$item};
	}
	
	if (scalar(@only)) {
		print "New functions not yet in template:\n\t" . join("\n\t", @only) . "\n\n";
	}
	
	undef %seen; undef @only;
	@seen{@newfunctions} = ();
	foreach my $item (@oldfunctions) {
		push(@only, $item) unless exists $seen{$item};
	}
	
	if (scalar(@only)) {
		print "Old functions not yet removed in template:\n\t" . join("\n\t", @only) . "\n";
	}
}

