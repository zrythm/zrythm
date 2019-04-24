#!/usr/bin/perl -w
# 
# A plain-text documentation processor.
# Copyright Chris Cannam 1998

use strict;

my $hbase = 2; # main heading is h$hbase, next is h($hbase+1) etc

my @p;
my $laststyle = "none";

my $title = $ARGV[0];
if (defined $title) { $title =~ s/\.txt//; }
else { $title = "Untitled"; }

my $filename = $ARGV[0];
if (!defined $filename) { $filename = "(stdin)"; }

print '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0//EN" "http://www.w3.org/TR/REC-html40/strict.dtd">';
print "\n<html><head>\n";
print '<meta http-equiv="Content-Type" content="text/html; charset=ISO-8859-1">';
print "\n<title>$title</title></head><body>\n<!-- Do not edit! Automatically generated from $filename by $0 -->\n";
#print "<contents>\n"; # for automatic TOC generator

my $hadtoc = 0;

# Returned strings here are symbolic -- they only resemble HTML.
# Don't change them, change the interpretation of them in write_para

sub style_para
{
    # heading?
    if ($#p == 1) {
	$p[1] =~ s/^=+$// and return "h1";
	$p[1] =~ s/^-+$// and return "h2";
	$p[1] =~ s/^~+$// and return "h3";
    }

    # something wholly indented?
    my @nni = grep { /^\s/ } @p;
    if (@p and $#nni == $#p) {
	$p[0] =~ s/^\s+[^\w\s\$\%\#]\s+([\w\"&])/$1/ and return "uli";
	$p[0] =~ s/^\s+\d+[^\w\s]\s+([\w\"&])/$1/ and return "oli";
	$p[0] =~ s/^\s+[a-z][^\w\s]\s+([\w\"&])/$1/ and return "cli";
	$p[0] =~ s/^(\s+[^\w\s\d])/$1/ and return "pre";
	return "cont";
    }
    
    # anything else
    return "p";
}

sub write_head
{
    my $n = shift;
    my $content = shift;
    chop $content;
    my $m = $hbase + $n - 1;
    print "<h$m>$content</h$m>\n";
} 

sub write_para
{
    my $style = shift;

    if ($style eq "cont" and
	!($laststyle eq "uli" or $laststyle eq "oli" or $laststyle eq "cli")) {
	$style = "pre";
    }

    if (!($style eq $laststyle or $style eq "cont")) {

	if ($laststyle eq "uli") {
	    print "</ul>";
	} elsif ($laststyle eq "oli") {
	    print "</ol>";
	} elsif ($laststyle eq "cli") {
	    print "</ol>";
	} elsif ($laststyle eq "pre") {
	    print "</pre>";
	}

	if ($style eq "uli") {
	    print "<ul>";
	} elsif ($style eq "oli") {
	    print "<ol>";
	} elsif ($style eq "cli") {
	    print "<ol type='a'>";
	} elsif ($style eq "pre") {
	    print "<pre>";
	}
    }

    print "\n";

    if (!$hadtoc and $style ne "h1") {
        print "<CONTENTS>\n";
        $hadtoc = 1;
    }

    if ($style eq "h1") {
        write_head 1, $p[0];
    } elsif ($style eq "h2") {
        write_head 2, $p[0];
    } elsif ($style eq "h3") {
        write_head 3, $p[0];
    } elsif ($style eq "uli" or $style eq "oli" or $style eq "cli") {
	print "<br/><br/></li>" if ($laststyle eq $style);
	$p[0] =~ s/^\s+(?:\d+)?[^\w\s]\s+//;
	print "<li>@p";
    } elsif ($style eq "cont") {
	$style = $laststyle;
	if ($style eq "uli" or $style eq "oli" or $style eq "cli") {
	  print "<br/><br/>@p";
	} else {
	  print "<p>@p";
	}
    } elsif ($style eq "p") {
	print "<p>@p</p>";
    } else {
	print @p;
    }

    if (!$hadtoc and $style eq "h1") {
        print "<CONTENTS>\n";
        $hadtoc = 1;
    }

    $laststyle = $style;
}

while (<>)
{
    s/&/&amp;/g;
    s/</&lt;/g;
    s/>/&gt;/g;

    s, &lt; URL: ( [^&:\"]* ) .txt &gt; ,<A HREF="$1.html">$1.html</A>,xg;
    s, &lt; URL: ( [^&\"]*  )      &gt; ,<A HREF="$1">$1</A>,xg;

    s,    \*( \w[\w\s0-9-]* )\*    ,<B>$1</B>,xg;
    s, \b  _( \w[\w\s0-9-]* )_  \b ,<I>$1</I>,xg;

    s, \\([\[\]]) ,$1,xg or s, \[( .*? )\] ,<CODE>$1</CODE>,xg;

    if (/^\s*$/) {
	write_para style_para if $#p >= 0;
	undef @p;
    } else {
	push @p, $_;
	if (/^[=~-]+$/ && $#p == 1) {
	    write_para style_para;
	    undef @p;
	}
    }
}

#write_para style_para if defined @p;
write_para style_para;
print "\n</CONTENTS>\n</body>\n</html>\n";


