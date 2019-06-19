rem netsh firewall set opmode ENABLE
netsh firewall set portopening TCP 445 Microsoft-ds ENABLE all
netsh firewall set portopening TCP 139 NetBIOS-ssn ENABLE all
pause

@echo off
rem  These lines open ports tcp/445 and tcp/139 for inbound connections
rem  Uncomment the first line for the script to enable the firewall.
rem  All changes apply to the current profile.
rem  ujr/2005-05-19, 2005-11-11
