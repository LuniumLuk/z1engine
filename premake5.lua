newoption {
	trigger     = "vs2026",
	description = "Use Visual Studio 2026 toolset"
}

workspace "z1engine"
	architecture "x64"
	startproject "game"
	configurations { "Debug", "Release", "Profile" }

	filter { "action:vs2022", "options:vs2026", "system:windows" }
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
				"engine/bakery/source"
			}
			links { "runtime" }
			filter "system:windows"
				includedirs { "engine/3rdparty/python314/include" }
				libdirs { "engine/3rdparty/python314/lib" }
				links { "python314" }
				systemversion "latest"
				defines "PLATFORM_WINDOWS"
				postbuildcommands {
					"if not exist \"%{cfg.targetdir}/python314.dll\" {COPYFILE} \"%{wks.location}/engine/3rdparty/python314/python314.dll\" \"%{cfg.targetdir}\"",
					"if not exist \"%{cfg.targetdir}/python314.zip\" {COPYFILE} \"%{wks.location}/engine/3rdparty/python314/python314.zip\" \"%{cfg.targetdir}\""
				}
			filter "system:macosx"
				includedirs { path.join(os.outputof("python3 -c \"import sysconfig; print(sysconfig.get_config_var('INCLUDEPY'))\""), "") }
				libdirs { path.join(os.outputof("python3 -c \"import sysconfig; print(sysconfig.get_config_var('LIBDIR'))\""), "") }
				linkoptions { "-lpython3.14" }
				defines "PLATFORM_MACOS"
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
		include "engine/3rdparty/nfd"
		include "engine/bakery"

	group "test"

		local testfiles = os.matchfiles("engine/test/**.cpp")
		for _, filepath in ipairs(testfiles) do
			local testname = path.getbasename(filepath)
			create_test(testname, filepath)
		end

	group "engine"

		include "engine/runtime"
		include "engine/editor"

	group "executable"

		include "engine/game"
		include "engine/tool/shader_validator"
		include "engine/tool/importer"
