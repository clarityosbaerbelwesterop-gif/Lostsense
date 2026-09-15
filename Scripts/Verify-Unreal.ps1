param(
    [Parameter(Mandatory = $true)]
    [string]$EngineRoot,

    [ValidateSet("DebugGame", "Development", "Shipping")]
    [string]$Configuration = "Development",

    [switch]$SkipHeadlessSmoke,
    [switch]$Package
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$RepositoryRoot = Split-Path -Parent $PSScriptRoot
$ProjectFile = Join-Path $RepositoryRoot "Lostsense.uproject"
$Ubt = Join-Path $EngineRoot "Engine/Binaries/DotNET/UnrealBuildTool/UnrealBuildTool.exe"
$EditorCmd = Join-Path $EngineRoot "Engine/Binaries/Win64/UnrealEditor-Cmd.exe"
$RunUat = Join-Path $EngineRoot "Engine/Build/BatchFiles/RunUAT.bat"
$Artifacts = Join-Path $RepositoryRoot "Artifacts/Unreal"

function Assert-Path([string]$Path, [string]$Description) {
    if (-not (Test-Path $Path)) {
        throw "$Description not found: $Path"
    }
}

function Invoke-Checked([string]$Executable, [string[]]$Arguments) {
    Write-Host "> $Executable $($Arguments -join ' ')"
    & $Executable @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed with exit code $LASTEXITCODE: $Executable"
    }
}

Assert-Path $ProjectFile "Lostsense project"
Assert-Path $Ubt "UnrealBuildTool"
Assert-Path $EditorCmd "UnrealEditor-Cmd"
Assert-Path $RunUat "RunUAT"

$CommonBuildArguments = @(
    "Win64",
    $Configuration,
    "-Project=$ProjectFile",
    "-WaitMutex",
    "-NoHotReloadFromIDE"
)

Write-Host "=== UBT: LostsenseGame ==="
Invoke-Checked $Ubt (@("LostsenseGame") + $CommonBuildArguments)

Write-Host "=== UBT: LostsenseGameEditor ==="
Invoke-Checked $Ubt (@("LostsenseGameEditor") + $CommonBuildArguments)

if (-not $SkipHeadlessSmoke) {
    Write-Host "=== Headless startup smoke ==="
    Invoke-Checked $EditorCmd @(
        $ProjectFile,
        "/Engine/Maps/Entry",
        "-game",
        "-nullrhi",
        "-unattended",
        "-nosplash",
        "-nop4",
        "-NoSound",
        "-ExecCmds=quit"
    )
}

if ($Package) {
    Write-Host "=== BuildCookRun: Development package ==="
    New-Item -ItemType Directory -Force -Path $Artifacts | Out-Null
    Invoke-Checked $RunUat @(
        "BuildCookRun",
        "-project=$ProjectFile",
        "-noP4",
        "-platform=Win64",
        "-clientconfig=$Configuration",
        "-build",
        "-cook",
        "-stage",
        "-pak",
        "-archive",
        "-archivedirectory=$Artifacts",
        "-map=/Engine/Maps/Entry",
        "-utf8output"
    )
}

Write-Host "Unreal verification completed successfully."
