@echo off
pushd %~dp0\..
if exist "release" (
    echo Deleting existing release folder...
    rmdir /s /q "release"
)
mkdir "release"
mkdir "release\engine"
xcopy "engine\content" "release\engine\content" /E /I /H /Y
xcopy "engine\config" "release\engine\config" /E /I /H /Y
copy "engine\bin\Release\editor.exe" "release\editor.exe" /Y
copy "3rdparty\python314\python314.dll" "release\python314.dll" /Y
copy "3rdparty\python314\python314.zip" "release\python314.zip" /Y
popd