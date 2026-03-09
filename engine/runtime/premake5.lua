project "runtime"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("%{wks.location}/engine/bin/%{cfg.buildcfg}")
	objdir ("%{wks.location}/engine/intermediate")

	pchheader "pch.h"
	pchsource "source/pch.cpp"

	defines { "YAML_CPP_STATIC_DEFINE" }

	files
	{
		"source/**.h",
		"source/**.cpp",
	}

	includedirs
	{
		"%{wks.location}/engine/runtime/source",
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
		"glfw",
		"glad",
		"imgui",
		"imguizmo",
		"bakery",
		"yaml-cpp",
		"opengl32.lib",
		"python314",
	}

	linkoptions { "/IGNORE:4006" }

	filter "system:windows"
		systemversion "latest"

		defines
		{
			"PLATFORM_WINDOWS",
			"BUILD_DLL",
			"ENGINE_DIR=\"" .. path.getabsolute("%{prj.name}") .. "/\"",
			"glfw_INCLUDE_NONE",
		}

	filter "configurations:Debug"
		defines
		{
			"DEBUG",
			"ENABLE_ASSERTS",
		}
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "RELEASE"
		runtime "Release"
		optimize "on"

	filter "configurations:Profile"
		defines
		{
			"ENABLE_PROFILE",
			"RELEASE",
		}
		runtime "Release"
		optimize "on"