project "nfd"
	kind "StaticLib"
	staticruntime "on"

	targetdir ("%{wks.location}/engine/bin/" .. outputdir)
	objdir ("%{wks.location}/engine/intermediate")

	includedirs
	{
		"src/include"
	}

	filter "system:macosx"
		language "C++"
		files
		{
			"src/include/nfd.h",
			"src/nfd_cocoa.m"
		}
		buildoptions { "-x", "objective-c++" }
		links
		{
			"Cocoa"
		}

	filter "system:windows"
		language "C++"
		files
		{
			"src/include/nfd.h",
			"src/nfd_win.cpp"
		}

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"
