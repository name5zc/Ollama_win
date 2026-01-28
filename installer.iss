[Setup]
AppName=SearchAgent
AppVersion=1.0.0
DefaultDirName={pf}\SearchAgent
DisableDirPage=yes
OutputBaseFilename=SearchAgentSetup
Compression=lzma
SolidCompression=yes

[Languages]
Name: "chinese"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "include_ollama"; Description: "附带 Ollama 安装器"; Flags: unchecked

[Files]
Source: "payload\search_agent_cpp.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "payload\run.bat"; DestDir: "{app}"; Flags: ignoreversion
Source: "payload\OllamaSetup.exe"; DestDir: "{app}"; Flags: ignoreversion; Tasks: include_ollama
Source: "payload\README.txt"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{commondesktop}\SearchAgent"; Filename: "{app}\run.bat"

[Run]
Filename: "{app}\run.bat"; Description: "启动 SearchAgent"; Flags: nowait postinstall skipifsilent
