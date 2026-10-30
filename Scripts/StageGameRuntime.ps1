param(
    [Parameter(Mandatory = $true)][string]$RuntimeDirectory,
    [Parameter(Mandatory = $true)][string]$RedistDirectory,
    [Parameter(Mandatory = $true)][string]$Configuration
)

$ErrorActionPreference = 'Stop'
$RuntimePath = [IO.Path]::GetFullPath($RuntimeDirectory)
$RedistPath = [IO.Path]::GetFullPath($RedistDirectory)
$RuntimeConfigurations = @('Release')

if ($Configuration.EndsWith('Debug')) {
    $RuntimeConfigurations += 'Debug'
}

New-Item -ItemType Directory -Path $RuntimePath -Force | Out-Null

foreach ($RuntimeConfiguration in $RuntimeConfigurations) {
    $RuntimeSourcePath = if ($RuntimeConfiguration -eq 'Debug') { Join-Path $RedistPath 'debug_nonredist/x64' } else { Join-Path $RedistPath 'x64' }
    $RuntimePatterns = if ($RuntimeConfiguration -eq 'Debug') { @('Microsoft.VC*.DebugCRT', 'Microsoft.VC*.DebugOpenMP') } else { @('Microsoft.VC*.CRT', 'Microsoft.VC*.OpenMP') }

    foreach ($RuntimePattern in $RuntimePatterns) {
        $RuntimeSourceDirectory = Get-ChildItem -LiteralPath $RuntimeSourcePath -Directory -Filter $RuntimePattern | Sort-Object Name -Descending | Select-Object -First 1

        if (-not $RuntimeSourceDirectory) {
            throw "Cannot locate $RuntimePattern in $RuntimeSourcePath"
        }

        Get-ChildItem -LiteralPath $RuntimeSourceDirectory.FullName -File -Filter '*.dll' | Copy-Item -Destination $RuntimePath -Force
    }
}

if ($Configuration.EndsWith('Debug')) {
    Copy-Item -LiteralPath (Join-Path $env:SystemRoot 'System32/ucrtbased.dll') -Destination $RuntimePath -Force
}
