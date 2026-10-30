local Reflection = dofile("Scripts/GenerateReflection.lua")

newoption { trigger = "reflection-module", value = "MODULE", description = "Generate reflection for one module" }
newaction { trigger = "generate-reflection", description = "Generate reflection from property annotations", execute = function() Reflection.Generate(_OPTIONS["reflection-module"]) end }

if _ACTION == "generate-reflection" then
    return
end

local ReflectedModules = Reflection.Generate()

workspace "Macaw"
    location "."
    configurations { "Debug", "Release", "ObjViewerDebug", "ObjViewerRelease" }
    platforms { "x64" }
    defaultplatform "x64"
    startproject "Macaw"
    system "windows"
    architecture "x86_64"

filter "configurations:*Debug"
    symbols "On"
    runtime "Debug"
    defines { "_DEBUG" }

filter "configurations:*Release"
    optimize "Speed"
    linktimeoptimization "On"
    runtime "Release"
    defines { "NDEBUG" }

filter {}

filter "configurations:ObjViewer*"
    defines { "MACAW_OBJ_VIEWER=1" }
filter {}

local Modules = json.decode(io.readfile("ModuleDependencies.json"))

local function AddIncludes(Name, Visited)
    if Visited[Name] then
        return
    end
    Visited[Name] = true
    includedirs { "Source/" .. Name .. "/Public" }

    if ReflectedModules[Name] then
        includedirs { Reflection.GetGeneratedDirectory(Name) }
    end

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
        targetdir "Binaries/%{cfg.buildcfg}"
        objdir "Intermediate/%{prj.name}/%{cfg.buildcfg}"
        defines { "NOMINMAX", "WIN32_LEAN_AND_MEAN", "SOL_LUAJIT=1" }
        buildoptions { "/utf-8", "/Zc:preprocessor", "/permissive-" }
        multiprocessorcompile "On"
        warnings "Extra"
        externalincludedirs { "Externals/Include", "ThirdParty/ImGui/Public/ImGui", "vcpkg_installed/x64-windows/include/luajit" }
end

for _, Name in ipairs({ "Core", "CoreUObject", "Serialization", "RenderCore", "Engine", "Script", "Renderer", "Platform", "Editor" }) do
    Configure(Name, "StaticLib")
    files { "Source/" .. Name .. "/Public/**.h", "Source/" .. Name .. "/Private/**.h", "Source/" .. Name .. "/Private/**.cpp" }
    includedirs { "Source/" .. Name .. "/Private" }

    if ReflectedModules[Name] then
        local GeneratedDirectory = Reflection.GetGeneratedDirectory(Name)
        files { GeneratedDirectory .. "/**.h", GeneratedDirectory .. "/" .. Name .. "Reflection.cpp" }
        includedirs { GeneratedDirectory }
        prebuildcommands { '"' .. _PREMAKE_COMMAND .. '" --file="' .. path.getabsolute("premake5.lua") .. '" generate-reflection --reflection-module=' .. Name }
    end

    local Visited = {}
    AddIncludes(Name, Visited)
    for _, Dependency in ipairs(Modules[Name].Private) do
        AddIncludes(Dependency, Visited)
    end
    links(Modules[Name].Public)
    links(Modules[Name].Private)

    if Name == "Engine" then
        defines { "FBXSDK_SHARED" }
    end
end

Configure("ImGui", "StaticLib")
    warnings "Off"
    files { "ThirdParty/ImGui/Private/imgui.cpp", "ThirdParty/ImGui/Private/imgui_draw.cpp", "ThirdParty/ImGui/Private/imgui_tables.cpp", "ThirdParty/ImGui/Private/imgui_widgets.cpp", "ThirdParty/ImGui/Private/imgui_impl_dx11.cpp", "ThirdParty/ImGui/Private/imgui_impl_win32.cpp", "ThirdParty/ImGui/Public/ImGui/**.h" }

Configure("Shaders", "Utility")
    files { "Templates/Project/Content/Shaders/**.hlsl", "Templates/Project/Content/Shaders/**.hlsli" }
    vpaths { ["*"] = "Templates/Project/Content/Shaders" }
    filter "files:**"
        buildaction "None"
    filter {}

local function ConfigureExecutable(Name, Kind, Directory)
    Configure(Name, Kind)
    files { Directory .. "/**.h", Directory .. "/**.cpp" }
    includedirs { Directory }

    for Module, _ in pairs(ReflectedModules) do
        includedirs { Reflection.GetGeneratedDirectory(Module) }

        if Modules[Module] then
            AddIncludes(Module, {})
        end
    end

    local GeneratedDirectory = Reflection.GetGeneratedDirectory("App")
    files { GeneratedDirectory .. "/**.h", GeneratedDirectory .. "/AppReflection.cpp" }
    prebuildcommands { '"' .. _PREMAKE_COMMAND .. '" --file="' .. path.getabsolute("premake5.lua") .. '" generate-reflection --reflection-module=App' }
    AddIncludes("Editor", {})
    AddIncludes("Renderer", {})
    links { "Editor", "Script", "Engine", "Renderer", "Platform", "Serialization", "RenderCore", "CoreUObject", "Core", "ImGui", "DirectXTex", "lua51", "d3d11", "dxgi", "d3dcompiler", "bcrypt", "user32", "gdi32", "dwmapi", "imm32", "ole32", "windowscodecs", "comdlg32", "shell32" }
    links { "libfbxsdk" }
    filter "configurations:*Debug"
        libdirs { "Externals/bin/debug" }
    filter "configurations:*Release"
        libdirs { "Externals/bin/release" }
    filter {}
    debugdir "%{wks.location}"
end

ConfigureExecutable("Macaw", "WindowedApp", "Source/App")
    files { "Source/App/**.rc", "Macaw.ico" }
    resincludedirs { "." }
    postbuildcommands { '{COPYDIR} "%{wks.location}/Templates/Project" "%{cfg.targetdir}/Templates/Project"' }
    postbuildcommands { '{COPYFILE} "%{wks.location}/vcpkg_installed/x64-windows/tools/luajit/lua51.dll" "%{cfg.targetdir}/lua51.dll"' }
