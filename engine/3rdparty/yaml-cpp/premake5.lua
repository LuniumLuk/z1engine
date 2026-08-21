project "yaml-cpp"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	-- Add this line to prevent DLL import/export attributes
	defines { "YAML_CPP_STATIC_DEFINE" } -- This is critical for static builds

	targetdir ("%{wks.location}/engine/bin/" .. outputdir)
	objdir ("%{wks.location}/engine/intermediate")

	files
	{
		"src/**.h",
		"src/**.cpp",
	}

	includedirs
	{
		"include",
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
