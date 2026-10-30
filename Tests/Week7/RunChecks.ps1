param([ValidateSet('Debug', 'Release')][string]$Configuration = 'Debug')

$ErrorActionPreference = 'Stop'
$Workspace = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$Locator = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$Installation = & $Locator -latest -products '*' -requires Microsoft.Component.MSBuild -property installationPath
$MSBuild = Join-Path $Installation 'MSBuild/Current/Bin/MSBuild.exe'

Push-Location -LiteralPath $Workspace
try {
    $Premake = Join-Path $Workspace '.tools/premake-5.0.0-beta8/premake5.exe'
    & $Premake vs2026
    if ($LASTEXITCODE -ne 0) {
        throw 'Project generation failed.'
    }

    [xml]$Project = Get-Content -LiteralPath 'Macaw.vcxproj' -Raw
    $Namespace = [Xml.XmlNamespaceManager]::new($Project.NameTable)
    $Namespace.AddNamespace('MSBuild', 'http://schemas.microsoft.com/developer/msbuild/2003')
    foreach ($Node in @($Project.SelectNodes('//MSBuild:ClCompile[@Include] | //MSBuild:ClInclude[@Include] | //MSBuild:ResourceCompile[@Include] | //MSBuild:PreBuildEvent', $Namespace))) {
        $Node.ParentNode.RemoveChild($Node) | Out-Null
    }
    foreach ($Node in $Project.SelectNodes('//MSBuild:TargetName | //MSBuild:RootNamespace', $Namespace)) {
        $Node.InnerText = 'Week7Checks'
    }
    foreach ($Node in $Project.SelectNodes('//MSBuild:IntDir', $Namespace)) {
        $Node.InnerText = $Node.InnerText.Replace('Intermediate\Macaw\', 'Intermediate\Week7\')
    }
    foreach ($Node in $Project.SelectNodes('//MSBuild:SubSystem', $Namespace)) {
        $Node.InnerText = 'Console'
    }
    foreach ($Node in $Project.SelectNodes('//MSBuild:ClCompile/MSBuild:AdditionalIncludeDirectories', $Namespace)) {
        $Node.InnerText = 'Source\Renderer\Private;' + $Node.InnerText
    }
    $Project.SelectSingleNode('//MSBuild:ProjectGuid', $Namespace).InnerText = '{279351A0-6D41-4BD8-9952-A935978172EE}'
    $Group = $Project.CreateElement('ItemGroup', $Namespace.LookupNamespace('MSBuild'))
    $Compile = $Project.CreateElement('ClCompile', $Namespace.LookupNamespace('MSBuild'))
    $Compile.SetAttribute('Include', 'Tests\Week7\Week7Checks.cpp')
    $Group.AppendChild($Compile) | Out-Null
    $Project.Project.AppendChild($Group) | Out-Null
    $Project.Save((Join-Path $Workspace 'Week7Checks.vcxproj'))

    & $MSBuild 'Week7Checks.vcxproj' /m "/p:Configuration=$Configuration" /p:Platform=x64 "/p:SolutionDir=$Workspace\" /v:minimal /nologo
    if ($LASTEXITCODE -ne 0) {
        throw 'Week 7 integration check build failed.'
    }
    & (Join-Path $Workspace "Binaries/$Configuration/Week7Checks.exe")
    if ($LASTEXITCODE -ne 0) {
        throw 'Week 7 integration checks failed.'
    }
} finally {
    Pop-Location
}
