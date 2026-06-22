@echo off
cd /d "%~dp0"
start "" http://localhost:8099
python -m http.server 8099
