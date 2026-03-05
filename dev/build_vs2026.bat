@echo off
pushd %~dp0\..
call "%~dp0\generate_vs2026.bat"
call "%~dp0\compile_vs2026.bat"
call "%~dp0\create_release.bat"
popd