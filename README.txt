Search Agent (C++) - packaged files

Included:
- search_agent_cpp.exe   (built Release, static-linked deps recommended)
- run.bat                (launcher; checks/installs Ollama, pulls model)
- OllamaSetup.exe        (optional; official installer if included)

Usage (end user):
1) Run run.bat (desktop shortcut points here).
2) If Ollama not installed, the script will launch installer or open download page.
3) It will pull model (default phi4) then start the agent.

Env vars (optional):
- OFFLINE_MODE=0         # default=offline; set 0/false to enable web search
- OLLAMA_MODEL=phi4      # choose model
- OLLAMA_URL=http://localhost:11434/api/chat
