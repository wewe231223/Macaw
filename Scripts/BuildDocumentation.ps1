param([switch]$InstallTools)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function GetVerifiedDownload([object]$Package, [string]$Destination) {
    if (-not (Test-Path -LiteralPath $Destination)) {
        if (-not $InstallTools) {
            throw "Missing documentation dependency: $Destination. Run with -InstallTools to download pinned tools."
        }
        Write-Host "Downloading $($Package.Url)"
        Invoke-WebRequest -Uri $Package.Url -OutFile $Destination
    }
    if ((Get-FileHash -LiteralPath $Destination -Algorithm SHA256).Hash -ne $Package.Sha256) {
        throw "SHA-256 mismatch: $Destination. Remove this file and retry -InstallTools."
    }
}

function GetDocumentationTool([string]$Name, [object]$Package, [string]$ArchiveName, [string]$DirectoryName, [string]$ExecutableName) {
    $ArchivePath = Join-Path $ToolsDirectory $ArchiveName
    $ToolDirectory = Join-Path $ToolsDirectory $DirectoryName
    GetVerifiedDownload $Package $ArchivePath
    $Executables = @(Get-ChildItem -LiteralPath $ToolDirectory -Filter $ExecutableName -File -Recurse -ErrorAction SilentlyContinue)
    if ($Executables.Count -eq 0) {
        New-Item -ItemType Directory -Path $ToolDirectory -Force | Out-Null
        if ($ArchiveName.EndsWith('.zip')) {
            Expand-Archive -LiteralPath $ArchivePath -DestinationPath $ToolDirectory -Force
        } else {
            & tar -xzf $ArchivePath -C $ToolDirectory
            if ($LASTEXITCODE -ne 0) {
                throw "$Name archive extraction failed."
            }
        }
        $Executables = @(Get-ChildItem -LiteralPath $ToolDirectory -Filter $ExecutableName -File -Recurse)
    }
    if ($Executables.Count -ne 1) {
        throw "Expected one $ExecutableName in $ToolDirectory."
    }
    return $Executables[0].FullName
}

function GetDoxygenPath([string]$Value) {
    if ($Value.Contains('"') -or $Value.Contains("`n") -or $Value.Contains("`r")) {
        throw "Unsupported character in documentation path: $Value"
    }
    return '"' + $Value.Replace('\', '/') + '"'
}

$RepositoryRoot = Split-Path -Parent $PSScriptRoot
$ToolsDirectory = Join-Path $RepositoryRoot '.tools/Documentation'
$OutputDirectory = Join-Path $RepositoryRoot 'Build/Documentation'
$SiteDirectory = Join-Path $OutputDirectory 'Site'
$Manifest = Get-Content -LiteralPath (Join-Path $RepositoryRoot 'Docs/Tools.json') -Raw | ConvertFrom-Json
$Modules = Get-Content -LiteralPath (Join-Path $RepositoryRoot 'ModuleDependencies.json') -Raw | ConvertFrom-Json -AsHashtable
$ModuleDescriptions = Get-Content -LiteralPath (Join-Path $RepositoryRoot 'Docs/Modules.json') -Raw | ConvertFrom-Json -AsHashtable
New-Item -ItemType Directory -Path $ToolsDirectory -Force | Out-Null

if ([Runtime.InteropServices.RuntimeInformation]::ProcessArchitecture -ne [Runtime.InteropServices.Architecture]::X64) {
    throw 'Documentation tool downloads require an x64 PowerShell 7 process.'
}
if ($IsWindows) {
    $Doxygen = GetDocumentationTool 'Doxygen' $Manifest.WindowsDoxygen "Doxygen-$($Manifest.DoxygenVersion).zip" "Doxygen-$($Manifest.DoxygenVersion)" 'doxygen.exe'
    $Dot = GetDocumentationTool 'Graphviz' $Manifest.WindowsGraphviz "Graphviz-$($Manifest.GraphvizVersion).zip" "Graphviz-$($Manifest.GraphvizVersion)" 'dot.exe'
} elseif ($IsLinux) {
    $Doxygen = GetDocumentationTool 'Doxygen' $Manifest.LinuxDoxygen "Doxygen-$($Manifest.DoxygenVersion).tar.gz" "Doxygen-$($Manifest.DoxygenVersion)" 'doxygen'
    $DotCommand = Get-Command dot -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($null -eq $DotCommand) {
        throw 'Graphviz is required. Install it with sudo apt-get install graphviz.'
    }
    $Dot = $DotCommand.Source
} else {
    throw 'This documentation build supports Windows x64 and Linux x64 with PowerShell 7.'
}

$DoxygenVersion = (& $Doxygen --version | Out-String).Trim()
if ($LASTEXITCODE -ne 0 -or $DoxygenVersion -notmatch ('^' + [regex]::Escape($Manifest.DoxygenVersion) + '(\s|$)')) {
    throw "Unexpected Doxygen version: $DoxygenVersion"
}
$GraphvizVersion = (& $Dot -V 2>&1 | Out-String).Trim()
if ($LASTEXITCODE -ne 0) {
    throw 'Graphviz could not start.'
}
$ThemePath = Join-Path $ToolsDirectory 'doxygen-awesome.css'
$ThemeLicensePath = Join-Path $ToolsDirectory 'ThemeLicense.txt'
GetVerifiedDownload $Manifest.ThemeStylesheet $ThemePath
GetVerifiedDownload $Manifest.ThemeLicense $ThemeLicensePath

$InputPaths = [Collections.Generic.List[string]]::new()
$IncludePaths = [Collections.Generic.List[string]]::new()
$GraphLines = [Collections.Generic.List[string]]::new()
$ModuleRows = [Collections.Generic.List[string]]::new()
$ModuleCount = 0
$GraphLines.Add('digraph Macaw {')
$GraphLines.Add('    graph [rankdir=LR, bgcolor="transparent", pad="0.3", nodesep="0.35", ranksep="0.8"];')
$GraphLines.Add('    node [shape=box, style="rounded,filled", fillcolor="#eef7f8", color="#9ebec5", fontname="Arial", fontsize=14, margin="0.18,0.12"];')
$GraphLines.Add('    edge [color="#247b88", arrowsize=0.7];')
foreach ($ModuleName in $Modules.Keys) {
    if ($ModuleName -notmatch '^[A-Za-z][A-Za-z0-9]*$') {
        throw "Invalid module name: $ModuleName"
    }
    $Module = $Modules[$ModuleName]
    if ($Module['External']) {
        $GraphLines.Add('    "' + $ModuleName + '" [fillcolor="#f1f2f5", color="#a0a6b2", style="rounded,filled,dashed"];')
    } else {
        $ModuleCount += 1
        $GraphLines.Add('    "' + $ModuleName + '";')
        foreach ($SourceDirectory in @('Public', 'Private')) {
            $SourcePath = Join-Path $RepositoryRoot "$ModuleName/$SourceDirectory"
            if (-not (Test-Path -LiteralPath $SourcePath -PathType Container)) {
                throw "Missing module source directory: $SourcePath"
            }
            $InputPaths.Add((GetDoxygenPath $SourcePath))
        }
        $IncludePaths.Add((GetDoxygenPath (Join-Path $RepositoryRoot "$ModuleName/Public")))
    }
    $Description = if ($ModuleDescriptions.Contains($ModuleName)) { $ModuleDescriptions[$ModuleName] } else { 'API에서 공개 타입과 구현을 확인하세요.' }
    $PublicDependencies = if ($Module.Public.Count -gt 0) { $Module.Public -join ', ' } else { '—' }
    $PrivateDependencies = if ($Module.Private.Count -gt 0) { $Module.Private -join ', ' } else { '—' }
    $ModuleSource = if ($Module['External']) { $ModuleName } else { "$ModuleName/Public" }
    $ExternalBadge = if ($Module['External']) { '<span class="Tag">외부</span>' } else { '' }
    $ModuleRows.Add('<tr><th scope="row"><a href="https://github.com/wewe231223/Macaw/tree/@@Revision@@/' + $ModuleSource + '">' + $ModuleName + '</a>' + $ExternalBadge + '</th><td>' + [Net.WebUtility]::HtmlEncode($Description) + '</td><td>' + [Net.WebUtility]::HtmlEncode($PublicDependencies) + '</td><td>' + [Net.WebUtility]::HtmlEncode($PrivateDependencies) + '</td></tr>')
    foreach ($Visibility in @('Public', 'Private')) {
        foreach ($Dependency in $Module[$Visibility]) {
            if (-not $Modules.Contains($Dependency)) {
                throw "Unknown dependency: $ModuleName -> $Dependency"
            }
            $EdgeStyle = if ($Visibility -eq 'Public') { 'solid' } else { 'dashed' }
            $GraphLines.Add('    "' + $ModuleName + '" -> "' + $Dependency + '" [style="' + $EdgeStyle + '", tooltip="' + $Visibility + '"];')
        }
    }
}
$GraphLines.Add('}')
foreach ($ApplicationPath in @('Application', 'Macaw.cpp', 'Macaw.h', 'framework.h', 'targetver.h', 'Resource.h')) {
    $InputPaths.Add((GetDoxygenPath (Join-Path $RepositoryRoot $ApplicationPath)))
}
$InputPaths.Add((GetDoxygenPath (Join-Path $RepositoryRoot 'Docs/ApiReference.dox')))
$IncludePaths.Add((GetDoxygenPath $RepositoryRoot))

$PreviousPath = $env:PATH
Push-Location -LiteralPath $RepositoryRoot
try {
    $env:PATH = (Split-Path -Parent $Dot) + [IO.Path]::PathSeparator + $PreviousPath
    $Revision = (& git rev-parse HEAD | Out-String).Trim()
    if ($LASTEXITCODE -ne 0 -or $Revision -notmatch '^[0-9a-f]{40}$') {
        throw 'Could not identify the source revision.'
    }
    $WorkingChanges = @(& git status --porcelain --untracked-files=normal)
    if ($LASTEXITCODE -ne 0) {
        throw 'Could not inspect the source working tree.'
    }
    $BuildLabel = $Revision.Substring(0, 12)
    if ($WorkingChanges.Count -gt 0) {
        $BuildLabel += ' + local changes'
    }

    if (Test-Path -LiteralPath $OutputDirectory) {
        $ExpectedOutput = [IO.Path]::GetFullPath((Join-Path $RepositoryRoot 'Build/Documentation'))
        $ResolvedOutput = (Resolve-Path -LiteralPath $OutputDirectory).Path
        if ($ResolvedOutput -ne $ExpectedOutput -or (Get-Item -LiteralPath (Split-Path -Parent $OutputDirectory)).LinkType -or (Get-Item -LiteralPath $OutputDirectory).LinkType -or @(Get-ChildItem -LiteralPath $OutputDirectory -Recurse -Force -Attributes ReparsePoint).Count -gt 0) {
            throw 'Refusing to clean a documentation output directory containing redirected paths.'
        }
        Remove-Item -LiteralPath $ResolvedOutput -Recurse -Force
    }
    New-Item -ItemType Directory -Path $SiteDirectory -Force | Out-Null
    $GraphPath = Join-Path $OutputDirectory 'ModuleDependencies.dot'
    $GraphLines | Set-Content -LiteralPath $GraphPath -Encoding utf8
    & $Dot -Tsvg $GraphPath -o (Join-Path $SiteDirectory 'ModuleDependencies.svg')
    if ($LASTEXITCODE -ne 0) {
        throw 'Module dependency graph generation failed.'
    }

    $Configuration = Get-Content -LiteralPath (Join-Path $RepositoryRoot 'Docs/Doxyfile') -Raw
    $Configuration += "`nOUTPUT_DIRECTORY = " + (GetDoxygenPath $OutputDirectory)
    $Configuration += "`nSTRIP_FROM_PATH = " + (GetDoxygenPath $RepositoryRoot)
    $Configuration += "`nINPUT = " + ($InputPaths -join ' ')
    $Configuration += "`nINCLUDE_PATH = " + ($IncludePaths -join ' ')
    $Configuration += "`nDOT_PATH = " + (GetDoxygenPath (Split-Path -Parent $Dot))
    $Configuration += "`nHTML_EXTRA_STYLESHEET = " + (GetDoxygenPath $ThemePath)
    $Configuration += "`nWARN_LOGFILE = " + (GetDoxygenPath (Join-Path $OutputDirectory 'DoxygenWarnings.log')) + "`n"
    $ConfigurationPath = Join-Path $OutputDirectory 'Doxyfile'
    Set-Content -LiteralPath $ConfigurationPath -Value $Configuration -Encoding utf8
    Write-Host "Building Macaw documentation ($BuildLabel) with Doxygen $DoxygenVersion."
    & $Doxygen $ConfigurationPath
    if ($LASTEXITCODE -ne 0) {
        throw 'Doxygen generation failed.'
    }

    $ClassListPath = Join-Path $SiteDirectory 'api/annotated.html'
    $ClassList = Get-Content -LiteralPath $ClassListPath -Raw
    $ClassList = $ClassList -replace '(?s)<td class="desc">.*?</td>', ''
    Set-Content -LiteralPath $ClassListPath -Value $ClassList -NoNewline -Encoding utf8

    $LandingPage = Get-Content -LiteralPath (Join-Path $RepositoryRoot 'Docs/Index.html') -Raw
    $LandingPage = $LandingPage.Replace('@@ModuleRows@@', ($ModuleRows -join "`n")).Replace('@@ModuleCount@@', [string]$ModuleCount).Replace('@@Revision@@', $Revision)
    Set-Content -LiteralPath (Join-Path $SiteDirectory 'index.html') -Value $LandingPage -Encoding utf8
    Copy-Item -LiteralPath (Join-Path $RepositoryRoot 'Docs/Documentation.css') -Destination $SiteDirectory
    Copy-Item -LiteralPath $ThemeLicensePath -Destination $SiteDirectory
    Set-Content -LiteralPath (Join-Path $SiteDirectory '.nojekyll') -Value '' -NoNewline -Encoding utf8
    $BuildInformation = [ordered]@{
        Revision = $Revision
        WorkingTreeChanges = $WorkingChanges.Count -gt 0
        Doxygen = $DoxygenVersion
        Graphviz = $GraphvizVersion
        Theme = $Manifest.ThemeVersion
    }
    $BuildInformation | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $SiteDirectory 'BuildInformation.json') -Encoding utf8
    & (Join-Path $PSScriptRoot 'VerifyDocumentation.ps1') -OutputDirectory $OutputDirectory
    $WarningPath = Join-Path $OutputDirectory 'DoxygenWarnings.log'
    $WarningCount = @(Select-String -LiteralPath $WarningPath -Pattern 'warning:').Count
    Write-Host "Doxygen warnings: $WarningCount. Details: $WarningPath"
    Write-Host "Documentation ready: $(Join-Path $SiteDirectory 'index.html')"
} finally {
    $env:PATH = $PreviousPath
    Pop-Location
}
