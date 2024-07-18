@ECHO OFF

REM - This is a Windows CMD batch file to create a hardware build of a Playdate project 
REM   that can be played on either the Playdate Simulator or an actual Playdate
REM   device.
REM
REM - Some edits need to be made to the ENV, or to this script's assignments below, before executing this on a new system:
REM   - Define ENV's PD_MAKE_PATH to point to make.exe.
REM   - Define ENV'S PD_CMAKE_C_COMPILER to point to C compiler exe.
REM   - Define ENV's PD_CMAKE_TOOLCHAIN_FILE to point to Playdate SDK's arm.cmake.
REM
REM - Execute this script from the project's root directory, referred to as 
REM   "~PROJECT_ROOT". The following will happen:
REM   - CMake build artifacts are generated into "~PROJECT_ROOT/build",
REM   - Playdate's CMake scripts will generate the distributable as the
REM     "~PROJECT_ROOT/<PLAYDATE_GAME_NAME>.pdx" directory, and create the 
REM     intermediary "~PROJECT_ROOT/Source" directory.
REM   - A zip archive of the Playdate distributable directory is 
REM     generated into "~PROJECT_ROOT/dist" (if command exists).
REM
REM - Developed against Playdate SDK v2.5.0.


set PD_MAKE_PATH=%PD_MAKE_PATH%
if "%PD_MAKE_PATH%"=="" (set PD_MAKE_PATH=)

set PD_CMAKE_C_COMPILER=%PD_CMAKE_C_COMPILER%
if "%PD_CMAKE_C_COMPILER%"=="" (set PD_CMAKE_C_COMPILER=)

set PD_CMAKE_TOOLCHAIN_FILE=%PD_CMAKE_TOOLCHAIN_FILE%
if "%PD_CMAKE_TOOLCHAIN_FILE%"=="" (set PD_CMAKE_TOOLCHAIN_FILE=)

REM - argv[0] == Release || Debug
set PD_BUILD_TYPE=%1
if "%PD_BUILD_TYPE"=="" (set PD_BUILD_TYPE=Release)

where.exe /q tar && (set CREATE_ZIP=true) || (set CREATE_ZIP=false)

if "%PD_MAKE_PATH%"=="" (echo -- PD_MAKE_PATH Path not found; set ENV value PD_MAKE_PATH to make.exe or edit script && exit /b 1)
if "%PD_CMAKE_C_COMPILER%"=="" (echo -- PD_CMAKE_C_COMPILER Path not found; set ENV value PD_CMAKE_C_COMPILER to C compiler exe or edit script && exit /b 1)
if "%PD_CMAKE_TOOLCHAIN_FILE%"=="" (echo -- PD_CMAKE_TOOLCHAIN_FILE Path not found; set ENV value PD_CMAKE_TOOLCHAIN_FILE to Playdate SDK's arm.cmake or edit script && exit /b 1)


REM - clean any previous build artifacts to ensure build always has latest:
for /d %%d in ("build" "Source" "*.pdx" "dist") do (
  (rmdir /s /q "%%~d" && echo -- goodbye, %%~d) || echo -- did not find to remove: %%~d
)

REM - run cmake/make config and build, in ~PROJECT_ROOT/build dir (FIXME: have to run this set twice to ensure all needed Playdate files are generated):
for /l %%i in (1, 1, 2) do (
	(cmake -G "MinGW Makefiles" -DCMAKE_C_COMPILER="%PD_CMAKE_C_COMPILER%" -DCMAKE_TOOLCHAIN_FILE="%PD_CMAKE_TOOLCHAIN_FILE%" -DCMAKE_BUILD_TYPE=%PD_BUILD_TYPE% -B "./build" && echo -- CONFIG COMPLETED) && ^^
	("%PD_MAKE_PATH%" -C "./build" && echo -- BUILD COMPLETED)
)

REM - generate zip archive of the Playdate distributable directory:
if "%CREATE_ZIP%"=="true" (
	for /d %%d in ("*.pdx") do (
		(mkdir "dist" && tar -a -cf "dist/%%~d.zip" %%~d && echo -- generated zip archive into dist/) || (echo -- failed generating zip archive into dist/)
	)
) else (
	echo -- zip command not found, zip archive will not be generated!
)


REM - if truly successful build:
REM - "~PROJECT_ROOT/Source" directory should contain pdex.dll, pdex.elf, and the rest of the project
REM - "~PROJECT_ROOT/<PLAYDATE_GAME_NAME>.pdx" directory should contain pdex.bin, pdex.dll, and the rest of the project
for /f %%f in ("Source/pdex.dll" "Source/pdex.elf" "*.pdx/pdex.bin" "*.pdx/pdex.dll") do (
  if not exist "%%~f" echo -- BUILD FAILURE && exit /b 1
)

echo -- BUILD SUCCESS
