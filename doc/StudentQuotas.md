(This is a minimally edited local copy of
[printer_quota.txt][printer_quota]  
by Anthony Thyssen of Griffith University in Brisbane, Australia.)


Implementing Student Printing Quotas  
(on networked PostScript printers)
====================================

**Prolog:**
With the advent of the WWW we had problems with students printing
too much. The solution however was incrementally created in my spare
time over 4 years and was successfully implemented last year (1997).
The student printer usage dropped to a few percent of the usage of
the year before.

Staff are not limited as they are expected to be circumspect
in their printing. Student however will print everything and
anything if it does not cost them money!

For reasons for this method see [page_counting.txt][page_counting].


1/ Limit Printer Access
-----------------------

The first step is to limit the network printers the students have
access to so that printing comes from some specific print server only.

EG: all printing to the printer can only be done via a single print
queue, which can be fed from all student machines.

For this we use a machine called "citadel" which has a hostname
alias "lprhost" to specify the print server host. That way if
anything goes wrong we can switch hosts FAST. Citadel is a very
old slow machine but is plenty fast enough with enough spooling
space to handle the task.

The printer spooling and service is via the old BSD Printer Daemon
as it have the easiest extendability and reporting facilities. It
is also easiest to arrange clients to pass print jobs to. The SysV
printer system is a big mess in my opinion and does not provide a
simple printer status reporting that the BSD system provides. (See
"lpq" command). The GNU bsd-lpr package should work as a replacement
on SysV machines.

To prevent direct contact to the HP network printers the printers
were configured via "bootp" protocal which allows them to download
a configuration file via tftp. This file only allows the printer
to recieve jobs from the primary and backup printer server IP's
which students don't have access to. Apple talk and NT connections
are also disallowed. The printer itself is in a specially created
locked box which prevents the printer or paper from being stolen,
the students accessing the control panel but still able to pickup
output.


2/ Printer Communication
------------------------

I wanted to see the errors returned by the PostScript engine of the
printer and to send jobs to it via its telnet connection (port 9100).
So using the HP printer driver code as an example I wrote a Perl script
to talk to the printer in both directions and MAIL ERRORS BACK to the
sender. This was called "pstalk" and could handle both networked and
tty based printers.

Later page counting was added to ask the printer the current page count.

It seems that the only practical way of counting pages with PostScript
is to ask the printer its page count before and after the print job.
See [page_counting][page_counting]. Something like...

    %!
    (%%[ pagecount: )print
    statusdict/pagecount get exec(                )cvs print
    ( ]%%)=
    flush

with other bits added to ensure it is the current page count and not
program output or a page count buffered from before the print job.

The result is logged into the lpd accounting system files and output
to the lpd handling file, along with any PostScript program output
to be mailed back to the user.

Future extensions, which would require a full program re-write to
implement, would be to report out-of-paper or offline problems to
the printer status file (seen in the lpq status reports) and to the
computer operators for faster resolution of such problems.


3/ Printer Server Filter
------------------------

A shell script BSD-lpd 'input' filter (psfilter) links the printer
comunications program above to the BSD printer system. This program
also handles the selection of text or other file format filter
programs which is passed to the above communications program,
which does the actual filtering as well as the mailing of any
results and output returned from the printer back to the user
requesting the printer "job".

This was then later extended to lookup a quota file to see if
specific students have gone over quota (and need to purchase more)
before printing jobs.

After the job the script then increments the quota file with number
of pages printed and reports back to student the pages printed and
quota left. Also if the student goes over quota in a big way a
report is mailed to the printmaster (me).


4/ Quota database
-----------------

A second Perl program (pquota) handles this quota file (unix DB file)
and allows the computer operators to increse student quotas, etc. very
simplly and easily. Students are also added via this program and batch
scripts.

Students are given 450 pages of quota for the year initally, after
that either they require exceptions for special project work from
academic staff or can purchase more quota at 10 cents a sheet (in
units of 5 dollars).

Note this does not prevent student going over quota but they need to
pay for that overquota before their current page quota becomes positive
again. This is planned to carry into the next year too, which will I
think be a shock for the ten students which thought they were smart
and over printed before leaving for the year ;-)


Problems
--------

This system is not perfect and does have problems, but it does achieve
the goal of limiting paper/toner usage by the students. The problems
as known at this time are...

Quota System Problems

  * Students can only print to one printer at a time (DB update)

  * The quota system only stops users if they are over quota.
    It does not stop them going over quota. We however continue
    'overquotas' into the next year, to prevent cheaters.

Printer Comunications (fixable)

  * The current printer comunication script uses a kludge in the
    form of a child process to feed the printer and filter incoming
    "jobs" to leave the parent to wait for the "EOF" marker. This
    should really be done with a select() multi-tasking IO system.

  * Output from the printer during the printing of the job is not
    checked for "out of paper" or "offline" status messages. This
    would be a great improvment to the printing system.

HP printers (annoying)

  * HP 4 and above report they are idle BEFORE they have finished
    processing a job and ejected the last page! This means that on
    many jobs the page count will be one page smaller! HP probably
    did this to allow 'pipelining' of print jobs through the printer.
    But to printer accounting it is just a problem of extreme annoyance.

    A solution to this is to just pause a few seconds before requesting
    the final page count. But pause for how long? Also any such pause
    slows down the printing of multiple jobs.

  * HP 4 and 5 printers when in power save mode when receiving an
    initial job seem to have the page count GO DOWN between the start
    of the job and the request for final page count, probably before
    the printer has even output the page! This results in a NEGITIVE
    page count for the first job of the day!

    The result of this is I usaly have to ignore the first job of the day!

    LPRng documentation reveiled this was due to the way the HP printer
    saves the software current page count into eprom (power on count).

  * Early versions of the HP 4000 N printer NEVER REPORTED IT IS IDLE!
    This means the above quota system has no way to tell when the printer
    is finished. It seems that HP changed the PostScript software chip.
    Such printers can NOT be used for implimenting print quotas, with
    this method. The new HP 4000 printers do not have this problem.


**Postscript:**
As you can see the printing system is complex with a lot of things
to consider, was created over a long period of time, works. Over
the last year it has saved us a huge bill for maintenance materials,
but the same effect could probably have been achieved with a simpler
and slower `ghostview` page count scheme.


Anthony Thyssen, <A.Thyssen@griffith.edu.au>, 8 January 1998

---

Summery, relation to LPRng HP drivers and our setup....

> I'm having a little bit problem on my LPRng installation to get
> page count on my HP printer using HP jetdirect card.. can u help
> me on this.. please... thanks in advance..

We are using the LPRng for handling print jobs. Specifically so we
don't have to distribute the printcap file or setup mail spools.

All machines except print master has the following lpd.conf...

    default_remote_host={our printing machine}
    default_printer={this machine default printer}
    return_short_status=*
    force_localhost@

However we do NOT use the HP printer filters that LPRng seems to
suggest you use. Before we installed LPRng we had our own version
of those HP printer drivers, which were developed in-house (and
are currently being re-written - see above).

These are almost exactly like the HP filters except internally they
call a separate script to test the print job to see what PostScript
conversion filter is required, and also attempt the printer connection
multiple times until success.

The main filter is wrapped by a separate shell script which performed
some minor modifications to the options before feeding them to our
HP filter (called "pstalk"). In particular it fixes the username from
Novel systems back to a UNIX user name (removing the 'cn=' and other
junk the Novel system uses).

There are also two 'accounting scripts' just as there can be for
the LPRng HP filters, which are called with the current printer
page counter and the total pages printed.

The scripts check if the user is allowed to print to that printer,
either due to quota limits (which if they are negitive are NOT allowed
until they purchase more printer quota), or because of a `-T` per printer
accounting option which prevents students printing to staff printers.

The printers themselves are protected with a boot file which tells
the student and staff printers they should only talk to either our
print manager machine or to our system programmers machine (for
testing purposes).

Anthony Thyssen, <anthony@cit.gu.edu.au>, 26 September 2000

[printer_quota]: http://www.ict.griffith.edu.au/anthony/info/postscript/printer_quota.txt
[page_counting]: http://www.ict.griffith.edu.au/~anthony/info/postscript/page_counting.txt
