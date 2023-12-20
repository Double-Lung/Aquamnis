project "MainExe"
   kind "ConsoleApp"
   language "C++"
   cppdialect "C++17"
   targetdir "binaries/%{cfg.buildcfg}"
   staticruntime "off"

   files { "**.h", "**.cpp", "**.cxx", "**.hpp", "**.inl" }

   includedirs
   {
	  -- Include Core
	  "../core"
   }

   links
   {
      "AMRenderCore"
   }

   targetdir ("../Binaries/" .. OutputDir .. "/%{prj.name}")
   objdir ("../Binaries/Intermediates/" .. OutputDir .. "/%{prj.name}")

   filter "system:windows"
       systemversion "latest"
       defines { "WINDOWS" }

   filter "configurations:Debug"
       defines { "DEBUG" }
       runtime "Debug"
	   optimize "Off"
       symbols "On"

   filter "configurations:Release"
       kind "WindowedApp"
       defines { "NDEBUG" }
       runtime "Release"
       optimize "On"
       symbols "On"
