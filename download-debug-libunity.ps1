
$unityAndroidSupportURL = "https://download.unity3d.com/download_unity/1381962e9d08/TargetSupportInstaller/UnitySetup-Android-Support-for-Editor-2019.4.28f1.exe"
$pathTo7Zip = "C:\Program Files\7-Zip\7z.exe"

if (-not (Test-Path -Path "unity-android-support.exe")) {
  Invoke-WebRequest $unityAndroidSupportURL -OutFile "unity-android-support.exe"
}

& $pathTo7Zip e "unity-android-support.exe" '$INSTDIR$_59_/Variations/il2cpp/Release/Symbols/arm64-v8a/libunity.sym.so'
del "unity-android-support.exe"
move libunity.sym.so debug-libs\libunity.so
