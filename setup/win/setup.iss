; Inno Setup script of StepIt Grader's installer, run by package-stepit-grader.sh.
; It installs the whole deploy folder: the editor, the command line, their DLLs
; and Qt's plugins.

#define MyAppName "StepIt Grader"
#define MyAppExeName "stepit-grader.exe"
#define MyAppPublisher "Giovanni Remigi"
#define MyAppURL "https://github.com/kineticsystem/stepit-grader"
#ifndef MyAppVersion
  #define MyAppVersion "0.0.1"
#endif

; The deploy folder made by package-stepit-grader.sh.
#define DeployDir "..\..\build\win-package\deploy-stepit-grader"
#define IconFile "..\..\src\editor-app\resources\stepit-grader.ico"

[Setup]
; Never change it: Windows knows the installed program by it, to update it.
AppId={{F62CC4AA-E645-48D9-89C6-F33D4E4A8119}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
VersionInfoVersion={#MyAppVersion}
OutputDir=..\..\build\win-package
OutputBaseFilename=StepItGrader-{#MyAppVersion}-setup
Compression=lzma2/max
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
SetupIconFile={#IconFile}
UninstallDisplayIcon={app}\stepit-grader.ico
WizardStyle=modern
LicenseFile=..\..\LICENSE

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[InstallDelete]
; The files of the version before, which this one may no longer have.
Type: filesandordirs; Name: "{app}\*.dll"
Type: filesandordirs; Name: "{app}\*.exe"
Type: filesandordirs; Name: "{app}\*.ico"
Type: filesandordirs; Name: "{app}\generic"
Type: filesandordirs; Name: "{app}\iconengines"
Type: filesandordirs; Name: "{app}\imageformats"
Type: filesandordirs; Name: "{app}\networkinformation"
Type: filesandordirs; Name: "{app}\platforms"
Type: filesandordirs; Name: "{app}\styles"
Type: filesandordirs; Name: "{app}\tls"

[Files]
Source: "{#DeployDir}\*.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#DeployDir}\*.dll"; DestDir: "{app}"; Flags: ignoreversion

; The application's icon.
Source: "{#IconFile}"; DestDir: "{app}"; Flags: ignoreversion

; Qt's plugins.
Source: "{#DeployDir}\generic\*"; DestDir: "{app}\generic"; Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist
Source: "{#DeployDir}\iconengines\*"; DestDir: "{app}\iconengines"; Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist
Source: "{#DeployDir}\imageformats\*"; DestDir: "{app}\imageformats"; Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist
Source: "{#DeployDir}\networkinformation\*"; DestDir: "{app}\networkinformation"; Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist
Source: "{#DeployDir}\platforms\*"; DestDir: "{app}\platforms"; Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist
Source: "{#DeployDir}\styles\*"; DestDir: "{app}\styles"; Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist
Source: "{#DeployDir}\tls\*"; DestDir: "{app}\tls"; Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"; IconFilename: "{app}\stepit-grader.ico"
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon; IconFilename: "{app}\stepit-grader.ico"

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent
