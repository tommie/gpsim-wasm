rem This file was made to be run from gpsim/gpsim directory.
rem the only parameter would be the target directory.
echo on
set TargetDir=%1
echo %0 %1
if not exist %TargetDir%\bin\*.*                 md %TargetDir%\bin
if not exist %TargetDir%\etc\*.*                 md %TargetDir%\etc
if not exist %TargetDir%\lib\*.*                 md %TargetDir%\lib
if not exist %TargetDir%\popt1.dll               copy ..\..\popt\bin\popt1.dll %TargetDir%
if not exist %TargetDir%\libcairo-2.dll          copy ..\..\cairo\bin\libcairo-2.dll %TargetDir%
if not exist %TargetDir%\libpng13.dll            copy ..\..\png\bin\libpng13.dll %TargetDir%
if not exist %TargetDir%\zlib1.dll               copy ..\..\png\bin\zlib1.dll %TargetDir%
if not exist %TargetDir%\libintl-2.dll           copy ..\..\intl\bin\libintl-2.dll %TargetDir%
if not exist %TargetDir%\libiconv-2.dll          copy ..\..\iconv\bin\libiconv-2.dll %TargetDir%
if not exist %TargetDir%\libglib-2.0-0.dll       copy ..\..\glib\bin\libglib-2.0-0.dll %TargetDir%
if not exist %TargetDir%\iconv.dll               copy ..\..\iconv\bin\iconv.dll %TargetDir%
if not exist %TargetDir%\intl.dll                copy ..\..\gettext\bin\intl.dll %TargetDir%
if not exist %TargetDir%\libgobject-2.0-0.dll    copy ..\..\glib\bin\libgobject-2.0-0.dll %TargetDir%
if not exist %TargetDir%\libgthread-2.0-0.dll    copy ..\..\glib\bin\libgthread-2.0-0.dll %TargetDir%
if not exist %TargetDir%\libgmodule-2.0-0.dll    copy ..\..\glib\bin\libgmodule-2.0-0.dll %TargetDir%

if not exist %TargetDir%\libgdk-win32-2.0-0.dll  copy "..\..\gtk+\bin\libgdk-win32-2.0-0.dll" %TargetDir%
if not exist %TargetDir%\libgdk_pixbuf-2.0-0.dll copy "..\..\gtk+\bin\libgdk_pixbuf-2.0-0.dll" %TargetDir%
if not exist %TargetDir%\libgtk-win32-2.0-0.dll  copy "..\..\gtk+\bin\libgtk-win32-2.0-0.dll" %TargetDir%
if not exist %TargetDir%\etc\gtk-2.0\*.*         md %TargetDir%\etc\gtk-2.0
if not exist %TargetDir%\etc\gtk-2.0\g*.*        xcopy /s "..\..\gtk+\etc\gtk-2.0\*.*" %TargetDir%\etc\gtk-2.0
if not exist %TargetDir%\lib\gtk-2.0\*.*         md %TargetDir%\lib\gtk-2.0
if not exist %TargetDir%\lib\gtk-2.0\2.4.0\*.*   xcopy /s "..\..\gtk+\lib\gtk-2.0\*.*" %TargetDir%\lib\gtk-2.0

if not exist %TargetDir%\gtkextra-win32-2.1.dll  copy "..\..\gtkextra-2\bin\gtkextra-win32-2.1.dll" %TargetDir%

if not exist %TargetDir%\libpango-1.0-0.dll      copy ..\..\pango\bin\libpango-1.0-0.dll %TargetDir%
if not exist %TargetDir%\libpangocairo-1.0-0.dll copy ..\..\pango\bin\libpangocairo-1.0-0.dll %TargetDir%
if not exist %TargetDir%\libpangowin32-1.0-0.dll copy ..\..\pango\bin\libpangowin32-1.0-0.dll %TargetDir%
if not exist %TargetDir%\libpangoft2-1.0-0.dll   copy ..\..\pango\bin\libpangoft2-1.0-0.dll %TargetDir%
if not exist %TargetDir%\etc\pango\*.*           md %TargetDir%\etc\pango
if not exist %TargetDir%\lib\pango\*.*           md %TargetDir%\lib\pango
if not exist %TargetDir%\etc\pango\pango.*       xcopy /s ..\..\pango\etc\pango\*.* %TargetDir%\etc\pango
if not exist %TargetDir%\lib\pango\pango.*       xcopy /s ..\..\pango\lib\pango\*.* %TargetDir%\lib\pango

if not exist %TargetDir%\libatk-1.0-0.dll        copy "..\..\atk\bin\libatk-1.0-0.dll" %TargetDir%
if not exist %TargetDir%\pthreadVC2.dll          copy "..\..\pthreads\lib\pthreadVC2.dll" %TargetDir%

rem Sanity check
if not exist %TargetDir%\libiconv-2.dll if exist %TargetDir%\iconv.dll copy ..\..\iconv\bin\iconv.dll %TargetDir%\libiconv-2.dll
