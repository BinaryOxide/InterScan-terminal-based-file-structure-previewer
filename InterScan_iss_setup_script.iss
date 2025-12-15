; -----------------------------------------------------------
;  InterScan Installer (by BinaryOxide)
;  Installs InterScan.exe and adds it to system PATH (safe)
; -----------------------------------------------------------

#define MyAppName "InterScan"
#define MyAppVersion "1.0"
#define MyAppPublisher "BinaryOxide"
#define MyAppURL "https://github.com/BinaryOxide/InterScan-terminal-based-file-structure-previewer"
#define MyAppExeName "InterScan.exe"

[Setup]
AppId={{D16D13F8-F0B4-4512-B280-FDE325C86796}}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}

DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
UninstallDisplayIcon={app}\{#MyAppExeName}

OutputBaseFilename=InterScan-Setup
SolidCompression=yes
Compression=lzma
WizardStyle=modern

PrivilegesRequired=admin
ChangesEnvironment=yes

ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "H:\programming\production\InterScan\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#MyAppName}}"; Flags: nowait postinstall skipifsilent

[Code]

function NeedsAddPath(): Boolean;
var
  Paths: string;
begin
  Result := True;
  if RegQueryStringValue(
      HKLM,
      'SYSTEM\CurrentControlSet\Control\Session Manager\Environment',
      'Path',
      Paths) then
  begin
    Result := Pos(Lowercase(ExpandConstant('{app}')), Lowercase(Paths)) = 0;
  end;
end;

procedure AddAppPathToSystemPath();
var
  Paths, NewPaths, AppPath: string;
begin
  AppPath := ExpandConstant('{app}');
  if RegQueryStringValue(
      HKLM,
      'SYSTEM\CurrentControlSet\Control\Session Manager\Environment',
      'Path',
      Paths) then
  begin
    if Pos(Lowercase(AppPath), Lowercase(Paths)) = 0 then
    begin
      NewPaths := Paths;
      if (Length(NewPaths) > 0) and (NewPaths[Length(NewPaths)] <> ';') then
        NewPaths := NewPaths + ';';
      NewPaths := NewPaths + AppPath;
      RegWriteStringValue(
        HKLM,
        'SYSTEM\CurrentControlSet\Control\Session Manager\Environment',
        'Path',
        NewPaths);
    end;
  end
  else
  begin
    RegWriteStringValue(
      HKLM,
      'SYSTEM\CurrentControlSet\Control\Session Manager\Environment',
      'Path',
      AppPath);
  end;
end;

procedure RemoveAppPathFromSystemPath();
var
  Paths, NewPaths, AppPath: string;
begin
  AppPath := ExpandConstant('{app}');
  if not RegQueryStringValue(
      HKLM,
      'SYSTEM\CurrentControlSet\Control\Session Manager\Environment',
      'Path',
      Paths) then
    Exit;

  NewPaths := Paths;

  { Remove all occurrences of the app path }
  StringChangeEx(NewPaths, ';' + AppPath, '', True);
  StringChangeEx(NewPaths, AppPath + ';', '', True);
  StringChangeEx(NewPaths, AppPath, '', True);

  { Clean up extra semicolons }
  while Pos(';;', NewPaths) > 0 do
    StringChangeEx(NewPaths, ';;', ';', True);

  { Trim leading semicolon }
  if (Length(NewPaths) > 0) and (NewPaths[1] = ';') then
    Delete(NewPaths, 1, 1);

  { Trim trailing semicolon }
  if (Length(NewPaths) > 0) and (NewPaths[Length(NewPaths)] = ';') then
    Delete(NewPaths, Length(NewPaths), 1);

  if NewPaths <> Paths then
    RegWriteStringValue(
      HKLM,
      'SYSTEM\CurrentControlSet\Control\Session Manager\Environment',
      'Path',
      NewPaths);
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usUninstall then
    RemoveAppPathFromSystemPath();
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
  begin
    if NeedsAddPath() then
      AddAppPathToSystemPath();

    MsgBox(
      'InterScan installed successfully.'#13#13 +
      'Open a NEW command prompt or PowerShell to use it.'#13#13 +
      'Then type: "interscan"', mbInformation, MB_OK);
  end;
end;

