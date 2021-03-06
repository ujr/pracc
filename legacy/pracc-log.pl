#!/usr/bin/perl -w
#
# Create a HTML report from the pracc log file.
# Access to this CGI script should be protected by the
# web server. The value of the REMOTE_USER env variable
# must occur in the @MASTERS array for access to be
# granted. (Later: check user is in group pracc)
# 
# ujr/2007-01-04 started

my $PROGNAME = $0; $PROGNAME =~ s:^.*/::; # basename
my $PRACCLOG = "/var/print/bin/pracc-log";
my @MASTERS = ("ujr","beni","ortwin","hermann","ghenrich","claudio");

my $SCRIPT = $ENV{"SCRIPT_NAME"} || $PROGNAME;
my $QUERYSTRING = $ENV{"QUERY_STRING"};
my %QUERY = (); # ditto, parsed into a hash
my $METHOD = $ENV{"REQUEST_METHOD"} || "none";
if ($METHOD eq "POST") { $QUERYSTRING = <STDIN>; }
foreach my $pair (split /&/, $QUERYSTRING || "") {
  my ($name, $value) = split /=/, $pair;
  $QUERY{$name} = $value;
}

my $first;
$first = $QUERY{"first"} if exists $QUERY{"first"};
$first = &todayminus(100) unless exists $QUERY{"first"};

my $until;
$until = $QUERY{"until"} if exists $QUERY{"until"};
$until = &todayminus(0) unless exists $QUERY{"until"};

my $acctname;
$acctname = $QUERY{"acctname"} if exists $QUERY{"acctname"};

# Print HTTP and HTML header stuff:
print "Content-type: text/html\r\n\r\n"; # HTTP header
print "<html><head><title>Printer Accounting</title>\n";
print "<style type=\"text/css\"><!--\n";
print " body{background-color:white;font-family:sans-serif;margin:2pc}\n";
print " .box{background-color:#ccc;padding:8px;border:1px solid #444}\n";
print "--></style>\n</head><body><!--auto-generated html-->\n";
print "<h1>Printer Accounting: Log File</h1>\n";

# Bail out if user is not authenticated:
$ENV{'REMOTE_USER'}="ujr"; # DEBUG
unless (exists $ENV{"REMOTE_USER"}) {
  print "<p>Sorry, you are not authenticated. This is not supposed\n";
  print " to happen, so please notify the system administrators at\n";
  print " " . &mailto($CONTACT) . "\n Thank you!</p>\n";
  print "<p>Tech info: env <tt>REMOTE_USER</tt> is missing.</p>\n";
  print "</body></html>\n";
  exit 0;
}

# Bail out if user is not authorised:
my $user = $ENV{"REMOTE_USER"};
unless (&inarray($user, @MASTERS)) {
  print "<p>Sorry, you ($user) are not allowed to use this tool.\n";
  print " If you think this should be otherwise, please contact\n";
  print " the system administrators.</p>\n";
  print "</body></html>\n";
  exit 0;
}

# Print the options dialog box:
print "<form method=\"GET\" action=\"$SCRIPT\">\n";
print "<p><table class=\"box\" border=0 cellpadding=8>\n";
print "<tr><td>Auszug\n";
print " <b>von</b>";
print " <input type=\"text\" name=\"first\" value=\"$first\" size=15>\n";
print " <b>bis</b>";
print " <input type=\"text\" name=\"until\" value=\"$until\" size=15>\n";
print " <input type=\"submit\" value=\"zeigen\"><br>\n";
print " <i>Datum im Format JJJJ-MM-TT eingeben;\n";
print " leerlassen f&uuml;r kompletten Auszug.</i></td></tr>\n";
unless (($first =~ m/^[0-9]+-[0-9][0-9]-[0-9][0-9]$/ or !$first) ||
        ($until =~ m/^[0-9]+-[0-9][0-9]-[0-9][0-9]$/ or !$until)) {
  print "<tr><td bgcolor=#cc0000>Bitte Daten im Format YYYY-MM-DD\n";
  print " eingeben, zB 2005-12-24</td></tr>\n";
}
print "<tr><td><b>Typen:</b>\n";
print " " . &radiobtn('type', '', 1) . "&nbsp;Alle,\n";
print " " . &radiobtn('type', 'init') . "&nbsp;init,\n";
print " " . &radiobtn('type', 'debit') . "&nbsp;debit,\n";
print " " . &radiobtn('type', 'credit') . "&nbsp;credit,\n";
print " " . &radiobtn('type', 'limit') . "&nbsp;limit,\n";
print " " . &radiobtn('type', 'note') . "&nbsp;note,\n";
print " " . &radiobtn('type', 'purge') . "&nbsp;purge\n";
print " <input type=\"submit\" value=\"zeigen\"></td></tr>\n";
print "<tr><td>Nur Modifikationen am <b>Konto</b>\n";
print " <input type=\"text\" name=\"acctname\" value=\"\" size=20>\n";
print " <input type=\"submit\" value=\"zeigen\"><br>\n";
print " <i>Benutzername eingeben;\n";
print " leer lassen um alle Konten zu sehen.</i></td></tr>\n";
print "</table></p>\n</form>\n";

my $type = $QUERY{'type'};
my $logopts = ""; # options to pracc-log
$logopts .= " -t $type" if ($type);
$logopts .= " -f $first" if ($first);
$logopts .= " -u $until" if ($until);
$logopts .= " $acctname" if ($acctname);
open PRACCLOG, "$PRACCLOG $logopts 2>&1 |"
 or &error("Error running $PRACCLOG");

print "<p>Command: <b>$PRACCLOG $logopts</b></p>\n"; # XXX DEBUG
print "<pre>\n";
while (my $line = <PRACCLOG>) {
  chomp($line);
  my ($date, $time, $acct, $action, $args) = split /\s+/, $line, 5;

  print "$line\n"; # Nice table TODO
}
close PRACCLOG;
print "</pre>\n";

# Finish HTML file and quit!
print "</body>\n</html>\n";
exit 0;

sub error {
  my $msg = shift;
  print "<p><b>Error:</b> $msg</p>\n";
  print "</body>\n</html>\n";
  exit 0;
}

# Today minus the given number of days, ISO-formatted:
sub todayminus {
  my $days = shift;
  my $secs = $days*86400;
  my @tm = localtime(time()-$secs);
  my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = @tm;
  return sprintf "%d-%02d-%02d", 1900+$year, 1+$mon, $mday;
}

# Return a string representing a HTML checkbox:
#sub checkbox {
#  my $name = shift;    # required
#  my $value = shift;   # required
#  my $checked = shift; # optional, unchecked if false or missing
#  my $result =  qq/<input type="checkbox" name="$name" value="$value"/;
#  $result .= qq/ checked="1"/ if ($checked);
#  return $result . ">";
#}

# Return a string representing a HTML radiobutton:
sub radiobtn {
  my $name = shift;
  my $value = shift;
  my $checked = shift; # optional; unchecked if false or missing
  my $c = ($checked) ? "checked" : "";
  my $result = qq/<input type="radio" name="$name" value="$value" $c>/;
  return $result;
}

# Return true if shift is in @_, false otherwise.
sub inarray { my $val = shift;
  foreach (@_) { return 1 if ($val eq $_); }
  return 0; # not in array
}
