@echo off
python "%~dp0dev\z1.py" validate-shaders %*
exit /b %ERRORLEVEL%
