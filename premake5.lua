workspace "Game"
	architecture "x64"
	startproject "Game"

	configurations
	{
		"Debug",
		"Release",
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
debugdir "%{wks.location}"

IncludeDir = {}
IncludeDir["stb"] = "Game/vendor/stb"

project "Game"
	location "Game"
	kind "WindowedApp"
	language "C++"
	cppdialect "C++latest"
	staticruntime "on"

	warnings "Extra"
	disablewarnings {"4100"}
	flags { "FatalWarnings" }

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "pch.h"
	pchsource "Game/src/pch.cpp"

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
	}

	defines
	{
		"_CRT_SECURE_NO_WARNINGS"
	}

	includedirs
	{
		"%{prj.name}/src",
		"%{IncludeDir.stb}",
	}

	links 
	{ 
		"d3d11.lib", 
		"dxgi.lib", 
		"d3dcompiler.lib"
	}

	filter "system:windows"
		systemversion "latest"

	filter "configurations:Debug"
		defines "GAME_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "GAME_RELEASE"
		runtime "Release"
		optimize "on"
