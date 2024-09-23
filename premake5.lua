workspace "z1engine"
    architecture "x64"
    startproject "editor"

    configurations
    {
        "Debug",
        "Release",
    }

    outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
    enginedir = path.getabsolute("%{prj.name}")

    group "deps"

        include "3rdparty/glfw"
        include "3rdparty/glad"
        include "3rdparty/imgui"

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

        files
        {
            "%{prj.name}/source/**.h",
            "%{prj.name}/source/**.cpp",
        }

        includedirs
        {
            "%{prj.name}/source",
            "3rdparty",
            "3rdparty/glfw/include",
            "3rdparty/glad/include",
            "3rdparty/imgui",
            "3rdparty/glm",
        }

        links
        {
            "glfw",
            "glad",
            "imgui",
            "opengl32.lib",
        }

        linkoptions { "/IGNORE:4006" }

        filter "system:windows"
            systemversion "latest"

            defines
            {
                "ENABLE_PROFILE",
                "PLATFORM_WINDOWS",
                "BUILD_DLL",
                "ENABLE_ASSERTS",
                "ENGINE_DIR=\"" .. enginedir .. "/\"",
                "glfw_INCLUDE_NONE",
            }

        filter "configurations:Debug"
            defines "DEBUG"
            runtime "Debug"
            symbols "on"

        filter "configurations:Release"
            defines "RELEASE"
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
            "3rdparty",
            "3rdparty/glfw/include",
            "3rdparty/glad/include",
            "3rdparty/imgui",
            "3rdparty/glm",
        }

        links
        {
            "runtime"
        }

        filter "system:windows"
            systemversion "latest"
            defines
            {
                "ENABLE_PROFILE",
                "PLATFORM_WINDOWS",
                "ENABLE_ASSERTS"
            }

        filter "configurations:Debug"
            defines "DEBUG"
            symbols "on"

        filter "configurations:Release"
            defines "RELEASE"
            optimize "on"
