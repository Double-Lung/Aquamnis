project "MainExe"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"
    staticruntime "off"
	
	buildmessage "===Custom Build Step==="
	buildcommands 
	{
		"../scripts/compile_shaders.bat"
	}
	buildoutputs 
	{  
		"../../data/shader_bytecode"
	}
	
    files { "**.h", "**.cpp", "**.cxx", "**.hpp", "**.inl" }

    includedirs
    {
		"../core",
		"$(VULKAN_SDK)/Include",
	    "../extern/glm",
	    "../extern/stb",
  	    "../extern/tinyobjloader",
		"../extern/VMA"
    }

	libdirs
	{
		"$(VULKAN_SDK)/Lib"
	}

	links
	{
		"AMRenderCore",
		"glfw3",
		"sdl3",
		"vulkan-1",
	}

    targetdir ("../../bin/" .. OutputDir .. "/%{prj.name}")
    objdir ("../../bin/" .. OutputDir .. "/%{prj.name}/obj")

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
			"../extern/SDL3_Debug/include",
		}
	    libdirs
		{
			"../extern/GLFW_Debug/lib",
			"../extern/SDL3_Debug/lib",
		}
		postbuildcommands
		{
			"{COPY} ../code/extern/SDL3_Debug/bin/* %{cfg.targetdir}",
		}

    filter "configurations:Release"
        kind "WindowedApp"
		entrypoint "mainCRTStartup"
        defines { "NDEBUG" }
        runtime "Release"
        optimize "On"
        symbols "On"
		includedirs 
		{ 
			"../extern/GLFW/include",
			"../extern/SDL3/include",
		}
	    libdirs
		{
			"../extern/GLFW/lib",
			"../extern/SLD3/lib",
		}
		postbuildcommands
		{
			"{COPY} ../code/extern/SDL3/bin/* %{cfg.targetdir}",
		}
