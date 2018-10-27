#!/usr/bin/perl
# Fetch and format data to update NEWS/NEWS-x.x from Freeciv wiki.
# Usage: perl fetchnews.pl URL LOCALFILE
# Some manual tweaks will be required afterward.

$test_data = "";
# uncomment the following to run from named file
#$test_data = "sample.txt";

@content;
$contentLine;
$EOD = 0;
$line;
@graph;

if (@ARGV < 2) {
  die "usage: $0 URL file";
  exit 1;
}
($url, $file) = @ARGV;
$hdr = $url;
$hdr =~ s#^.*/([^/]*)$#$1#;
$hdr =~ s#_# #g;

open(FILE, ">$file") || die ("Cannot open file $file for writing.\n");

# collect input data
&init_data;

if ( &clearTo($hdr) || &clearToPrefix("what's changed since ") )
{ die "Doesn't look like a NEWS page\n"; }

print FILE <<HEND;
CHANGES FROM x.x.x to y.y.y
---------------------------

   (from <$url>)

HEND

&processText;

close(FILE);

print "File $file has been written successfully\n";

sub clearTo
{
  for ( ; ; )
  {
    &readLine  || return 1;
    if ( $line =~ /^\s*$_[0]$/ ) { last; }
  }
  &processBlankLine;
}

# clear to line starting with arg, case-insensitive
sub clearToPrefix
{
  for ( ; ; )
  {
    &readLine  || return 1;
    if ( $line =~ /^\s*$_[0]/i ) { last; }
  }
  &processBlankLine;
}

# get line, required blank
# HACK: a line containing "Edit" on its own is assumed to be run-on from a
# heading and is discarded.
sub processBlankLine
{
  do {
    if ( ! &readLine ) { die "Data runout in processBlankLine\n"; }
  } while ($line eq "Edit");
  if ( $line!~/^\s*$/ ) { die "processBlankLine read '$line'\n"; }
}

# write answer section
sub processText
{
  while ( ! $EOD ) {
    my $type = &readGraph;
    # FIXME do something special for $type==2?
    &writeGraph();
  }
}

# read paragraph, return type
#   0: end of main section
#   1: other (part of body of section)
#   2: section
sub readGraph
{
  &readLine  || die "Data runout in readGraph\n";
  if ( $line eq "" ) { die "Expecting graph, read blank line\n"; }
  @graph = ( $line );
  if ( $line =~ /^\s*Retrieved from/ ) { $EOD = 1; return 0; }
  # read rest of graph
  for ( ; ; )
  {
    &readLine  || die "Data runout in readGraph\n";
    if ( $line eq "" ) { last; }
    if ( $line !~ /^\s*Retrieved from/ )
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
    print "using url $url\n";
    @content = `lynx -nolist -reload -dump "$url"`;
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
