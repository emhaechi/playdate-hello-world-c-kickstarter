
# - This is a Windows PowerShell script file to create a new Playdate project
#   based on a template within "~PROJECT_ROOT/kickstart_templates".
# - (See CMD batch file "kickstart_win.cmd" for more high-level details.)


"Found the following templates:" | Out-Host
Get-ChildItem -LiteralPath ./kickstart_templates -Directory -Name | Out-Host

$templateName = Read-Host "Which template do you want to use? (*)"
$templatePath = "./kickstart_templates/$templateName"
if (($templateName.Length -eq 0) -or !(Test-Path -LiteralPath "$templatePath" -PathType Container)) {
	"-- error: template name not given or failed to find template at path: $templatePath" | Out-String
	exit 1
}
"-- using template: $templateName" | Out-String


#### prompts into replace variables

$templateParams = @{}

# prompt for project's name in Playdate game metadata / pdxinfo, shown when installed onto devices: __KICKSTART_PUBLIC_NAME__
$templateParams.__KICKSTART_PUBLIC_NAME__ = Read-Host "Enter your project's public/display name (ex, Example Project Name) (*)"
if ($templateParams.__KICKSTART_PUBLIC_NAME__.Length -eq 0) {
	"-- error: public name not given" | Out-String
	exit 1
}
"-- setting public name: $($templateParams.__KICKSTART_PUBLIC_NAME__)" | Out-String

# prompt for project's name in CMake build scripts, shown as distributable game file names: __KICKSTART_PLAYDATE_GAME_NAME__
$templateParams.__KICKSTART_PLAYDATE_GAME_NAME__ = Read-Host "Enter your project's build/file name (ex, example_project_name) (*)"
if ($templateParams.__KICKSTART_PLAYDATE_GAME_NAME__.Length -eq 0) {
	"-- error: build name not given" | Out-String
	exit 1
}
if ($templateParams.__KICKSTART_PLAYDATE_GAME_NAME__ -match '[^A-Za-z0-9_]') {
	"-- error: invalid build name given, must only contain ASCII alphanumeric, _ characters" | Out-String
	exit 1
}
"-- setting build name: $($templateParams.__KICKSTART_PLAYDATE_GAME_NAME__)" | Out-String

# prompt for project's Playdate game metadata's bundle id: __KICKSTART_BUNDLE_ID__
$templateParams.__KICKSTART_BUNDLE_ID__ = Read-Host "Enter your project's build bundle id (ex, com.example.projectDomain) (*)"
if ($templateParams.__KICKSTART_BUNDLE_ID__.Length -eq 0) {
	"-- error: bundle id not given" | Out-String
	exit 1
}
if ($templateParams.__KICKSTART_BUNDLE_ID__ -match '[^A-Za-z0-9_.]') {
	"-- error: invalid bundle id given, must only contain ASCII alphanumeric, _, . characters (reverse DNS format)" | Out-String
	exit 1
}
"-- setting bundle id: $($templateParams.__KICKSTART_BUNDLE_ID__)" | Out-String

# prompt for project's Playdate game metadata's author: __KICKSTART_AUTHOR__
$templateParams.__KICKSTART_AUTHOR__ = Read-Host "Enter your project's author"
"-- setting author: $($templateParams.__KICKSTART_AUTHOR__)" | Out-String

# prompt for project's Playdate game metadata's description: __KICKSTART_DESC__
$templateParams.__KICKSTART_DESC__ = Read-Host "Enter your project's description"
"-- setting description: $($templateParams.__KICKSTART_DESC__)" | Out-String


#### create "~PROJECT_ROOT/projects" directory if needed
New-Item -Path "./projects" -ItemType Directory -Force


#### copy out of template into new project dir
$newProjectPath = "./projects/$($templateParams.__KICKSTART_PLAYDATE_GAME_NAME__)"
if (!(Test-Path -LiteralPath $newProjectPath -PathType Container)) {

	try {
		Copy-Item -LiteralPath "$templatePath" -Destination $newProjectPath -Recurse -ErrorAction Stop
	}
	catch {
		"-- error: failed copying template files to destination: $newProjectPath" | Out-String
		$_ | Out-String
		exit 1
	}

	try {
		Get-ChildItem -LiteralPath $newProjectPath -Recurse |
			ForEach-Object -Process {
				$fileName = $_.Name
				$fileDir = $_.DirectoryName
				$fileAbsPath = $_.FullName
				
				# do string interpolation op only on "include list" files:
				# - (encodings: ascii, utf-8)
				# - playdate game metadata file: "pdxinfo"
				# - c files: "*.c", "*.h"
				# - plaintext/script/doc files: "*.txt", "*.cmake", "*.cmd", "*.ps1", "*.json", "*.md", "*.html", "*.css", "*.js"
				# TODO: maybe allow defining an "exclude list".
				if (!$_.PSIsContainer -and ($fileName -match '^pdxinfo$' -or $fileName -match '(\.txt|\.cmake|\.c|\.h|\.cmd|\.ps1|\.json|\.md|\.html|\.css|\.js)$')) {
					"-- found file on interpolation include list: $($_.Name)" | Out-String
					
					$tempFileName = "__TEMP__$($_.Name)"
					$tempFileAbsPath = "$fileDir/$tempFileName"
					
					New-Item -Path $fileDir -Name $tempFileName -ItemType File -ErrorAction Stop
					
					Get-Content -LiteralPath $fileAbsPath -ErrorAction Stop | ForEach-Object -Process {
						$lineInitial = "$_"
						$line = $lineInitial
						$limit = 10 # note: arbitrary max
						while (($line -match '({}{{(.+)}}{})') -and $templateParams.ContainsKey($Matches.2)) {
							if ($limit -le 0) {
								"-- warning: found more placeholders to replace than allowed so moving on, in line: $lineInitial"
								break;
							}
							"-- found placeholder to replace: $($Matches.1), line: $line" | Out-String
							$line = $line.Replace($Matches.1, $templateParams[$Matches.2])
							--$limit
						}
						
						Add-Content -LiteralPath $tempFileAbsPath -Value "$($line + [Environment]::NewLine)" -NoNewline -ErrorAction Stop  # note: explicit -Value alternatives: Windows: -Value "$($line + `r`n)", Unix: # -Value "$($line + `n)"
					}
					
					Remove-Item -LiteralPath $fileAbsPath -ErrorAction Stop
					Rename-Item -LiteralPath $tempFileAbsPath -NewName $fileName -ErrorAction Stop
				}
			}
	}
	catch {
		"-- error: failed template string interpolation" | Out-String
		$_ | Out-String
		exit 1
	}
}
else {
	"-- error: requested project directory already exists in projects/: $($templateParams.__KICKSTART_PLAYDATE_GAME_NAME__)" | Out-String
	exit 1
}
