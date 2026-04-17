# demo.ps1 — Demonstrate all four diary programs.
# Usage: ./demo.ps1 [[-BuildDir] <string>]
#   BuildDir defaults to .\build

param(
    [string]$BuildDir = ".\build"
)

$env:DIARY_FILE = Join-Path $BuildDir "diary.dat"
if (Test-Path $env:DIARY_FILE) { Remove-Item $env:DIARY_FILE }

function Invoke-Diary {
    param([string]$Exe, [string[]]$Args, [string]$Stdin)
    $proc = Start-Process -FilePath (Join-Path $BuildDir $Exe) `
        -ArgumentList $Args -NoNewWindow -Wait -PassThru `
        -RedirectStandardInput (if ($Stdin) { "$env:TEMP\_diary_in.txt" } else { $null }) `
        -RedirectStandardOutput "$env:TEMP\_diary_out.txt" `
        -RedirectStandardError "$env:TEMP\_diary_err.txt"
    if ($Stdin) {
        Set-Content -Path "$env:TEMP\_diary_in.txt" -Value $Stdin
    }
    if (Test-Path "$env:TEMP\_diary_out.txt") {
        Get-Content "$env:TEMP\_diary_out.txt"
        Remove-Item "$env:TEMP\_diary_out.txt"
    }
    if ($proc.ExitCode -ne 0 -and (Test-Path "$env:TEMP\_diary_err.txt")) {
        Get-Content "$env:TEMP\_diary_err.txt"
        Remove-Item "$env:TEMP\_diary_err.txt"
    }
}

Write-Host "===== 1. pdadd: Add three entries ====="
"One line diary content.`n." | & "$BuildDir\pdadd.exe" 2026-04-15  2>&1 | ForEach-Object { $_ }
"Debugging the binary serializer took all afternoon.`n." | & "$BuildDir\pdadd.exe" 2026-04-16  2>&1 | ForEach-Object { $_ }
"All four programs are done!`n." | & "$BuildDir\pdadd.exe" 2026-04-17  2>&1 | ForEach-Object { $_ }

Write-Host "`n===== 2. pdlist: List all entries ====="
& "$BuildDir\pdlist.exe"

Write-Host "`n===== 3. pdlist with date range (2026-04-15 to 2026-04-16) ====="
& "$BuildDir\pdlist.exe" 2026-04-15 2026-04-16

Write-Host "`n===== 4. pdshow: Show entry for 2026-04-17 ====="
& "$BuildDir\pdshow.exe" 2026-04-17

Write-Host "`n===== 5. pdadd: Overwrite existing entry ====="
"Rewritten: finished early, going for a walk.`n." | & "$BuildDir\pdadd.exe" 2026-04-16 2>&1 | ForEach-Object { $_ }
Write-Host "--- After overwrite ---"
& "$BuildDir\pdshow.exe" 2026-04-16

Write-Host "`n===== 6. pdremove: Remove entry for 2026-04-15 ====="
& "$BuildDir\pdremove.exe" 2026-04-15
Write-Host "--- After removal ---"
& "$BuildDir\pdlist.exe"

Write-Host "`n===== 7. pdremove: Non-existent date (expect error) ====="
& "$BuildDir\pdremove.exe" 2020-01-01 2>&1 | ForEach-Object { $_ }
if ($LASTEXITCODE -ne 0) { Write-Host "(exit code: $LASTEXITCODE)" }

if (Test-Path $env:DIARY_FILE) { Remove-Item $env:DIARY_FILE }
Write-Host "`n===== Demo complete ====="
