param([System.Diagnostics.Process]$Process, [string]$Out)
# Potret HANYA jendela game (PrintWindow), bukan seluruh layar pengguna.
Add-Type -AssemblyName System.Drawing
if (-not ('W32' -as [type])) {
Add-Type @"
using System; using System.Runtime.InteropServices;
public static class W32 {
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr dc, uint f);
  [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECT r);
  public struct RECT { public int L, T, R, B; }
}
"@
}
$Process.Refresh()
$h = $Process.MainWindowHandle
if ($h -eq [IntPtr]::Zero) { 'Jendela game belum ada, tidak dipotret'; return }
$r = New-Object W32+RECT
[void][W32]::GetClientRect($h, [ref]$r)
$b = New-Object System.Drawing.Bitmap ([Math]::Max(1, $r.R)), ([Math]::Max(1, $r.B))
$g = [System.Drawing.Graphics]::FromImage($b)
$dc = $g.GetHdc()
[void][W32]::PrintWindow($h, $dc, 3)   # PW_CLIENTONLY | PW_RENDERFULLCONTENT
$g.ReleaseHdc($dc)
$b.Save($Out)
"Tersimpan $Out ($($r.R)x$($r.B))"
