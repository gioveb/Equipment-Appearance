param(
 [Parameter(Mandatory=$true)][string]$Toolchain,
 [string]$Dependencies = "$PSScriptRoot/vendor"
)
$ErrorActionPreference='Stop'
if (!(Test-Path -LiteralPath $Dependencies) -and $Dependencies -eq "$PSScriptRoot/vendor") {
 Expand-Archive -LiteralPath "$PSScriptRoot/vendor_sources.zip" -DestinationPath $PSScriptRoot -Force
}

$sdk=Join-Path $Dependencies 'DAOLib'
$addr=Join-Path $Dependencies 'DAOAddressLibrary'
$mh=Join-Path $Dependencies 'minhook'
$cc=Join-Path $Toolchain 'i686-w64-mingw32-clang.exe'
$cxx=Join-Path $Toolchain 'i686-w64-mingw32-clang++.exe'
$out=Join-Path $PSScriptRoot 'build'
$dest=Join-Path $out 'bin_ship/daolib/plugins/equipment_appearance'
New-Item -ItemType Directory -Force $out,$dest | Out-Null
$inc=@('-I',"$PSScriptRoot/src",'-I',"$sdk/src",'-I',"$addr/src",'-I',"$mh/include",'-I',"$Dependencies/spdlog-1.15.3/include")
$common=@('-include',"$PSScriptRoot/src/BuildConfig.h",'-O2','-DNDEBUG','-DNOMINMAX','-DWIN32_LEAN_AND_MEAN','-Wno-unknown-pragmas','-ffunction-sections','-fdata-sections')
$objects=@()
foreach($s in @('buffer.c','hook.c','trampoline.c','hde/hde32.c')){
 $obj=Join-Path $out ($s.Replace('/','_')+'.o')
 if(!(Test-Path $obj)){
 & $cc @common -c "$mh/src/$s" -o $obj
 if($LASTEXITCODE -ne 0){throw "Failed: $s"}
 };$objects+=$obj
}
$sources=@("$PSScriptRoot/src/Probe.cpp","$sdk/src/dao/Gda.cpp","$sdk/src/dao/String.cpp","$sdk/src/dao/Log.cpp","$sdk/src/dao/Module.cpp","$sdk/src/dao/core/Pe.cpp","$sdk/src/dao/core/Fingerprint.cpp","$addr/src/daoaddr/CodeIndex.cpp","$addr/src/daoaddr/Library.cpp","$PSScriptRoot/src/HelmetQuickbar.cpp","$sdk/src/dao/Gui.cpp","$PSScriptRoot/src/Messages.cpp","$sdk/src/dao/core/Rtti.cpp","$sdk/src/dao/Frame.cpp","$sdk/src/dao/Data.cpp","$sdk/src/dao/Resources.cpp","$sdk/src/dao/TypeName.cpp","$sdk/src/dao/core/GfxMovie.cpp","$sdk/src/dao/core/Inflate.cpp")
foreach($s in $sources){
 $obj=Join-Path $out ((Split-Path $s -Leaf)+'.o')
 if(!(Test-Path $obj) -or $s -eq "$PSScriptRoot/src/Probe.cpp" -or (Get-Item $s).LastWriteTime -gt (Get-Item $obj).LastWriteTime){
 & $cxx @common -std=c++20 -Wall -Wextra -Werror -Wno-unknown-pragmas @inc -c $s -o $obj
 if($LASTEXITCODE -ne 0){throw "Failed: $s"}
 };$objects+=$obj
}
$dll=Join-Path $dest 'equipment_appearance.dll'
& $cxx -shared -static @objects '-Wl,--gc-sections' '-Wl,--exclude-all-symbols' -lshell32 -lole32 -luuid -o $dll
if($LASTEXITCODE -ne 0){throw 'Link failed'}
Copy-Item -LiteralPath "$PSScriptRoot/equipment_appearance.ini" -Destination $dest
Copy-Item -LiteralPath "$PSScriptRoot/dalls_weapon_scales.json","$PSScriptRoot/dalls_weapon_scales.schema.json" -Destination $dest
& $cxx -std=c++20 -O2 -static -municode @inc "$PSScriptRoot/src/LoadSmoke.cpp" -o "$out/load_smoke.exe"
if($LASTEXITCODE -ne 0){throw 'Smoke build failed'}
& "$out/load_smoke.exe" $dll
if($LASTEXITCODE -ne 0){throw 'Smoke failed'}
& $cxx -std=c++20 -O2 -static "$PSScriptRoot/src/OffsetChecks.cpp" -o "$out/offset_checks.exe"
if($LASTEXITCODE -ne 0){throw 'Offset checks build failed'}
& "$out/offset_checks.exe"
if($LASTEXITCODE -ne 0){throw 'Offset checks failed'}
& $cxx -std=c++20 -O2 -static @inc "$PSScriptRoot/src/VariableChecks.cpp" -o "$out/variable_checks.exe"
if($LASTEXITCODE -ne 0){throw 'Variable checks build failed'}
& "$out/variable_checks.exe"
if($LASTEXITCODE -ne 0){throw 'Variable checks failed'}
foreach($test in @('ActorChecks','CacheChecks','StagedChecks','CinematicChecks')){
 & $cxx -std=c++20 -O2 -static @inc "$PSScriptRoot/src/$test.cpp" -o "$out/$test.exe"
 if($LASTEXITCODE -ne 0){throw "Build failed: $test"}
 & "$out/$test.exe"
 if($LASTEXITCODE -ne 0){throw "Checks failed: $test"}
}
$cameraObjects=@($objects | Where-Object {(Split-Path $_ -Leaf) -ne 'Probe.cpp.o'})
& $cxx @common -std=c++20 -Wall -Wextra -Werror -Wno-unknown-pragmas @inc "$PSScriptRoot/src/CameraChecks.cpp" @cameraObjects -static -lshell32 -lole32 -luuid -o "$out/camera_checks.exe"
if($LASTEXITCODE -ne 0){throw 'Camera callbacks build failed'}
& "$out/camera_checks.exe"
if($LASTEXITCODE -ne 0){throw 'Camera callback checks failed'}
$transitionObjects=@($cameraObjects | Where-Object {(Split-Path $_ -Leaf) -ne 'Module.cpp.o'})
& $cxx @common -std=c++20 -Wall -Wextra -Werror -Wno-unknown-pragmas @inc "$PSScriptRoot/src/TransitionChecks.cpp" @transitionObjects -static -lshell32 -lole32 -luuid -o "$out/transition_checks.exe"
if($LASTEXITCODE -ne 0){throw 'Transition callbacks build failed'}
& "$out/transition_checks.exe" "$PSScriptRoot/src/fixtures"
if($LASTEXITCODE -ne 0){throw 'Transition callback checks failed'}
Write-Output "BUILT $dll"

& $cxx -std=c++20 -O2 -static -I "$PSScriptRoot/src" "$PSScriptRoot/src/FullBodyPolicyChecks.cpp" -o "$out/fullbody_policy.exe"
if($LASTEXITCODE -ne 0){throw 'Full Body policy build failed'}
& "$out/fullbody_policy.exe"
if($LASTEXITCODE -ne 0){throw 'Full Body policy checks failed'}
& $cxx -std=c++20 -O2 -shared -static "$PSScriptRoot/src/MockExtended.cpp" -o "$out/extended_variations.dll"
if($LASTEXITCODE -ne 0){throw 'EV mock build failed'}
& $cxx @common -std=c++20 -O1 -static -municode -Wall -Wextra -Werror -Wno-unknown-pragmas @inc "$PSScriptRoot/src/FullBodyChecks.cpp" @($cameraObjects | Where-Object {(Split-Path $_ -Leaf) -in @('Gda.cpp.o','String.cpp.o','buffer.c.o','hook.c.o','trampoline.c.o','hde_hde32.c.o')}) -lshell32 -lole32 -luuid -o "$out/fullbody_checks.exe"
if($LASTEXITCODE -ne 0){throw 'Full Body callbacks build failed'}
& "$out/fullbody_checks.exe" "$out/extended_variations.dll"
if($LASTEXITCODE -ne 0){throw 'Full Body callbacks failed'}






# UI command queue and additive movie patch checks, without running DAO.
& $cxx @common -std=c++20 -static @inc "$PSScriptRoot/src/QuickbarChecks.cpp" "$PSScriptRoot/src/HelmetQuickbar.cpp" "$sdk/src/dao/core/GfxMovie.cpp" "$sdk/src/dao/core/Inflate.cpp" -o "$out/quickbar_checks.exe"
if($LASTEXITCODE -ne 0){throw 'Quickbar checks build failed'}
& "$out/quickbar_checks.exe" "$PSScriptRoot/ui/empty.swf"
if($LASTEXITCODE -ne 0){throw 'Quickbar checks failed'}

& $cxx -std=c++20 -O1 -static -I "$PSScriptRoot/src" "$PSScriptRoot/src/WeaponScaleChecks.cpp" -o "$out/weapon_scale_checks.exe"
if($LASTEXITCODE -ne 0){throw 'Weapon scale policy build failed'}
& "$out/weapon_scale_checks.exe" "$PSScriptRoot/dalls_weapon_scales.json"
if($LASTEXITCODE -ne 0){throw 'Weapon scale policy checks failed'}

& $cxx -std=c++20 -O2 -static -I "$PSScriptRoot/src" "$PSScriptRoot/src/WeaponFrameChecks.cpp" -o "$out/weapon_frame_checks.exe"
if($LASTEXITCODE -ne 0){throw 'Weapon frame build failed'}
& "$out/weapon_frame_checks.exe"
if($LASTEXITCODE -ne 0){throw 'Weapon frame checks failed'}
