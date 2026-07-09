$ShaderCross = "shadercross"
$Fallback = "C:\Users\ilyas\Downloads\SDL3_shadercross-VC-x64\SDL3_shadercross-3.0.0-windows-VC-x64\SDL3_shadercross-3.0.0-windows-VC-x64\bin\shadercross.exe"

if (-not (Get-Command $ShaderCross -ErrorAction SilentlyContinue)) {
    if (Test-Path $Fallback) {
        $ShaderCross = $Fallback
    } else {
        Write-Error "shadercross not found in PATH or at fallback path"
        exit 1
    }
}

$OutDirs = @{
    "SPIRV" = "compiled/SPIRV"
    "MSL"   = "compiled/MSL"
    "DXIL"  = "compiled/DXIL"
}

$SourceDir = Split-Path -Parent $MyInvocation.MyCommand.Path

Get-ChildItem "$SourceDir\*.hlsl" | ForEach-Object {
    $stem = $_.BaseName
    foreach ($fmt in $OutDirs.Keys) {
        $outDir = "$SourceDir/$($OutDirs[$fmt])"
        $outFile = "$outDir/$stem.$($fmt.ToLower())"
        & $ShaderCross $_.FullName -o $outFile
        Write-Output "$($_.Name) -> $outFile"
    }
}
