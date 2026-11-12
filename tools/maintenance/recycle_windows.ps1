param(
    [Parameter(Mandatory=$true)][string]$Plan,
    [Parameter(Mandatory=$true)][string]$Journal
)
$ErrorActionPreference = 'Stop'
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
[ComImport, Guid("43826D1E-E718-42EE-BC55-A1E261C37BFE"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
public interface LilacShellItem {
    void BindToHandler(IntPtr a, ref Guid b, ref Guid c, out IntPtr d);
    void GetParent(out LilacShellItem item);
    void GetDisplayName(uint name, out IntPtr value);
    void GetAttributes(uint mask, out uint attrs);
    void Compare(LilacShellItem item, uint hint, out int order);
}
[ComImport, Guid("947AAB5F-0A5C-4C13-B4D6-4BF7836FC9F8"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
public interface LilacFileOperation {
    void Advise(IntPtr sink, out uint cookie);
    void Unadvise(uint cookie);
    void SetOperationFlags(uint flags);
    void SetProgressMessage([MarshalAs(UnmanagedType.LPWStr)] string message);
    void SetProgressDialog(IntPtr dialog);
    void SetProperties(IntPtr properties);
    void SetOwnerWindow(uint hwnd);
    void ApplyPropertiesToItem(LilacShellItem item);
    void ApplyPropertiesToItems([MarshalAs(UnmanagedType.IUnknown)] object items);
    void RenameItem(LilacShellItem item, [MarshalAs(UnmanagedType.LPWStr)] string name, IntPtr sink);
    void RenameItems([MarshalAs(UnmanagedType.IUnknown)] object items, [MarshalAs(UnmanagedType.LPWStr)] string name);
    void MoveItem(LilacShellItem item, LilacShellItem dest, [MarshalAs(UnmanagedType.LPWStr)] string name, IntPtr sink);
    void MoveItems([MarshalAs(UnmanagedType.IUnknown)] object items, LilacShellItem dest);
    void CopyItem(LilacShellItem item, LilacShellItem dest, [MarshalAs(UnmanagedType.LPWStr)] string name, IntPtr sink);
    void CopyItems([MarshalAs(UnmanagedType.IUnknown)] object items, LilacShellItem dest);
    void DeleteItem(LilacShellItem item, IntPtr sink);
    void DeleteItems([MarshalAs(UnmanagedType.IUnknown)] object items);
    void NewItem(LilacShellItem dest, uint attrs, [MarshalAs(UnmanagedType.LPWStr)] string name, [MarshalAs(UnmanagedType.LPWStr)] string template, IntPtr sink);
    void PerformOperations();
    void GetAnyOperationsAborted([MarshalAs(UnmanagedType.Bool)] out bool aborted);
}
public static class LilacRecycle {
    [DllImport("shell32.dll", CharSet=CharSet.Unicode, PreserveSig=false)]
    private static extern void SHCreateItemFromParsingName(string path, IntPtr bind, ref Guid iid, [MarshalAs(UnmanagedType.Interface)] out LilacShellItem item);
    public static void Send(string path) {
        var iid = new Guid("43826D1E-E718-42EE-BC55-A1E261C37BFE");
        LilacShellItem item;
        SHCreateItemFromParsingName(path, IntPtr.Zero, ref iid, out item);
        var type = Type.GetTypeFromCLSID(new Guid("3AD05575-8857-4850-9277-11B85BDB8E09"));
        var op = (LilacFileOperation)Activator.CreateInstance(type);
        // RECYCLEONDELETE + EARLYFAILURE: do not substitute a permanent deletion.
        op.SetOperationFlags(0x00080000u | 0x00100000u | 0x0040u | 0x0010u | 0x0004u | 0x0400u);
        op.DeleteItem(item, IntPtr.Zero);
        op.PerformOperations();
        bool aborted; op.GetAnyOperationsAborted(out aborted);
        Marshal.FinalReleaseComObject(op); Marshal.FinalReleaseComObject(item);
        if (aborted) throw new InvalidOperationException("Recycle operation aborted");
    }
}
'@
$recyclePlan = Get-Content -LiteralPath $Plan -Raw | ConvertFrom-Json
$allowedRoots = @($recyclePlan.allowed_roots | ForEach-Object { [IO.Path]::GetFullPath($_).TrimEnd('\') })
$recycleShell = New-Object -ComObject Shell.Application
foreach ($entry in $recyclePlan.items) {
    $source = [IO.Path]::GetFullPath($entry.path)
    $inside = @($allowedRoots | Where-Object { $source.StartsWith($_ + '\',[StringComparison]::OrdinalIgnoreCase) }).Count -gt 0
    if (-not $inside) { throw "Path outside named workspaces: $source" }
    if (-not (Test-Path -LiteralPath $source)) { throw "Missing source: $source" }
    $resolved = (Resolve-Path -LiteralPath $source).ProviderPath
    if ($resolved -ne $source) { throw "Unexpected path resolution: $source -> $resolved" }
    $name = [IO.Path]::GetFileName($source)
    if ($name -match '^(flash|unlock|backup|backups|fw|\.git)$') { throw "Protected directory: $source" }
    $started = Get-Date
    [LilacRecycle]::Send($source)
    if (Test-Path -LiteralPath $source) { throw "Source did not enter recycle bin: $source" }
    $items = @($recycleShell.Namespace(10).Items() | Where-Object {
        $_.Name -eq $name -and $_.ExtendedProperty('System.Recycle.DeletedFrom') -eq [IO.Path]::GetDirectoryName($source)
    })
    if ($items.Count -eq 0) { throw "Native recycle entry not found: $source" }
    $record = @{source=$source;recycle_path=$items[-1].Path;verified_native_recycle=$true;time=$started.ToString('o')}
    $record | ConvertTo-Json -Compress | Add-Content -LiteralPath $Journal -Encoding utf8
    $record | ConvertTo-Json -Compress
}
