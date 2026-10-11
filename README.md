Fast screenshot taker for windows for multimonitor environment. 
It saves separate PNG file from every monitor. 

By default it takes stcreenshots from all monitors and saves into current directory
monitor_0.png
monitor_1.png
...
monitor_N.png

Example usage:
screenshot.exe -m 0 -t -p -o c:\temp\
will 
- take primary monitor screentshot
- save it into temp dir
- add datetime to filename
- open it in mspaint.exe using "mspaint.exe c:\temp\2026-10-11_14-48-28_0.png"




Usage: screenshot.exe [-m 0 -m 1 ...] [-o file_prefix] [--help|-h] [-t]
By default it saves screenshots from monitors into separate files
-m monitor number, for example -m 0 -m 2 
-o output file prefix for example c:\temp\screenshot 
-t add date-time to file name
-p run mspaint.exe
-h help
Example screenshot.exe -m 0
