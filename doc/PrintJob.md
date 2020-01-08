
Example Print Job
=================

At the time of Pracc, the typical Windows printer driver
generated either PCL or PostScript, and wrapped it in PJL
to create the print job that was then sent to the printing
device. Here is a typical example (PostScript wrapped in PJL):

```PostScript
^[%-12345X@PJL JOB
@PJL SET STRINGCODESET=UTF8
@PJL COMMENT "Username: UNTITLED; App Filename: Testseite; 12-13-2003"
@PJL SET JOBATTR="JobAcct1=UNTITLED"
@PJL SET JOBATTR="JobAcct2=jupiter"
@PJL SET JOBATTR="JobAcct3=WINGHOSTBOOK"
@PJL SET JOBATTR="JobAcct4=20031213151817"
@PJL SET USERNAME="UNTITLED"
@PJL SET RESOLUTION=600
@PJL SET BITSPERPIXEL=2
@PJL SET ECONOMODE=OFF
@PJL ENTER LANGUAGE=POSTSCRIPT
%!PS-Adobe-3.0
%%Title: Testseite
%%Creator: PScript5.dll Version 5.2
%%BoundingBox: (atend)
%%Pages: (atend)
%%EndComments
...
%%Pages: 1
%%EOF
^D^[%-12345X@PJL EOJ
^[%-12345X
```

The magic string `^[%-12345X` is the UEL (universal exit language).
Here `^[` stands for ASCII ESC (27 decimal) and `^D` stands for
ASCII EOT (4 decimal), which terminates the PostScript code.
Only the very beginning and the very end of the PostScript part
is shown. We often found the wrapped PostScript to use Windows line
endings (CR LF) while the outer PJL used Unix line endings (LF).
