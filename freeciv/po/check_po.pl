#!/usr/bin/perl -w
#
# check_po.pl  -  check po file translations for likely errors
#
# Written by David W. Pfitzner dwp@mso.anu.edu.au
# This script is hereby placed in the Public Domain.
#
# Various checks on po file translations:
# - printf-style format strings;
# - differences in trailing newlines;
# - empty (non-fuzzy) msgid;
# - likely whitespace errors on joining multi-line entries
# Ignores all fuzzy entries.
#
# Options:
#  -x  Don't do standard checks above (eg, just check one of below).
#  -n  Check newlines within strings; ie, that have equal numbers
#      of newlines in msgstr and msgid.  (Optional because this may
#      happen legitimately.)
#  -w  Check leading whitespace.  Sometimes whitespace is simply 
#      spacing (eg, for widget labels etc), or punctuation differences,
#      so this may be ok.
#  -W  Check trailing whitespace.  See -w above.
#  -p  Check trailing punctuation.
#  -c  Check capitalization of first non-whitespace character 
#      (only if [a-zA-Z]).
#  -e  Check on empty (c.q. new) msgstr
#
# Reads stdin (or filename args, via <>), writes any problems to stdout.
#
# Modified by Davide Pagnin nightmare@freeciv.it to support plural forms
#
# Version: 0.41     (2002-06-06)

use strict;
use vars qw($opt_c $opt_n $opt_p $opt_w $opt_W $opt_x $opt_e);
use Getopt::Std;

getopts('cnpwWxe');

# Globals, for current po entry:
#
# Note that msgid and msgstr have newlines represented by the 
# two characters '\' and 'n' (and similarly for other escapes).

my @amsgid;			# lines exactly as in input
my @amsgstr;
my $entryline;                  # lineno where entry starts
my $msgid;			# lines joined by ""
my $msgstr;
my $is_fuzzy;
my $is_cformat;
my $state;			# From constant values below.
my $did_print;                  # Whether we have printed this entry, to
				# print only once for multiple problems.

use constant S_LOOKING_START => 0;   # looking for start of entry
use constant S_DOING_MSGID   => 1;   # doing msgid part
use constant S_DOING_MSGSTR  => 2;   # doing msgstr part 

# Initialize or reinitalize globals to prepare for new entry:
sub new_entry {
    @amsgid = ();
    @amsgstr = ();
    $msgid = undef;
    $msgstr = undef;
    $entryline = 0;
    $is_fuzzy = 0;
    $is_cformat = 0;
    $did_print = 0;
    $state = S_LOOKING_START;
}

# Nicely print either a "msgid" or "msgstr" (name is one of these) 
# with given array of data.
sub print_one {
    my $name = shift;
    print "  $name \"", join("\"\n  \"", @_), "\"\n";
}

# Print a problem (args like print()), preceeded by entry unless
# we have already printed that: label, and msgid and msgstr.
#
sub print_problem {
    unless ($did_print) {
	print "ENTRY:", ($ARGV eq "-" ? "" : " ($ARGV, line $entryline)"), "\n";
	print_one("msgid", @amsgid);
	print_one("msgstr", @amsgstr);
	$did_print = 1;
    }
    print "*** ", @_;
}

# Check final newline: probably, translations should end in a newline 
# if and only if the original string does. 
# (See also check_trailing_whitespace and check_num_newlines below.)
#
sub check_trailing_newlines {
    if ($opt_x) { return; }

    my ($ichar, $schar);
    
    $ichar = (length($msgid)>=2) ? substr($msgid, -2, 2) : "";
    $schar = (length($msgstr)>=2) ? substr($msgstr, -2, 2) : "";

    if ($ichar eq "\\n" && $schar ne "\\n") {
	print_problem "Missing trailing newline\n";
    }
    if ($ichar ne "\\n" && $schar eq "\\n") {
	print_problem "Extra trailing newline\n";
    }

}

# Check leading whitespace.  In general, any leading whitespace should 
# be the same in msgstr and msgid -- but not always.
#
sub check_leading_whitespace {
    unless ($opt_w) { return; }

    my ($id, $str);

    if ($msgid =~ m/^(\s+)/) {
	$id = $1;
    } else {
	$id = "";
    }
    if ($msgstr =~ m/^(\s+)/) {
	$str = $1;
    } else {
	$str = "";
    }
    if ($id ne $str) {
	print_problem "Different leading whitespace\n";
    }
}

# Check trailing whitespace.  In general, any trailing whitespace should 
# be the same in msgstr and msgid -- but not always.
#
sub check_trailing_whitespace {
    unless ($opt_W) { return; }

    my ($id, $str);

    if ($msgid =~ m/((?:\s|\\n)+)$/) {
	$id = $1;
    } else {
	$id = "";
    }
    if ($msgstr =~ m/((?:\s|\\n)+)$/) {
	$str = $1;
    } else {
	$str = "";
    }
    if ($id ne $str) {
	print_problem "Different trailing whitespace\n";
    }
}

# Check equal numbers of newlines.  In general ... etc.
#
sub check_num_newlines {
    unless ($opt_n) { return; }
    
    my $num_i = ($msgid =~ m(\\n)g);
    my $num_s = ($msgstr =~ m(\\n)g);

    if ($num_i != $num_s) {
	print_problem "Mismatch in newline count\n";
    }
     
}

# Check capitalization of first non-whitespace character (for [a-zA-Z] 
# only).  In general ... etc.
#
sub check_leading_capitalization {
    unless ($opt_c) { return; }

    my ($id, $str);

    if ($msgid =~ m/^\s*([a-zA-Z])/) {
	$id = $1;
    } 
    if ($msgstr =~ m/^\s*([a-zA-Z])/) {
	$str = $1;
    }
    if (defined($id) && defined($str)) {
	if (($id =~ /^[a-z]$/ && $str =~ /^[A-Z]$/) ||
	    ($id =~ /^[A-Z]$/ && $str =~ /^[a-z]$/)) {
	    print_problem "Different leading capitalization\n";
	}
    }
}

# Check trailing 'punctuation' characters (ignoring trailing whitespace).
# In general .. etc.
#
sub check_trailing_punctuation {
    unless ($opt_p) { return; }

    my ($id, $str);

    # Might want more characters: 
    if ($msgid =~ m/([\\\.\/\,\!\?\"\'\:\;])+(?:\s|\\n)*$/) {
	$id = $1;
    } else {
	$id = "";
    }
    if ($msgstr =~ m/([\\\.\/\,\!\?\"\'\:\;])+(?:\s|\\n)*$/) {
	$str = $1;
    } else {
	$str = "";
    }
    ##print "$id $str\n";
    if ($id ne $str) {
	print_problem "Different trailing punctuation\n";
    }
}

# Check that multiline strings have whitespace separation, since
# otherwise, eg:
#   msgstr "this is a multiline"
#          "string"
# expands to:
#   "this is a multilinestring"
#
sub check_whitespace_joins {
    if ($opt_x) { return; }

    my $ok = 1;
    my $i = 0;

    foreach my $aref (\@amsgid, \@amsgstr) {
	my $prev = undef;
      LINE:
	foreach my $line (@$aref) {
	    if (defined($prev)
		&& length($prev)
		&& $prev !~ /\s$/ 
		&& $prev !~ /\\n$/
		&& $line !~ /^\s/
		&& $line !~ /^\\n/) 
	    {
		$ok = 0;
		last LINE;
	    }
	    $prev = $line;
	}
	if (!$ok) {
	    print_problem("Possible non-whitespace line-join problem in ",
			  ($i==0 ? "msgid" : "msgstr"), " \n");
	}
	$i++;
    }
}

# Check printf-style format entries.
# Non-trivial, because translation strings may use format specifiers
# out of order, or skip some specifiers etc.  Also gettext marks
# anything with '%' as cformat, though not all are.
#
sub check_cformat {
    unless ($is_cformat) { return; }
    if ($opt_x)          { return; }

    my (@iform, @sform);
    @iform = ($msgid =~ m/\%[0-9\.\$]*[a-z]/g);
    @sform = ($msgstr =~ m/\%[0-9\.\$]*[a-z]/g);

    ##print join("::", @iform), "\n";
    ##print join("::", @sform), "\n";

    my $js;			# index in sform
    my $j;			# index into iform
  SFORM:
    for ($js=0; $js < @sform; $js++) {
	my $sf = $sform[$js];
	my $sf_orig = $sf;
	if ($sf =~ s/^\%([0-9]+)\$(.*[a-z])$/\%$2/) {
	    $j = $1-1;
	} else {
	    $j = $js;
	}
	if ($j > $#iform) {
	    print_problem("Format number mismatch for $sf_orig [msgstr:",
			  ($js+1), "]\n");
	    next SFORM;
	}
	my $if = $iform[$j];
	if ($sf ne $if) {
	    print_problem("Format mismatch: $sf_orig [msgstr:", ($js+1), "]",
			  " vs $if [msgid:", ($j+1), "]\n");
	}
    }
}

# Run all individual checks on current entry, reporting any problems.
sub check_entry {
    if ($is_fuzzy) {
	return;
    }
    $msgid = join("", @amsgid);
    $msgstr = join("", @amsgstr);

    unless ($opt_x) {
	if (length($msgid)==0) {
	    print_problem "Zero length msgid\n";
	}
    }
    if (length($msgstr)==0) {
        unless ($opt_e) { return; }
        print_problem "Untranslated msgid\n";
    }
    check_cformat;
    check_whitespace_joins;
    check_num_newlines;
    check_leading_whitespace;
    check_trailing_newlines;
    check_trailing_whitespace;
    check_leading_capitalization;
    check_trailing_punctuation;
}

new_entry;

LINE:
while(<>) {
    if ( m(^\s*$) ) {
	if ($state==S_DOING_MSGSTR) {
	    check_entry;
	    new_entry;
	}
	next LINE;
    }
    if ( m(^\#, fuzzy) ) {
        $is_fuzzy = 1;
    }
    if ( m(^\#, .*c-format) ) {
        # .* is because can have fuzzy, c-format
        $is_cformat = 1;
    }
    if ( m(^\#) ) {
        next LINE;
    }
    if ( m(^msgid \"(.*)\"$) ) {
        $entryline = $.;
        @amsgid = ($1);
        $state = S_DOING_MSGID;
        next LINE;
    }
    if ( m(^msgid_plural \"(.*)\"$) ) {
        $entryline = $.;
        @amsgid = ($1);
        $state = S_DOING_MSGID;
        next LINE;
    }
    if ( m(^msgstr \"(.*)\"$) ) {
        @amsgstr = ($1);
        $state = S_DOING_MSGSTR;
        next LINE;
    }
    if ( m(^msgstr\[[0-2]\] \"(.*)\"$) ) {
        @amsgstr = ($1);
        $state = S_DOING_MSGSTR;
        next LINE;
    }
    if ( m(^\"(.*)\"$) ) {
        if ($state==S_DOING_MSGID) {
    	    push @amsgid, $1;
        } elsif($state==S_DOING_MSGSTR) {
    	    push @amsgstr, $1;
        } else {
	    die "Looking at string $_ in bad state $state,";
        }
        next LINE;
    }
    die "Unexpected at $.: ", $_;
}
