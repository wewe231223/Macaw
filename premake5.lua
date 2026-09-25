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

function ConfigureProject(ProjectName, ProjectKind)
    project(ProjectName)
        kind(ProjectKind)
        language "C++"
        cppdialect "C++20"
        characterset "Unicode"
        staticruntime "Off"
        toolset "msc-v145"
        targetdir "bin/%{cfg.buildcfg}/%{cfg.platform}"
        objdir "bin-int/%{prj.name}/%{cfg.buildcfg}/%{cfg.platform}"
        includedirs { ".", "range_v_3", "Externals/Include" }
        defines { "NOMINMAX" }
        buildoptions { "/utf-8" }
        pchheader "pch.h"
        pchsource "pch.cpp"
        files { "pch.cpp" }
end

function ConfigureExecutableLinks()
    links {
        "Editor",
        "Render",
        "World",
        "Asset",
        "Serialization",
        "Core",
        "Math",
        "DirectXTex",
        "d3d11",
        "dxgi",
        "d3dcompiler",
        "dwmapi",
        "gdi32",
        "imm32",
        "shell32",
        "user32",
        "kernel32",
    }

    filter "configurations:Debug"
        libdirs { "Externals/bin/debug" }

    filter "configurations:Release"
        libdirs { "Externals/bin/release" }

    filter "configurations:Viewer"
        libdirs { "Externals/bin/release" }

    filter {}
end

ConfigureProject("Math", "StaticLib")
    files { "Math/**.h", "Math/**.cpp" }
    removefiles { "pch.cpp" }
    enablepch "Off"

ConfigureProject("Core", "StaticLib")
    files { "Core/**.h", "Core/**.cpp", "Core/**.cc" }
    dependson { "Math" }

ConfigureProject("Serialization", "StaticLib")
    files { "Serialization/**.h", "Serialization/**.cpp" }
    dependson { "Core" }

ConfigureProject("Asset", "StaticLib")
    files { "Asset/**.h", "Asset/**.cpp", "Asset/**.cc" }
    dependson { "Serialization" }

ConfigureProject("World", "StaticLib")
    files { "World/**.h", "World/**.cpp" }
    dependson { "Asset" }

ConfigureProject("Render", "StaticLib")
    files { "Render/**.h", "Render/**.cpp", "ImGui/**.h", "ImGui/**.cpp" }
    dependson { "Asset" }

ConfigureProject("Editor", "StaticLib")
    files { "Editor/**.h", "Editor/**.cpp" }
    dependson { "World", "Render" }

ConfigureProject("Macaw", "WindowedApp")
    -- 파일은 보존하되 생성되는 Macaw 프로젝트에는 포함하지 않는다.
    files { "Macaw.cpp", "Macaw.h", "framework.h", "targetver.h", "Resource.h", "Macaw.rc", "Scripts/GenerateGizmoTorus.cpp" }
    ConfigureExecutableLinks()

ConfigureProject("MacawTests", "ConsoleApp")
    -- 테스트는 실제 엔진 소스를 함께 링크하되, Win32/ImGui 에디터 진입점은 제외한다.
    files { "Tests/**.h", "Tests/**.cpp" }
    removefiles { "Tests/TestUndo.cpp" }
    ConfigureExecutableLinks()

-- MacawTests 설정 이후 Macaw 전용 파일 필터를 다시 선택한다.
project "Macaw"

-- ImGui와 SimpleMath는 PCH를 사용하지 않는다.
project "Render"
filter "files:ImGui/**.cpp"
    enablepch "Off"

project "Math"
filter "files:Math/SimpleMath/SimpleMath.cpp"
    enablepch "Off"

project "MacawTests"
filter "files:Tests/**.cpp"
    enablepch "Off"

project "Macaw"
filter {}
