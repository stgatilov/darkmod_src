if "%1" == "32" set PLATFORM=Win32
if "%1" == "64" set PLATFORM=x64
if "%PLATFORM%" == "32" set PLATFORM=Win32
if "%PLATFORM%" == "64" set PLATFORM=x64

msbuild ../TheDarkMod.sln -p:Configuration="Debug Fast"     -p:Platform="%PLATFORM%"    || exit /b
msbuild ../TheDarkMod.sln -p:Configuration="Debug Editable" -p:Platform="%PLATFORM%"    || exit /b
msbuild ../TheDarkMod.sln -p:Configuration="Sanitize"       -p:Platform="%PLATFORM%"    || exit /b
