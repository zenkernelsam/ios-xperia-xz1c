# Read the lilac USB CDC console (COMx) into a file; save partial data on error.
# usage: powershell -ExecutionPolicy Bypass -File research/read_com.ps1 -Port COM17 -Seconds 8 -Out <log>
param(
    [string]$Port = "COM17",
    [int]$Seconds = 8,
    [string]$Out = "C:\Users\Cisco He\AppData\Local\Temp\com.log"
)
$p = $null
$sb = New-Object Text.StringBuilder
$err = ""
try {
    $p = New-Object System.IO.Ports.SerialPort $Port,115200,None,8,one
    $p.ReadBufferSize = 1048576
    $p.DtrEnable = $true
    $p.Open()
    $sw = [Diagnostics.Stopwatch]::StartNew()
    while ($sw.Elapsed.TotalSeconds -lt $Seconds) {
        try {
            $s = $p.ReadExisting()
            if ($s) { [void]$sb.Append($s) }
        } catch { $err = $_.Exception.Message; break }
        Start-Sleep -Milliseconds 30
    }
} catch {
    $err = $_.Exception.Message
} finally {
    if ($p) { try { $p.Close() } catch {} }
    $sb.ToString() | Out-File -Encoding utf8 $Out
    Write-Output ("bytes=" + $sb.Length + " err=" + $err)
}
