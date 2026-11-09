param([Parameter(Mandatory = $true, Position = 0)][string]$SourcePath, [string]$OutputPath = '')

$ErrorActionPreference = 'Stop'
$Source = [IO.File]::ReadAllText($SourcePath)
$MaskPattern = '(?ms)(?:u8|u|U|L)?R"(?<Delimiter>[^ ()\\\t\r\n]{0,16})\(.*?\)\k<Delimiter>"|"(?:\\.|[^"\\])*"|(?<![\w])(?:u8|u|U|L)?''(?:\\.|[^''\\])*''|/\*.*?\*/|//(?:[^\r\n\\]|\\[^\r\n]|\\\r?\n)*|^[\t ]*\#(?:[^\r\n]*\\\r?\n)*[^\r\n]*'
$Mask = [regex]::Replace($Source, $MaskPattern, { param($Match) [regex]::Replace($Match.Value, '[^\r\n]', ' ') })
$Tokens = [regex]::Matches($Mask, '[A-Za-z_]\w*|::|[{};=]')
$Scopes = [Collections.Generic.Stack[string]]::new()
$Namespace = ''
$Output = $Source.ToCharArray()

for ($Index = 0; $Index -lt $Tokens.Count; $Index++) {
    $Token = $Tokens[$Index]

    if ($Token.Value -eq 'namespace' -and ($Index -eq 0 -or $Tokens[$Index - 1].Value -ne 'using')) {
        $Next = $Index + 1
        $Name = ''

        while ($Next -lt $Tokens.Count -and ($Tokens[$Next].Value -match '^[A-Za-z_]\w*$' -or $Tokens[$Next].Value -eq '::')) {
            $Name += $Tokens[$Next].Value
            $Next++
        }

        if ($Next -lt $Tokens.Count -and $Tokens[$Next].Value -eq '{') {
            $Scopes.Push($Namespace)
            $Namespace = if ($Name.Length -eq 0) { $Namespace } elseif ($Namespace.Length -eq 0) { $Name } else { $Namespace + '::' + $Name }
            $Index = $Next
            continue
        }
    }

    if ($Token.Value -eq '{') {
        $Scopes.Push($Namespace)
    } elseif ($Token.Value -eq '}') {
        if ($Scopes.Count -gt 0) {
            $Namespace = $Scopes.Pop()
        }
    } elseif ($Namespace.Length -gt 0) {
        $Prefix = $Namespace + '::'

        if ($Token.Index + $Prefix.Length -le $Mask.Length -and [string]::CompareOrdinal($Mask, $Token.Index, $Prefix, 0, $Prefix.Length) -eq 0) {
            $Remainder = $Mask.Substring($Token.Index + $Prefix.Length)
            $Declarator = [regex]::IsMatch($Remainder, '^(?:[A-Za-z_]\w*::)*~?[A-Za-z_]\w*\s*\(')
            $TypeDefinition = $Index -gt 0 -and $Tokens[$Index - 1].Value -in @('class', 'struct', 'union')

            if ($Declarator -or $TypeDefinition) {
                for ($Offset = 0; $Offset -lt $Prefix.Length; $Offset++) {
                    $Output[$Token.Index + $Offset] = ' '
                }
            }
        }
    }
}

$FilteredSource = [string]::new($Output)

if ($OutputPath.Length -gt 0) {
    [IO.File]::WriteAllText($OutputPath, $FilteredSource, [Text.UTF8Encoding]::new($false))
} else {
    [Console]::OutputEncoding = [Text.UTF8Encoding]::new($false)
    [Console]::Write($FilteredSource)
}
