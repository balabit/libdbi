#!/usr/bin/perl -w

use strict; 

local *DOCBOOK;
local *MARKUP;

inject_markup($ARGV[0], $ARGV[1]);

sub inject_markup {
	my $template = shift;
	my $markup = shift;

	my $docbookout = $template;
	$docbookout =~ s/\.tmpl$/.sgml/;

	open(MARKUP, "< $markup");
	open(TMPL, "< $template");
	open(DOCBOOK, "> $docbookout");
	
	while (<TMPL>) {
		my $line = $_;
		if ($line =~ /^(\s*)%%(\w+)\s*$/) {
			my $func = $2;
			my $tabs = $1;
			my $numtabs = 0;
			if (defined($tabs) && $tabs) {
				$numtabs++ while ($tabs =~ /\t/g);
			}
			print DOCBOOK parse_markup($numtabs, $func) . "\n";
		}
		else {
			print DOCBOOK $line;
		}
	}
	
	close(DOCBOOK);
	close(TMPL);
	close(MARKUP);
	print "Done: $docbookout\n";
}

sub parse_markup {
	my $extratab = shift;
	my $function = shift;
	local $/ = "*\n";
	seek(MARKUP, 0, 0);
	while (<MARKUP>) {
		my ($proto, $descverbatim, $desc, $returns, @args);
		my $blob = $_; $blob =~ s/\n\*\n/\n/g;
		($proto, $desc, $returns) = (($blob =~ /\tPROTO (.+)\n\tDESC[-\w]*\n\t(.*)ENDDESC\n\t.*RET (.*)[\s\*]*/s) && (strip($1), strip($2), strip($3)));
		my $funcbasename = (($proto =~ /\w+ \**(\w+)\(.*\)/) && $1);
		next unless $funcbasename eq $function;
		$descverbatim = (($blob =~ /\n\t(DESC[-\w]*)\n/) && ($1 eq 'DESC-VERBATIM'));
		my $argsraw = (($blob =~ /\n\tENDDESC\n\t(ARG .*)\n\tRET /s) && $1);
		foreach my $arg (split(/\n/, $argsraw)) {
			$arg = strip($arg);
			$arg =~ s/^\s*ARG (\w+ .+)\s*$/$1/;
			push(@args, $arg);
		}
		#print "SENDING ARGS FOR $funcbasename: " . join(" -- ", @args) . "\n";
		return spit_out_docbook($extratab, $proto, $descverbatim, $desc, $returns, @args);
	}
	return "";
}

sub spit_out_docbook {
	my $extratab = shift;
	my $funcproto = shift;
	my $descraw = shift;
	my $desc = shift;
	my $returns = shift;
	my @arguments = @_;

	my $funcbasename = (($funcproto =~ /\w+ \**(\w+)\(.*\)/) && $1);
	my $functoken = $funcbasename; $functoken =~ s/_/-/g;
	$functoken =~ s/^-/internal-/;
	my $tabs = "\t"x$extratab; #x2;

	my $OUTPUT;

	$OUTPUT .= "$tabs<Section id=\"$functoken\" XRefLabel=\"$funcbasename\"><Title>$funcbasename</Title>\n$tabs\t<Para><ProgramListing>$funcproto</ProgramListing></Para>";
	if (defined($desc) && ($desc ne "")) {
		if ($descraw) {
			foreach my $line (split(/\n/, $desc)) {
				$OUTPUT .= "\n$tabs\t$line";
			}
		}
		else {
			$OUTPUT .= "\n$tabs\t<Para>$desc</Para>";
		}
	}
	if ((defined(@arguments) && (scalar(@arguments) > 0)) || (defined($returns) && ($returns ne ""))) {
		$OUTPUT .= "\n$tabs\t<VariableList>";
		if (scalar(@arguments) > 0) {
			$OUTPUT .= "\n$tabs\t\t<VarListEntry>";
			$OUTPUT .= "\n$tabs\t\t\t<Term><Emphasis>Arguments</Emphasis></Term>";
			$OUTPUT .= "\n$tabs\t\t\t<ListItem>";
			foreach (@arguments) {
				my $line = strip($_);
				my ($token, $more) = (($line =~ /^(\w+)\s+(.+)$/) && (strip($1), strip($2)));
				if ($token ne "") {
					$OUTPUT .= "\n$tabs\t\t\t\t<Para><Literal>$token</Literal>: $more</Para>";
				}
			}
			$OUTPUT .= "\n$tabs\t\t\t</ListItem>";
			$OUTPUT .= "\n$tabs\t\t</VarListEntry>";
		}
		if ($returns ne "") {
			$OUTPUT .= "\n$tabs\t\t<VarListEntry>";
			$OUTPUT .= "\n$tabs\t\t\t<Term><Emphasis>Returns</Emphasis></Term>";
			$OUTPUT .= "\n$tabs\t\t\t<ListItem><Para>$returns</Para></ListItem>";
			$OUTPUT .= "\n$tabs\t\t</VarListEntry>";
		}
		$OUTPUT .= "\n$tabs\t</VariableList>";
	}
	$OUTPUT .= "\n$tabs</Section>";
	return $OUTPUT;
}

sub strip {
	my $str = shift;
	$str =~ s/^\s+//;
	$str =~ s/\s+$//;
	return $str;
}
