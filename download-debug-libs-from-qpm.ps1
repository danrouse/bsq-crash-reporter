$packages = Invoke-WebRequest "https://qpackages.com" | ConvertFrom-Json
$packages.Split() | foreach {
  $packageDetails = Invoke-WebRequest "https://qpackages.com/$_" | ConvertFrom-Json
  $packageMeta = Invoke-WebRequest "https://qpackages.com/$_/$($packageDetails.version)" | ConvertFrom-Json
  $libName = $packageMeta.config.info.name
  $libFilename = $packageMeta.config.info.additionalData.overrideSoName
  $libUrl = $packageMeta.config.info.additionalData.debugSoLink
  if (!$libFilename -and $libUrl) {
    $libFilename = Split-Path $packageMeta.config.info.additionalData.soLink -leaf
  }
  
  if ($libUrl) {
    Write-Output "Downloading $libName ($libFilename)..."
    Invoke-WebRequest -Uri $libUrl -OutFile "debug-libs\$libFilename"
  } else {
    Write-Output "No debug lib found for $libName, skipping..."
  }
}
