@echo off
python "%~dp0z1.py" compile %*
exit /b %ERRORLEVEL%
