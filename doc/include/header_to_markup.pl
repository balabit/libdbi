#!/usr/bin/perl -w

my @oldprototypes;
my @newprototypes;

start_parse($ARGV[0], $ARGV[1]);

sub start_parse {
	my $filename = shift;
	my $oldmarkup = shift;
	
	open(CTAGS, "ctags -x -u --c-types=p $filename |");
	while (<CTAGS>) {
		if (/^\w+\s+\w+\s+\d+\s+\S+\s+(.+);.*$/) {
			push(@newprototypes, $1);
		}
	}
	close(CTAGS);

	open(NEW, "> doc-markup.$$");
	if ($oldmarkup) { fill_old_prototypes($oldmarkup); }
	foreach my $prototype (@newprototypes) {
		if ($oldmarkup && (my $oldentry = get_old_markup($oldmarkup, $prototype))) {
			$oldentry =~ s/\n\*\n/\n/g;
			print NEW "*\n$oldentry";
		}
		else {
			my ($return_type, $function_name, $argtext) = (($prototype =~ /(\w+ \**)(\w+)\((.*)\)/) && ($1, $2, $3));
			my @args = split(/,/, $argtext);
			my @argnames = @args;
			@argnames = grep(s/^.+ \**(\w+)$/$1/, @argnames);
			print NEW "*\n\nNAME $function_name";
			if ($oldmarkup) { print NEW " # XXX TODO"; }
			print NEW "\n\tPROTO $prototype\n\tDESC\n\t\n\tENDDESC\n";
			foreach my $arg (@argnames) {
				print NEW "\tARG $arg \n";
			}
			print NEW "\tRET \n\n";
		}
	}
	if ($oldmarkup) {
		my (%seen, @only);
		@seen{@oldprototypes} = ();
		foreach my $item (@newprototypes) {
			push(@only, $item) unless exists $seen{$item};
		}
		print NEW "\n%\n\% New prototypes:\n%\t" . join("\n%\t", @only) . "\n%";
		
		undef %seen; undef @only;
		@seen{@newprototypes} = ();
		foreach my $item (@oldprototypes) {
			push(@only, $item) unless exists $seen{$item};
		}
		print NEW "\n% Removed prototypes:\n%\t" . join("\n%\t", @only) . "\n%";
		my @union = my @isect = my @diff = (); my %union = my %isect = (); my %count = ();
		foreach my $e (@oldprototypes, @newprototypes) { $union{$e}++ && $isect{$e}++ }
		@union = keys %union;
		@isect = keys %isect;
		print NEW "\n% Unchanged prototypes:\n%\t" . join("\n%\t", @isect) . "\n%\n";
	}
	close(NEW);
	print "All done: doc-markup.$$\n";
}

sub fill_old_prototypes {
	my $filename = shift;
	local $/ = "\n";
	open(OLD2, "< $filename");
	while (<OLD2>) {
		if (/^\tPROTO (.+)\s*$/) {
			push(@oldprototypes, $1);
		}
	}
	close(OLD2);
	return 1;
}

sub get_old_markup {
	my $filename = shift;
	my $prototype = shift;
	my $result;
	local $/ = "*\n";
	open(OLD, "< $filename");
	while (<OLD>) {
		my $foo = $_;
		if (/\n\tPROTO (.+)\s*\n/) {
			if ($1 eq $prototype) {
				$result = $foo;
			}
		}
	}
	close(OLD);
	if ($result) { $result =~ s/^\*$//g; }
	return $result;
}

sub strip {
	my $str = shift;
	$str =~ s/^\s+//;
	$str =~ s/\s+$//;
	return $str;
}

