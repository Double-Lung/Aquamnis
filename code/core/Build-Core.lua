project("AMRenderCore")
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    staticruntime "off"

    files { "**.h", "**.cpp", "**.cxx", "**.hpp", "**.inl" }
    removefiles { "*_DEPRECATED/**" }
   
    includedirs 
    { 
		"$(VULKAN_SDK)/Include",
	    "../extern/glm",
	    "../extern/stb",
  	    "../extern/tinyobjloader",
		"../extern/VMA",
		"../extern/imgui",
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
			"../extern/SDL3_Debug/include",
		}

	filter "configurations:Release"
		defines { "NDEBUG" }
		runtime "Release"
		symbols "On"
		optimize "On"
		includedirs 
		{ 
			"../extern/SDL3/include",
		}
	  