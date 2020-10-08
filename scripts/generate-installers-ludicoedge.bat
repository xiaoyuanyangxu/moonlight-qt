@echo off
setlocal enableDelayedExpansion

rem Run from Qt command prompt with working directory set to root of repo

set BUILD_CONFIG=%1
set ARCH=%2

rem Convert to lower case for windeployqt
if /I "%BUILD_CONFIG%"=="debug" (
    set BUILD_CONFIG=debug
    set WIX_MUMS=10
) else (
    if /I "%BUILD_CONFIG%"=="release" (
        set BUILD_CONFIG=release
        set WIX_MUMS=10
    ) else (
        if /I "%BUILD_CONFIG%"=="signed-release" (
            set BUILD_CONFIG=release
            set SIGN=1
            set MUST_DEPLOY_SYMBOLS=1

            rem Fail if there are unstaged changes
            git diff-index --quiet HEAD --
            if !ERRORLEVEL! NEQ 0 (
                echo Signed release builds must not have unstaged changes!
                exit /b 1
            )
        ) else (
            echo Invalid build configuration - expected 'debug' or 'release'
            exit /b 1
        )
    )
)

if /I "%ARCH%" NEQ "x86" (
    if /I "%ARCH%" NEQ "x64" (
        echo Invalid build architecture - expected 'x86' or 'x64'
        exit /b 1
    )
)

set SIGNTOOL_PARAMS=sign /tr http://timestamp.digicert.com /td sha256 /fd sha256 /sha1 1B3C676E831A94EC0327C3347EB32C68C26B3A67 /v

set BUILD_ROOT=%cd%\build
set SOURCE_ROOT=%cd%
set BUILD_FOLDER=%BUILD_ROOT%\build-%ARCH%-%BUILD_CONFIG%
set DEPLOY_FOLDER=%BUILD_ROOT%\deploy-%ARCH%-%BUILD_CONFIG%
set INSTALLER_FOLDER=%BUILD_ROOT%\installer-%ARCH%-%BUILD_CONFIG%
set SYMBOLS_FOLDER=%BUILD_ROOT%\symbols-%ARCH%-%BUILD_CONFIG%
set /p VERSION=<%SOURCE_ROOT%\app\version.txt

rem Find Visual Studio and run vcvarsall.bat
set VSWHERE="%SOURCE_ROOT%\scripts\vswhere.exe"
for /f "usebackq delims=" %%i in (`%VSWHERE% -latest -property installationPath`) do (
    call "%%i\VC\Auxiliary\Build\vcvarsall.bat" %ARCH%
)
if !ERRORLEVEL! NEQ 0 goto Error

rem Find VC redistributable DLLs
for /f "usebackq delims=" %%i in (`%VSWHERE% -latest -find VC\Redist\MSVC\*\%ARCH%\Microsoft.VC*.CRT`) do set VC_REDIST_DLL_PATH=%%i

echo Cleaning output directories
rmdir /s /q %DEPLOY_FOLDER%
rmdir /s /q %BUILD_FOLDER%
rmdir /s /q %INSTALLER_FOLDER%
rmdir /s /q %SYMBOLS_FOLDER%
mkdir %BUILD_ROOT%
mkdir %DEPLOY_FOLDER%
mkdir %BUILD_FOLDER%
mkdir %INSTALLER_FOLDER%
mkdir %SYMBOLS_FOLDER%

echo Configuring the project
pushd %BUILD_FOLDER%
qmake %SOURCE_ROOT%\moonlight-qt.pro
if !ERRORLEVEL! NEQ 0 goto Error
popd

echo Compiling LudicoEdge in %BUILD_CONFIG% configuration
pushd %BUILD_FOLDER%
%SOURCE_ROOT%\scripts\jom.exe %BUILD_CONFIG%
if !ERRORLEVEL! NEQ 0 goto Error
popd

echo Saving PDBs
for /r "%BUILD_FOLDER%" %%f in (*.pdb) do (
    copy "%%f" %SYMBOLS_FOLDER%
    if !ERRORLEVEL! NEQ 0 goto Error
)
copy %SOURCE_ROOT%\libs\windows\lib\%ARCH%\*.pdb %SYMBOLS_FOLDER%
if !ERRORLEVEL! NEQ 0 goto Error
7z a %SYMBOLS_FOLDER%\MoonlightDebuggingSymbols-%ARCH%-%VERSION%.zip %SYMBOLS_FOLDER%\*.pdb
if !ERRORLEVEL! NEQ 0 goto Error

if "%ML_SYMBOL_STORE%" NEQ "" (
    echo Publishing PDBs to symbol store: %ML_SYMBOL_STORE%
    symstore add /f %SYMBOLS_FOLDER%\*.pdb /s %ML_SYMBOL_STORE% /t Moonlight
    if !ERRORLEVEL! NEQ 0 goto Error
) else (
    if "%MUST_DEPLOY_SYMBOLS%"=="1" (
        echo "A symbol server must be specified in ML_SYMBOL_STORE for signed release builds"
        exit /b 1
    )
)

if "%ML_SYMBOL_ARCHIVE%" NEQ "" (
    echo Copying PDB ZIP to symbol archive: %ML_SYMBOL_ARCHIVE%
    copy %SYMBOLS_FOLDER%\MoonlightDebuggingSymbols-%ARCH%-%VERSION%.zip %ML_SYMBOL_ARCHIVE%
    if !ERRORLEVEL! NEQ 0 goto Error
) else (
    if "%MUST_DEPLOY_SYMBOLS%"=="1" (
        echo "A symbol archive directory must be specified in ML_SYMBOL_ARCHIVE for signed release builds"
        exit /b 1
    )
)

echo Copying DLL dependencies
copy %SOURCE_ROOT%\libs\windows\lib\%ARCH%\*.dll %DEPLOY_FOLDER%
if !ERRORLEVEL! NEQ 0 goto Error

echo Copying AntiHooking.dll
copy %BUILD_FOLDER%\AntiHooking\%BUILD_CONFIG%\AntiHooking.dll %DEPLOY_FOLDER%
if !ERRORLEVEL! NEQ 0 goto Error

echo Copying d3dx9_43.dll from DirectX SDK
expand "%DXSDK_DIR%\Redist\Jun2010_d3dx9_43_%ARCH%.cab" -F:d3dx9_43.dll %DEPLOY_FOLDER%
if !ERRORLEVEL! NEQ 0 goto Error

echo Copying GC mapping list
copy %SOURCE_ROOT%\app\SDL_GameControllerDB\gamecontrollerdb.txt %DEPLOY_FOLDER%
if !ERRORLEVEL! NEQ 0 goto Error

echo Deploying Qt dependencies
windeployqt.exe --dir %DEPLOY_FOLDER% --%BUILD_CONFIG% --qmldir %SOURCE_ROOT%\app\gui --no-opengl-sw --no-compiler-runtime --no-qmltooling %BUILD_FOLDER%\app\%BUILD_CONFIG%\LudicoEdge.exe
if !ERRORLEVEL! NEQ 0 goto Error

echo Generating QML cache
forfiles /p %DEPLOY_FOLDER% /m *.qml /s /c "cmd /c qmlcachegen.exe @path"
if !ERRORLEVEL! NEQ 0 goto Error

echo Deleting original QML files
forfiles /p %DEPLOY_FOLDER% /m *.qml /s /c "cmd /c del @path"
if !ERRORLEVEL! NEQ 0 goto Error

echo %WIX% %DEPLOY_FOLDER% %BUILD_FOLDER% 
echo Harvesting files for WiX
"%WIX%\bin\heat" dir %DEPLOY_FOLDER% -srd -sfrag -ag -sw5150 -cg MoonlightDependencies -var var.SourceDir -dr INSTALLFOLDER -out %BUILD_FOLDER%\Dependencies.wxs
if !ERRORLEVEL! NEQ 0 goto Error

echo Copying application binary to deployment directory
copy %BUILD_FOLDER%\app\%BUILD_CONFIG%\LudicoEdge.exe %DEPLOY_FOLDER%
if !ERRORLEVEL! NEQ 0 goto Error

if "%SIGN%"=="1" (
    echo Signing deployed binaries
    set FILES_TO_SIGN=
    for /r "%DEPLOY_FOLDER%" %%f in (*.dll *.exe) do (
        set FILES_TO_SIGN=!FILES_TO_SIGN! %%f
    )
    signtool %SIGNTOOL_PARAMS% !FILES_TO_SIGN!
    if !ERRORLEVEL! NEQ 0 goto Error
)

if "%ML_SYMBOL_STORE%" NEQ "" (
    echo Publishing binaries to symbol store: %ML_SYMBOL_STORE%
    symstore add /r /f %DEPLOY_FOLDER%\*.* /s %ML_SYMBOL_STORE% /t Moonlight
    if !ERRORLEVEL! NEQ 0 goto Error
)

echo Building MSI
msbuild %SOURCE_ROOT%\wix\LudicoEdge\LudicoEdge.wixproj /p:Configuration=%BUILD_CONFIG% /p:Platform=%ARCH%
if !ERRORLEVEL! NEQ 0 goto Error

if "%SIGN%"=="1" (
    echo Signing MSI
    signtool %SIGNTOOL_PARAMS% %BUILD_FOLDER%\LudicoEdge.msi
    if !ERRORLEVEL! NEQ 0 goto Error
)

echo Building bundle
rem Bundles are always x86 binaries
msbuild %SOURCE_ROOT%\wix\LudicoEdgeSetup\LudicoEdgeSetup.wixproj /p:Configuration=%BUILD_CONFIG% /p:Platform=x86
if !ERRORLEVEL! NEQ 0 goto Error

if "%SIGN%"=="1" (
    echo Signing bundle
    "%WIX%\bin\insignia" -ib %INSTALLER_FOLDER%\LudicoEdgeSetup.exe -o %BUILD_FOLDER%\engine.exe
    if !ERRORLEVEL! NEQ 0 goto Error
    signtool %SIGNTOOL_PARAMS% %BUILD_FOLDER%\engine.exe
    if !ERRORLEVEL! NEQ 0 goto Error
    "%WIX%\bin\insignia" -ab %BUILD_FOLDER%\engine.exe %INSTALLER_FOLDER%\LudicoEdgeSetup.exe -o %INSTALLER_FOLDER%\LudicoEdgeSetup.exe
    if !ERRORLEVEL! NEQ 0 goto Error
    signtool %SIGNTOOL_PARAMS% %INSTALLER_FOLDER%\LudicoEdgeSetup.exe
    if !ERRORLEVEL! NEQ 0 goto Error
)

rem Rename the installer to match the publishing convention
ren %INSTALLER_FOLDER%\LudicoEdgeSetup.exe LudicoEdgeSetup-%ARCH%-%VERSION%.exe

echo Building portable package
rem This must be done after WiX harvesting and signing, since the VCRT dlls are MS signed
rem and should not be harvested for inclusion in the full installer
copy "%VC_REDIST_DLL_PATH%\*.dll" %DEPLOY_FOLDER%
if !ERRORLEVEL! NEQ 0 goto Error
rem This file tells Moonlight that it's a portable installation
echo. > %DEPLOY_FOLDER%\portable.dat
if !ERRORLEVEL! NEQ 0 goto Error
7z a %INSTALLER_FOLDER%\LudicoEdgeSetup-%ARCH%-%VERSION%.zip %DEPLOY_FOLDER%\*
if !ERRORLEVEL! NEQ 0 goto Error

echo Build successful for Moonlight v%VERSION%!
exit /b 0

:Error
echo Build failed!
exit /b !ERRORLEVEL!
