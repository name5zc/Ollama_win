# Search Agent (C++)

Minimal C++ version using libcurl + nlohmann_json. It calls a free search
tool (Wikipedia then DuckDuckGo) and sends context to a local Ollama model.

## Prereqs
- CMake ≥ 3.20, a C++17 compiler (MSVC/clang/gcc).
- vcpkg (recommended) with:
  ```powershell
  vcpkg install curl nlohmann-json
  ```
- Ollama installed and running (Windows installer from https://ollama.com).
  Pull a small model, e.g.:
  ```powershell
  ollama pull phi4
  ```

## Build (with vcpkg)
```powershell
cd F:\转化C++\agent\search_agent_cpp
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=C:\path\to\vcpkg.cmake
cmake --build build --config Release
```

## Run
```powershell
# 可选：切换模型或端口
$env:OLLAMA_MODEL="phi4"
$env:OLLAMA_URL="http://localhost:11434/api/chat"

.\build\Release\search_agent_cpp.exe
```
Type a question; it will fetch search results and ask the local model to
answer. `exit` to quit.

## Windows one-click-ish installer (offline by default)
- 准备 payload 目录，包含：`search_agent_cpp.exe`（Release）、`run.bat`、可选官方 `OllamaSetup.exe`、`README.txt`。
- 使用提供的 `installer.iss`（Inno Setup）打包：
  1) 安装 Inno Setup，打开 `installer.iss`。
  2) 确保 `payload` 路径正确（同级目录），勾选/不勾选 “附带 Ollama 安装器” 任务。
  3) Build 得到 `SearchAgentSetup.exe`。
- 用户安装后桌面有快捷方式；首次运行会检测 Ollama，若未安装则运行内置安装器或跳转官网，并自动 `ollama pull phi4` 后启动。

## Notes
- This version targets local Ollama only (no API key needed). If you want
  OpenAI/other endpoints, we can extend the code.
# Ollama_win
