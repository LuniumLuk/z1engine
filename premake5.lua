workspace "z1engine"
	architecture "x64"
	startproject "editor"
	configurations { "Debug", "Release", "Profile" }

	configurations
	{
		"Debug",
		"Release",
	}

	outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
	enginedir = path.getabsolute("%{prj.name}")

	function create_test(testname, filepath)
		project(testname)
			location "test/workspace"
			kind "ConsoleApp"
			language "C++"
			cppdialect "C++17"
			targetdir ("%{wks.location}/build/" .. outputdir .. "/%{prj.name}")
			objdir ("%{wks.location}/build-int/" .. outputdir .. "/%{prj.name}")
			files { filepath }
			defines { "YAML_CPP_STATIC_DEFINE" }
			includedirs {
				"runtime/source",
				"3rdparty",
				"3rdparty/glfw/include",
				"3rdparty/glad/include",
				"3rdparty/imgui",
				"3rdparty/glm",
				"3rdparty/entt",
				"3rdparty/yaml-cpp/include",
				"bakery/source"
			}
			links { "runtime" }
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

		include "3rdparty/glfw"
		include "3rdparty/glad"
		include "3rdparty/imgui"
		include "3rdparty/lz4"
		include "3rdparty/yaml-cpp"
		include "bakery"

	group "test"

		local testfiles = os.matchfiles("test/**.cpp")
		for _, filepath in ipairs(testfiles) do
			local testname = path.getbasename(filepath)
			create_test(testname, filepath)
		end

	group ""

	project "runtime"
		location "runtime"
		kind "StaticLib"
		language "C++"
		cppdialect "C++17"
		staticruntime "on"

		targetdir ("%{wks.location}/build/" .. outputdir .. "/%{prj.name}")
		objdir ("%{wks.location}/build-int/" .. outputdir .. "/%{prj.name}")

		pchheader "pch.h"
		pchsource "runtime/source/pch.cpp"

		defines { "YAML_CPP_STATIC_DEFINE" }

		files
		{
			"%{prj.name}/source/**.h",
			"%{prj.name}/source/**.cpp",
		}

		includedirs
		{
			"%{prj.name}/source",
			"bakery/source",
			"3rdparty",
			"3rdparty/glfw/include",
			"3rdparty/glad/include",
			"3rdparty/imgui",
			"3rdparty/glm",
			"3rdparty/entt",
			"3rdparty/yaml-cpp/include",
		}

		links
		{
			"glfw",
			"glad",
			"imgui",
			"bakery",
			"yaml-cpp",
			"opengl32.lib",
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
		location "editor"
		kind "ConsoleApp"
		language "C++"
		cppdialect "C++17"
		staticruntime "on"

		targetdir ("%{wks.location}/build/" .. outputdir .. "/%{prj.name}")
		objdir ("%{wks.location}/build-int/" .. outputdir .. "/%{prj.name}")

		files
		{
			"%{prj.name}/source/**.h",
			"%{prj.name}/source/**.cpp"
		}

		includedirs
		{
			"runtime/source",
			"editor/source",
			"bakery/source",
			"3rdparty",
			"3rdparty/glfw/include",
			"3rdparty/glad/include",
			"3rdparty/imgui",
			"3rdparty/glm",
			"3rdparty/entt",
		}

		links
		{
			"runtime"
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