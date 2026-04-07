@echo off
python "%~dp0z1.py" release %*
exit /b %ERRORLEVEL%
