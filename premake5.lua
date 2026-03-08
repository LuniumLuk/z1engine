newoption {
	trigger     = "vs2026",
	description = "Use Visual Studio 2026 toolset"
}

workspace "z1engine"
	architecture "x64"
	startproject "editor"
	configurations { "Debug", "Release", "Profile" }

	filter { "action:vs2022", "options:vs2026" }
		toolset "v145"

	filter {}

	configurations
	{
		"Debug",
		"Release",
	}

	-- outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
	outputdir = "%{cfg.buildcfg}" -- Only windows x86_64 is supported for now
	enginedir = path.getabsolute("%{prj.name}")

	function create_test(testname, filepath)
		project(testname)
			location "%{wks.location}/engine/intermediate/test"
			kind "ConsoleApp"
			language "C++"
			cppdialect "C++17"
			staticruntime "on"
			targetdir ("%{wks.location}/engine/bin/test/" .. outputdir)
			objdir ("%{wks.location}/engine/intermediate/test")
			debugdir ("%{wks.location}")
			files { filepath }
			defines { "YAML_CPP_STATIC_DEFINE" }
			includedirs {
				"engine/runtime/source",
				"engine/3rdparty",
				"engine/3rdparty/glfw/include",
				"engine/3rdparty/glad/include",
				"engine/3rdparty/imgui",
				"engine/3rdparty/glm",
				"engine/3rdparty/entt",
				"engine/3rdparty/yaml-cpp/include",
				"engine/3rdparty/python314/include",
				"engine/bakery/source"
			}
			libdirs { "engine/3rdparty/python314/lib" }
			links { "runtime", "python314" }
			postbuildcommands {
				"{COPYFILE} \"%{wks.location}/engine/3rdparty/python314/python314.dll\" \"%{cfg.targetdir}\""
			}
			filter "system:windows"
				systemversion "latest"
				defines "PLATFORM_WINDOWS"
			filter "configurations:Debug"
				defines { "DEBUG", "ENABLE_ASSERTS" }
				staticruntime "on"
				symbols "on"
			filter "configurations:Release"
				defines "RELEASE"
				staticruntime "on"
				optimize "on"
	end

	group "dependency"

		include "engine/3rdparty/glfw"
		include "engine/3rdparty/glad"
		include "engine/3rdparty/imgui"
		include "engine/3rdparty/lz4"
		include "engine/3rdparty/yaml-cpp"
		include "engine/3rdparty/imguizmo"
		include "engine/bakery"

	group "test"

		local testfiles = os.matchfiles("engine/test/**.cpp")
		for _, filepath in ipairs(testfiles) do
			local testname = path.getbasename(filepath)
			create_test(testname, filepath)
		end

	group ""

	project "runtime"
		location "engine/runtime"
		kind "StaticLib"
		language "C++"
		cppdialect "C++17"
		staticruntime "on"

		targetdir ("%{wks.location}/engine/bin/" .. outputdir)
		objdir ("%{wks.location}/engine/intermediate")

		pchheader "pch.h"
		pchsource "engine/runtime/source/pch.cpp"

		defines { "YAML_CPP_STATIC_DEFINE" }

		files
		{
			"engine/runtime/source/**.h",
			"engine/runtime/source/**.cpp",
		}

		includedirs
		{
			"engine/runtime/source",
			"engine/bakery/source",
			"engine/3rdparty",
			"engine/3rdparty/glfw/include",
			"engine/3rdparty/glad/include",
			"engine/3rdparty/imgui",
			"engine/3rdparty/imguizmo",
			"engine/3rdparty/glm",
			"engine/3rdparty/entt",
			"engine/3rdparty/yaml-cpp/include",
			"engine/3rdparty/python314/include",
		}

		libdirs { "engine/3rdparty/python314/lib" }
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
				"ENGINE_DIR=\"" .. enginedir .. "/\"",
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

	project "editor"
		location "engine/editor"
		kind "ConsoleApp"
		language "C++"
		cppdialect "C++17"
		staticruntime "on"

		targetdir ("%{wks.location}/engine/bin/" .. outputdir)
		objdir ("%{wks.location}/engine/intermediate")
		debugdir ("%{wks.location}")

		defines { "YAML_CPP_STATIC_DEFINE" }

		files
		{
			"engine/editor/source/**.h",
			"engine/editor/source/**.cpp"
		}

		includedirs
		{
			"engine/runtime/source",
			"engine/editor/source",
			"engine/bakery/source",
			"engine/3rdparty",
			"engine/3rdparty/glfw/include",
			"engine/3rdparty/glad/include",
			"engine/3rdparty/imgui",
			"engine/3rdparty/imguizmo",
			"engine/3rdparty/glm",
			"engine/3rdparty/entt",
			"engine/3rdparty/yaml-cpp/include",
			"engine/3rdparty/python314/include",
		}

		libdirs { "engine/3rdparty/python314/lib" }
		links
		{
			"runtime", "python314"
		}

		postbuildcommands {
			"{COPYFILE} \"%{wks.location}engine/3rdparty/python314/python314.dll\" \"%{cfg.targetdir}\"",
			"{COPYFILE} \"%{wks.location}engine/3rdparty/python314/python314.zip\" \"%{cfg.targetdir}\"",
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

		filter "configurations:Profile"
			defines
			{
				"ENABLE_PROFILE",
				"RELEASE"
			}
			optimize "on"