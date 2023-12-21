echo Compiling shaders
@echo off

pushd ..
%VULKAN_SDK%/Bin/glslc.exe data/shaders/shader.vert -o data/shader_bytecode/vert.spv
%VULKAN_SDK%/Bin/glslc.exe data/shaders/shader.frag -o data/shader_bytecode/frag.spv
popd


