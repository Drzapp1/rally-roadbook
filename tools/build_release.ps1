# Assemble the full public release package (plugin + data + companions + docs)
# into a versioned zip under dist\.
param([string]$Version = "1.0.0")
$ErrorActionPreference = "Stop"
$root = "D:\BikesRoadbook"
$rel  = Join-Path $root "release\RallyRoadbook"
$data = Join-Path $rel  "Roadbook_data"

# 0) build the optimized, self-contained Release plugin (/MT static CRT) for distribution
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$msbuild = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
& $msbuild (Join-Path $root "Roadbook.sln") /t:Roadbook /p:Configuration=Release /p:Platform=x64 /m /nologo /v:quiet | Out-Null
$dlo = Join-Path $root "build\Release\Roadbook.dlo"
if (-not (Test-Path $dlo)) { throw "Release build failed - $dlo not found" }

# 0b) asset-completeness gate: every referenced sign / web PNG / voice clip /
#     device sprite / font must exist (codifies the manual audits).
& python (Join-Path $root "tools\check_assets.py")
if ($LASTEXITCODE -ne 0) { throw "Asset check failed (see list above) - aborting release" }

# 1) refresh the binary (Release) + data into the release tree
Copy-Item $dlo (Join-Path $rel "Roadbook.dlo") -Force
New-Item -ItemType Directory -Force (Join-Path $data "icons") | Out-Null
Copy-Item (Join-Path $root "data\icons\*.tga") (Join-Path $data "icons") -Force
# audio (tones + voice bank) and fonts are maintained in-place under Roadbook_data

# 2) companions + content into the release tree
foreach ($d in @("web", "content")) {
  $dst = Join-Path $rel $d
  New-Item -ItemType Directory -Force $dst | Out-Null
  Copy-Item (Join-Path (Join-Path $root $d) "*") $dst -Recurse -Force
}
$toolsDst = Join-Path $rel "tools"
New-Item -ItemType Directory -Force $toolsDst | Out-Null
foreach ($t in @("rbtool.py","make_scenarios.py","make_real_stages.py","make_voicebank.ps1","trim_voicebank.py","gpx_to_roadbook.py","check_assets.py")) {
  $src = Join-Path $root "tools\$t"
  if (Test-Path $src) { Copy-Item $src $toolsDst -Force }
}

# 2b) content gate: every bundled roadbook must pass `rbtool validate` (no malformed
#     schema, non-monotonic distance, out-of-range CAP, coincident boxes, NaN, etc.)
$books = Get-ChildItem -Recurse -Filter *.json -ErrorAction SilentlyContinue `
  (Join-Path $rel "web\roadbooks"), (Join-Path $rel "content\scenarios"), (Join-Path $rel "example-roadbooks")
$badBooks = @()
foreach ($b in $books) {
  & python (Join-Path $root "tools\rbtool.py") validate $b.FullName | Out-Null
  if ($LASTEXITCODE -ne 0) { $badBooks += $b.Name }
}
if ($badBooks.Count) { throw "Content check failed - invalid roadbook(s): $($badBooks -join ', ')" }
Write-Host "content gate: $($books.Count) bundled roadbooks valid"

# 3) counts for the summary
$signs  = (Get-ChildItem (Join-Path $data "icons\rb_*.tga") | Where-Object { $_.Name -notmatch '^rb_(device|arrow)' }).Count
$devs   = (Get-ChildItem (Join-Path $data "icons\rb_device*.tga")).Count
$voices = (Get-ChildItem (Join-Path $data "audio\voice\*.wav") -ErrorAction SilentlyContinue).Count
$ex     = (Get-ChildItem (Join-Path $rel "example-roadbooks\*.json") -ErrorAction SilentlyContinue).Count
$scn    = (Get-ChildItem (Join-Path $rel "content\scenarios\roadbook_*.json") -ErrorAction SilentlyContinue).Count

# 4) zip it
$dist = Join-Path $root "dist"; New-Item -ItemType Directory -Force $dist | Out-Null
$stamp = Get-Date -Format "yyyyMMdd-HHmm"
$zip = Join-Path $dist "RallyRoadbook_v${Version}_$stamp.zip"
if (Test-Path $zip) { Remove-Item $zip -Force }
Compress-Archive -Path $rel -DestinationPath $zip -CompressionLevel Optimal
$mb = [math]::Round((Get-Item $zip).Length / 1MB, 2)
Write-Host "package: $zip ($mb MB)"
Write-Host "  signs=$signs  devices=$devs  voiceclips=$voices  examples=$ex  scenarios=$scn"
