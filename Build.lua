-- premake5.lua
workspace("Aquamnis")
	architecture "x64"
	configurations({ "Debug", "Release" })
	startproject "MainExe"
	location "generated"
   
    -- Workspace-wide build options for MSVC
    filter "system:windows"
		buildoptions { "/EHsc", "/Zc:preprocessor", "/Zc:__cplusplus" }
	  
OutputDir = "%{cfg.buildcfg}_%{cfg.system}_%{cfg.architecture}/"

group "Data"
	project "Shaders"
		kind "None"
		files { "data/shaders/**.vert", "data/shaders/**.frag", "data/shaders/**.comp" }
	project "Scripts"
		kind "None"
		files { "scripts/**.bat" }
group ""

group "Core"
	include "code/core/Build-Core.lua"
group ""

group "Extern"
	include "code/extern/imgui/Build-Imgui.lua"
group ""

group "Exetutables"
	include "code/app/Build-App.lua"
group ""
