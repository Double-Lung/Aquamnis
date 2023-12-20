project("AMRenderCore")
   kind "StaticLib"
   language "C++"
   cppdialect "C++17"
   targetdir "binaries/%{cfg.buildcfg}"
   staticruntime "off"

   files { "**.h", "**.cpp", "**.cxx", "**.hpp", "**.inl" }
   
   targetdir ("../Binaries/" .. OutputDir .. "/%{prj.name}")
   objdir ("../Binaries/Intermediates/" .. OutputDir .. "/%{prj.name}")
   
   filter "system:windows"
      systemversion "latest"
      defines { }
   
   filter "configurations:Debug"
      defines { "DEBUG" }
	  runtime "Debug"
      symbols "On"
	  optimize "Off"

   filter "configurations:Release"
      defines { "NDEBUG" }
	  runtime "Release"
	  symbols "On"
      optimize "On"
	  