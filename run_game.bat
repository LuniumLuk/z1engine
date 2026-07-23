@echo off
pushd %~dp0
call "%~dp0\engine\bin\Debug\game.exe" "--game" "--scene=$engine/scene/demo_scene"
popd
