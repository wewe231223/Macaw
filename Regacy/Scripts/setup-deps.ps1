Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$RepositoryRoot = Split-Path -Parent $PSScriptRoot
$ToolsDirectory = Join-Path $RepositoryRoot ".tools"
$VcpkgRoot = Join-Path $ToolsDirectory "vcpkg"
$VcpkgExecutable = Join-Path $VcpkgRoot "vcpkg.exe"

$Manifest = Get-Content -LiteralPath (Join-Path $RepositoryRoot "vcpkg.json") -Raw | ConvertFrom-Json
$VcpkgCommit = $Manifest.'builtin-baseline'

if ($VcpkgCommit -notmatch '^[0-9a-f]{40}$') {
    throw "vcpkg.json must specify a full builtin-baseline commit."
}

New-Item -ItemType Directory -Force -Path $ToolsDirectory | Out-Null

if (-not (Test-Path -LiteralPath (Join-Path $VcpkgRoot ".git"))) {
    if (Test-Path -LiteralPath $VcpkgRoot) {
        throw "vcpkg 경로가 존재하지만 Git 저장소가 아닙니다: $VcpkgRoot"
    }

    git clone --filter=blob:none --no-checkout https://github.com/microsoft/vcpkg.git $VcpkgRoot

    if ($LASTEXITCODE -ne 0) {
        throw "vcpkg 저장소를 복제하지 못했습니다."
    }
}

$CurrentCommit = git -C $VcpkgRoot rev-parse HEAD

if ($LASTEXITCODE -ne 0) {
    throw "Could not read the current vcpkg commit."
}

$NeedsCheckout = $CurrentCommit -ne $VcpkgCommit -or -not (Test-Path -LiteralPath (Join-Path $VcpkgRoot "bootstrap-vcpkg.bat"))

if ($NeedsCheckout) {
    git -C $VcpkgRoot fetch origin $VcpkgCommit --depth 1

    if ($LASTEXITCODE -ne 0) {
        throw "고정된 vcpkg 커밋을 가져오지 못했습니다."
    }

    git -C $VcpkgRoot checkout --detach $VcpkgCommit

    if ($LASTEXITCODE -ne 0) {
        throw "고정된 vcpkg 커밋으로 전환하지 못했습니다."
    }
}

if ($NeedsCheckout -or -not (Test-Path -LiteralPath $VcpkgExecutable)) {
    & (Join-Path $VcpkgRoot "bootstrap-vcpkg.bat") -disableMetrics

    if ($LASTEXITCODE -ne 0) {
        throw "vcpkg bootstrap에 실패했습니다."
    }
}

Push-Location -LiteralPath $RepositoryRoot

try {
    & $VcpkgExecutable install "--triplet=x64-windows" "--x-wait-for-lock"

    if ($LASTEXITCODE -ne 0) {
        throw "프로젝트 의존성 설치에 실패했습니다."
    }
}
finally {
    Pop-Location
}

Write-Host "Macaw dependencies are ready."
