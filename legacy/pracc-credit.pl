#!/usr/bin/perl -w
#
# Text user interface for updating a user's printing credits.
# Interactively prompts for a login name and an amount to be
# credited. Relies on the external tools pracc-{sum,edit}.
#
# This tool is intended to run in a terminal session, such
# as a PuTTY connection from the Kanzlei to the print server.
#
# ujr/2003-09-18 version 1
# ujr/2005-08-03 rewrote for pracc v2

my $PROGNAME = $0; $PROGNAME =~ s:^.*/::;  # basename
my $VERSION = "2005-08-03";
my $CONTACT = "ujr\@ict.pzm-luzern.ch";
my $PRACCEDIT = "/var/print/bin/pracc-edit";
my $PRACCSUM = "/var/print/bin/pracc-sum";
#my $PRACCCLEAN = "/var/print/bin/pracc-clean";  # TODO

my $login;
my @infos;
my $credits;
my $bonus;

sub fail {
  my $msg = shift;
  print "$msg\nPlease notify $CONTACT about this problem.\n\n";
  &prompt("Enter druecken, um dieses Programm zu beenden");
  exit 127;
}

print "This is $PROGNAME, version $VERSION\n\n";
-x "$PRACCSUM" or fail "Fatal: no executable $PRACCSUM";
-x "$PRACCEDIT" or fail "Fatal: no executable $PRACCEDIT";

print "Gutschrift von Druck-Credits\n";
print "----------------------------\n\n";

while (1) {
  my @pwent;
  while (1) {
    $login = promptv('Benutzername', '^[a-z0-9]+$');
    @pwent = getpwnam($login) or print "Benutzer $login unbekannt!\n";
    last if @pwent;
  }
  my ($name,$passwd,$uid,$gid,$quota,$comment,$gcos,$dir,$shell) = @pwent;
  last if yesno("$gcos, ok?"); print "\n";
}
print "\n";

@infos = &acctinfo($login);
unless (@infos) {
  print "Fuer Benutzer $login ist kein Printer Accounting eingerichtet.\n";
  print "Falls Sie denken, dass das anders sein sollte, bitte eMail an:\n";
  print "$CONTACT\n\n";
  &prompt("Enter druecken, um dieses Programm zu beenden");
  exit 127;
}

$credits = $infos[0]/100;
print "Druck-Credits aktuell: CHF $credits\n";

while (1) {
  my $msg = "";  print "\n";
  $bonus = &promptv('Neue Credits (in CHF)', '^[0-9]+\.?[0-9]*$');
  $bonus += 0;  # force numeric type
  next unless ($bonus <= 20) || &noyes("CHF $bonus, das ist sehr viel! Ok?");
  $msg = ", das ist weder 5 noch 10" if ($bonus != 5) && ($bonus != 10);
  last if &yesno("CHF $bonus$msg, ok?");
}
$bonus *= 100;  # convert to Rappen
print "\n";

# please, don't disturb now!
#$SIG{INT} = sub { print "got sigint, but we carry on!\n" };
#$SIG{TERM} = sub { print "got sigterm, but we carry on!\n" };
#$SIG{QUIT} = sub { print "got sigquit, but we carry on!\n" };
#unlink $pracctemp;  # don't care about exit status
#system($PRACCCLEAN ...) == 0 or fail "cannot clean pracc file (\$?=$?).";
#rename($pracctemp, $praccfile) or fail "cannot rename temp file (\$?=$?).";

my $msg = "bought new credits";
my $out = `$PRACCEDIT $login credit $bonus $msg 2>&1`;
my $ret = $? >> 8;
&fail("$PRACCEDIT $login: $!") if ($? & 255);
&fail("$PRACCEDIT $login: $out") if ($ret > 0);

my @new = &acctinfo($login);
$credits = $new[0]/100;
print "Ok, Benutzer $login hat jetzt CHF $credits Credit";
print "\nund kann somit immer noch nicht drucken.."
  if (!($new[1] eq "none") && ($new[0] < $new[1]));
print ".\n\n";

&prompt("Enter druecken, um dieses Programm zu beenden");
exit 0;


##----------------------------------------------------------------------
# acctinfo - invoke pracc-sum for the account named in $1;
#            return account's balance and limit as an array;
#            return empty array if no account named $1;
#            die on error.
sub acctinfo {
  my $acctname = shift;
  my $info = `$PRACCSUM $acctname 2>&1`;
  my $retval = $? >> 8;  # XXX
  &fail("$PRACCSUM $acctname: $!") if ($? & 255);
  &fail("$PRACCSUM $acctname: $info") if ($retval > 2);
  return () if $retval == 2;  # no pracc file
  my @info = split /\s+/, $info;
  return ($info[3], $info[5]);
}

##----------------------------------------------------------------------
# Prompt user for input: prompt to STDOUT, read response from STDIN.
# An optional default answer may be specified in the last argument.
# The promptv variant validates against a regex in the 2nd argument.
#
# Examples:   $gaga = prompt('Name', 'John');
#             $gaga = promptv('Login', '^[a-z0-9]+$');
#             last if yesno('Ok?');
sub prompt {
  my ($ps1,$deflt,@gaga) = @_;
  my $suggestion = ($deflt) ? " [$deflt]" : '';
  my $line;  $deflt = '' unless defined $deflt;

  print "$ps1$suggestion: ";  $line = <STDIN>;
  if (defined $line) {chomp($line); return $line;}
  return $deflt;
}
sub promptv {
  my ($ps1,$regex,$deflt,@gaga) = @_;
  my $line;

  while (1) {
    $line = prompt($ps1, $deflt);
    $line =~ s/^\s*//;  # remove leading white space
    $line =~ s/\s*$//;  # remove trailing white space
    last if $line =~ qr/$regex/ || ($line eq '' && defined $deflt);
  }
  return ($line) ? $line : $deflt;
}
sub yesno {  # true if yes; default is yes
  my $question = shift;
  my $answer = promptv($question, '^[JjYyNn](o|es|a|ein)?$', 'Ja');
  return $answer =~ m/^[jy]/i;
}
sub noyes {  # true if yes, but default is no
  my $question = shift;
  my $answer = promptv($question, '^[JjYyNn](o|es|a|ein)?$', 'Nein');
  return $answer =~ m/^[jy]/i;
}
