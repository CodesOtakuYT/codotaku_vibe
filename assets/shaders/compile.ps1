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
    "SPIRV" = @{ dir = "compiled/SPIRV"; ext = "spv" }
    "MSL"   = @{ dir = "compiled/MSL";   ext = "msl" }
    "DXIL"  = @{ dir = "compiled/DXIL";  ext = "dxil" }
}

$SourceDir = Split-Path -Parent $MyInvocation.MyCommand.Path

foreach ($fmt in $OutDirs.Keys) {
    $outDir = "$SourceDir/$($OutDirs[$fmt].dir)"
    if (-not (Test-Path $outDir)) { New-Item -ItemType Directory -Path $outDir | Out-Null }
}

Get-ChildItem "$SourceDir\*.hlsl" | ForEach-Object {
    $stem = $_.BaseName
    foreach ($fmt in $OutDirs.Keys) {
        $info = $OutDirs[$fmt]
        $outDir = "$SourceDir/$($info.dir)"
        $outFile = "$outDir/$stem.$($info.ext)"
        & $ShaderCross $_.FullName -o $outFile
        Write-Output "$($_.Name) -> $outFile"
    }
}
