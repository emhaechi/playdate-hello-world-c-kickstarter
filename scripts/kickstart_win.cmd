@ECHO OFF

REM - This is a Windows CMD batch file to create a new Playdate project
REM   based on a template within "~PROJECT_ROOT/kickstart_templates".
REM   (It defers to a PowerShell script, "kickstart_win__impl.ps1", to do the heavy lifting.)
REM
REM - Execute this script from this project's root directory, referred to here as
REM   "~PROJECT_ROOT". The following will happen:
REM   - A prompt will ask for:
REM     - a template name from which to generate the project (a directory in "~PROJECT_ROOT/kickstart_templates"),
REM     - the new project's public/display name, author, description, and Playdate build bundle id you want to init the Playdate game metadata with,
REM     - the new project's Playdate build name (which is to be the value of "PLAYDATE_GAME_NAME" in build processes).
REM   - The new project's files will be generated into "~PROJECT_ROOT/projects/<PLAYDATE_GAME_NAME>"


powershell -ExecutionPolicy Bypass -File "%~dp0kickstart_win__impl.ps1"
if %errorlevel%==0 (
	echo -- KICKSTART SUCCESS
) else (
	echo -- KICKSTART FAILURE
)
