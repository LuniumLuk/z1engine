project "importer"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("%{wks.location}/engine/bin")
	objdir ("%{wks.location}/engine/intermediate")
	debugdir ("%{wks.location}")

	files { "**.h", "**.cpp" }

	includedirs {
		"%{wks.location}/engine/runtime/source",
		"%{wks.location}/engine/3rdparty",
		"%{wks.location}/engine/3rdparty/glm",
		"%{wks.location}/engine/3rdparty/yaml-cpp/include",
		"%{wks.location}/engine/bakery/source",
		"%{wks.location}/engine/3rdparty/entt",
		"%{wks.location}/engine/3rdparty/python314/include"
	}

	libdirs { "%{wks.location}/engine/3rdparty/python314/lib" }

	defines { "YAML_CPP_STATIC_DEFINE" }

	links { "runtime", "python314" }

	filter "system:windows"
		postbuildcommands {
			"if not exist \"%{cfg.targetdir}/python314.dll\" {COPYFILE} \"%{wks.location}/engine/3rdparty/python314/python314.dll\" \"%{cfg.targetdir}\""
		}

	filter "system:windows"
		systemversion "latest"
		defines "PLATFORM_WINDOWS"

	filter "system:macosx"
		defines "PLATFORM_MACOS"

	filter "configurations:Debug"
		defines { "DEBUG", "ENABLE_ASSERTS" }
		symbols "on"

	filter "configurations:Release"
		defines "RELEASE"
		optimize "on"
