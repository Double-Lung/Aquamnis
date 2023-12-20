@echo off

pushd ..
premake5\premake5.exe --file=Build.lua vs2022
popd
pause