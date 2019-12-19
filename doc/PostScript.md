
PostScript
==========

There are two important things to remember when dealing
with a PostScript printer:

  1. Communication with a PostScript printer is *full duplex*,
     i.e., the printer may talk back to you!
  2. You don't just send a file to be printed, you send a *program*
     to be interpreted. The printed sheet of paper is a side effect
     of this program execution.

Here is an example PostScript program. When interpreted by
a PostScript printer, it outputs a sheet of paper with the
text “Hello, World!” printed.

```PostScript
%!                       % PostScript signature
/Times-Roman findfont    % locate font named "Times-Roman"
15 scalefont setfont     % 15 point and set current font
300 500 moveto           % position on paper: x=300, y=500
(Hello, World!) show     % render string in raster memory
showpage                 % print raster memory to paper
```

From the PostScript interpreter's point of view, a *job* lasts
from the first byte received over some communications channel
through the next end-of-file (EOF) indication, e.g. Ctrl-D over
a serial connection or a network shutdown.


Communicating with a PostScript Printer
---------------------------------------

PostScript interpreters understand 7-bit ASCII.
Some of the non-printing ASCII characters have special meaning:

|Character|Octal|Description|
|---------|-----|-----------|
|Ctrl-C   | 003 | Interrupt, see below |
|Ctrl-D   | 004 | End of file, see below |
|Line Feed| 012 | End of line, the PostScript newline character |
|Return   | 015 | End of line, mapped to the PostScript newline |
|Ctrl-Q   | 021 | Start output (XON flow control) |
|Ctrl-S   | 023 | Stop output (XOFF flow control) |
|Ctrl-T   | 024 | Status query: interpreter sends a one-line status message|

Ctrl-D synchronizes the printer with the host: send a PostScript
program to the printer, then send an EOF to the printer. When the
printer has finished executing the program, it sends an EOF back.

While the interpreter is executing a PostScript program we can
send it an **interrupt** (Ctrl-C). This normally causes the program
being executed to terminate.

The **status query message** (Ctrl-T) causes a one-line status
message to be returned by the printer. All messages received
from the printer have the following format:

    %%[ Key: Value ]%%

Any number of key-value pairs can appear in a message, separated
by semicolons. Example:

    %%[ Error: undefined; OffendingCommand: ssetfont ]%%
    %%[ Flushing: rest of job (to end-of-file) will be ignored ]%%

Status messages have the format

    %%[ Status: idle ]%%

where other states besides `idle` (no job processing) are:
`busy` (executing a program), `waiting` (for more of the
PostScript program), `printing` (paper in motion),
`initializing`, and `printing test page`.

There are about 25 different errors that can occur. They have
the format as shown above. The most common errors are
`dictstackunderflow`, `invalidaccess`, `typecheck`, and `undefined`.
The operator in the second key-value pair is the operator that
caused the error.

When there is a problem with printing, a printer error is generated:

    %%[ PrinterError: reason ]%%

where _reason_ is often `Out Of Paper`, `Cover Open`, or
`Miscellaneous Error`.

The PostScript program itself can also generate output using
the PostScript `print` operator. Such output should not be
interpreted by the program driving the printer; instead it
should be sent to the user who sent the program to the printer.


The PostScript Language
-----------------------

### PostScript Objects

  * simple: integer, mark, real, boolean, name, null
  * composite: string, dictionary, array (comprises procedure bodies)

Objects have attributes:

  * literal/executable
  * access (read/write/execute)
  * length (for composite objects only)
  * type (PostScript objects are typed)

### Dictionary Stack

  * only dictionary type objects can be pushed
  * two entries initially:
    1. systemdict (below; PostScript operators)
    2. userdict (above; writable, ca 200 entries)
  * current dictionary is the one on top
  * use `begin` to push another dictionary
  * use `end` to pop the topmost dictionary
  * new defs are placed in the current dictionary
  * `currentdict` pushes current dict on op stack

Create dictionary with three entries:

```PostScript
3 dict begin
 /proc1 { pop } def
 /two 2 def
 /three (trois) def
currentdict end
```

Equivalent:

```PostScript
3 dict
dup /proc1 { pop } put
dup /two 2 put
dup /three (trois) put
```

### Operators and Name Lookup

 * literal name: `/moveto`
 * executable name: `moveto`

When PostScript encounters an executable name, it is looked up
in the current environment and what is found is executed.

How to read the page counter:

```PostScript
statusdict begin pagecount == end
```

How to busy-wait for 1000 ms:

```PostScript
usertime dup 1000 add
{ %loop
    dup usertime lt { pop exit } if
} bind loop
```

The `usertime` operator takes no arguments and returns
execution time in milliseconds. A few other operators are
listed below; left of the operator are its arguments (`--`
indicates none) and right of it its return values.

| Operator | Description |
|----------|-------------|
| -- **version** string | interpreter identification string |
| -- **realtime** int   | real time in milliseconds |
| -- **usertime** int   | execution time in msec (for interval timing) |
| -- **languagelevel** int | ll. supported by interpreter (1 if undefined) |
| -- **product** string | product name (e.g. Printer model) |
| -- **revision** int   | product revision level |
| -- **serialnumber** int | product serial number |
| int **string** string |create string of given length, init with '\0' |
| any string **cvs** string | convert 'any' to string representation |


References
----------

The information here is taken from various sources,
but mostly from the following:

W. Richard Stevens, 1993, *Advanced Programming
in the UNIX Environment*, Addison-Wesley.
[ISBN 0201563177](https://www.amazon.com/dp/0201563177),
[www.apuebook.com](http://www.apuebook.com/)

Adobe Systems, 1988, *PostScript Language Program Design*,
Addison-Wesley. Known as the “Green Book”.
[ISBN 0201143968](https://www.amazon.com/dp/0201143968)
