project("ImguiVkSDL3")
	kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    staticruntime "off"
	
	files { "**.h", "**.cpp", "**.cxx", "**.hpp", "**.inl" }
	
	includedirs 
    { 
		"$(VULKAN_SDK)/Include",
    }
	
	targetdir ("../../bin/" .. OutputDir .. "/%{prj.name}")
    objdir ("../../bin/" .. OutputDir .. "/%{prj.name}/obj")
	
	filter "system:windows"
		systemversion "latest"
		defines { }
   
	filter "configurations:Debug"
		defines { "_DEBUG" }
		runtime "Debug"
		symbols "On"
		optimize "Off"
		includedirs 
		{ 
			"../SDL3_Debug/include",
		}

	filter "configurations:Release"
		defines { "NDEBUG" }
		runtime "Release"
		symbols "On"
		optimize "On"
		includedirs 
		{ 
			"../SDL3/include",
		}