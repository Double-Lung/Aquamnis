echo Compiling shaders
::@echo off

pushd ..
for /f %%X in ('dir /b data\shaders\*.*') do (
	%VULKAN_SDK%\Bin\glslc.exe data\shaders\%%X -o data\shader_bytecode\%%X.spv
	echo %%X
)
popd


