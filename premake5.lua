newoption {
	trigger     = "vs2026",
	description = "Use Visual Studio 2026 toolset"
}

newoption {
	trigger     = "probing",
	description = "Enable the frame prober (ENABLE_PROBING) in Hybrid builds"
}

newoption {
	trigger     = "full-symbols",
	description = "macOS: compile Hybrid/Profile with full -g instead of line-table debug info"
}

newoption {
	trigger     = "unity",
	description = "Build engine projects from generated unity blob translation units (faster cold builds)"
}

-- premake 5.0.0-beta8 only implements unity builds for VS actions, and the engine wants the same blob layout
-- on both supported platforms, so the blobs are generated here for macOS gmake and Windows MSBuild. Sources of
-- one project are split round-robin (sorted, so the output is deterministic) into groups of
-- UNITY_SOURCES_PER_BLOB and replace the individual files in the project.
-- `pch_name` (optional) is emitted verbatim as the blob's first include: MSVC's /Yu check (C1010) wants the
-- precompiled header textually first and spelled exactly like the /Yu argument, which including a source
-- file (or a path-qualified spelling) does not satisfy.
UNITY_SOURCES_PER_BLOB = 8
-- captured while the root script executes so the helper does not depend on the caller's script context
UNITY_WORKSPACE_ROOT = path.getabsolute(".")

-- relative path from the directory containing `blob` to `src` (both absolute); premake's path.getrelative
-- resolves against the script directory in some contexts, so this is computed explicitly
function unity_relative_include(blob, src)
	local a, b = {}, {}
	for seg in path.getdirectory(blob):gmatch("[^/\\]+") do table.insert(a, seg) end
	for seg in src:gmatch("[^/\\]+") do table.insert(b, seg) end
	local i = 1
	while a[i] and b[i] and a[i] == b[i] do i = i + 1 end
	local parts = {}
	for _ = i, #a do table.insert(parts, "..") end
	for j = i, #b do table.insert(parts, b[j]) end
	return table.concat(parts, "/")
end

function unity_blob_project(project_dir, project_name, pch_name)
	local root = UNITY_WORKSPACE_ROOT
	local sourceroot = path.join(root, project_dir, "source")
	local sources = {}
	for _, f in ipairs(os.matchfiles(path.join(sourceroot, "**.cpp"))) do
		local name = path.getname(f)
		-- vendor implementation units and the PCH source stay standalone (unity hostile)
		local is_vendor = name == "pch.cpp" or name:endswith("_build.cpp") or f:find("/3rdparty/", 1, true)
		if not is_vendor then
			table.insert(sources, f)
		end
	end
	table.sort(sources)

	local group_count = math.max(1, math.ceil(#sources / UNITY_SOURCES_PER_BLOB))
	local outdir = path.join(root, "engine", "intermediate", "unity", project_name)
	os.mkdir(outdir)
	local blobs = {}
	for i = 1, group_count do
		local members = {}
		for j = i, #sources, group_count do
			table.insert(members, sources[j])
		end
		if #members > 0 then
			local blob = path.join(outdir, string.format("%s_unity_%d.cpp", project_name, i))
			local fh = io.open(blob, "w")
			fh:write("// [AI-GENERATED] unity translation unit: regenerate with `dev/z1.py generate --unity`, do not edit\n")
			if pch_name then
				fh:write(string.format('#include "%s"\n', pch_name))
			end
			for _, src in ipairs(members) do
				fh:write(string.format('#include "%s"\n', unity_relative_include(blob, src)))
			end
			fh:close()
			table.insert(blobs, blob)
		end
	end

	removefiles(sources)
	files(blobs)
end

workspace "z1engine"
	architecture "x64"
	startproject "game"
	configurations { "Debug", "Release", "Profile", "Hybrid" }

	filter { "action:vs2022", "options:vs2026", "system:windows" }
		toolset "v145"

	filter {}

	-- Hybrid is the optimized build with asserts; the frame prober (CPU scopes + GPU timers) is opt-in
	-- and only enabled when projects are generated with --probing (see the option above)
	filter { "configurations:Hybrid", "options:probing" }
		defines { "ENABLE_PROBING" }

	-- unity blobs are the largest translation units in the tree; Debug emits enough sections to need /bigobj
	filter { "system:windows", "options:unity" }
		buildoptions { "/bigobj" }

	-- macOS: optimized configs use line tables by default (faster codegen, smaller objects, line-level
	-- breakpoints still work); `generate --full-symbols` restores full -g for variable inspection
	if not _OPTIONS["full-symbols"] then
		filter { "system:macosx", "configurations:Hybrid or Profile" }
			buildoptions { "-gline-tables-only" }
	end

	filter {}

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
		filter "configurations:Hybrid"
			defines { "NDEBUG", "DEBUG", "ENABLE_ASSERTS" }
			staticruntime "on"
			symbols "on"
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
		include "engine/tool/assetkit"
