project "lz4"
	kind "StaticLib"
	language "C"
	staticruntime "on"

	targetdir ("%{wks.location}/engine/bin/" .. outputdir)
	objdir ("%{wks.location}/engine/intermediate")

	files
	{
		"lz4.c",
		"lz4.h",
	}

	filter "system:windows"
		systemversion "latest"

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"
