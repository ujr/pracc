
Network Printing
================

There are several options to communicate with
printers over a TCP/IP network; two are presented
here, the Socket API and the AppSocket.
This information is from the LPRng-HOWTO.


Socket API
----------

Widely used by most print server manufacturers.  
HP's JetDirect defines the _de facto_ standard.  
The socket API is simple:

1. Establish a TCP/IP connection to the printer.  
   HP JetDirect uses **port 9100** by default.  
   This connection may be refused if the printer is
   busy printing a job.

2. Once the connection is established, send the print job.
   The connection is bidirectional: error messages may be
   returned at any time over this same connection.

3. After all job data is sent, _half close_ the connection
   (the `shutdown()` network system call) to indicate that
   no more data will be sent.

4. The printer may terminate the connection immediately
   (no more messages will be received) or after the job
   finished printing (so that you may receive messages).

While still printing, the printer may refuse new connections,
or it may refuse all data transfers on the connection.

The socket API can be used with the [netcat][nc] (nc) command
line tool, for example to send the job *file.ps* to the printer
at IP 192.168.1.123, do:

```Shell
nc 192.168.1.123 9100 < file.ps
```

[nc]: http://nc110.sourceforge.net/


AppSocket
---------

The AppSocket interface is supported by Tektronix and some other
printer vendors. It is similar to the Socket API, with a couple
of significant differences.

1. The printer has two ports for network connections:
   * a TCP port 9100 for TCP/IP stream connections
   * a UDP port for UDP packet connections

2. When a zero length UDP packet or a UDP packet containing only
   `CR/LF` is sent to UDP port 9101, the printer will return
   a UDP packet to the sender containing print status information.
   This information indicates the printer's current status (busy,
   idle, printing) and any error conditions.

3. The format, reliability, and repeatability of the UDP format
   and information is totally undocumented.

4. To send a print job, make a TCP connection to port 9100.
   This connection will be refused while the printer is busy or
   has a connection to another host.

5. After the connection is established, send the job data.

6. When the job has been transferred, close the connection
   for sending (but keep it open for receiving messages).
   Alternatively, an end-of-job (EOJ) indication in the data
   stream (such as a Ctrl-D in a PostScript job) will
   terminate the connection.

7. Some printer models will not terminate the connection, but
   simply ignore any data following the EOJ indication.

8. Some printers support bidirectional AppSocket communication.
   They return error indications and status info while the
   connection is open.

9. Once all data has been received and the job has finished
   printing, the connection will be terminated by the printer.

(Unser Xerox Phaser 8200DP zeigt auf dem Frontpanel immer an,
dass die Jobquelle 'AppSocket' sei, verhaelt sich aber nicht
entsprechend dem eben beschriebenen Protokoll.)
