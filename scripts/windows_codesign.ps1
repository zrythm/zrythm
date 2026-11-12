# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense
#
# Signs Windows PE files (executables and libraries) through Certum's
# SimplySign cloud service via ssign, then verifies every signature with
# signtool (verification needs no certificate on this machine).
#
# Parameters (specify exactly one):
#   -Root  <directory>  Sign every *.exe and *.dll file under this directory
#   -Files <list>       Sign exactly these files (semicolon-separated list;
#                       semicolons cannot appear in these paths)
#
# Environment:
#   CERTUM_EMAIL                        Certum account e-mail (required to
#                                       sign)
#   CERTUM_OTP                          TOTP seed; ssign derives the one-time
#                                       code (required to sign)
#   WINDOWS_CODESIGN_SSIGN              Path to the ssign binary (default:
#                                       searched on PATH)
#   WINDOWS_CODESIGN_TIMESTAMP_SERVER   Optional RFC 3161 timestamp server
#                                       URL (ssign defaults to
#                                       time.certum.pl)
#   WINDOWS_CODESIGN_REQUIRED           Set on release pipelines: missing
#                                       credentials fail the run instead of
#                                       skipping signing
#
# Signing runs only when CERTUM_EMAIL and CERTUM_OTP are set; pipelines
# without them leave the artifacts unsigned. A signing or verification
# failure always exits with a nonzero status.

param(
  [string]$Root = "",
  [string[]]$Files = @()
)

$ErrorActionPreference = "Stop"

# powershell.exe -File cannot pass array parameters, so callers join
# multiple paths with semicolons
$Files = @($Files | ForEach-Object { $_ -split ";" } | Where-Object { $_.Trim() -ne "" })

$hasRoot = -not [string]::IsNullOrEmpty($Root)
$hasFiles = $Files.Count -gt 0
if ($hasRoot -eq $hasFiles) {
  [Console]::Error.WriteLine("specify exactly one of -Root or -Files")
  exit 1
}

if ([string]::IsNullOrWhiteSpace($env:CERTUM_EMAIL) -or [string]::IsNullOrWhiteSpace($env:CERTUM_OTP)) {
  if ("$env:WINDOWS_CODESIGN_REQUIRED" -match "^(?i:1|yes|true)$") {
    [Console]::Error.WriteLine("windows codesign: CERTUM_EMAIL and CERTUM_OTP must be set to sign")
    exit 1
  }
  Write-Warning "windows codesign: CERTUM_EMAIL/CERTUM_OTP are not set, leaving the artifacts UNSIGNED"
  exit 0
}

# ssign defaults to Certum's own timestamp server; the variable overrides it
$timestampArgs = @()
if (-not [string]::IsNullOrWhiteSpace($env:WINDOWS_CODESIGN_TIMESTAMP_SERVER)) {
  $timestampArgs = @("--timestamp-url", $env:WINDOWS_CODESIGN_TIMESTAMP_SERVER)
}

function Get-HostArchitecture
{
  # PROCESSOR_ARCHITEW6432 names the real processor architecture when a
  # 32-bit process runs on 64-bit Windows
  $arch = $env:PROCESSOR_ARCHITEW6432
  if ([string]::IsNullOrEmpty($arch)) { $arch = $env:PROCESSOR_ARCHITECTURE }
  switch ($arch) {
    "AMD64" { return "x64" }
    "ARM64" { return "arm64" }
    "ARM" { return "arm" }
    "x86" { return "x86" }
    default { return "" }
  }
}

function Find-SignTool
{
  $fromPath = Get-Command signtool.exe -ErrorAction SilentlyContinue
  if ($fromPath) { return $fromPath.Source }

  $roots = @(
    "${env:ProgramFiles(x86)}\Windows Kits\10\bin",
    "$env:ProgramFiles\Windows Kits\10\bin"
  )
  $candidates = @(foreach ($root in $roots) {
    if (Test-Path $root) {
      Get-ChildItem -Path $root -Recurse -Filter signtool.exe -ErrorAction SilentlyContinue
    }
  })
  if ($candidates.Count -eq 0) { return $null }

  # layout: bin\<sdk-version>\<host-arch>\signtool.exe. The signtool
  # binary must run on this host, so prefer the host architecture and fall
  # back to x86, which every 64-bit Windows can emulate
  $archTiers = @((Get-HostArchitecture), "x86") |
    Where-Object { $_ -ne "" } | Select-Object -Unique
  foreach ($arch in $archTiers) {
    $inArch = @($candidates | Where-Object { $_.Directory.Name -eq $arch })
    if ($inArch.Count -eq 0) { continue }
    $newest = $inArch | Sort-Object -Descending -Property {
      $version = $null
      if (-not [version]::TryParse($_.Directory.Parent.Name, [ref]$version)) {
        $version = New-Object Version 0, 0
      }
      $version
    }
    return $newest[0].FullName
  }
  return $null
}

$signtool = Find-SignTool
if (-not $signtool) {
  [Console]::Error.WriteLine("signtool.exe not found on PATH or under the Windows Kits directory")
  exit 1
}

function Find-Ssign
{
  $fromEnv = $env:WINDOWS_CODESIGN_SSIGN
  if (-not [string]::IsNullOrEmpty($fromEnv) -and (Test-Path $fromEnv -PathType Leaf)) {
    return $fromEnv
  }
  $fromPath = Get-Command ssign.exe -ErrorAction SilentlyContinue
  if ($fromPath) { return $fromPath.Source }
  return $null
}

$ssign = Find-Ssign
if (-not $ssign) {
  [Console]::Error.WriteLine("ssign not found: install it on PATH or set WINDOWS_CODESIGN_SSIGN to its full path")
  exit 1
}

if ($hasRoot) {
  if (-not (Test-Path $Root -PathType Container)) {
    [Console]::Error.WriteLine("directory not found: $Root")
    exit 1
  }
  $targets = @(Get-ChildItem -Path $Root -Recurse -File -Include *.exe, *.dll |
    ForEach-Object { $_.FullName })
} else {
  $targets = @($Files | ForEach-Object { $_.Trim() })
  $missing = @($targets | Where-Object { -not (Test-Path $_ -PathType Leaf) })
  if ($missing.Count -gt 0) {
    [Console]::Error.WriteLine("file(s) not found: " + ($missing -join ", "))
    exit 1
  }
}

if ($targets.Count -eq 0) {
  Write-Output "windows codesign: no PE files to sign"
  exit 0
}

Write-Output ("windows codesign: signing {0} file(s) with {1}" -f $targets.Count, $ssign)

function Invoke-SignBatch
{
  param([string[]]$Batch)

  # Redirected native stderr lines become error records, which
  # $ErrorActionPreference "Stop" (Windows PowerShell 5.1) would turn into
  # a script-terminating error; success is judged by the exit code
  $ErrorActionPreference = "Continue"

  # ssign reads CERTUM_EMAIL and CERTUM_OTP from the environment, signs every
  # file of the batch in one cloud session, and embeds an RFC 3161 timestamp
  $output = & $ssign $timestampArgs @Batch 2>&1
  if ($LASTEXITCODE -ne 0) {
    [Console]::Error.WriteLine("ssign failed:`n" + ($output | Out-String))
    exit 1
  }
  Write-Output $output
}

function Invoke-VerifyBatch
{
  param([string[]]$Batch)

  # see Invoke-SignBatch for why the preference is relaxed here
  $ErrorActionPreference = "Continue"

  $output = & $signtool verify /pa /all @Batch 2>&1
  if ($LASTEXITCODE -ne 0) {
    [Console]::Error.WriteLine("signature verification failed:`n" + ($output | Out-String))
    exit 1
  }
}

# ssign accepts multiple files per invocation; batch to keep the command
# line well below the 32 KiB Windows limit
$batchSize = 30
for ($i = 0; $i -lt $targets.Count; $i += $batchSize) {
  $end = [Math]::Min($i + $batchSize - 1, $targets.Count - 1)
  $batch = $targets[$i..$end]
  Invoke-SignBatch -Batch $batch
  Invoke-VerifyBatch -Batch $batch
}

Write-Output "windows codesign: all files signed and verified"
exit 0
