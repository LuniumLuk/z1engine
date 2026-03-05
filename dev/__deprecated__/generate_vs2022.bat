@echo off
pushd %~dp0\..
call utils\premake\premake5.exe vs2022
popd