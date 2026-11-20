# Paket .exe Windows (Shipping) + ZIP untuk dibagikan / Steam.
$ErrorActionPreference = 'Stop'
$Proj = 'D:\Dev\JuraganRental'
$ini = Get-Content "$Proj\Config\DefaultEngine.ini"
$ver = (($ini | Select-String '^VersionDisplayName=') -replace '.*=', '').Trim()
New-Item -ItemType Directory -Force 'D:\Tmp' | Out-Null
$log = 'D:\Tmp\pc_build.log'
"Build PC v$ver berjalan. Log: $log"
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat' BuildCookRun "-project=$Proj\JuraganRental.uproject" -platform=Win64 -clientconfig=Shipping -build -cook -stage -package -pak -archive "-archivedirectory=$Proj\Build" -nop4 -utf8output -unattended *> $log
if ($LASTEXITCODE -ne 0) { Get-Content $log -Tail 40; throw "Build gagal (exit $LASTEXITCODE)" }
$zip = "$Proj\Build\JuraganRental_PC_v$ver.zip"
Compress-Archive -Path "$Proj\Build\Windows\*" -DestinationPath $zip -Force
'{0}  {1:N0} MB' -f $zip, ((Get-Item $zip).Length / 1MB)
