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

	filter "system:macosx"
		-- strict baseline for engine code; unnamed parameters are a deliberate style pattern
		buildoptions { "-Wall", "-Wextra", "-Wno-unused-parameter" }

	-- opt-in unity build (see unity_blob_project in the root premake5.lua)
	filter { "system:macosx or system:windows", "options:unity" }
		unity_blob_project("engine/bakery", "bakery")
	filter {}

	-- the vendored stb implementation uses sprintf() (deprecated on macOS) and its aggregate
	-- initializers trip -Wmissing-field-initializers: scope diagnostics to the TU that builds it
	-- instead of editing engine/3rdparty sources
	filter { "system:macosx", "files:**/stb_build.cpp" }
		buildoptions { "-Wno-deprecated-declarations", "-Wno-missing-field-initializers" }

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
