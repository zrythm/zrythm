
echo on

set STARTPWD=%CD%
set ORIGINALPATH=%PATH%
set PATH=C:\Program Files (x86)\Windows Kits\10\bin\x64;%PATH%

set vcvarsall="C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"

if not exist %vcvarsall% (
@   echo "Could not find MSVC vars batch file"
@   exit /b 2
)

set VERSION=%1
shift
if "%VERSION%" == "" (
@echo "Usage: win.bat <version>"
exit /b 1
)

@echo Building version %VERSION%

call %vcvarsall% amd64
if errorlevel 1 exit /b %errorlevel%

del /q /s build

meson setup build --buildtype release "-Dextra_include_dirs=C:\Program Files\libsndfile\include" "-Dextra_lib_dirs=C:\Program Files\libsndfile\lib" "-Db_vscrt=mt"
if errorlevel 1 exit /b %errorlevel%

ninja -C build
if errorlevel 1 exit /b %errorlevel%

cd build
ren rubberband-program.exe rubberband.exe
ren rubberband-program-r3.exe rubberband-r3.exe
set NAME=Christopher Cannam
signtool sign /v /n "%NAME%" /t http://time.certum.pl /fd sha1 /a rubberband.exe
if errorlevel 1 exit /b %errorlevel%

cd ..
set DIR=rubberband-%VERSION%-gpl-executable-windows
del /q /s %DIR%
mkdir %DIR%
copy build\rubberband.exe %DIR%
copy build\rubberband-r3.exe %DIR%
copy "c:\Program Files\libsndfile\bin\sndfile.dll" %DIR%
copy COPYING %DIR%\COPYING.txt
copy README.md %DIR%\README.txt
copy CHANGELOG %DIR%\CHANGELOG.txt

set PATH=%ORIGINALPATH%
cd %STARTPWD%
@echo Done, now test and zip the directory %DIR%

