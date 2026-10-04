workspace "Macaw"
    location ""
    configurations { "Debug", "Release", "Viewer" }
    platforms { "x64" }
    defaultplatform "x64"
    startproject "Macaw"

filter "platforms:x64"
    architecture "x86_64"

filter "system:windows"
    systemversion "latest"

filter "configurations:Debug"
    defines { "_DEBUG" }
    symbols "On"
    editandcontinue "Off"
    runtime "Debug"

filter "configurations:Release"
    defines { "NDEBUG" }
    optimize "Speed"
    runtime "Release"

filter "configurations:Viewer"
    defines { "NDEBUG", "OBJ_VIEWER" }
    optimize "Speed"
    runtime "Release"

filter {}

local Modules = json.decode(io.readfile("ModuleDependencies.json"))

local function AddPublicIncludes(ModuleName, Visited)
    if Visited[ModuleName] then
        return
    end
    Visited[ModuleName] = true
    includedirs { ModuleName .. "/Public" }
    for _, Dependency in ipairs(Modules[ModuleName].Public) do
        AddPublicIncludes(Dependency, Visited)
    end
end

local function ConfigureProject(ProjectName, ProjectKind)
    project(ProjectName)
        kind(ProjectKind)
        language "C++"
        cppdialect "C++20"
        characterset "Unicode"
        staticruntime "Off"
        toolset "msc-v145"
        targetdir "bin/%{cfg.buildcfg}/%{cfg.platform}"
        objdir "bin-int/%{prj.name}/%{cfg.buildcfg}/%{cfg.platform}"
        externalincludedirs { "Externals/Include" }
        defines { "NOMINMAX" }
        buildoptions { "/utf-8" }
end

local function ConfigureModule(ModuleName)
    ConfigureProject(ModuleName, "StaticLib")
    files { ModuleName .. "/Public/**.h", ModuleName .. "/Public/**.inl", ModuleName .. "/Private/**.h", ModuleName .. "/Private/**.cpp", ModuleName .. "/Private/**.cc" }
    includedirs { ModuleName .. "/Private" }
    local Visited = {}
    AddPublicIncludes(ModuleName, Visited)
    for _, Dependency in ipairs(Modules[ModuleName].Private) do
        AddPublicIncludes(Dependency, Visited)
    end
    links(Modules[ModuleName].Public)
    links(Modules[ModuleName].Private)
    if ModuleName == "ImGui" then
        includedirs { "ImGui/Public/ImGui" }
        enablepch "Off"
    else
        pchheader "pch.h"
        pchsource(ModuleName .. "/Private/pch.cpp")
    end
end

local function ConfigureSystemLinks()
    links { "DirectXTex", "nvapi64", "d3d11", "dxgi", "d3dcompiler", "dwmapi", "gdi32", "imm32", "shell32", "user32", "kernel32", "ole32" }
    filter "configurations:Debug"
        libdirs { "Externals/bin/debug" }
    filter "configurations:Release or Viewer"
        libdirs { "Externals/bin/release" }
    filter {}
end

for _, ModuleName in ipairs({ "Core", "CoreUObject", "RenderCore", "Serialization", "ImGui", "Asset", "World", "Render", "Editor" }) do
    ConfigureModule(ModuleName)
end

project "Core"
    filter "files:Core/Private/Spatial/FBVH8AVX.cpp"
        enablepch "Off"
        vectorextensions "AVX"
    filter "files:Core/Private/Math/**.cpp"
        enablepch "Off"
    filter {}

ConfigureProject("Macaw", "WindowedApp")
    files { "Application/**.h", "Application/**.cpp", "Macaw.cpp", "Macaw.h", "framework.h", "targetver.h", "Resource.h", "Macaw.rc", "pch.h", "pch.cpp" }
    includedirs { "." }
    AddPublicIncludes("Editor", {})
    links { "Editor", "World", "Render", "Asset", "Serialization", "RenderCore", "CoreUObject", "Core", "ImGui" }
    pchheader "pch.h"
    pchsource "pch.cpp"
    filter "configurations:Viewer"
        removefiles { "Application/FEditorApplication.cpp" }
    filter "configurations:Debug or Release"
        removefiles { "Application/FViewApplication.cpp" }
    filter {}
    ConfigureSystemLinks()

local function ConfigureValidation(ProjectName, ModuleName, Source)
    ConfigureProject(ProjectName, "ConsoleApp")
    files { Source }
    AddPublicIncludes(ModuleName, {})
    links { ModuleName }
    enablepch "Off"
    ConfigureSystemLinks()
end

ConfigureValidation("CoreBoundaryTests", "Core", "Validation/CoreBoundaryTests.cpp")
ConfigureValidation("RuntimeBoundaryTests", "World", "Validation/RuntimeBoundaryTests.cpp")
ConfigureValidation("EditorBoundaryTests", "Editor", "Validation/EditorBoundaryTests.cpp")
