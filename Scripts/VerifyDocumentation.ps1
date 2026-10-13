param([string]$OutputDirectory = (Join-Path (Split-Path -Parent $PSScriptRoot) 'Build/Documentation'))

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$SiteDirectory = [System.IO.Path]::GetFullPath((Join-Path $OutputDirectory 'Site'))
$XmlDirectory = Join-Path $OutputDirectory 'Xml'
$Files = @(Get-ChildItem -LiteralPath $SiteDirectory -File -Recurse)
$FilePaths = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)
foreach ($File in $Files) {
    [void]$FilePaths.Add($File.FullName)
}

foreach ($RequiredFile in @('index.html', 'ModuleDependencies.svg')) {
    if (-not $FilePaths.Contains((Join-Path $SiteDirectory $RequiredFile))) {
        throw "Missing documentation output: $RequiredFile"
    }
}
[xml]$ModuleGraph = Get-Content -LiteralPath (Join-Path $SiteDirectory 'ModuleDependencies.svg') -Raw
if ((Get-Content -LiteralPath (Join-Path $SiteDirectory 'index.html') -Raw) -match '@@[A-Za-z]+@@') {
    throw 'Unresolved placeholders remain in the wiki home page.'
}
if ($ModuleGraph.DocumentElement.LocalName -ne 'svg') {
    throw 'ModuleDependencies.svg is not a valid SVG document.'
}
foreach ($GraphPattern in @('*__inherit__graph.svg', '*__incl.svg')) {
    if (-not ($Files | Where-Object { $_.Name -like $GraphPattern } | Select-Object -First 1)) {
        throw "Missing generated Graphviz graph: $GraphPattern"
    }
}

$MissingTargets = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)
$HtmlFiles = @($Files | Where-Object { $_.Extension -eq '.html' })
foreach ($HtmlFile in $HtmlFiles) {
    $Content = Get-Content -LiteralPath $HtmlFile.FullName -Raw
    foreach ($Link in [System.Text.RegularExpressions.Regex]::Matches($Content, '\b(?:href|src)\s*=\s*["'']([^"'']+)["'']', [System.Text.RegularExpressions.RegexOptions]::IgnoreCase)) {
        $Reference = [System.Net.WebUtility]::HtmlDecode($Link.Groups[1].Value)
        if ($Reference -match '^(?:[a-z][a-z0-9+.-]*:|//|#)' -or $Reference -match '\$\{|\{\{|<%|%>') {
            continue
        }
        $Reference = [System.Uri]::UnescapeDataString(($Reference -split '[?#]', 2)[0])
        if ([string]::IsNullOrWhiteSpace($Reference)) {
            continue
        }
        $BaseDirectory = $HtmlFile.DirectoryName
        if ($Reference.StartsWith('/')) {
            $BaseDirectory = $SiteDirectory
            $Reference = $Reference.TrimStart('/')
        }
        if ($Reference.EndsWith('/')) {
            $Reference += 'index.html'
        }
        $Target = [System.IO.Path]::GetFullPath((Join-Path $BaseDirectory $Reference))
        if (-not $FilePaths.Contains($Target)) {
            [void]$MissingTargets.Add("$($HtmlFile.Name): $Reference")
        }
    }
}
if ($MissingTargets.Count -gt 0) {
    throw "Found $($MissingTargets.Count) missing or incorrectly cased local links:`n$(($MissingTargets | Select-Object -First 20) -join "`n")"
}

[xml]$Index = Get-Content -LiteralPath (Join-Path $XmlDirectory 'index.xml') -Raw
$RequiredClasses = @('UObject', 'UWorld', 'UEngine', 'AActor', 'UClass', 'FProperty')
$ClassDocuments = @{}
$Compounds = @($Index.SelectNodes('/doxygenindex/compound'))
foreach ($Compound in $Compounds) {
    $ReferenceId = $Compound.GetAttribute('refid')
    $CompoundName = $Compound.SelectSingleNode('name').InnerText
    [xml]$Document = Get-Content -LiteralPath (Join-Path $XmlDirectory "$ReferenceId.xml") -Raw
    foreach ($Location in $Document.SelectNodes('//location/@file | //location/@bodyfile')) {
        if ($Location.Value -match '(^|[/\\])(?:Externals|ImGui|SimpleMath|vcpkg|Build|\.tools)([/\\]|$)') {
            throw "Excluded source was indexed: $($Location.Value)"
        }
    }
    if ($Document.SelectNodes('//memberdef[@kind="function"]/name[text()="ReflectProperty" or text()="ReflectTypes"]').Count -gt 0) {
        throw "Reflection macros were incorrectly indexed as functions in $CompoundName."
    }
    if ($Compound.GetAttribute('kind') -in @('class', 'struct') -and $CompoundName -cin $RequiredClasses) {
        if (-not $FilePaths.Contains((Join-Path $SiteDirectory "api/$ReferenceId.html"))) {
            throw "Missing API page for $CompoundName."
        }
        $ClassDocuments[$CompoundName] = $Document
    }
}
foreach ($ClassName in $RequiredClasses) {
    if (-not $ClassDocuments.ContainsKey($ClassName)) {
        throw "Required API class was not indexed: $ClassName"
    }
}
if (-not $ClassDocuments['UObject'].SelectSingleNode('/doxygen/compounddef/sectiondef/memberdef[@kind="function" and name="StaticClass"]')) {
    throw 'UObject::StaticClass was not extracted from the reflection macros.'
}
if (-not $ClassDocuments['UWorld'].SelectSingleNode('/doxygen/compounddef/basecompoundref[text()="UObject"]')) {
    throw 'The UWorld inheritance relationship with UObject was not extracted.'
}

Write-Host "Verified $($HtmlFiles.Count) HTML pages, $($Compounds.Count) XML compounds, local links, reflection APIs, and Graphviz output."
