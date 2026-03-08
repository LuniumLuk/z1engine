project "bakery"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("%{wks.location}/engine/bin/" .. outputdir)
	objdir ("%{wks.location}/engine/intermediate")

	files
	{
		"source/**.h",
		"source/**.cpp",
	}

	includedirs
	{
		"source",
		"%{wks.location}/engine/3rdparty",
		"%{wks.location}/engine/3rdparty/stb",
		"%{wks.location}/engine/3rdparty/glm",
		"%{wks.location}/engine/3rdparty/lz4",
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
