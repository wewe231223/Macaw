workspace "Macaw"
    location "."
    configurations { "Debug", "Release" }
    platforms { "x64" }
    defaultplatform "x64"
    startproject "Macaw"
    system "windows"
    architecture "x86_64"

filter "configurations:Debug"
    symbols "On"
    runtime "Debug"
    defines { "_DEBUG" }

filter "configurations:Release"
    optimize "Speed"
    runtime "Release"
    defines { "NDEBUG" }

filter {}

local Modules = json.decode(io.readfile("ModuleDependencies.json"))

local function AddIncludes(Name, Visited)
    if Visited[Name] then
        return
    end
    Visited[Name] = true
    includedirs { "Source/" .. Name .. "/Public" }
    for _, Dependency in ipairs(Modules[Name].Public) do
        AddIncludes(Dependency, Visited)
    end
end

local function Configure(Name, Kind)
    project(Name)
        kind(Kind)
        language "C++"
        cppdialect "C++20"
        characterset "Unicode"
        systemversion "latest"
        staticruntime "Off"
        targetdir "Binaries/Week1/%{cfg.buildcfg}"
        objdir "Intermediate/Week1/%{cfg.buildcfg}/%{prj.name}"
        defines { "NOMINMAX", "WIN32_LEAN_AND_MEAN" }
        buildoptions { "/utf-8", "/Zc:preprocessor", "/permissive-" }
        multiprocessorcompile "On"
        warnings "Extra"
        externalincludedirs { "Externals/Include", "ThirdParty/ImGui/Public/ImGui" }
end

for _, Name in ipairs({ "Core", "CoreUObject", "Serialization", "RenderCore", "Engine", "Renderer", "Platform", "Editor" }) do
    Configure(Name, "StaticLib")
    files { "Source/" .. Name .. "/Public/**.h", "Source/" .. Name .. "/Private/**.h", "Source/" .. Name .. "/Private/**.cpp" }
    includedirs { "Source/" .. Name .. "/Private" }
    local Visited = {}
    AddIncludes(Name, Visited)
    for _, Dependency in ipairs(Modules[Name].Private) do
        AddIncludes(Dependency, Visited)
    end
    links(Modules[Name].Public)
    links(Modules[Name].Private)
end

Configure("ImGui", "StaticLib")
    warnings "Off"
    files { "ThirdParty/ImGui/Private/imgui.cpp", "ThirdParty/ImGui/Private/imgui_draw.cpp", "ThirdParty/ImGui/Private/imgui_tables.cpp", "ThirdParty/ImGui/Private/imgui_widgets.cpp", "ThirdParty/ImGui/Private/imgui_impl_dx11.cpp", "ThirdParty/ImGui/Private/imgui_impl_win32.cpp", "ThirdParty/ImGui/Public/ImGui/**.h" }

local function ConfigureExecutable(Name, Kind, Directory)
    Configure(Name, Kind)
    files { Directory .. "/**.h", Directory .. "/**.cpp" }
    includedirs { Directory }
    AddIncludes("Editor", {})
    AddIncludes("Renderer", {})
    links { "Editor", "Engine", "Renderer", "Platform", "Serialization", "RenderCore", "CoreUObject", "Core", "ImGui", "d3d11", "dxgi", "d3dcompiler", "user32", "gdi32", "dwmapi", "imm32", "ole32", "comdlg32", "shell32" }
    debugdir "%{wks.location}"
end

ConfigureExecutable("Macaw", "WindowedApp", "Source/App")
