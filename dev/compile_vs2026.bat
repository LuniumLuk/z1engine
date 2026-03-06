@echo off
pushd %~dp0\..
set VS_PATH="C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\devenv.com"
@REM call %VS_PATH% z1engine.sln /Build "Release|x64"
call %VS_PATH% z1engine.sln /Build "Debug|x64"
popd