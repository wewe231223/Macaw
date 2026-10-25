param([ValidateSet('Debug', 'Release')][string]$Configuration = 'Debug')
$ErrorActionPreference = 'Stop'
$WorkspaceRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
Push-Location -LiteralPath $WorkspaceRoot
try {
    & (Join-Path $PSScriptRoot 'SetupDependencies.ps1')
    $MSBuild = 'C:/Program Files/Microsoft Visual Studio/18/Community/MSBuild/Current/Bin/MSBuild.exe'
    $Generator = 'vs2026'
    if (-not (Test-Path -LiteralPath $MSBuild)) {
        $Locator = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
        $MSBuild = & $Locator -latest -products '*' -requires Microsoft.Component.MSBuild -find 'MSBuild/**/Bin/MSBuild.exe' | Select-Object -First 1
        $Installation = & $Locator -latest -products '*' -requires Microsoft.Component.MSBuild -format json | ConvertFrom-Json | Select-Object -First 1
        if ([version]$Installation.installationVersion -lt [version]'18.0') {
            $Generator = 'vs2022'
        }
    }
    & premake5 $Generator
    if ($LASTEXITCODE -ne 0) { throw 'Project generation failed' }
    & $MSBuild 'Macaw.slnx' /m /nologo /verbosity:minimal "/p:Configuration=$Configuration" /p:Platform=x64
    if ($LASTEXITCODE -ne 0) { throw 'Build failed' }
} finally {
    Pop-Location
}
