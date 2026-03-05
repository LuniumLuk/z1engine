@echo off
pushd %~dp0\..
if exist "release" (
    echo Deleting existing release folder...
    rmdir /s /q "release"
)
mkdir "release"
mkdir "release\editor"
xcopy "engine" "release\engine" /E /I /H /Y
copy "build\Release-windows-x86_64\editor\editor.exe" "release\editor.exe" /Y
copy "editor\default.ini" "release\editor\default.ini" /Y
copy "3rdparty\python314\python314.dll" "release\python314.dll" /Y
copy "3rdparty\python314\python314.zip" "release\python314.zip" /Y
popd