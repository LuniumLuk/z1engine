project "shader_validator"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"
	optimize "on"
	systemversion "latest"
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
		"opengl32.lib"
	}
