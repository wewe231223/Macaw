param(
    [Parameter(Mandatory = $true)][string]$RuntimeDirectory,
    [Parameter(Mandatory = $true)][string]$WorkspaceRoot,
    [Parameter(Mandatory = $true)][string]$Configuration
)

# Macaw.Engine을 빌드하고 호스팅에 필요한 .NET 런타임 사본을 실행 파일 옆에 배치합니다.

$ErrorActionPreference = 'Stop'
$RuntimePath = [IO.Path]::GetFullPath($RuntimeDirectory)
$WorkspacePath = [IO.Path]::GetFullPath($WorkspaceRoot)
$MajorVersion = 10

$DotnetCommand = Get-Command dotnet.exe -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1

if (-not $DotnetCommand) {
    throw ".NET $MajorVersion SDK is required to build C# scripting. Install it from https://dot.net/download."
}

$DotnetRoot = Split-Path -Parent $DotnetCommand.Source

# dotnet 출력은 로캘에 따라 달라지므로 폴더 이름에서 버전을 읽습니다.
$Framework = Get-ChildItem -LiteralPath (Join-Path $DotnetRoot 'shared/Microsoft.NETCore.App') -Directory |
    Where-Object { $_.Name -match "^$MajorVersion\.\d+\.\d+$" } |
    Sort-Object { [version]$_.Name } -Descending |
    Select-Object -First 1

if (-not $Framework) {
    throw "The .NET $MajorVersion runtime was not found in $DotnetRoot."
}

$Version = $Framework.Name
$HostFxr = Join-Path $DotnetRoot "host/fxr/$Version/hostfxr.dll"

if (-not (Test-Path -LiteralPath $HostFxr)) {
    throw "hostfxr $Version was not found in $DotnetRoot."
}

# Macaw.Engine은 솔루션의 C# 프로젝트가 Managed 폴더로 빌드합니다. 여기서는 결과만 확인합니다.
$ManagedPath = Join-Path $RuntimePath 'Managed'

if (-not (Test-Path -LiteralPath (Join-Path $ManagedPath 'Macaw.Engine.dll'))) {
    Write-Warning "Macaw.Engine.dll is missing in $ManagedPath. Build the Macaw.Engine project in the solution."
}

$PrivateRoot = Join-Path $RuntimePath 'dotnet'
$PrivateFxr = Join-Path $PrivateRoot "host/fxr/$Version"
$PrivateFramework = Join-Path $PrivateRoot "shared/Microsoft.NETCore.App/$Version"

# 다른 버전의 사본이 남아 있으면 지우고 새로 배치합니다.
if (-not (Test-Path -LiteralPath (Join-Path $PrivateFxr 'hostfxr.dll')) -or -not (Test-Path -LiteralPath (Join-Path $PrivateFramework 'coreclr.dll'))) {
    if (Test-Path -LiteralPath $PrivateRoot) {
        Remove-Item -LiteralPath $PrivateRoot -Recurse -Force
    }

    New-Item -ItemType Directory -Path $PrivateFxr -Force | Out-Null
    New-Item -ItemType Directory -Path (Split-Path -Parent $PrivateFramework) -Force | Out-Null
    Copy-Item -LiteralPath $HostFxr -Destination $PrivateFxr -Force
    Copy-Item -LiteralPath $Framework.FullName -Destination $PrivateFramework -Recurse -Force
}
