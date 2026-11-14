--[[!
@file premake5.lua
@brief 모듈 의존성과 UObject 코드 생성 단계를 반영해 Visual Studio 프로젝트를 생성합니다.
@details Cascade는 Core·CoreUObject에 의존하는 정적 라이브러리이며 Asset과 실행 파일에서 연결합니다. 모듈 의존성 및 문서용 설명은 별도 JSON 설정을 사용합니다.
@see ModuleDependencies.json
@see Docs/Modules.json
]]

--[[!
@brief 모듈별 리플렉션 생성 코드의 출력 폴더를 반환합니다.
@param[in] Module 생성 코드를 사용하는 모듈 이름입니다.
@return 작업 공간 기준 Intermediate 하위 경로입니다.
]]
local function GetGeneratedDirectory(Module)
    return "Intermediate/" .. (Module == "App" and "Macaw" or Module) .. "/Generated"
end

-- 헤더 툴은 엔진에 의존하지 않는 단독 실행 파일입니다. 각 모듈은 빌드 전에 이 툴로 자기 generated.h와 등록 코드를 만듭니다.
local HeaderTool = '"%{wks.location}/Binaries/%{cfg.buildcfg}/MacawHeaderTool.exe" "%{wks.location}."'

--[[!
@brief UObject 매크로를 사용하는 모듈에 헤더 툴 의존성과 빌드 전 생성 명령을 연결합니다.
@param[in] Module 생성할 모듈 이름입니다.
]]
local function UseHeaderTool(Module)
    dependson { "MacawHeaderTool" }
    prebuildcommands { HeaderTool .. " " .. Module }
end

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

--[[!
@brief 모듈과 공개 의존 모듈의 공개·생성 헤더 경로를 중복 없이 추가합니다.
@param[in] Name 포함 경로를 추가할 모듈 이름입니다.
@param[in,out] Visited 이미 처리한 모듈을 기록하는 이름 집합입니다.
]]
local function AddIncludes(Name, Visited)
    if Visited[Name] then
        return
    end
    Visited[Name] = true
    includedirs { "Source/" .. Name .. "/Public" }

    if Name == "Asset" then
        defines { "FBXSDK_SHARED" }
    end

    includedirs { GetGeneratedDirectory(Name) }

    for _, Dependency in ipairs(Modules[Name].Public) do
        AddIncludes(Dependency, Visited)
    end
end

--[[!
@brief C++20 모듈과 실행 파일에 공통 출력 경로·컴파일러·런타임 설정을 적용합니다.
@param[in] Name 프로젝트 이름입니다.
@param[in] Kind 정적 라이브러리 또는 실행 파일 등의 Premake 프로젝트 종류입니다.
]]
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
        defines { "NOMINMAX", "WIN32_LEAN_AND_MEAN" }
        buildoptions { "/utf-8", "/Zc:preprocessor", "/permissive-" }
        multiprocessorcompile "On"
        warnings "Extra"
end

Configure("MacawHeaderTool", "ConsoleApp")
    files { "Source/Programs/MacawHeaderTool/**.h", "Source/Programs/MacawHeaderTool/**.cpp" }
    includedirs { "Source/Core/Public" }
    -- 빌드마다 모듈 수만큼 실행되므로 Debug 구성에서도 최적화합니다. 디버그 CRT로는 열 배 가까이 느립니다.
    optimize "Speed"
    runtime "Release"
    removedefines { "_DEBUG" }
    defines { "NDEBUG" }

--[[!
@var ModuleNames
@brief UObject 기반 Cascade를 포함해 정적 라이브러리와 리플렉션 코드를 생성할 모듈 목록입니다.
]]
local ModuleNames = { "Core", "CoreUObject", "Cascade", "Serialization", "Asset", "RenderCore", "Engine", "Script", "Renderer", "Platform", "Viewer", "Editor" }

for _, Name in ipairs(ModuleNames) do
    Configure(Name, "StaticLib")
    files { "Source/" .. Name .. "/Public/**.h", "Source/" .. Name .. "/Private/**.h", "Source/" .. Name .. "/Private/**.cpp" }
    includedirs { "Source/" .. Name .. "/Private" }
    UseHeaderTool(Name)

    -- Core 타입은 CoreUObject가 등록하므로 Core에는 등록 소스가 없습니다.
    if Name ~= "Core" then
        files { GetGeneratedDirectory(Name) .. "/" .. Name .. "Reflection.cpp" }
    end

    local Visited = {}
    AddIncludes(Name, Visited)
    for _, Dependency in ipairs(Modules[Name].Private) do
        AddIncludes(Dependency, Visited)
    end
    links(Modules[Name].Public)
    links(Modules[Name].Private)

    if Name == "Asset" then
        defines { "DIRECTX_TEX_IMPORT" }
    end
end

project "Script"
    files { GetGeneratedDirectory("Script") .. "/ScriptBindings.cpp" }

-- C# 엔진 API는 솔루션의 C# 프로젝트로 빌드합니다. 생성 바인딩을 쓰므로 Script의 프리빌드 뒤에 빌드합니다.
externalproject "Macaw.Engine"
    location "Source/Managed/Macaw.Engine"
    uuid "6D3F4A52-8B1C-4E7A-9F21-3C5B7D9E1A40"
    kind "SharedLib"
    language "C#"
    dependson { "Script" }
    configmap { ["x64"] = "x64", ["ObjViewerDebug"] = "Debug", ["ObjViewerRelease"] = "Release" }

Configure("Shaders", "Utility")
    files { "Templates/Project/Content/Shaders/**.hlsl", "Templates/Project/Content/Shaders/**.hlsli" }
    vpaths { ["*"] = "Templates/Project/Content/Shaders" }
    filter "files:**"
        buildaction "None"
    filter {}

--[[!
@brief 런타임 실행 파일에 엔진·파티클·렌더러·스크립트 모듈의 헤더와 링크 의존성을 연결합니다.
@param[in] Name 실행 파일 프로젝트 이름입니다.
@param[in] Kind Premake 실행 파일 종류입니다.
@param[in] Directory 실행 파일의 원본 소스 폴더입니다.
]]
local function ConfigureRuntimeExecutable(Name, Kind, Directory)
    Configure(Name, Kind)
    files { Directory .. "/**.h", Directory .. "/**.cpp" }
    includedirs { Directory }

    for _, Module in ipairs({ "Engine", "Renderer", "Platform", "Script" }) do
        AddIncludes(Module, {})
    end

    links { "Script", "Engine", "Renderer", "Platform", "Asset", "Cascade", "Serialization", "RenderCore", "CoreUObject", "Core", "d3d11", "dxgi", "d3dcompiler", "bcrypt", "user32", "gdi32", "dwmapi", "imm32", "ole32", "windowscodecs", "comdlg32", "shell32" }
    debugdir "%{wks.location}"
end

ConfigureRuntimeExecutable("MacawGame", "WindowedApp", "Source/Game")
    files { GetGeneratedDirectory("Game") .. "/GameReflection.cpp" }
    includedirs { GetGeneratedDirectory("Game") }
    UseHeaderTool("Game")
    targetdir "Binaries/%{cfg.buildcfg}/GameRuntime"
    files { "Source/App/**.rc", "Macaw.ico" }
    resincludedirs { "." }
    postbuildcommands { '{COPYDIR} "%{wks.location}/Templates/Project/Content" "%{cfg.targetdir}/Templates/Project/Content"' }
    postbuildcommands { 'powershell -NoProfile -ExecutionPolicy Bypass -File "%{wks.location}/Scripts/StageGameRuntime.ps1" -RuntimeDirectory "%{cfg.targetdir}" -RedistDirectory "$(VCToolsRedistInstallDir)." -Configuration "%{cfg.buildcfg}"' }

ConfigureRuntimeExecutable("Macaw", "WindowedApp", "Source/App")
    -- 앱 등록 코드는 모든 모듈의 등록 헤더를 포함합니다.
    for _, Module in ipairs(ModuleNames) do
        AddIncludes(Module, {})
    end

    files { GetGeneratedDirectory("App") .. "/AppReflection.cpp" }
    includedirs { GetGeneratedDirectory("App") }
    UseHeaderTool("App")
    links { "Editor", "oleaut32" }
    dependson { "MacawGame", "Macaw.Engine" }
    files { "Source/App/**.rc", "Resources/Editor.rc", "Macaw.ico" }
    resincludedirs { "." }
    postbuildcommands { 'powershell -NoProfile -ExecutionPolicy Bypass -File "%{wks.location}/Scripts/StageProjectTemplates.ps1" -RuntimeDirectory "%{cfg.targetdir}" -WorkspaceRoot "%{wks.location}."' }
    filter "configurations:not ObjViewer*"
        postbuildcommands { 'powershell -NoProfile -ExecutionPolicy Bypass -File "%{wks.location}/Scripts/StageManagedRuntime.ps1" -RuntimeDirectory "%{cfg.targetdir}" -WorkspaceRoot "%{wks.location}." -Configuration "%{cfg.buildcfg}"' }
    filter {}
