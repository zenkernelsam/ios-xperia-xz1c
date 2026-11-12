# Host-only inventory. Prints identifiers for conflicting legacy flows, never command lines.
$ErrorActionPreference = 'Stop'
$legacy = 'retry_rpm_loop\.sh|auto_resume_v637\.sh|post_rpm_gl5\.sh|read_com(?:_once|_loop|_live|_follow|_auto)?\.ps1|com_supervisor\.ps1|run_v238\.sh'
$found = @(Get-CimInstance Win32_Process | Where-Object {
    $_.ProcessId -ne $PID -and (
        $_.Name -match '^(fastboot|xperable)\.exe$' -or
        ($_.Name -match '^(bash|python|python3|powershell|pwsh)\.exe$' -and $_.CommandLine -match $legacy)
    )
} | ForEach-Object { @{pid=$_.ProcessId; name=$_.Name; reason='conflicting_test_or_capture_process'} })
ConvertTo-Json -InputObject $found -Compress
