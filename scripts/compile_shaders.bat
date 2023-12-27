echo Compiling shaders
@echo off

pushd ..
%VULKAN_SDK%/Bin/glslc.exe data/shaders/shader.vert -o data/shader_bytecode/vert.spv
%VULKAN_SDK%/Bin/glslc.exe data/shaders/shader.frag -o data/shader_bytecode/frag.spv
%VULKAN_SDK%/Bin/glslc.exe data/shaders/pointlight.vert -o data/shader_bytecode/pointlight_vert.spv
%VULKAN_SDK%/Bin/glslc.exe data/shaders/pointlight.frag -o data/shader_bytecode/pointlight_frag.spv
popd


