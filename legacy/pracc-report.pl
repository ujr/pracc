#!/usr/bin/perl -w
#
# Create an HTML report of the print credit history for the user
# named in env REMOTE_USER (this is a CGI variable, specified if
# a remote user successfully authenticated him/herself).
#
# We've to handle the cases where no REMOTE_USER is supplied, or
# REMOTE_USER is an unknown user or has printer accounting disabled.
#
# Users in group $ADMINGROUP (pracc-admin) can look at the print
# credits of any user by adding user=name to the query string.
# Announced on 2005-01-27 in the TechBit Series to team@ict.
#
# ujr/2003-09-12 started
# ujr/2005-01-27 added /?username feature for "master" users
# ujr/2005-07-18 adapted for pracc v2
# ujr/2005-11-17 changed to handle both v1 and v2 pracc files
# ujr/2005-12-22 use pracc-sum and pracc-view; etc.
# ujr/2006-05-18 implemented options box above table
# ujr/2007-12-14 added preset periods and debit/credit summary
# ujr/2007-12-17 added CSV export and did some cleanup
# ujr/2007-12-18 adjusted to new pracc-sum output format
# ujr/2007-12-21 show "Dieser Monat" by default
# ujr/2007-12-21 check if $user in $ADMINGROUP instead of @MASTERS
# ujr/2007-12-21 Submit and button didn't work with IE. Now two submits.
# ujr/2008-01-24 adjusted to new pracc-view output (username)
# TODO: drop ?user=username feature once pracc-admin is ready.

my $PROGNAME = $0; $PROGNAME =~ s:^.*/::;  # basename
my $VERSION = "2007-12-21";
my $TITLE = "Drucksystem PZM und KSM Luzern";
my $CONTACT = "team-technik\@ict.luzern.phz.ch";
my $DEBUG = 1;  # set to zero once confident!
my $PRACCSUM = "/var/print/bin/pracc-sum";
my $PRACCVIEW = "/var/print/bin/pracc-view";
my $ADMINGROUP = "pracc-admin";

my $COLOR1A = '#ddccbb';  # dirty rose
my $COLOR1B = '#eeddcc';  # baby rose
my $COLOR2A = '#bbddbb';  # dirty green
my $COLOR2B = '#aaeeaa';  # gentle green
my @COLORS = ($COLOR1A,$COLOR1B,$COLOR2A,$COLOR2B);

$ENV{'REMOTE_USER'}="ujr"; # DEBUG
unless (exists $ENV{"REMOTE_USER"}) {
  &header($TITLE, "Not Authenticated");
  print <<EOT;
<p>Sorry, you are not authenticated. This is not supposed
 to happen, so please notify the system administrators at
 <a href="mailto:$CONTACT">$CONTACT</a>,
 thank you!</p>
<p>Tech info: env <tt>REMOTE_USER</tt> is missing.</p>
EOT
  &finish();
}

my $SCRIPT = $ENV{"SCRIPT_NAME"};
my $QUERYSTRING = $ENV{"QUERY_STRING"};
my %QUERY = ();  # ditto, parsed into a hash
my $METHOD = $ENV{"REQUEST_METHOD"} || "none";
if ($METHOD eq "POST") { $QUERYSTRING = <STDIN>; }
foreach my $pair (split /&/, $QUERYSTRING || "") {
  my ($name, $value) = split /=/, $pair;
  $QUERY{$name} = $value;
}

my $user = $ENV{"REMOTE_USER"};
my $master = &checkAdmin($user);
$user = $QUERY{"user"} if ($master && $QUERY{"user"});

my $realname = &getRealName($user);
my $hint = "Did you mistype a user's name?" if ($master);
&error("Cannot getpwnam for $user", $hint) unless ($realname);

my $ret = &getAcctInfo($user);
&error("Error running $PRACCSUM: $!") unless (defined $ret);
unless (($ret == 0) || ($ret == 1)) {
  &header($TITLE, "Kein Printer Accounting");
  print "<p>F&uuml;r Benutzer $user ($realname)\n";
  print " ist kein Printer Accounting eingerichtet.</p>\n";
  print "<p>Tech info: pracc-sum said &quot;$praccsum&quot;</p>\n";
  &finish();
}

# XXX The "export" here must match what's shown on the button!
if (exists $QUERY{'submit'} && $QUERY{'submit'} eq "export") {
  &exportCSV();
  exit 0;
}

&header("$TITLE", "Ihr Druck-Konto");

my $overdrawn = (!($limit eq "none") && ($balance <= $limit));
print "<p>Benutzername: <b>$user</b>";
print " ($realname)" if ($realname);
print "<br>\n Kontostand: ";
if ($overdrawn) {
  my $color = "color=\"#cc0000\"";
  printf("<b><font $color>Fr. %.2f</font></b>", $balance/100);
}
else {
  printf("<b>Fr. %.2f</b>", $balance/100);
}
print "<br>\n Kontolimite: ";
if ($limit eq "none") {
  print " unlimitiert</p>\n";
}
else {
  printf(" Fr %.2f</p>\n", $limit/100);
}
if ($overdrawn) {
  print "<p><b>Sie k&ouml;nnen nicht mehr drucken,</b>\n";
  print " weil Ihr Kontostand unter Ihrer Kontolimite liegt.<br>\n";
  print " Neue Druck-Credits: Kanzlei (PHZ) bzw Sekretariat (KSM).</p>\n";
}

my $types = ""; # all types
$types = $QUERY{"types"} if exists $QUERY{"types"};

my $period = "month"; # default is this month
$period = $QUERY{"period"} if exists $QUERY{"period"};

my $first = "";
$first = $QUERY{"first"} if exists $QUERY{"first"};

my $until = "";
$until = $QUERY{"until"} if exists $QUERY{"until"};

print <<EOT;
<script type="text/javascript">
function update(elem) {
  var period = elem.options[elem.selectedIndex].value
  var firstField = document.getElementsByName("first")[0]
  var untilField = document.getElementsByName("until")[0]
  firstField.value = period ? isodate(backDate(period)) : ""
  untilField.value = period ? isodate(new Date()) : ""
}
function initFields() {
  var selectList = document.getElementsByName("period")[0]
  for (var i = 0; i < selectList.length; i++)
    if (selectList.options[i].value == "$period")
      selectList.selectedIndex = i
  var radios = document.forms['settings'].elements['types'];
  for (var i = 0; i < radios.length; i++) {
    radios[i].checked = (radios[i].value == "$types")
  }
}
function onLoadFunc() {
  initFields()
  update(document.getElementsByName('period')[0])
}
</script>
EOT

print <<EOT;
<form name="settings" method="GET" action="$SCRIPT">
<p><table class=graybox border=0 cellpadding=8>
EOT

if ($master) {
  print <<EOT;
<tr><td align="right"><b>Konto</b></td>
  <td><input type="text" name="user" value="$user" size="20"></td>
  <td colspan="2">Kontoname = username</td></tr>
EOT
}

print <<EOT;
<tr><td align="right"><b>Auszug</b></td>
  <td><select name="period" size="1" onchange="update(this.form.period)">
    <option value="">Alles</option>
    <option value="today">Heute</option>
    <option value="week">Diese Woche</option>
    <option value="month">Dieser Monat</option>
    <option value="year">Dieses Jahr</option>
    <option value="7">Letzte 7 Tage</option>
    <option value="30">Letzte 30 Tage</option>
    </select></td>
  <td width="15%" align="right">mit</td>
  <td><input type="radio" name="types" value="d">
    Ausdrucken</td></tr>
<tr><td align="right">von</td>
  <td><input type="text" name="first" value="$first" size=15></td>
  <td></td>
  <td><input type="radio" name="types" value="c">
    Gutschriften</td></tr>
<tr><td align="right">bis</td>
  <td><input type="text" name="until" value="$until" size=15></td>
  <td></td>
  <td><input type="radio" name="types" value="">
    allen Kontobewegungen</td></tr>
<tr><td align="right"><input type="submit" name="submit" value="zeigen"></td>
    <td colspan="3">Kontoauszug mit obigen Einstellungen anzeigen</td></tr>
<tr><td align="right"><input type="submit" name="submit" value="export"></td>
    <td colspan="3">Download im CSV-Format (Excel und OpenOffice)</td></tr>
EOT

unless (($first =~ m/^[0-9]+-[0-9][0-9]-[0-9][0-9]$/ or !$first) &&
        ($until =~ m/^[0-9]+-[0-9][0-9]-[0-9][0-9]$/ or !$until)) {
  print <<EOT;
<tr><td colspan="4" bgcolor="#cc0000">Daten im Format
  JJJJ-MM-DD eingeben, z.B. 2005-12-24</td></tr>
EOT
}

print <<EOT;
</table></p>
</form>
EOT

# Don't show the table on first invocation.
# XXX Fragile: user could enter manually ...?user=name
unless (exists $QUERY{'user'}) {
  &finish();
  exit(0);
}

# Table with totals; filled in later by JavaScript
print <<EOT;
<p><table border="0" cellpadding="4"><tbody>
<tr bgcolor="#aaaaaa"><td><b>Umsatz</b></td>
 <td colspan="2">in der angezeigten Periode</td></tr>
<tr bgcolor="$COLOR1B"><td>Ausdrucke:</td>
 <td><b><span id="debitSum">?</span></b></td>
 <td>in <span id="debitCount">?</span> Transaktion/en</td></tr>
<tr bgcolor="$COLOR2B"><td>Gutschriften:</td>
 <td><b><span id="creditSum">?</span></b></td>
 <td>in <span id="creditCount">?</span> Transaktion/en</td></tr>
</tbody></table></p>
EOT

my $viewopts = "";
$viewopts .= " -f $first" if ($first);
$viewopts .= " -u $until" if ($until);
$viewopts .= " -t debit" if ($types =~ m/d/);
$viewopts .= " -t credit" if ($types =~ m/c/);

open PRACCVIEW, "$PRACCVIEW $viewopts $user 2>&1 |" 
 or &error("Error running $PRACCVIEW");
print <<EOT;
<p><table border="0" cellpadding="4"><tbody>
<tr><th bgcolor="#aaaaaa">Datum</th>
 <th bgcolor="#aaaaaa">Zeit</th>
 <th bgcolor="#aaaaaa">User</th>
 <th bgcolor="#aaaaaa">Aktion</th>
 <th bgcolor="#aaaaaa">Betrag</th>
 <th bgcolor="#aaaaaa">Infos</th></tr>
EOT

my $color = '#aaaaaa'; # reset depending on row type
my ($debitSum, $debitCount, $creditSum, $creditCount) = (0,0,0,0);
while (my $line = <PRACCVIEW>) { chomp($line);
  next if ($line =~ m/^#/); # comment line
  my ($date, $time, $user, $action, $value, $comment) = split /\s+/, $line, 6;
  if ($action eq "debit") {
    $action = "Ausdruck";
    $debitSum += $value;
    $debitCount += 1;
    $value = -$value;
    $color = &getcolor(0);
  }
  elsif ($action eq "credit") {
    $action = "<b>Gutschrift</b>";
    $creditSum += $value;
    $creditCount += 1;
    $color = &getcolor(1);
  }
  elsif ($action eq "reset") {
    $action = "<b>Saldo</b>";
    $color = '#aaaaaa';
    # resets do not qualify as a transaction!
  }
  else { next; } # other line types not shown
  print "<tr bgcolor=\"$color\"><td>$date</td>\n";
  print " <td>$time</td>\n";
  print " <td>$user</td>\n";
  print " <td>$action</td>\n";
  printf " <td align=\"right\">%.2f</td>\n", $value/100;
  print " <td>$comment</td></tr>\n";
}
close PRACCVIEW;

print "</tbody></table></p>\n";

# Now that we know the totals, add them to the top of the page:
$debitSum = sprintf "Fr %.2f", $debitSum/100;  # convert...
$creditSum = sprintf "Fr %.2f", $creditSum/100; # ...to string
print <<EOT;
<script type="text/javascript">
var debitSumElem = document.getElementById("debitSum");
debitSumElem.innerHTML = "$debitSum"
var debitCountElem = document.getElementById("debitCount");
debitCountElem.innerHTML = "$debitCount"
var creditSumElem = document.getElementById("creditSum");
creditSumElem.innerHTML = "$creditSum"
var creditCountElem = document.getElementById("creditCount");
creditCountElem.innerHTML = "$creditCount"
</script>
EOT

print <<EOT;
<p><b>Tech info</b> for support personnel:<br>
$praccsum<br>
$PRACCVIEW $viewopts $user</p>
EOT

&finish();

exit 0;


  
#my $praccfile = "$PRACCDIR/$user";
#unless (open PRACCFILE, $praccfile) {
#  &header("$TITLE", "Kein Printer Accounting");
#  print "<p>F&uuml;r Benutzer $user ($realname)\n";
#  print " ist kein Printer Accounting eingerichtet.</p>\n";
#  &finish();
#}
#
# pass 1: summarize account
#
#my $balance = 0;
#my $limit = $UNLIMITED;
#
#while ($line = <PRACCFILE>) { chomp($line);
#  if ($line =~ m/^\-([0-9]+)/) { $balance -= $1; }
#  if ($line =~ m/^\+([0-9]+)/) { $balance += $1; }
#  if ($line =~ m/^\=(-?[0-9]+)/) { $balance = $1; }
#  if ($line =~ m/^\$(-?[0-9]+)/) { $limit = $1; }
#  if ($line =~ m/^\$\*/) { $limit = $UNLIMITED; }
#}
#
# rewind pracc file for 2nd pass
#
#unless(seek PRACCFILE, 0, 0) {
#  print "<p>Sorry, a system error occurred:\n";
#  print " seek on $praccfile failed: $!</p>\n";
#  &finish();
#}
#
# pass 2: show details
#
#my ($odd, $even);
#print "<p><table border=\"0\" cellpadding=\"4\">\n";
#print "<tr><th bgcolor=\"#aaaaaa\">Datum</th>\n";
#print "    <th bgcolor=\"#aaaaaa\">Zeit</th>\n";
#print "    <th bgcolor=\"#aaaaaa\">Aktion</th>\n";
#print "    <th bgcolor=\"#aaaaaa\">Betrag</th>\n";
#print "    <th bgcolor=\"#aaaaaa\">Infos</th></tr>\n";
#while ($line = <PRACCFILE>) { chomp($line);
#  # reset line
#  if ($line =~ m/^\=(-?[0-9]+)\s+@([^T]+)T([^Z]+)Z\s+(.*)/ ||
#      $line =~ m/^\=(-?[0-9]+)\s+date\s+([^\s]+)[\stime]+([^\s]+)\s+(.*)/) {
#  	($odd, $even) = getcolor();
#  	print "<tr><td bgcolor=\"$odd\">$2</td>\n";   # date
#  	print "    <td bgcolor=\"$even\">$3</td>\n";  # time
#  	print "    <td bgcolor=\"$odd\"><b>Saldo</b></td>\n";
#  	print "    <td bgcolor=\"$even\" align=\"right\">$1</td>\n";
#  	my $note = ($4 eq 'saldo') ? '&nbsp;' : $4;
#  	print "    <td bgcolor=\"$odd\">$note</td></tr>\n";
#  }
#  # credit line
#  if ($line =~ m/^\+([0-9]+)\s+@([^T]+)T([^Z]+)Z\s+(.*)/ ||
#      $line =~ m/^\+([0-9]+)\s+date\s+([^\s]+)[\stime]+([^\s]+)\s+(.*)/) {
#  	($odd, $even) = getcolor();
#  	print "<tr><td bgcolor=\"$odd\">$2</td>\n";   # date
#  	print "    <td bgcolor=\"$even\">$3</td>\n";  # time
#  	print "    <td bgcolor=\"$odd\"><b>Credit</b></td>\n";
#  	print "    <td bgcolor=\"$even\" align=\"right\">+$1</td>\n";
#  	print "    <td bgcolor=\"$odd\">$4</td></tr>\n";
#  }
#  # debit line
#  if ($line =~ m/^\-([0-9]+)\s+@([^T]+)T([^Z]+)Z\s+(.*)/ ||
#      $line =~ m/^\-([0-9]+)\s+date\s+([^\s]+)[\stime]+([^\s]+)\s+(.*)/) {
#  	($odd, $even) = getcolor();
#  	print "<tr><td bgcolor=\"$odd\">$2</td>\n";   # date
#  	print "    <td bgcolor=\"$even\">$3</td>\n";  # time
#  	print "    <td bgcolor=\"$odd\">Ausdruck</td>\n";
#  	print "    <td bgcolor=\"$even\" align=\"right\">-$1</td>\n";
#  	print "    <td bgcolor=\"$odd\">$4</td></tr>\n";
#  }
#}
#print "</table></p>\n";
#close PRACCFILE;
#
#&finish();


##----------------------------------------------------------------
# getcolor - Return a color for the given color scheme (0 or 1),
#            strictly alternating between the two colors in a
#            color scheme. Useful for coloring rows in a table...
#
my $colrow = 0;
sub getcolor {
  my $scheme = shift || 0;
  my $index = 2*$scheme + (++$colrow % 2);
  return $COLORS[$index];
}

#-----------------------------------------------------------------------
# Write the account in CSV format to stdout.
# The separator is taken from sep= query string entry.
#
sub exportCSV {
  my $viewopts = "";
  $viewopts .= " -f $QUERY{'first'}" if ($QUERY{'first'});
  $viewopts .= " -u $QUERY{'until'}" if ($QUERY{'until'});
  $viewopts .= " -t debit" if ($QUERY{'types'} =~ m/d/);
  $viewopts .= " -t credit" if ($QUERY{'types'} =~ m/c/);

  open PRACCVIEW, "$PRACCVIEW $viewopts $user 2>&1 |"
   or &error("Error running $PRACCVIEW");

  # Get separator character from query string.
  # Semicolon is default because Excel requires it!
  my $sep = (exists $QUERY{'sep'}) ? urldecode($QUERY{'sep'}) : ';';

  print "Content-type: text/csv\r\n";  # HTTP header (see also RFC 2183)
  print "Content-disposition: attachment; filename=PrintAccount.csv\r\n\r\n";
  print join $sep, "Date", "Time", "Type", "Amount", "Comment\n";

  while (my $line = <PRACCVIEW>) { chomp($line);
    my ($date, $time, $type, $value, $comment) = split /\s+/, $line, 5;
    if ($type eq 'debit' || $type eq 'credit' || $type eq 'reset') {
      print join $sep, $date, $time, $type, $value, "\"$comment\"\n";
    }
  }

  close PRACCVIEW;
}

##===============================================================
# Check if the given user is a member in group $ADMINGROUP.
# This replaces the former
#   my $master = &inarray($user, @MASTERS);
# where @MASTERS was a hard-coded list of "master" users.
#
sub checkAdmin {
  my $user = shift;
  my @group = getgrnam($ADMINGROUP);
  return 0 unless @group;

  my ($name, $passwd, $gid, $members) = @group; # split
  my @members = split(/ /, $members);
  return &inarray($user, @members);
}

##===============================================================
# Return ISO-format date of today minus the given number of days
#
#sub todayminus {
#  my $days = shift;
#  my $secs = $days*86400;
#  my @tm = localtime(time()-$secs);
#  my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = @tm;
#  return sprintf "%d-%02d-%02d", 1900+$year, 1+$mon, $mday;
#}

#==================================================================
# Invoke pracc-sum for the account named in $1;
# if successful, set the global variables $balance,
# $limit, and $acctsum; return the status of pracc-sum
# or undef if pracc-sum cannot be properly executed.
#
sub getAcctInfo {
  my $acctname = shift;

  my $ret = 0;
  my $pid = open(PRACCSUM, "$PRACCSUM $acctname 2>&1 |");
  return undef unless ($pid);
  my $info = <PRACCSUM>; $!=0;
  close PRACCSUM or $ret = ($!) ? 65536 : $? & 65535;
#  print "DEBUG getAcctInfo: \$!=$!, \$?=$?\n";
  return undef if (($ret > 65535) || ($ret & 255));
  $ret = ($ret >> 8) & 255;  # pracc-sum return value
  return undef unless ($info);

  chomp($info);
#  print "DEBUG getAcctInfo: $info\n";
  $praccsum = $info;  # global!
  my @info = split /\s+/, $info;
  if (($info[0] eq "acct")
   && ($info[2] eq "balance")
   && ($info[4] eq "limit")) {
  	$balance = $info[3];  # global!
  	$limit = $info[5];  # global!
  }
  return $ret;
}

#==================================================================
# Given a user's login name, find its real name (gecos field).
# Return undef on error (no such user, etc)
#
sub getRealName {
  my @infos = getpwnam shift;
  return undef unless (@infos);
  my ($name,$pwd,$uid,$gid,$quota,$comment,$gcos,$dir,$shell) = @infos;
  my $realname = $gcos; $realname =~ s/,.*$//;
  return $realname;
}

#==================================================================
# My standard HTTP/HTML helper functions:
#  * use header(title, h1) at the beginning
#  * use finish() when done generating the page
#  * use error(msg, hint) to report an error
# All functions provide reasonable defaults for missing args.

sub header {
  my $title = shift || $TITLE || "";
  my $heading = shift;

  return if ($main::header_written);
  
  print "Content-type: text/html\r\n\r\n";  # HTTP header
  print "<html><head><title>$title</title>\n";
  print "<style type=\"text/css\"><!--\n";
  print "body {background-color:white;font-family:sans-serif;margin:2pc}\n";
  print ".graybox {background-color:#ccc;padding:8px;border:1px solid #444}\n";
  print "--></style>\n";
  &emitHeaderJS();
  print "</head><body onload=\"onLoadFunc();\"><!--auto-generated html-->\n";
  print "<h1>$heading</h1>\n" if defined $heading;

  $main::header_written = 1;
}

sub finish {
  my @tm = localtime;
  my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = @tm;

  print "<div><hr>\n<i>";
  printf "Generated on %d-%02d-%02d at %02d:%02d:%02d\n",
  	1900+$year, 1+$mon, $mday, $hour, $min, $sec;
  print "by $PROGNAME, version $VERSION,\n";
  print "running as $>:$)\n";
  print "<br>Contact: <a href=\"mailto:$CONTACT\">$CONTACT</a>\n";
  print "</i></div>\n</body>\n</html>\n";

  exit 0;
}

sub error {
  my $msg = shift || "I'm as confused as you are!";
  my $hint = shift;
  &header($TITLE, $TITLE);
  print "<p><b>Error:</b> $msg</p>\n";
  if ($hint) { print "<p>$hint</p>\n"; }
  else {
  	print "<p>Please notify the system administrators about this\n";
  	print " problem and where and when it occurred; use the eMail\n";
  	print " address shown below. Thank you!</p>\n";
  }
  &finish();
}

sub emitHeaderJS {
  print <<EOJS;
<script type="text/javascript">
/* BackDate by Urs Jakob Ruetschi */
function backDate(p) {
  var then = new Date()
  switch (p) {
    case "today": /* nothing */ break
    case "year": then.setMonth(0) // FALLTHRU
    case "month": then.setDate(1); break
    case "week": p = then.getDay()
      if (p == 0) p = 7 // make Sun the 7th day
      // FALLTHRU
    default: p *= 1 // convert string to number
      if ((typeof p) != "number") p = 100 // last 100 days
      if (p > 0) p -= 1; else p = 0 // don't go negative
      then.setTime(then.getTime() - p*24*60*60*1000)
  }
  then.setHours(0,0,0,0) // hours, mins, secs, millies
  return then
}
function isodate(d) {
  var month = d.getMonth() + 1
  var date = d.getDate()
  return d.getFullYear()
    + "-" + (month < 10 ? "0" : "") + month
    + "-" + (date < 10 ? "0" : "") + date
}
</script>
EOJS
}

#==================================================================
# Return true if shift is in @_, false otherwise.
#
sub inarray { my $val = shift;
  foreach (@_) { return 1 if ($val eq $_); }
  return 0;
}

sub urlencode { my $str = shift;
  $str =~ s/([^A-Za-z0-9])/sprintf("%%%02X", ord($1))/seg;
  return $str;
}

sub urldecode { my $str = shift;
  $str =~ tr/+/ /;  # Some browsers encode blanks as plus signs
  $str =~ s/\%([a-fA-F0-9][a-fA-F0-9])/pack("C", hex($1))/seg;
  return $str;
}
