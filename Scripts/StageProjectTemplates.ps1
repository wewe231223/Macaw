param(
    [Parameter(Mandatory = $true)][string]$RuntimeDirectory,
    [Parameter(Mandatory = $true)][string]$WorkspaceRoot
)

$ErrorActionPreference = 'Stop'
$RuntimePath = [IO.Path]::GetFullPath($RuntimeDirectory)
$WorkspacePath = [IO.Path]::GetFullPath($WorkspaceRoot)
$SourcePath = [IO.Path]::GetFullPath((Join-Path $WorkspacePath 'Templates'))
$DestinationPath = [IO.Path]::GetFullPath((Join-Path $RuntimePath 'Templates'))
$Comparison = [StringComparison]::OrdinalIgnoreCase

if (-not (Test-Path -LiteralPath (Join-Path $SourcePath 'Project/Template.mproject') -PathType Leaf)) {
    throw "The project templates are missing in $SourcePath."
}

if ($DestinationPath.Equals($SourcePath, $Comparison) -or $DestinationPath.StartsWith($SourcePath + [IO.Path]::DirectorySeparatorChar, $Comparison) -or $SourcePath.StartsWith($DestinationPath + [IO.Path]::DirectorySeparatorChar, $Comparison) -or -not $DestinationPath.StartsWith($RuntimePath.TrimEnd([IO.Path]::DirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar, $Comparison)) {
    throw "The runtime templates must be separate from the source templates: $DestinationPath."
}

& robocopy.exe $SourcePath $DestinationPath /MIR /XJ /R:1 /W:1 /NFL /NDL /NJH /NJS /NP

if ($LASTEXITCODE -ge 8) {
    throw "Project template staging failed with robocopy exit code $LASTEXITCODE."
}

exit 0
