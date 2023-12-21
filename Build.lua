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

group "Core"
	include "code/core/Build-Core.lua"
group ""

include "code/app/Build-App.lua"
