project "game"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("%{wks.location}/engine/bin/%{cfg.buildcfg}")
	objdir ("%{wks.location}/engine/intermediate")
	debugdir ("%{wks.location}")

	defines { "YAML_CPP_STATIC_DEFINE" }

	files
	{
		"source/**.h",
		"source/**.cpp"
	}

	includedirs
	{
		"%{wks.location}/engine/runtime/source",
		"%{wks.location}/engine/editor/source",
		"%{wks.location}/engine/bakery/source",
		"%{wks.location}/engine/3rdparty",
		"%{wks.location}/engine/3rdparty/glfw/include",
		"%{wks.location}/engine/3rdparty/glad/include",
		"%{wks.location}/engine/3rdparty/imgui",
		"%{wks.location}/engine/3rdparty/imguizmo",
		"%{wks.location}/engine/3rdparty/glm",
		"%{wks.location}/engine/3rdparty/entt",
		"%{wks.location}/engine/3rdparty/yaml-cpp/include",
		"%{wks.location}/engine/3rdparty/python314/include",
	}

	libdirs { "%{wks.location}/engine/3rdparty/python314/lib" }
	links
	{
		"runtime", "editor", "python314"
	}

	postbuildcommands {
		"{COPYFILE} \"%{wks.location}/engine/3rdparty/python314/python314.dll\" \"%{cfg.targetdir}\"",
		"{COPYFILE} \"%{wks.location}/engine/3rdparty/python314/python314.zip\" \"%{cfg.targetdir}\"",
	}

	filter "system:windows"
		systemversion "latest"
		defines
		{
			"PLATFORM_WINDOWS",
		}
	filter "configurations:Debug"
		defines
		{
			"DEBUG",
			"ENABLE_ASSERTS",
		}
		symbols "on"

	filter "configurations:Release"
		defines "RELEASE"
		optimize "on"