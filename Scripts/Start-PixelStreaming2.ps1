param(
    [Parameter(Mandatory = $true)]
    [string]$Executable,

    [string]$SignallingUrl = "ws://127.0.0.1:8888",
    [ValidateRange(640, 7680)]
    [int]$Width = 1920,
    [ValidateRange(360, 4320)]
    [int]$Height = 1080,
    [switch]$Windowed
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path -LiteralPath $Executable -PathType Leaf)) {
    throw "Packaged Lostsense executable not found: $Executable"
}

if ($SignallingUrl -notmatch '^wss?://') {
    throw "SignallingUrl must use ws:// or wss://"
}

$resolvedExecutable = (Resolve-Path -LiteralPath $Executable).Path
$arguments = @(
    "-PixelStreamingURL=$SignallingUrl",
    "-ForceRes",
    "-ResX=$Width",
    "-ResY=$Height",
    "-AudioMixer",
    "-Unattended",
    "-StdOut",
    "-FullStdOutLogOutput"
)

if (-not $Windowed) {
    $arguments += "-RenderOffscreen"
}

Write-Host "Starting Lostsense Pixel Streaming 2 runtime"
Write-Host "Executable: $resolvedExecutable"
Write-Host "Signalling: $SignallingUrl"
Write-Host "Resolution: ${Width}x${Height}"
Write-Host "Mode: $(if ($Windowed) { 'windowed' } else { 'offscreen' })"

$process = Start-Process -FilePath $resolvedExecutable -ArgumentList $arguments -PassThru -NoNewWindow
Write-Host "Lostsense process started with PID $($process.Id)"
Write-Host "The signalling/web infrastructure must be running separately and must match Unreal Engine 5.8."

$process.WaitForExit()
if ($process.ExitCode -ne 0) {
    throw "Lostsense Pixel Streaming process exited with code $($process.ExitCode)"
}
