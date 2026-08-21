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
		"%{wks.location}/engine/3rdparty/nfd/src/include",
	}

	filter "system:windows"
		includedirs { "%{wks.location}/engine/3rdparty/python314/include" }
	filter "system:macosx"
		includedirs { path.join(os.outputof("python3 -c \"import sysconfig; print(sysconfig.get_config_var('INCLUDEPY'))\""), "") }

	filter "system:windows"
		includedirs { "%{wks.location}/engine/3rdparty/physx/include" }
		defines { "PX_PHYSX_STATIC_LIB" }
	filter {}

	libdirs
	{
	}

	filter "system:windows"
		libdirs { "%{wks.location}/engine/3rdparty/python314/lib" }
	filter "system:macosx"
		libdirs { path.join(os.outputof("python3 -c \"import sysconfig; print(sysconfig.get_config_var('LIBDIR'))\""), "") }

	filter { "system:windows", "configurations:Debug" }
		libdirs { "%{wks.location}/engine/3rdparty/physx/lib/Debug" }
	filter { "system:windows", "configurations:Release or Hybrid" }
		libdirs { "%{wks.location}/engine/3rdparty/physx/lib/Release" }
	filter {}

	links
	{
		"glfw",
		"glad",
		"imgui",
		"imguizmo",
		"bakery",
		"yaml-cpp",
		"nfd",
	}

	filter "system:windows"
		links { "python314" }
	filter "system:macosx"
		linkoptions { "-lpython3.14" }

	filter "system:windows"
		links
		{
			"opengl32.lib",
			"PhysX_static_64",
			"PhysXCommon_static_64",
			"PhysXCooking_static_64",
			"PhysXExtensions_static_64",
			"PhysXFoundation_static_64",
			"PhysXPvdSDK_static_64",
		}
		linkoptions { "/IGNORE:4006" }
		buildoptions { "/bigobj" }
	filter "system:macosx"
		links
		{
			"OpenGL.framework",
			"Cocoa.framework",
			"IOKit.framework",
			"CoreVideo.framework",
		}
	filter {}

	filter "system:windows"
		systemversion "latest"

		defines
		{
			"PLATFORM_WINDOWS",
			"BUILD_DLL",
			"ENGINE_DIR=\"" .. path.getabsolute("%{prj.name}") .. "/\"",
			"glfw_INCLUDE_NONE",
		}

	filter "system:macosx"
		defines
		{
			"PLATFORM_MACOS",
			"ENGINE_DIR=\"" .. path.getabsolute("%{prj.name}") .. "/\"",
			"glfw_INCLUDE_NONE",
		}

	filter "configurations:Debug"
		defines
		{
			"_DEBUG",
			"DEBUG",
			"ENABLE_ASSERTS",
		}
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines
		{
			"NDEBUG",
			"RELEASE",
		}
		runtime "Release"
		optimize "on"

	filter "configurations:Profile"
		defines
		{
			"NDEBUG",
			"ENABLE_PROFILE",
			"RELEASE",
		}
		runtime "Release"
		optimize "on"

	filter "configurations:Hybrid"
		defines
		{
			"NDEBUG",
			"DEBUG",
			"ENABLE_ASSERTS",
		}
		runtime "Release"
		optimize "on"
		symbols "on"