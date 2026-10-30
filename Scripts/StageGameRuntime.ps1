param(
    [Parameter(Mandatory = $true)][string]$RuntimeDirectory,
    [Parameter(Mandatory = $true)][string]$RedistDirectory,
    [Parameter(Mandatory = $true)][string]$Configuration
)

$ErrorActionPreference = 'Stop'
$RuntimePath = [IO.Path]::GetFullPath($RuntimeDirectory)
$RedistPath = [IO.Path]::GetFullPath($RedistDirectory)
$CrtPath = if ($Configuration.EndsWith('Debug')) {
    Join-Path $RedistPath 'debug_nonredist/x64'
} else {
    Join-Path $RedistPath 'x64'
}
$CrtPattern = if ($Configuration.EndsWith('Debug')) { 'Microsoft.VC*.DebugCRT' } else { 'Microsoft.VC*.CRT' }
$CrtDirectory = Get-ChildItem -LiteralPath $CrtPath -Directory -Filter $CrtPattern | Sort-Object Name -Descending | Select-Object -First 1

if (-not $CrtDirectory) {
    throw "Cannot locate the Visual C++ runtime in $CrtPath"
}

New-Item -ItemType Directory -Path $RuntimePath -Force | Out-Null
Get-ChildItem -LiteralPath $CrtDirectory.FullName -File -Filter '*.dll' | Copy-Item -Destination $RuntimePath -Force

if ($Configuration.EndsWith('Debug')) {
    Copy-Item -LiteralPath (Join-Path $env:SystemRoot 'System32/ucrtbased.dll') -Destination $RuntimePath -Force
}
