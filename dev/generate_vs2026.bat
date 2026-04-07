@echo off
python "%~dp0z1.py" generate %*
exit /b %ERRORLEVEL%
