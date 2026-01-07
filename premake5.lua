workspace "game"
	architecture "x64"
	startproject "game"

	configurations
	{
		"Debug",
		"Release",
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
debugdir "%{wks.location}"

IncludeDir = {}
IncludeDir["stb"] = "game/vendor/stb"
IncludeDir["json"] = "game/vendor/json"
IncludeDir["tinygltf"] = "game/vendor/tinygltf"

project "game"
	location "game"
	kind "WindowedApp" --"ConsoleApp"
	language "C++"
	cppdialect "C++latest"
	staticruntime "on"

	warnings "Extra"
	-- 4100: unused funtion parameter
	disablewarnings {"4100"}
	flags { "FatalWarnings" }

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "pch.h"
	pchsource "game/src/pch.cpp"

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
		"%{IncludeDir.json}",
		"%{IncludeDir.tinygltf}",
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
