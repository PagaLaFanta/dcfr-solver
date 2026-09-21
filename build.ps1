# -----------------------------------------------------------------------------
#  DCFR River Solver -- Windows build script
#  Detects whatever C++17 toolchain is available and builds solver.exe.
#
#    .\build.ps1              build only
#    .\build.ps1 -Gui         build and launch the web UI
#    .\build.ps1 -Console     build and open the text console
#    .\build.ps1 -Test        build and run the evaluator unit tests
# -----------------------------------------------------------------------------
[CmdletBinding()]
param(
    [switch]$Gui,
    [switch]$Console,
    [switch]$Test,
    [switch]$Bench
)

$ErrorActionPreference = 'Stop'
Set-Location -Path $PSScriptRoot

$src = Join-Path $PSScriptRoot 'src\main.cpp'
$exe = Join-Path $PSScriptRoot 'solver.exe'
if (-not (Test-Path $src)) { throw "src\main.cpp not found in $PSScriptRoot" }

function Find-Tool([string]$name) {
    $c = Get-Command $name -ErrorAction SilentlyContinue
    if ($c) { return $c.Source }
    $candidates = @(
        "$env:LOCALAPPDATA\Microsoft\WinGet\Links\$name",
        "C:\mingw64\bin\$name",
        "C:\msys64\ucrt64\bin\$name",
        "C:\msys64\mingw64\bin\$name",
        "C:\Program Files\LLVM\bin\$name"
    )
    foreach ($p in $candidates) { if (Test-Path $p) { return $p } }
    # winget portable packages (e.g. WinLibs) put the toolchain under a
    # versioned directory and only add it to the *persisted* user PATH, so a
    # shell started before the install will not see it. Look there too.
    $pkgRoot = "$env:LOCALAPPDATA\Microsoft\WinGet\Packages"
    if (Test-Path $pkgRoot) {
        $hit = Get-ChildItem -Path $pkgRoot -Recurse -Filter $name -File `
                             -Depth 4 -ErrorAction SilentlyContinue |
               Select-Object -First 1
        if ($hit) { return $hit.FullName }
    }
    return $null
}

$gpp     = Find-Tool 'g++.exe'
$clangpp = Find-Tool 'clang++.exe'
$clexe   = Find-Tool 'cl.exe'

# -static matters on Windows: Git for Windows, Qt, and plenty of other tools
# ship their own libstdc++-6.dll in PATH. A dynamically linked exe can pick up
# one built by a different GCC and crash on an ABI mismatch (it segfaults inside
# the std::ifstream constructor, of all places). Static linking makes the
# binary self-contained and immune to whatever happens to be on PATH.
# ws2_32 is the socket library the local web UI listens on.
if ($gpp) {
    Write-Host "Compiler: $gpp" -ForegroundColor Cyan
    & $gpp -std=c++17 -O3 -march=native -Wall -Wextra -Isrc -static -o $exe $src -lws2_32
} elseif ($clangpp) {
    Write-Host "Compiler: $clangpp" -ForegroundColor Cyan
    & $clangpp -std=c++17 -O3 -march=native -Wall -Wextra -Isrc -static -o $exe $src -lws2_32
} elseif ($clexe) {
    Write-Host "Compiler: $clexe" -ForegroundColor Cyan
    & $clexe /nologo /std:c++17 /O2 /W4 /EHsc /permissive- /Isrc $src /Fe:$exe /link ws2_32.lib
    Remove-Item -Path (Join-Path $PSScriptRoot 'main.obj') -ErrorAction SilentlyContinue
} else {
    Write-Host "No C++ compiler found." -ForegroundColor Red
    Write-Host "Install one with:  winget install BrechtSanders.WinLibs.POSIX.UCRT"
    Write-Host "then open a new shell so PATH picks up g++."
    exit 1
}

if ($LASTEXITCODE -ne 0) { throw "Build failed (exit $LASTEXITCODE)" }
Write-Host "Built $exe" -ForegroundColor Green

if ($Test) {
    $tsrc = Join-Path $PSScriptRoot 'src\test_eval.cpp'
    $texe = Join-Path $PSScriptRoot 'test_eval.exe'
    if ($gpp)          { & $gpp -std=c++17 -O2 -Wall -Wextra -Isrc -static -o $texe $tsrc }
    elseif ($clangpp)  { & $clangpp -std=c++17 -O2 -Wall -Wextra -Isrc -static -o $texe $tsrc }
    else               { & $clexe /nologo /std:c++17 /O2 /EHsc /Isrc $tsrc /Fe:$texe }
    if ($LASTEXITCODE -ne 0) { throw "Test build failed" }
    & $texe
} elseif ($Bench) {
    $bsrc = Join-Path $PSScriptRoot 'src\test_solve.cpp'
    $bexe = Join-Path $PSScriptRoot 'test_solve.exe'
    if ($gpp)         { & $gpp -std=c++17 -O3 -march=native -Isrc -static -o $bexe $bsrc }
    elseif ($clangpp) { & $clangpp -std=c++17 -O3 -march=native -Isrc -static -o $bexe $bsrc }
    else              { & $clexe /nologo /std:c++17 /O2 /EHsc /Isrc $bsrc /Fe:$bexe }
    if ($LASTEXITCODE -ne 0) { throw "Bench build failed" }
    & $bexe turn
} elseif ($Gui) {
    & $exe --gui
} elseif ($Console) {
    & $exe
}
