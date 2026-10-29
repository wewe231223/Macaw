param([ValidateSet('Debug', 'Release')][string]$Configuration = 'Debug')

$ErrorActionPreference = 'Stop'
$RepositoryRoot = Split-Path -Parent $PSScriptRoot
$Locator = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$Installation = & $Locator -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$CompilerEnvironment = Join-Path $Installation 'VC/Auxiliary/Build/vcvars64.bat'
$DebugRuntime = if ($Configuration -eq 'Debug') { '/MDd /D_DEBUG' } else { '/MD /DNDEBUG' }
$DependencyConfiguration = if ($Configuration -eq 'Debug') { 'debug' } else { 'release' }
$Assimp = Get-ChildItem -LiteralPath (Join-Path $RepositoryRoot "vcpkg_installed/x64-windows/$(if ($Configuration -eq 'Debug') { 'debug/lib' } else { 'lib' })") -Filter 'assimp-*.lib' | Select-Object -First 1
$Arguments = @('/nologo /std:c++20 /EHsc /utf-8 /DNOMINMAX /DWIN32_LEAN_AND_MEAN /Zc:preprocessor', $DebugRuntime)
foreach ($Module in @('Core', 'CoreUObject', 'Serialization', 'RenderCore', 'Engine', 'Renderer', 'Platform', 'Editor', 'Script')) {
    $Arguments += "/I`"Source/$Module/Public`" /I`"Intermediate/$Module/Generated`""
}
$Arguments += '/ISource/Editor/Private /ISource/Engine/Private /IExternals/Include /IThirdParty/ImGui/Public/ImGui'
$Arguments += 'Tests/Week3Regression.cpp /Fe.tools/Week3Regression.exe /Fo.tools/Week3Regression.obj /Fd.tools/Week3Regression.pdb /link'
$Arguments += "/LIBPATH:Binaries/$Configuration /LIBPATH:Externals/bin/$DependencyConfiguration /LIBPATH:`"$($Assimp.DirectoryName)`""
$Arguments += "Editor.lib Script.lib Engine.lib Renderer.lib Platform.lib Serialization.lib RenderCore.lib CoreUObject.lib Core.lib ImGui.lib DirectXTex.lib lua51.lib $($Assimp.Name) d3d11.lib dxgi.lib d3dcompiler.lib bcrypt.lib user32.lib gdi32.lib dwmapi.lib imm32.lib ole32.lib windowscodecs.lib comdlg32.lib shell32.lib /IGNORE:4099"
Push-Location $RepositoryRoot
try {
    New-Item -ItemType Directory -Path '.tools' -Force | Out-Null
    $Arguments -join ' ' | Set-Content '.tools/Week3Regression.rsp' -Encoding utf8
    "@echo off`r`ncall `"$CompilerEnvironment`" >nul`r`ncl @.tools/Week3Regression.rsp`r`n" | Set-Content '.tools/Week3Regression.cmd' -Encoding ascii
    & $env:ComSpec /c '.tools\Week3Regression.cmd'
    if ($LASTEXITCODE -ne 0) { throw 'Week3 regression test compilation failed.' }
    $PreviousPath = $env:PATH
    try {
        $BinaryDirectory = Join-Path $RepositoryRoot "vcpkg_installed/x64-windows/$(if ($Configuration -eq 'Debug') { 'debug/bin' } else { 'bin' })"
        $env:PATH = "$BinaryDirectory;$PreviousPath"
        & '.tools/Week3Regression.exe'
        if ($LASTEXITCODE -ne 0) { throw 'Week3 regression test failed.' }
    } finally {
        $env:PATH = $PreviousPath
    }
} finally {
    Pop-Location
}
