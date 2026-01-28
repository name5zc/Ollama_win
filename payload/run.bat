@echo off
setlocal

REM Optional: offline mode (no web search)
set "OFFLINE_MODE=1"
REM Choose model to pull/run
if "%OLLAMA_MODEL%"=="" set "OLLAMA_MODEL=phi4"
REM Ollama endpoint
if "%OLLAMA_URL%"=="" set "OLLAMA_URL=http://localhost:11434/api/chat"

echo === Search Agent Launcher ===
echo Model: %OLLAMA_MODEL%
echo Endpoint: %OLLAMA_URL%

REM Check Ollama service
curl --silent --fail http://localhost:11434/api/tags >nul 2>&1
if errorlevel 1 (
  echo Ollama not detected. Installing or opening download page...
  if exist "%~dp0OllamaSetup.exe" (
    "%~dp0OllamaSetup.exe"
  ) else (
    start https://ollama.com/download
    echo Please install Ollama, then re-run this shortcut.
  )
  pause
  exit /b 1
)

echo Pulling model (if not present): %OLLAMA_MODEL%
ollama pull %OLLAMA_MODEL%

echo Starting agent...
"%~dp0search_agent_cpp.exe"

endlocal
