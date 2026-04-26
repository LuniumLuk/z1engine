@echo off
pushd %~dp0\..
"C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" z1engine.sln /p:Configuration=Debug /p:Platform=x64 /m
echo BUILD_EXIT_CODE=%ERRORLEVEL%
popd
