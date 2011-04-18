@echo off
rem Batch file for mounting encfs encrypted folders
title encfs: Mount an encrypted folder to a decrypted one

rem Check if Windows XP or Windows 7
rem XP: C:\Documents and Settings (or language specific folder)
rem 7: C:\Users

set oprsystem=%appdata:~3,5%
if %oprsystem%==Users (
  set ops=win7
) else (
  set ops=winxp
)


set ininumber=1
rem An ini file will be created for future access 
if not exist encfs.ini goto firsttime
set ininumber=0

rem List existing encrypted / decrypted folder pairs
echo Already existing encrypted / decrypted folder pairs:
echo.
for /F "tokens=1,2,3,4 delims=µ" %%i in (encfs.ini) do (
  echo No.: %%i	Name:             %%j
  echo 	Encrypted folder: %%k
  echo 	Decrypted folder: %%l
  echo.
)
echo.
echo Choose which pair should be mounted.
echo Type the corresponding number and press ENTER
echo Just press ENTER if you want to create a new pair.
echo.

set /p ininumber=Type number: 


set new=yes
for /F "tokens=1,2,3,4 delims=µ" %%i in (encfs.ini) do (
  if %%i==%ininumber% (
    set crypt=%%k
    set decrypt=%%l
    set pair=%%j
    set new=no
  )
  set number=%%i
)


if %new%==no goto mount
rem Increment ininumber for new folder pair
set /a ininumber= %number% + 1 



:firsttime
rem First time use
rem Ask for folder locations

cls
echo Please enter the location for the folder
echo that will contain the encrypted files and press ENTER:
echo [e.g. d:\crypt - don't use a trailing backslash]
echo.

set /p crypt=


echo.
if %ops%==win7 echo Please enter the location for the drive
if %ops%==winxp echo Please enter the location for the folder
echo where you want to be able to access the decrypted files and press ENTER.
if %ops%==win7 echo [e.g. x: - don't use a trailing backslash]
if %ops%==winxp echo [e.g. d:\plain or x: - don't use a trailing backslash]
echo.

set /p decrypt=

echo.
echo Please enter a name for the encrypted / decrypted folder pair and press ENTER
echo [e.g. Secret Files]
echo.

set /p pair=

echo.
echo.
echo.

echo %ininumber%µ%pair%µ%crypt%µ%decrypt%>>encfs.ini



:mount
cls
echo Mount   "%crypt%"   to   "%decrypt%"
if not exist "%crypt%" md "%crypt%"
rem If decrypt folder is a drive and encfs is on its first run decrypt folder is set to a temp folder
set lastchar=%decrypt:~-1%
if "%lastchar%"==":" (
  if not exist "%crypt%\.encfs6.xml" (
    set decrypt="%temp%\decrypttemp"
    if not exist "%temp%\decrypttemp" md "%temp%\decrypttemp"
    echo.
    echo IMPORTANT
    echo After initialising encfs for the first time
    echo please close this window and start "encfs_mount" again
    echo.
    pause
    echo.
  )
) else (
  if not exist "%decrypt%" md "%decrypt%"
)


rem Mount encfs
echo.
encfs "%crypt%" "%decrypt%"

pause