project "glad"
	kind "StaticLib"
	language "C"
	staticruntime "on"

	targetdir ("%{wks.location}/engine/bin/" .. outputdir)
	objdir ("%{wks.location}/engine/intermediate")

	files
	{
		"include/glad/glad.h",
		"include/KHR/khrplatform.h",
		"src/glad.c",
	}

	includedirs
	{
		"include"
	}

	filter "system:windows"
		systemversion "latest"

	filter "system:macosx"

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"

	filter "configurations:Hybrid"
		runtime "Release"
		optimize "on"
		symbols "on"
