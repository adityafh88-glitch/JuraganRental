param(
    # Rilis = AAB + APK universal Shipping, ditandatangani kunci upload (untuk Play Store / dibagikan)
    # Dev   = APK Development untuk dicoba sendiri di HP
    [ValidateSet('Rilis', 'Dev')] [string]$Mode = 'Rilis'
)
# Build Android Juragan Rental. Jalankan dari PowerShell; Unreal Editor sebaiknya ditutup.
$ErrorActionPreference = 'Stop'
$Proj  = 'D:\Dev\JuraganRental'
$UProj = "$Proj\JuraganRental.uproject"
$UAT   = 'C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat'
$Sdk   = "$env:LOCALAPPDATA\Android\Sdk"

# Lingkungan: JDK 21, folder sementara di D: (C: sering penuh / path berspasi bikin Gradle gagal)
New-Item -ItemType Directory -Force 'D:\Tmp' | Out-Null
$env:JAVA_HOME = 'D:\Dev\Tools\jdk-21'
$env:TEMP = 'D:\Tmp'; $env:TMP = 'D:\Tmp'
$env:JAVA_TOOL_OPTIONS = '-Djava.io.tmpdir=D:\Tmp'

# Versi dari Config/DefaultEngine.ini
$ini = Get-Content "$Proj\Config\DefaultEngine.ini"
$ver = (($ini | Select-String '^VersionDisplayName=') -replace '.*=', '').Trim()
$code = (($ini | Select-String '^StoreVersion=') -replace '.*=', '').Trim()
"Versi $ver (kode $code), mode $Mode"

if ($Mode -eq 'Rilis') {
    if (-not (Test-Path "$Proj\Build\Android\kahgamedev-upload.keystore")) { throw 'Kunci upload tidak ditemukan di Build\Android. Jalankan Tools\buat_kunci_upload.ps1 dulu.' }
    $out = "$Proj\Build\PlayStore"; $cfg = @('-clientconfig=Shipping', '-distribution')
} else {
    $out = "$Proj\Build"; $cfg = @('-clientconfig=Development')
}
$log = "D:\Tmp\android_$Mode.log"
"Build berjalan (30-60 menit). Log: $log"
& $UAT BuildCookRun "-project=$UProj" -platform=Android -cookflavor=ASTC @cfg -build -cook -stage -package -pak -archive "-archivedirectory=$out" -nop4 -utf8output -unattended *> $log
if ($LASTEXITCODE -ne 0) { Get-Content $log -Tail 40; throw "Build gagal (exit $LASTEXITCODE). Lihat $log" }

$dir = "$out\Android_ASTC"
if ($Mode -eq 'Rilis') {
    $aab = "$out\JuraganRental-v$ver.aab"
    $apk = "$out\JuraganRental-v$ver-rilis.apk"
    Copy-Item "$dir\JuraganRental-Android-Shipping.aab" $aab -Force
    Copy-Item "$dir\JuraganRental-Android-Shipping_universal.apk" $apk -Force
    $files = @($aab, $apk)
} else {
    $apk = Get-ChildItem "$dir\JuraganRental-arm64.apk", "$dir\*Development*arm64.apk" -ErrorAction SilentlyContinue | Select-Object -First 1
    $files = @($apk.FullName)
}

# Cek tanda tangan APK
$bt = Get-ChildItem "$Sdk\build-tools" | Sort-Object Name -Descending | Select-Object -First 1
$apkFile = $files | Where-Object { $_ -like '*.apk' } | Select-Object -First 1
& "$($bt.FullName)\apksigner.bat" verify --print-certs $apkFile 2>&1 | Select-String 'DN|Verifies|ERROR' | Select-Object -First 3 | ForEach-Object { 'Tanda tangan: ' + $_.Line.Trim() }

# Pindai Windows Defender
foreach ($f in $files) {
    & "$env:ProgramFiles\Windows Defender\MpCmdRun.exe" -Scan -ScanType 3 -File $f -DisableRemediation | Select-String 'found no threats|threat' | ForEach-Object { "Defender $(Split-Path $f -Leaf): $($_.Line.Trim())" }
    $h = (Get-FileHash $f -Algorithm SHA256).Hash
    '{0}  {1:N0} MB  SHA256 {2}' -f (Split-Path $f -Leaf), ((Get-Item $f).Length / 1MB), $h
}
