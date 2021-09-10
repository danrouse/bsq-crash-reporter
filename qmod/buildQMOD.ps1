# Builds a .qmod file for loading with QuestPatcher
$NDKPath = Get-Content $PSScriptRoot/ndkpath.txt

$buildScript = "$NDKPath/build/ndk-build"
if (-not ($PSVersionTable.PSEdition -eq "Core")) {
    $buildScript += ".cmd"
}

$ArchiveName = "crash-reporter_v1.1.0-b1.qmod"
$TempArchiveName = "crash-reporter_v1.1.0-b1.qmod.zip"

& $buildScript NDK_PROJECT_PATH=$PSScriptRoot APP_BUILD_SCRIPT=$PSScriptRoot/Android.mk NDK_APPLICATION_MK=$PSScriptRoot/Application.mk
Compress-Archive -Path "./libs/arm64-v8a/libcrash-reporter.so", `
    "./libs/arm64-v8a/libbeatsaber-hook_2_3_0.so", `
    "./libs/arm64-v8a/libcustom-types.so", `
    "./libs/arm64-v8a/libquestui.so", `
    "./cover.png", `
    "./mod.json" -DestinationPath $TempArchiveName -Force
Move-Item $TempArchiveName $ArchiveName -Force
