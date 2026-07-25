project "imguizmo"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("%{wks.location}/engine/bin/" .. outputdir)
	objdir ("%{wks.location}/engine/intermediate")

	files
	{
		"**.h",
		"**.cpp",
	}

	includedirs
	{
		"%{wks.location}/engine/3rdparty/imgui",
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
