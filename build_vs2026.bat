set VS_PATH="C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\devenv.com"
call utils\premake\premake5.exe vs2022
call %VS_PATH% z1engine.sln /Build "Release|x64"
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
