@echo off
pushd %~dp0
set SCENE=demo_scene
if not "%~1"=="" set SCENE=%~1
call "%~dp0\engine\bin\Hybrid\game.exe" "--game" "--scene=$engine/scene/%SCENE%"
popd
