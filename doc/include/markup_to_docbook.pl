#!/usr/bin/perl -w

# XXX TODO skips any blocks with DESC-VERBATIM XXX

local *DOCBOOK;

parse_markup($ARGV[0]);

sub parse_markup {
	my $filename = shift;
	open(MARKUP, "< $filename");
	open(DOCBOOK, "> reference.sgml-$$");
	local $/ = "*\n";
	while (<MARKUP>) {
		my ($proto, $descverbatim, $desc, $returns, @args);
		my $blob = $_; $blob =~ s/\n\*\n/\n/g;
		($proto, $desc, $returns) = (($blob =~ /\tPROTO (.+)\n\tDESC[-\w]*\n\t(.*)ENDDESC\n\t.*RET (.*)[\s\*]*/s) && (strip($1), strip($2), strip($3)));
		$descverbatim = (($blob =~ /\n\t(DESC[-\w]*)\n/) && ($1 eq 'DESC-VERBATIM'));
		my $argsraw = (($blob =~ /\n\tENDDESC\n\t(ARG .*)\n\tRET /) && $1);
		foreach my $arg (split(/\n/, $argsraw)) {
			$arg = strip($arg);
			$arg =~ s/^\s*ARG (\w+ .+)\s*$/$1/;
			push(@args, $arg);
		}
		spit_out_docbook($proto, $descverbatim, $desc, $returns, @args) unless strip($proto) eq "";
	}
	print DOCBOOK "\n";
	close(DOCBOOK);
	close(MARKUP);
	print "Done: reference.sgml-$$\n";
}

sub spit_out_docbook {
	my $funcproto = shift;
	my $descraw = shift;
	my $desc = shift;
	my $returns = shift;
	my @arguments = @_;

	my $funcbasename = (($funcproto =~ /\w+ \**(\w+)\(.*\)/) && $1);
	my $functoken = $funcbasename; $functoken =~ s/_/-/g;
	my $tabs = "\t"x2; #x3;

	print DOCBOOK "\n$tabs<Section id=\"$functoken\" XRefLabel=\"$funcbasename\"><Title>$funcbasename</Title>\n$tabs\t<Para><ProgramListing>$funcproto</ProgramListing></Para>";
	if (defined($desc) && ($desc ne "")) {
		if ($descraw) {
			print DOCBOOK "\n$tabs\t$desc";
		}
		else {
			print DOCBOOK "\n$tabs\t<Para>$desc</Para>";
		}
	}
	if ((defined(@arguments) && (scalar(@arguments) > 0)) || (defined($returns) && ($returns ne ""))) {
		print DOCBOOK "\n$tabs\t<VariableList>";
		if (scalar(@arguments) > 0) {
			print DOCBOOK "\n$tabs\t\t<VarListEntry>";
			print DOCBOOK "\n$tabs\t\t\t<Term><Emphasis>Arguments</Emphasis></Term>";
			print DOCBOOK "\n$tabs\t\t\t<ListItem>";
			foreach (@arguments) {
				my $line = strip($_);
				my ($token, $more) = (($line =~ /^(\w+)\s+(.+)$/) && (strip($1), strip($2)));
				if ($token ne "") {
					print DOCBOOK "\n$tabs\t\t\t\t<Para><Literal>$token</Literal>: $more</Para>";
				}
			}
			print DOCBOOK "\n$tabs\t\t\t</ListItem>";
			print DOCBOOK "\n$tabs\t\t</VarListEntry>";
		}
		if ($returns ne "") {
			print DOCBOOK "\n$tabs\t\t<VarListEntry>";
			print DOCBOOK "\n$tabs\t\t\t<Term><Emphasis>Returns</Emphasis></Term>";
			print DOCBOOK "\n$tabs\t\t\t<ListItem><Para>$returns</Para></ListItem>";
			print DOCBOOK "\n$tabs\t\t</VarListEntry>";
		}
		print DOCBOOK "\n$tabs\t</VariableList>";
	}
	print DOCBOOK "\n$tabs</Section>";
}

sub strip {
	my $str = shift;
	$str =~ s/^\s+//;
	$str =~ s/\s+$//;
	return $str;
}
