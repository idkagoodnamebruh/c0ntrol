param(
    [Parameter(Mandatory = $true)][string]$WorkDirectory,
    [Parameter(Mandatory = $true)][string]$OutputDirectory,
    [Parameter(Mandatory = $true)][string]$OpenCvRoot
)

$ErrorActionPreference = "Stop"
$MediaPipeTag = "v0.10.26"
$MediaPipeCommit = "80ae8afbd03465b0d6d9f9e874f8cacf093d23e9"
$RepositoryRoot = Split-Path -Parent $PSScriptRoot
$SourceDirectory = Join-Path $WorkDirectory "mediapipe"

if (Test-Path -LiteralPath $SourceDirectory) {
    throw "Refusing to overwrite existing MediaPipe checkout: $SourceDirectory"
}

New-Item -ItemType Directory -Force -Path $WorkDirectory | Out-Null
New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
git clone --branch $MediaPipeTag --depth 1 `
    https://github.com/google-ai-edge/mediapipe.git $SourceDirectory
if ($LASTEXITCODE -ne 0) { throw "MediaPipe clone failed" }

$ActualCommit = (git -C $SourceDirectory rev-parse HEAD).Trim()
if ($ActualCommit -ne $MediaPipeCommit) {
    throw "MediaPipe commit mismatch: $ActualCommit"
}

Copy-Item -Recurse `
    (Join-Path $RepositoryRoot "third_party/mediapipe_bridge") `
    (Join-Path $SourceDirectory "c0ntrol_bridge")
Copy-Item -LiteralPath `
    (Join-Path $RepositoryRoot "third_party/mediapipe_patches/opencv_windows.BUILD") `
    -Destination (Join-Path $SourceDirectory "third_party/opencv_windows.BUILD")

if (-not (Test-Path -LiteralPath (Join-Path $OpenCvRoot "include/opencv4/opencv2"))) {
    throw "Verified vcpkg OpenCV headers are missing: $OpenCvRoot"
}
$WorkspacePath = Join-Path $SourceDirectory "WORKSPACE"
$Workspace = Get-Content -LiteralPath $WorkspacePath -Raw
$EscapedOpenCvRoot = $OpenCvRoot.Replace("\", "\\")
$Workspace = $Workspace.Replace(
    'path = "C:\\opencv\\build",',
    "path = `"$EscapedOpenCvRoot`",")
Set-Content -LiteralPath $WorkspacePath -Value $Workspace -NoNewline

$Bazel = Get-Command bazelisk -ErrorAction SilentlyContinue
if (-not $Bazel) { $Bazel = Get-Command bazel -ErrorAction SilentlyContinue }
if (-not $Bazel) { throw "bazelisk or bazel is required" }
$Bash = Get-Command bash -ErrorAction SilentlyContinue
if ($Bash) { $env:BAZEL_SH = $Bash.Source }
$Python = (Get-Command python -ErrorAction Stop).Source

Push-Location $SourceDirectory
try {
    & $Bazel.Source build -c opt `
        --define=MEDIAPIPE_DISABLE_GPU=1 `
        --conlyopt=/std:c11 `
        --conlyopt=/experimental:c11atomics `
        --action_env=PYTHON_BIN_PATH="$Python" `
        --repo_env=HERMETIC_PYTHON_VERSION=3.12 `
        //c0ntrol_bridge:libc0ntrol_mediapipe_bridge.so
    if ($LASTEXITCODE -ne 0) { throw "MediaPipe bridge build failed" }
} finally {
    Pop-Location
}

$BridgeDirectory = Join-Path $SourceDirectory "bazel-bin/c0ntrol_bridge"
$RuntimeCandidates = Get-ChildItem -LiteralPath $BridgeDirectory -File |
    Where-Object { $_.Name -match '\.(dll|so)$' }
$ImportCandidates = Get-ChildItem -LiteralPath $BridgeDirectory -File |
    Where-Object { $_.Name -match '\.(lib|if\.lib)$' }
if ($RuntimeCandidates.Count -ne 1 -or $ImportCandidates.Count -lt 1) {
    Get-ChildItem -LiteralPath $BridgeDirectory
    throw "Could not identify one bridge runtime and its import library"
}

$Runtime = $RuntimeCandidates[0]
$Import = $ImportCandidates[0]
Copy-Item -LiteralPath $Runtime.FullName -Destination $OutputDirectory
Copy-Item -LiteralPath $Import.FullName -Destination $OutputDirectory

Write-Output "MEDIAPIPE_SOURCE=$SourceDirectory"
Write-Output "MEDIAPIPE_COMMIT=$ActualCommit"
Write-Output "MEDIAPIPE_BRIDGE_RUNTIME=$(Join-Path $OutputDirectory $Runtime.Name)"
Write-Output "MEDIAPIPE_BRIDGE_IMPORT=$(Join-Path $OutputDirectory $Import.Name)"
