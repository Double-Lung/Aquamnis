project("AMRenderCore")
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    targetdir "binaries/%{cfg.buildcfg}"
    staticruntime "off"

    files { "**.h", "**.cpp", "**.cxx", "**.hpp", "**.inl" }
   
    includedirs 
    { 
		"$(VULKAN_SDK)/Include",
	    "../extern/glm",
	    "../extern/stb",
  	    "../extern/tinyobjloader"
    }
   
    targetdir ("../Binaries/" .. OutputDir .. "/%{prj.name}")
    objdir ("../Binaries/Intermediates/" .. OutputDir .. "/%{prj.name}")
   
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
			"../extern/GLFW_Debug/include",
		}

	filter "configurations:Release"
		defines { "NDEBUG" }
		runtime "Release"
		symbols "On"
		optimize "On"
		includedirs 
		{ 
			"../extern/GLFW/include",
		}
	  