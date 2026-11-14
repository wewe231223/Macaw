param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug',
    [switch]$SkipBuild
)

$ErrorActionPreference = 'Stop'
$WorkspaceRoot = Split-Path -Parent $PSScriptRoot
$Locator = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$Installation = & $Locator -latest -prerelease -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath | Select-Object -First 1

if (-not $Installation) {
    throw 'Visual Studio C++ tools were not found.'
}

$BuildDirectory = Join-Path $WorkspaceRoot 'Build/MaterialBlendModeTests'
$BinaryDirectory = Join-Path $WorkspaceRoot "Binaries/$Configuration"
$DependencyDirectory = Join-Path $WorkspaceRoot ('vcpkg_installed/x64-windows/' + $(if ($Configuration -eq 'Debug') { 'debug/lib' } else { 'lib' }))
$ResponseFile = Join-Path $BuildDirectory 'Arguments.rsp'
$Executable = Join-Path $BinaryDirectory 'MaterialBlendModeTests.exe'

New-Item -ItemType Directory -Force -Path $BuildDirectory | Out-Null
Push-Location -LiteralPath $WorkspaceRoot

try {
    if (-not $SkipBuild) {
        & (Join-Path $Installation 'MSBuild/Current/Bin/MSBuild.exe') Macaw.slnx /m "/p:Configuration=$Configuration" /p:Platform=x64 /p:VcpkgManifestInstall=false /v:minimal /nologo

        if ($LASTEXITCODE -ne 0) {
            throw 'Macaw build failed.'
        }
    }

    $CompilerOptions = @('/nologo', '/std:c++20', '/EHsc', '/utf-8', '/DNOMINMAX', '/DWIN32_LEAN_AND_MEAN', '/Zc:preprocessor')
    $CompilerOptions += if ($Configuration -eq 'Debug') { @('/MDd', '/D_DEBUG') } else { @('/MD', '/DNDEBUG') }

    foreach ($Module in @('Renderer', 'Engine', 'Core', 'CoreUObject', 'Serialization', 'RenderCore', 'Platform', 'Asset', 'Script')) {
        $CompilerOptions += '/I"' + (Join-Path $WorkspaceRoot "Source/$Module/Public") + '"'
        $CompilerOptions += '/I"' + (Join-Path $WorkspaceRoot "Intermediate/$Module/Generated") + '"'
    }

    $CompilerOptions += '/I"' + (Join-Path $WorkspaceRoot 'Source/Renderer/Private') + '"'
    $CompilerOptions += '/I"' + (Join-Path $WorkspaceRoot 'Source/Asset/Private') + '"'
    $CompilerOptions += '/I"' + (Join-Path $WorkspaceRoot 'vcpkg_installed/x64-windows/include') + '"'
    $CompilerOptions += '"' + (Join-Path $WorkspaceRoot 'Tests/MaterialBlendModeTests.cpp') + '"'
    $CompilerOptions += '/Fo"' + (Join-Path $BuildDirectory 'MaterialBlendModeTests.obj') + '"'
    $CompilerOptions += '/Fe"' + $Executable + '"'
    $CompilerOptions += @('/link', ('/LIBPATH:"' + $BinaryDirectory + '"'), ('/LIBPATH:"' + $DependencyDirectory + '"'))
    $CompilerOptions += @('Renderer.lib', 'Engine.lib', 'Asset.lib', 'RenderCore.lib', 'Platform.lib', 'Script.lib', 'Serialization.lib', 'CoreUObject.lib', 'Core.lib', 'DirectXTex.lib', 'libfbxsdk.lib', 'd3d11.lib', 'dxgi.lib', 'd3dcompiler.lib', 'bcrypt.lib', 'user32.lib', 'gdi32.lib', 'dwmapi.lib', 'imm32.lib', 'ole32.lib', 'windowscodecs.lib', 'comdlg32.lib', 'shell32.lib', 'oleaut32.lib', '/IGNORE:4099')
    $CompilerOptions += if ($Configuration -eq 'Debug') { 'assimp-vc145-mtd.lib' } else { 'assimp-vc145-mt.lib' }
    Set-Content -LiteralPath $ResponseFile -Value ($CompilerOptions -join ' ') -Encoding utf8

    $DeveloperShell = Join-Path $Installation 'VC/Auxiliary/Build/vcvars64.bat'
    & $env:ComSpec /d /s /c ('call "{0}" >nul && cl.exe @"{1}"' -f $DeveloperShell, $ResponseFile)

    if ($LASTEXITCODE -ne 0) {
        throw 'Material blend mode test compilation failed.'
    }

    & $Executable

    if ($LASTEXITCODE -ne 0) {
        throw 'Material blend mode tests failed.'
    }
} finally {
    Pop-Location
}
