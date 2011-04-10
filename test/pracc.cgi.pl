#!/usr/bin/perl -w
#
# Create an HTML report of the print credit history for the user
# named in env REMOTE_USER (this is a CGI variable, specified if
# a remote user successfully authenticated him/herself.
#
# We've to handle the cases where no REMOTE_USER is supplied, or
# REMOTE_USER is an unknown user or has printer accounting disabled.
#
# Users listed in @MASTERS below can look at the print credits of
# any user by appending the user's login name as query string to
# the URL of this script: pracc.cgi?username; this feature was
# announced on 2005-01-27 in the TechBit Series to team@ict.
#
# ujr/2003-09-12 started
# ujr/2005-01-27 added /?username feature for "master" users
# ujr/2005-07-18 adapted for pracc v2
# ujr/2005-11-17 changed to handle both v1 and v2 pracc files

my $PROGNAME = $0; $PROGNAME =~ s:^.*/::;  # basename
my $VERSION = "2005-11-17";
my $UNLIMITED = "9999999";
my $PRACCDIR = "/var/print/pracc";
my $TITLE = "Drucksystem PZM und PHZ Luzern";
my $CONTACT = "ujr\@ict.pzm-luzern.ch";
my $DEBUG = 1;  # set to zero once confident!
my @COLORS = ('#bbddbb', '#AAEEAA', '#DDCCBB', '#EEDDCC');
my @MASTERS = ("ujr","beni","patrick","ortwin","hermann","ghenrich","claudio");

unless (exists $ENV{"REMOTE_USER"}) {
  &header($TITLE, "Not Authenticated");
  print "<p>Sorry, you are not authenticated. This is not supposed\n";
  print " to happen, so please notify the system administrators at\n";
  print " <a href=\"mailto:$CONTACT\">$CONTACT</a>.\n";
  print " Thank you!</p>\n";
  print "<p>Tech info: env <tt>REMOTE_USER</tt> is missing.</p>\n";
  &finish();
}

my $login = $ENV{"REMOTE_USER"};
my $master = &inarray($login, @MASTERS);

my $query = $ENV{"QUERY_STRING"};
if ($master && $query) { $login = $query; }

my @infos = getpwnam($login);
unless (@infos) {
  &header($TITLE, "Unbekannter Benutzer");
  print "<p>&Uuml;ber Benutzer &quot;$login&quot; k&ouml;nnen\n";
  print " keine Informationen angezeigt werden.\n";
  print " &Uuml;berpr&uuml;fen Sie den Benutzernamen.\n";
  print " F&uuml;r weitere Informationen kontaktieren Sie:\n";
  print " <a href=\"mailto:$CONTACT\">$CONTACT</a></p>\n";
  &finish();
}
my ($name,$passwd,$uid,$gid,$quota,$comment,$gcos,$dir,$shell) = @infos;
my $realname = $gcos; $realname =~ s/,.*$//;

my $praccfile = "$PRACCDIR/$login";
unless (open PRACCFILE, $praccfile) {
  &header("$TITLE", "Kein Printer Accounting");
  print "<p>F&uuml;r Benutzer $login ($realname)\n";
  print " ist kein Printer Accounting eingerichtet.</p>\n";
  &finish();
}

# pass 1: summarize account

my $balance = 0;
my $limit = $UNLIMITED;

while ($line = <PRACCFILE>) { chomp($line);
  if ($line =~ m/^\-([0-9]+)/) { $balance -= $1; }
  if ($line =~ m/^\+([0-9]+)/) { $balance += $1; }
  if ($line =~ m/^\=(-?[0-9]+)/) { $balance = $1; }
  if ($line =~ m/^\$(-?[0-9]+)/) { $limit = $1; }
  if ($line =~ m/^\$\*/) { $limit = $UNLIMITED; }
}

&header("$TITLE", "Auszug Druck-Konto");
print "<p>Benutzername: <b>$login</b> ($realname)<br>\n";
print " Kontostand: ";
if (($limit eq $UNLIMITED) || ($balance > $limit)) {
  printf("<b>Fr %.2f</b><br>", $balance/100);
}
else {
  printf("<b><font color=\"#CC0000\">Fr %.2f</font></b><br>", $balance/100);
}
if ($limit >= $UNLIMITED) {
  print " Kontolimite: unlimitiert</p>\n";
}
else {
  printf(" Kontolimite: Fr %.2f</p>\n", $limit/100);
}
print "<p>Neue Druck-Credits k&ouml;nnen auf dem Sekretariat (PZM)\n";
print " bzw der Kanzlei (PHZ) erworben werden. Bitte halten Sie das\n";
print " entsprechende Formular und den passenden Geldbetrag bereit.</p>\n";


if (&inarray($login, @MASTERS)) {
  print "<p><b>Hint:</b> You're in the masters list, which gives\n";
  print " you the power to see other users' printing accounts.\n";
  print " To do so, append &quot;?username&quot; to the current\n";
  print " URL in your browser.\n";
  print " Contact <a href=\"mailto:$CONTACT\">$CONTACT</a> for\n";
  print " more information.</p>\n";
}
  
# rewind pracc file for 2nd pass

unless(seek PRACCFILE, 0, 0) {
  print "<p>Sorry, a system error occurred:\n";
  print " seek on $praccfile failed: $!</p>\n";
  &finish();
}

# pass 2: show details

my ($odd, $even);
print "<p><table border=\"0\" cellpadding=\"4\">\n";
print "<tr><th bgcolor=\"#aaaaaa\">Datum</th>\n";
print "    <th bgcolor=\"#aaaaaa\">Zeit</th>\n";
print "    <th bgcolor=\"#aaaaaa\">Aktion</th>\n";
print "    <th bgcolor=\"#aaaaaa\">Betrag</th>\n";
print "    <th bgcolor=\"#aaaaaa\">Infos</th></tr>\n";
while ($line = <PRACCFILE>) { chomp($line);
  # reset line
  if ($line =~ m/^\=(-?[0-9]+)\s+@([^T]+)T([^Z]+)Z\s+(.*)/ ||
      $line =~ m/^\=(-?[0-9]+)\s+date\s+([^\s]+)[\stime]+([^\s]+)\s+(.*)/) {
  	($odd, $even) = getcolor();
  	print "<tr><td bgcolor=\"$odd\">$2</td>\n";   # date
  	print "    <td bgcolor=\"$even\">$3</td>\n";  # time
  	print "    <td bgcolor=\"$odd\"><b>Saldo</b></td>\n";
  	print "    <td bgcolor=\"$even\" align=\"right\">$1</td>\n";
  	my $note = ($4 eq 'saldo') ? '&nbsp;' : $4;
  	print "    <td bgcolor=\"$odd\">$note</td></tr>\n";
  }
  # credit line
  if ($line =~ m/^\+([0-9]+)\s+@([^T]+)T([^Z]+)Z\s+(.*)/ ||
      $line =~ m/^\+([0-9]+)\s+date\s+([^\s]+)[\stime]+([^\s]+)\s+(.*)/) {
  	($odd, $even) = getcolor();
  	print "<tr><td bgcolor=\"$odd\">$2</td>\n";   # date
  	print "    <td bgcolor=\"$even\">$3</td>\n";  # time
  	print "    <td bgcolor=\"$odd\"><b>Credit</b></td>\n";
  	print "    <td bgcolor=\"$even\" align=\"right\">+$1</td>\n";
  	print "    <td bgcolor=\"$odd\">$4</td></tr>\n";
  }
  # debit line
  if ($line =~ m/^\-([0-9]+)\s+@([^T]+)T([^Z]+)Z\s+(.*)/ ||
      $line =~ m/^\-([0-9]+)\s+date\s+([^\s]+)[\stime]+([^\s]+)\s+(.*)/) {
  	($odd, $even) = getcolor();
  	print "<tr><td bgcolor=\"$odd\">$2</td>\n";   # date
  	print "    <td bgcolor=\"$even\">$3</td>\n";  # time
  	print "    <td bgcolor=\"$odd\">Ausdruck</td>\n";
  	print "    <td bgcolor=\"$even\" align=\"right\">-$1</td>\n";
  	print "    <td bgcolor=\"$odd\">$4</td></tr>\n";
  }
}
print "</table></p>\n";
close PRACCFILE;

&finish();
exit 0;


##----------------------------------------------------------------
# acctinfo - invoke pracc-sum for the account named in $1;
#            set global variables $balance and $limit;
#            return null on error.
#
#sub acctinfo {
#  my $acctname = shift;
#  my $info = `$PRACCSUM $acctname`;
#  my $retval = $? >> 8;  # XXX
#  return () if ($? & 255);
#  chomp($info);
#  my @info = split /\s+/, $info;
#  if ($info[5] eq "*" || $info[5] eq "none") {$info[5]=$UNLIMITED;}
#  my @ret = ($info[3], $info[5]);
#  return @ret;
#}

##----------------------------------------------------------------
# Parse arg1 into a date string, a time string, and a comment
# string; return those in an array.
#
sub parseline {
  my $line = shift;
  if ($line =~ m/\s*@([0-9a-fA-F]+)\s+(.*)/) {
  	my $taistamp = $1;
  	my $comment = $2;
  	my $t = &taiunix($taistamp);
  	my @tm = localtime $t;
  	my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = @tm;
	return (sprintf("%d-%02d-%02d", 1900+$year, 1+$mon, $mday),
	        sprintf("%02d:%02d:%02d", $hour, $min, $sec),
		$comment);
  }
  elsif ($line =~ m/\s*([^\s]+)\s+([^\s]+)\s+(.*)/) {
  	return ($1, $2, $3);
  }
  return undef; # no match
}
sub taiunix {
  my @dig = split //, shift;
  my $acc = 0;
  foreach my $digit (@dig) { $acc *= 16; $acc += &unhex($digit); }
  return $acc;
}
sub unhex {
  my $digit = shift;
  return 0 if ($digit eq "0");
  return 1 if ($digit eq "1");
  return 2 if ($digit eq "2");
  return 3 if ($digit eq "3");
  return 4 if ($digit eq "4");
  return 5 if ($digit eq "5");
  return 6 if ($digit eq "6");
  return 7 if ($digit eq "7");
  return 8 if ($digit eq "8");
  return 9 if ($digit eq "9");
  return 10 if ($digit eq "a" || $digit eq "A");
  return 11 if ($digit eq "b" || $digit eq "B");
  return 11 if ($digit eq "c" || $digit eq "C");
  return 11 if ($digit eq "d" || $digit eq "D");
  return 11 if ($digit eq "e" || $digit eq "E");
  return 15 if ($digit eq "f" || $digit eq "F");
  die "invalid hex digit";
}

##----------------------------------------------------------------
# getcolor - Return one of two color pairs, strictly alternating
#            between invocations (useful for coloring rows in a
#            table: even rows get slightly other colors than odd rows.
#
my $colrow = 0;
sub getcolor {
  $colrow += 1;
  my $index = 2 * ($colrow % 2);
  my @colors = @COLORS[$index, $index+1];
  return @colors;
}
  
##----------------------------------------------------------------
# header - Start the HTML output by writing the header tags
#          and starting the body.
#   arg1:  text for the <title> element
#   arg2:  text for <h1> in body (optional)
#
sub header {
  my $title = shift;
  my $heading = shift;

  if ($header_output) {return;}

  print "Content-type: text/html\r\n\r\n";  # HTTP header

  print "<html><head><title>$title</title>\n";
  #print "<link rel=\"stylesheet\" type=\"text/css\" href=\"pracc.css\">";
  print "<style type=\"text/css\"><!--\n";
  print "body {background-color:white;font-family:sans-serif;margin:2pc}\n";
  print "--></style>\n";
  print "</head><body>\n";
  print "<h1>$heading</h1>\n" if defined $heading;

  $header_output = 1;
}

##----------------------------------------------------------------
# finish - Finish the HTML output by writing a footer line
#          and the closing tags to stdout.
#
sub finish {
  my @tm = localtime;
  my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = @tm;
  $year += 1900; $mon += 1;  # adjust ranges

  print "<div align=\"right\"><hr>\n<i>";
  printf "Generated on %d-%02d-%02d at %02d:%02d:%02d<br>\n",
  	$year, $mon, $mday, $hour, $min, $sec;
  print "by $PROGNAME, version $VERSION";
  print "<br>\nid $>:$)" if $DEBUG;
  print "</i></div>\n</body></html>\n";
  exit 0;
}

##----------------------------------------------------------------
# inarray - true if shift is in @_
#
sub inarray { my $val = shift;
  foreach (@_) { return 1 if ($val eq $_); }
  return 0;
}
