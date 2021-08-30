$pathToIl2CppInspector = "C:\dev\Il2CppInspector-2021.1\Il2CppInspector-cli.exe"
$pathToIl2CppSym = "C:\dev\il2cpp_sym\target\debug\il2cpp_sym.exe"

# Extract APK from Quest
if (-not (Test-Path -Path game.apk.zip)) {
  $apkPath = adb shell pm path com.beatgames.beatsaber
  adb pull $apkPath.substring(8) ./game.apk.zip
}

# Extract libil2cpp and metadata from APK
if ((-not (Test-Path -Path libil2cpp.so)) -or (-not (Test-Path -Path global-metadata.dat))) {
  Add-Type -AssemblyName System.IO.Compression.FileSystem
  $zip = [IO.Compression.ZipFile]::OpenRead("$pwd/game.apk.zip")
  $zip.Entries | where {
    $_.FullName -in 'lib/arm64-v8a/libil2cpp.so', 'assets/bin/Data/Managed/Metadata/global-metadata.dat'
  } | foreach {
    $name = $_.Name
    [System.IO.Compression.ZipFileExtensions]::ExtractToFile($_, "$pwd\$name", $true)
  }
  $zip.Dispose()
}

if (-not (Test-Path -Path metadata.json)) {
  & $pathToIl2CppInspector --select-outputs --json-out metadata.json
}

if (-not (Test-Path -Path libil2cpp.sym.so)) {
  & $pathToIl2CppSym
  move libil2cpp.sym.so debug-libs\libil2cpp.so
}

del game.apk.zip
del libil2cpp.so
del global-metadata.dat
del metadata.json
