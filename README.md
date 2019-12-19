
Pracc (Printing with Accounting)
================================

Pracc complements CUPS to do printing with accounting.
The pracc package consists of a backend for CUPS and
a few tools to manage user's printer accounts.

![Pracc Logo](/doc/images/pracclogos.png)

Be sure to read all documentation that comes with pracc.
Pracc is not plug-and-play but easy once you understand
how it works.

Compilation and installation instructions:

  1.  Download the package.
  2.  tar xzf pracc-version.tgz
  3.  cd pracc/src
  4.  Adjust the Makefile!
  5.  make
  6.  sudo make install

Once pracc is installed, you can use the pracc-check
tool to check if the installation is (still) fine.


History
-------

Pracc was created in 2003 at PZM Luzern (a former teacher's college)
in response to the headmaster's urge to reduce paper waste
around the public printing stations.
The system evolved continually and was in heavy use for years
at PZM and later at the PHZ Luzern (which is now [PH Luzern][phlu]),
and reportedly also at [HSLU][hslu] (the Lucerne University
of Applied Sciences and Arts).

Technically, the system started as a *print command* for
[Samba][samba] that read the printer's page counter before
and after printing. When [CUPS][cups] became more widespread,
the system was rewritten as a *backend* for CUPS. The account
information was always kept in append-only plain text files,
which proved simple (no database maintenance) and reliable.
Students could recharge their printing accounts at the student
office, where the secretary had a simple console interface
that later changed to a full-fledged Web-based administration
interface. The latest changes, experimenting with counting
pages in print jobs (see joblex/pracc-scan) in addition to
querying the printer device remained unfinished.

Development of Pracc ended in 2008 when I left PHZ Luzern.
It remained in use at least until 2013 (when I last provided
some support).

[samba]: https://www.samba.org/
[cups]: https://www.cups.org/
[phlu]: https://www.phlu.ch/
[hslu]: https://www.hslu.ch/


Documentation
-------------

Pracc was mostly used in-house and there is no comprehensive
documentation. Some technical information can still be found
in these documents:

  *  [pracc-about.html](doc/pracc-about.html)
  *  [pracc-files.html](doc/pracc-files.html)
  *  [pracc-tools.html](doc/pracc-tools.html)
  *  [cupspracc.html](doc/cupspracc.html)
  *  [counting.html](doc/counting.html)

Some background can be found in [notes](doc/pracc-pzm.html) from the time
Pracc was first created and in a [leaflet](doc/PrintingPLU-20080318.pdf),
as well as in the slides from a [SWITCH](https://www.switch.ch)-organised
[printing workshop](doc/PrintingWorkshopPLU-20080318.pdf) and a longer
[presentation](doc/PrintingLecture20080325.pdf) (all in German). Also
available are some technical notes on [PostScript](doc/PostScript.md)
and [network printing](doc/Network.md).


References
----------

Essential sources of information for the development of Pracc's
CUPS backend (and its predecessor called *netprint*) were:

 * About communicating with a PostScript printer: there's a chapter
   devoted to that in *Advanced Programming in the UNIX Environment*
   by W. Richard Stevens (Addison-Wesley, 1993)
   ([www.apuebook.com](http://www.apuebook.com/))
 * The [LPRng](http://www.lprng.com/) documentation had useful
   information about HP's *JetDirect* protocol, also known as the
   *Socket API*, which was the *de facto* standard for communicating
   with network printers.
 * [CUPS documentation](https://www.cups.org/documentation.html)
 * PJL: [pjl-qref.html](doc/pjl-qref.html) is a local summary of
   HP's [Printer Job Language Technical Reference Manual](https://developers.hp.com/system/files/PJL_Technical_Reference_Manual.pdf)

The unfinished *pracc-scan* tool was supposed to count pages in
print jobs prior to printing, which required an understanding of
the print job and therefore typically PostScript, PCL5 and PCL XL
(aka PCL6); PCL5 documentation was easily available, but at the
time PCL XL was hard to find (nowadays search the Internet for
HP's *PCL XL Feature Reference* to find the specification).

