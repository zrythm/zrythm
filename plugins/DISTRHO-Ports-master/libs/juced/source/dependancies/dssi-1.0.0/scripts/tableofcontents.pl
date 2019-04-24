#!/usr/bin/perl -w
use strict;

# Add a table of contents to a basic HTML file.
# Useless if you have lots of layout, this is just for
# document-structured HTML with <h1>s and things.

# For a table of contents, surround all the stuff you want tabulated
# by a <contents>...</contents> pair.  <contents> will be replaced by
# a TOC for all <hX> tags from there up to </contents>, and <hX> will
# be given <a name=...> provided the </hX> is also present.

my $tocmode = 0;
my $outbuf;
my $toc;
my $toccount = 1;

my $ilevel = 0;
my $ibase = -1;

my $enddiv = "";
my $oddeven = "oddcontent";
my @sectionNumbers;

sub indentTo {
    my $level = shift;
    if ($ibase < 0) { $ibase = $level; $ilevel = $ibase-1; }
    if ($level == 0 and $ibase > 0) { $level = $ibase-1; }

    while ($level > $ilevel) {
#        $toc .= "<ul>";
        $ilevel ++;
    }
    while ($level < $ilevel) {
        undef $sectionNumbers[$ilevel];
#        $toc .= "</ul>";
        $ilevel --;
    }
}

sub newSectionNumber {
    my $level = shift;
    my $i = ($ibase < 0 ? 1 : $ibase);
    my $secno;
    while ($i <= $level) {
        if (!defined $sectionNumbers[$i]) { $sectionNumbers[$i] = 1; }
        elsif ($i == $level) { ++$sectionNumbers[$i]; }
        $secno .= $sectionNumbers[$i] . ".";
        ++$i;
    }
#    chop $secno;
    $secno;
}

while (<>) {
    if (!$tocmode) {
        m{<contents>}i and do {
            s{<contents>}{}i;
            $tocmode = 1;
        }
    }
    if (!$tocmode) { print; next; }

    # okay, we're building up the table-of-contents and storing
    # ongoing data into $outbuf in the meantime

    m{</contents>}i and do {
        $tocmode = 0;
        if (!defined $toc) { print "$outbuf\n$enddiv\n"; }
        else {
            indentTo 0;
#            print "<h2>Contents</h2>\n";
	    print "$toc\n$outbuf\n$enddiv\n";
        }
        next;
    };

    m{<h(\d)>(.*?)</h(\d)>}i and do {
        warn "<H$1> closed by <H$3>" if ($1 != $3);
        indentTo $1;
        my $secno = newSectionNumber $1;

#        $toc .= "<span class=\"toc$1\">$secno &nbsp;<a href=\"#toc$toccount\">$2</a></span><br>\n";
        $toc .= "<div class=\"toc$1\">$secno &nbsp;<a href=\"#toc$toccount\">$2</a></div>\n";

        s{(<h\d>)(.*?)(</h\d>)}
         {$enddiv<div class="$oddeven"><a name="toc$toccount"></a>$1$secno &nbsp;$2$3}i;

        $enddiv = "</div>";
        $oddeven = ($oddeven eq "oddcontent" ? "evencontent" : "oddcontent");
        $toccount++;
    };

    $outbuf .= $_;
}

if ($tocmode) {
    warn "Unclosed <contents>";
    indentTo 0;
#    print "<h2>Contents</h2>\n";
    print "$toc\n$outbuf";
}

