workspace "Abyss"
	architecture "x64"
	startproject "Abyss"

	configurations
	{
		"Debug",
		"Release",
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

debugdir "%{wks.location}"

IncludeDir = {}
IncludeDir["SDL3"] = "Abyss/vendor/SDL3/include"
IncludeDir["stb"] = "Abyss/vendor/stb"

project "Abyss"
	location "Abyss"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++latest"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

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
		"%{IncludeDir.SDL3}",
		"%{IncludeDir.stb}",
	}

	links 
	{ 
		"Abyss/vendor/SDL3/Release/SDL3.lib",
		"d3d11.lib", 
		"dxgi.lib", 
		"d3dcompiler.lib"
	}

	filter "system:windows"
		systemversion "latest"

	filter "configurations:Debug"
		defines "VK_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "VK_RELEASE"
		runtime "Release"
		optimize "on"
