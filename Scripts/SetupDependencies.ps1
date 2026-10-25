$ErrorActionPreference = 'Stop'
$WorkspaceRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$VcpkgRoot = Join-Path $WorkspaceRoot '.tools/vcpkg'
$VcpkgExecutable = Join-Path $VcpkgRoot 'vcpkg.exe'
$Manifest = Get-Content -LiteralPath (Join-Path $WorkspaceRoot 'vcpkg.json') -Raw | ConvertFrom-Json
$VcpkgCommit = $Manifest.'builtin-baseline'

if ($VcpkgCommit -notmatch '^[0-9a-f]{40}$') {
    throw 'vcpkg.json must specify a full builtin-baseline commit.'
}

if (-not (Test-Path -LiteralPath (Join-Path $VcpkgRoot '.git'))) {
    if (Test-Path -LiteralPath $VcpkgRoot) {
        throw "The vcpkg directory exists but is not a Git repository: $VcpkgRoot"
    }
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $VcpkgRoot) | Out-Null
    & git clone --filter=blob:none --no-checkout https://github.com/microsoft/vcpkg.git $VcpkgRoot
    if ($LASTEXITCODE -ne 0) {
        throw 'Could not clone vcpkg.'
    }
}

$CurrentCommit = & git -C $VcpkgRoot rev-parse HEAD
if ($LASTEXITCODE -ne 0) {
    throw 'Could not read the current vcpkg commit.'
}

if ($CurrentCommit -ne $VcpkgCommit -or -not (Test-Path -LiteralPath $VcpkgExecutable)) {
    & git -C $VcpkgRoot fetch origin $VcpkgCommit --depth 1
    if ($LASTEXITCODE -ne 0) {
        throw 'Could not fetch the pinned vcpkg commit.'
    }
    & git -C $VcpkgRoot checkout --detach $VcpkgCommit
    if ($LASTEXITCODE -ne 0) {
        throw 'Could not check out the pinned vcpkg commit.'
    }
    & (Join-Path $VcpkgRoot 'bootstrap-vcpkg.bat') -disableMetrics
    if ($LASTEXITCODE -ne 0) {
        throw 'Could not bootstrap vcpkg.'
    }
}

& $VcpkgExecutable install "--x-manifest-root=$WorkspaceRoot" --triplet=x64-windows --x-wait-for-lock
if ($LASTEXITCODE -ne 0) {
    throw 'Could not restore project dependencies.'
}
