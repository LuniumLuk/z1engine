project "bakery"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("%{wks.location}/build/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/build-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"source/**.h",
		"source/**.cpp",
	}

	includedirs
	{
		"source",
		"%{wks.location}/3rdparty",
		"%{wks.location}/3rdparty/stb",
		"%{wks.location}/3rdparty/glm",
		"%{wks.location}/3rdparty/lz4",
	}

	links
	{
		"LZ4"
	}

	filter "system:windows"
		systemversion "latest"

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"
