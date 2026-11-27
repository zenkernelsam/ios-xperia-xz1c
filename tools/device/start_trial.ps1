param(
    [Parameter(Mandatory=$true)][string]$Request,
    [Parameter(Mandatory=$true)][string]$Directory
)
# Start one regular run_trial supervisor independently of the caller's terminal.
# Device validation, one-shot dispatch, its owned worker and return policy stay
# inside run_trial. This launcher never opens USB/COM or retries a child.
$ErrorActionPreference = 'Stop'
$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..')).TrimEnd('\')
$tempRoot = [IO.Path]::GetFullPath($env:TEMP).TrimEnd('\')
$requestPath = (Resolve-Path -LiteralPath $Request).ProviderPath
$launchRoot = [IO.Path]::GetFullPath($Directory).TrimEnd('\')
if (-not $launchRoot.StartsWith($tempRoot + '\',[StringComparison]::OrdinalIgnoreCase)) { throw 'Launch output must be a new Windows TEMP directory' }
if (Test-Path -LiteralPath $launchRoot) { throw 'Launch output already exists; no relaunch' }
$requestBytes = [IO.File]::ReadAllBytes($requestPath)
$spec = [Text.Encoding]::UTF8.GetString($requestBytes).TrimStart([char]0xfeff) | ConvertFrom-Json
$allowed = @('bundle','directory','private_evidence','owner_evidence','ready_evidence','rpm_evidence','cpu_evidence','gles_evidence','native_evidence','png_evidence','wait_before_guard','live')
foreach ($property in $spec.PSObject.Properties.Name) { if ($property -notin $allowed) { throw "Unknown run_trial field: $property" } }
if ($spec.live -isnot [bool] -or -not $spec.live) { throw 'An explicit live:true request is required' }
$bundleValue = [string]$spec.bundle
$bundlePath = if ([IO.Path]::IsPathRooted($bundleValue)) { [IO.Path]::GetFullPath($bundleValue).TrimEnd('\') } else { [IO.Path]::GetFullPath((Join-Path $repoRoot $bundleValue)).TrimEnd('\') }
if (-not $bundlePath.StartsWith($repoRoot + '\build\',[StringComparison]::OrdinalIgnoreCase)) { throw 'Bundle must be inside this formal build directory' }
if (-not (Test-Path -LiteralPath (Join-Path $bundlePath 'artifacts.json'))) { throw 'Bundle artifact is absent' }
$runRoot = [IO.Path]::GetFullPath([string]$spec.directory).TrimEnd('\')
if (-not $runRoot.StartsWith($tempRoot + '\',[StringComparison]::OrdinalIgnoreCase)) { throw 'Run output must be Windows TEMP' }
if (Test-Path -LiteralPath $runRoot) { throw 'Run output exists; never retry an old run' }
if ($runRoot -eq $launchRoot -or $runRoot.StartsWith($launchRoot + '\',[StringComparison]::OrdinalIgnoreCase) -or $launchRoot.StartsWith($runRoot + '\',[StringComparison]::OrdinalIgnoreCase)) { throw 'Run and launch output must be separate directories' }
function Quote-Argument([string]$value) {
    if (-not $value -or $value.Contains('"') -or $value.Contains("`n") -or $value.Contains("`r") -or $value.Contains([char]0)) { throw 'Invalid command argument' }
    if ($value.EndsWith('\')) { throw 'Trailing slash in command argument' }
    return '"' + $value + '"'
}
function Hash-Path([string]$path) {
    $algorithm = [Security.Cryptography.SHA256]::Create()
    $stream = [IO.File]::OpenRead($path)
    try { return [BitConverter]::ToString($algorithm.ComputeHash($stream)).Replace('-','').ToLowerInvariant() } finally { $stream.Dispose(); $algorithm.Dispose() }
}
$runner = Join-Path $repoRoot 'tools/device/run_trial.py'
$arguments = @('-B',(Quote-Argument $runner),'--bundle',(Quote-Argument $bundlePath),'--directory',(Quote-Argument $runRoot))
foreach ($key in @('private_evidence','owner_evidence','ready_evidence','rpm_evidence','cpu_evidence','gles_evidence','native_evidence','png_evidence')) {
    $value = $spec.$key
    if ($null -ne $value) { $arguments += @('--' + $key.Replace('_','-'),(Quote-Argument ([string]$value))) }
}
if ($null -ne $spec.wait_before_guard) {
    $wait = [int]$spec.wait_before_guard
    if ($wait -ne 0 -and ($wait -lt 30 -or $wait -gt 120)) { throw 'Invalid pre-guard wait' }
    $arguments += @('--wait-before-guard',[string]$wait)
}
New-Item -ItemType Directory -Path $launchRoot | Out-Null
[IO.File]::WriteAllBytes((Join-Path $launchRoot 'request.json'),$requestBytes)
$spec.bundle = $bundlePath
$spec.directory = $runRoot
$normalizedRequest = Join-Path $launchRoot 'normalized-request.json'
$spec | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $normalizedRequest -Encoding utf8
$supervisor = Join-Path $repoRoot 'tools/device/background_trial.py'
$python = @(Get-Command python -CommandType Application)[0].Source
$arguments = @('-B',(Quote-Argument $supervisor),'--request',(Quote-Argument $normalizedRequest),'--directory',(Quote-Argument $launchRoot))
$record = @{
    launch_attempts=1; retries=0; device_operations_by_launcher=0
    request_sha256=(Hash-Path $requestPath)
    launcher_sha256=(Hash-Path $PSCommandPath)
    runner_sha256=(Hash-Path $runner)
    supervisor_sha256=(Hash-Path $supervisor)
    bundle=$bundlePath; run_directory=$runRoot; window_style='Hidden'
}
$record | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $launchRoot 'launch.json') -Encoding utf8
$child = Start-Process -FilePath $python -ArgumentList $arguments -WorkingDirectory $repoRoot -WindowStyle Hidden -PassThru
$record.pid = $child.Id
try { $record.start_time_utc = $child.StartTime.ToUniversalTime().ToString('o') } catch { $record.start_time_utc = $null; $record.process_query_error = $_.Exception.Message }
$record | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $launchRoot 'launch.json') -Encoding utf8
$record | ConvertTo-Json -Depth 4 -Compress
