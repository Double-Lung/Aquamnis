project "MainExe"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"
    targetdir "binaries/%{cfg.buildcfg}"
    staticruntime "off"

    files { "**.h", "**.cpp", "**.cxx", "**.hpp", "**.inl" }

    includedirs
    {
		"../core",
		"$(VULKAN_SDK)/Include",
	    "../extern/glm",
	    "../extern/stb",
  	    "../extern/tinyobjloader"
    }

	libdirs
	{
		"$(VULKAN_SDK)/Lib"
	}

	links
	{
		"AMRenderCore",
		"glfw3",
		"vulkan-1"
	}

    targetdir ("../Binaries/" .. OutputDir .. "/%{prj.name}")
    objdir ("../Binaries/Intermediates/" .. OutputDir .. "/%{prj.name}")

    filter "system:windows"
        systemversion "latest"
        defines { "WINDOWS" }

    filter "configurations:Debug"
        defines { "_DEBUG" }
        runtime "Debug"
	    optimize "Off"
        symbols "On"
		includedirs 
		{ 
			"../extern/GLFW_Debug/include",
		}
	    libdirs
		{
			"../extern/GLFW_Debug/lib",
		}

    filter "configurations:Release"
        kind "WindowedApp"
        defines { "NDEBUG" }
        runtime "Release"
        optimize "On"
        symbols "On"
		includedirs 
		{ 
			"../extern/GLFW/include",
		}
	    libdirs
		{
			"../extern/GLFW/lib",
		}
