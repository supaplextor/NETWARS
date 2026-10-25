@echo off
rem NETWARS builder for Borland C++ toolchain in ..\BCPP
rem DOS-style arguments:
rem   build (default), rebuild, clean
rem   debug, release
rem   pklite

set BCPP=..\BCPP
set BIN=%BCPP%\BIN
set INC=%BCPP%\INCLUDE
set LIBDIR=%BCPP%\LIB

set SRC=src
set OBJ=obj
set EXE=netwars.exe
set MAP=netwars.map

set TARGET=build
set CFG=release
set DO_PKLITE=0

:parse_args
if "%1"=="" goto args_done
if "%1"=="build" set TARGET=build
if "%1"=="rebuild" set TARGET=rebuild
if "%1"=="clean" set TARGET=clean
if "%1"=="debug" set CFG=debug
if "%1"=="release" set CFG=release
if "%1"=="pklite" set DO_PKLITE=1
if "%1"=="/?" goto usage
if "%1"=="-h" goto usage
if "%1"=="--help" goto usage
if "%1"=="build" goto next_arg
if "%1"=="rebuild" goto next_arg
if "%1"=="clean" goto next_arg
if "%1"=="debug" goto next_arg
if "%1"=="release" goto next_arg
if "%1"=="pklite" goto next_arg
echo Unknown argument: %1
goto usage

:next_arg
shift
goto parse_args

:args_done
if not exist %BIN%\BCC.EXE goto missing_toolchain
if not exist %BIN%\TASM.EXE goto missing_toolchain
if not exist %BIN%\TLINK.EXE goto missing_toolchain

set INCLUDE=%INC%
set LIB=%LIBDIR%
set PATH=%BIN%;%PATH%

if "%TARGET%"=="clean" goto clean_only
if "%TARGET%"=="rebuild" goto clean_then_build
goto build

:clean_then_build
if exist %OBJ%\startup.obj del %OBJ%\startup.obj
if exist %OBJ%\netwars.obj del %OBJ%\netwars.obj
if exist %EXE% del %EXE%
if exist %MAP% del %MAP%
goto build

:clean_only
if exist %OBJ%\startup.obj del %OBJ%\startup.obj
if exist %OBJ%\netwars.obj del %OBJ%\netwars.obj
if exist %EXE% del %EXE%
if exist %MAP% del %MAP%
echo.
echo Completed: target=clean
goto end

:build
if not exist %OBJ% md %OBJ%

if "%CFG%"=="debug" set CFLAGS=-c -ml -3 -v -Od -DDEBUG -I%SRC%
if "%CFG%"=="release" set CFLAGS=-c -ml -3 -O2 -I%SRC%

echo [1/3] Assembling %SRC%\startup.asm
%BIN%\TASM.EXE /ml /m2 %SRC%\startup.asm, %OBJ%\startup.obj
if errorlevel 1 goto fail

echo [2/3] Compiling %SRC%\netwars.cpp (%CFG%)
%BIN%\BCC.EXE %CFLAGS% -o%OBJ%\netwars.obj %SRC%\netwars.cpp
if errorlevel 1 goto fail

echo [3/3] Linking %EXE%
%BIN%\TLINK.EXE /m /s /l %OBJ%\startup.obj %OBJ%\netwars.obj, %EXE%, %MAP%, %LIBDIR%\cl.lib %LIBDIR%\emu.lib %LIBDIR%\maths.lib
if errorlevel 1 goto fail

if "%DO_PKLITE%"=="1" goto do_pklite
goto success

:do_pklite
echo [4/4] Compressing with PKLITE
pklite -x %EXE%
if errorlevel 1 goto fail
goto success

:missing_toolchain
echo ERROR: Borland tools not found under %BCPP%
echo Expected:
echo   %BIN%\BCC.EXE
echo   %BIN%\TASM.EXE
echo   %BIN%\TLINK.EXE
goto end

:usage
echo.
echo NETWARS build helper
echo.
echo Usage:
echo   builder.bat [build^|rebuild^|clean] [debug^|release] [pklite]
echo.
echo Examples:
echo   builder.bat
echo   builder.bat build
echo   builder.bat rebuild debug
echo   builder.bat build release pklite
goto end

:fail
echo.
echo Build failed.
goto end

:success
echo.
echo Completed: target=%TARGET% config=%CFG%
goto end

:end
