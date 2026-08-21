project "shader_validator"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"
	optimize "on"
	targetdir ("%{wks.location}/engine/bin")
	objdir ("%{wks.location}/engine/intermediate")
	debugdir ("%{wks.location}")
	files { "**.h", "**.cpp" }
	includedirs {
		"%{wks.location}/engine/3rdparty/glfw/include",
		"%{wks.location}/engine/3rdparty/glad/include",
	}
	links {
		"glfw",
		"glad",
	}

	filter "system:windows"
		systemversion "latest"
		links { "opengl32.lib" }

	filter "system:macosx"
		links { "OpenGL.framework", "Cocoa.framework", "IOKit.framework", "CoreVideo.framework" }

	filter "configurations:Debug"
		symbols "on"

	filter "configurations:Release"
		optimize "on"

	filter "configurations:Hybrid"
		symbols "on"
		optimize "on"
