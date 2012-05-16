#!/usr/bin/env ruby -w

require 'open3'

output_html = false

if ARGV[0] == "-h" or ARGV[0] == "--help"
  puts "Generate statistics about *.po files in current directory and sort them by completeness."
  puts "\t-h and --help show this message."
  puts "\t--html outputs a HTML page with a table."
  puts "\tBy default simply outputs sorted msgfmt --stat output."
  exit 0
elsif ARGV[0] == "--html"
  output_html = true
end

d = Dir.new('.')
results = []
=begin Single-threaded version
d.each do |file|
  next unless file =~ /.po$/
  results << [file.split('.')[0], Open3.popen3("msgfmt --stat #{file}") {|sin, sout, serr| serr.readlines[0]}]
end
=end

#multi-threaded version, theoretically number_of_cores times faster. In
#practice, Ruby lacks proper OS thread support, so it'll be just as slow --
#WRONG! Although Ruby runs in a single thread (supposedly), all those
#msgfmts are taking up the huge majority of CPU time, and THEY run in
#separate threads. This still results in double speedups.
thr = []
d.each do |file|
  next unless file =~ /.po$/
  name = file.split('.')[0]
  thr << Thread.new { results << [name, Open3.popen3("msgfmt --stat #{file}") {|sin, sout, serr| serr.readlines[0]}] }
end
thr.each {|i| i.join}

results.sort! {|a, b| b[1].split(' ')[0].to_i <=> a[1].split(' ')[0].to_i}

if output_html

  puts "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n\n<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\" lang=\"en\">"
  puts '<head>'
  puts '<title>Freeciv language statistics</title>'
  puts '<link href="style.css" rel="stylesheet" type="text/css" />'
  puts '</head>'
  puts '<body>'
  puts '<table border="1">'
  puts '<tr><th>Language</th><th>Translated</th><th>Fuzzy</th><th>Untranslated</th></tr>'


  results.each do |line|
    translated = fuzzy = untranslated = 0
    if line[1] =~ /([0-9]+) translated messages?\./
      translated = $1
    elsif line[1] =~ /([0-9]+) translated messages, ([0-9]+) fuzzy translations?\./
      translated = $1
      fuzzy = $2
    elsif line[1] =~ /([0-9]+) translated messages, ([0-9]+) untranslated messages?\./
      translated = $1
      untranslated = $2
    elsif line[1] =~ /([0-9]+) translated messages, ([0-9]+) fuzzy translations, ([0-9]+) untranslated messages?\./
      translated = $1
      fuzzy = $2
      untranslated = $3
    else
      translated = "Error"
      fuzzy = "while parsing msgfmt output"
      untranslated = line[1]
    end
    puts '<tr>'
    puts '<td>' + line[0] + '</td>'
    puts '<td>' + translated.to_s + '</td>'
    puts '<td>' + fuzzy.to_s + '</td>'
    puts '<td>' + untranslated.to_s + '</td>'
    puts '</tr>'
  end
  puts '</table>'
  puts '</body>'
  puts '</html>'

else
  results.each {|i| puts "#{i[0]}: #{i[1]}"}
end
