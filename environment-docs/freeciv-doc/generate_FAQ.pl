#!/usr/bin/perl

$test_data = "";
# uncomment the following to run from named file
#$test_data = "sample.txt";

$faq_url = "http://www.freeciv.org/wiki/FAQ";
$faq_file = "FAQ";

@sections;
@sectStrings;

@content;
$contentLine;
$EOD = 0;
$line;
@graph;

open(FILE, ">$faq_file") || die ("Cannot open file $faq_file for writing.\n");

# collect input data
&init_data;

# read data and write toc
&processToc;

&processAnswers;

close(FILE);

print "File $faq_file has been written successfully\n";

# collect table of contents
sub processToc
{

  # clear to FAQ
  if ( &clearTo("FAQ") || &clearTo("Contents") ) { die "No TOC found\n"; }

  # dump TOC

  # header
  print FILE <<HEND;
                  Freeciv Frequently Asked Questions (FAQ)

                 (from $faq_url)

   Contents

HEND

  for ( ; ; )
  {
    &readLine || die "Data runout reading TOC\n";
    if ( $line =~ /^\s*$/ )
    {
      print FILE "\n";
      return;
    }
    if ( $line =~ /^\s*(.) ([\.\d]+) (.+)$/ )
    {
      #handle section or question
      my $type = $1;
      my $num = $2;
      my $name = $3;
      my $pattern = " \\$type";
      $line =~ s/$pattern//;
      if ( $type eq "*" )
      {
	push( @sections, $name );
	my $l = @sections;
	if ( $l != $num ) { die "Section $l number is $num\n"; }
	push( @sectStrings, [] );
      }
      else
      {
	my $ref = $sectStrings[-1];
	push( @$ref, $num );
      }
    }
    print FILE "$line\n";
  }

}

sub clearTo
{
  for ( ; ; )
  {
    &readLine  || return 1;
    if ( $line =~ /^\s*$_[0]$/ ) { last; }
  }
  &processBlankLine;
}

# get line, required blank
sub processBlankLine
{
  if ( ! &readLine ) { die "Data runout in processBlankLine\n"; }
  if ( $line!~/^\s*$/ ) { die "processBlankLine read '$line'\n"; }
}

# write answer section
sub processAnswers
{
  my $ix = 0;
  my $sName;
  my @nums;
  while ( ! $EOD ) {
    my $type = &readGraph;
    # handle section or question
    if ( $type == 2 )
    {
      print FILE "-------\n\n";
      if ( $graph[0] eq $sections[$ix] )
      {
	$sName = $sections[$ix];
	my $nr = $sectStrings[$ix];
	@nums = @$nr;
	++$ix;
	&writeGraph( $ix );
      }
      else
      {
	my $num = shift( @nums );
	&writeGraph( $num );
      }
    }
    else
    {
      &writeGraph();
    }
  }
}

# read paragraph, return type
#   0: end of answers
#   1: other (part of answer)
#   2: section or question
sub readGraph
{
  &readLine  || die "Data runout in readGraph\n";
  if ( $line eq "" ) { die "Expecting graph, read blank line\n"; }
  @graph = ( $line );
  if ( $line =~ /^\s*^_+$/ ) { return 0; }
  # read rest of graph
  for ( ; ; )
  {
    &readLine  || die "Data runout in readGraph\n";
    if ( $line eq "" ) { last; }
    if ( $line !~ /^\s*_+$/ )
    {
      push( @graph, $line );
    }
    else
    {
      $EOD = 1;
      last;
    }
  }
  # test edit tag
  if ( &editTag ) { return 2; }
  1;
}

# test for and remove edit tag
sub editTag
{
  # look for on last line
  if ( $graph[-1] =~ s/ \[[^]]*\] Edit$// ) { return 1; }
  if ( 2 > @graph ) { return 0; }
  if ( $graph[-1] =~ m/^\[[^]]*\] Edit$/ )
  {
    pop( @graph );
    return 1;
  }
  # look for split
  if ( $graph[-1]eq"Edit" && $graph[-2]=~s/ \[[^]]*\]$// )
  {
    pop( @graph );
    return 1;
  }
  return 0;
}

# write paragraph
sub writeGraph
{
  if ( defined $_[0] ) { print FILE "$_[0] "; }
  while ( @graph )
  {
    my $data = shift( @graph );
    print FILE "$data\n";
  }
  print FILE "\n";
}

# pseudo-i/o methods

# 'open' data 'file'
sub init_data
{
  $contentLine = 0;
  if ( $test_data ) {
    print "using test data $test_data\n";
    open ( MYDATA, $test_data );
    @content = <MYDATA>;
    close MYDATA;
  } else {
    print "using url $faq_url\n";
    @content = `lynx -nolist -dump "$faq_url"`;
  }
  my $count = @content;
  print( "$count lines of content\n" );
}

# 'read' data line
sub readLine
{
  if ( $contentLine < @content )
  {
    $line = $content[$contentLine++];
    $line =~ s/\s+$//;
    1;
  }
  else
  {
    0;
  }
}
