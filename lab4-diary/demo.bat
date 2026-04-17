@echo off
rem demo.bat — Demonstrate all four diary programs.
rem Usage: demo.bat [build_dir]
rem   build_dir defaults to .\build

setlocal enabledelayedexpansion

if "%~1"=="" (
    set "BUILD=.\build"
) else (
    set "BUILD=%~1"
)

set "DIARY_FILE=%BUILD%\diary.dat"
if exist "%DIARY_FILE%" del "%DIARY_FILE%"

echo ===== 1. pdadd: Add three entries =====
echo Today I started the diary project. | "%BUILD%\pdadd.exe" 2026-04-15
echo Debugging the binary serializer took all afternoon. | "%BUILD%\pdadd.exe" 2026-04-16
echo All four programs are done! | "%BUILD%\pdadd.exe" 2026-04-17

echo.
echo ===== 2. pdlist: List all entries =====
"%BUILD%\pdlist.exe"

echo.
echo ===== 3. pdlist with date range (2026-04-15 to 2026-04-16) =====
"%BUILD%\pdlist.exe" 2026-04-15 2026-04-16

echo.
echo ===== 4. pdshow: Show entry for 2026-04-17 =====
"%BUILD%\pdshow.exe" 2026-04-17

echo.
echo ===== 5. pdadd: Overwrite existing entry =====
echo Rewritten: finished early, going for a walk. | "%BUILD%\pdadd.exe" 2026-04-16
echo --- After overwrite ---
"%BUILD%\pdshow.exe" 2026-04-16

echo.
echo ===== 6. pdremove: Remove entry for 2026-04-15 =====
"%BUILD%\pdremove.exe" 2026-04-15
echo --- After removal ---
"%BUILD%\pdlist.exe"

echo.
echo ===== 7. pdremove: Non-existent date (expect error) =====
"%BUILD%\pdremove.exe" 2020-01-01
if errorlevel 1 echo (exit code: non-zero)

if exist "%DIARY_FILE%" del "%DIARY_FILE%"
echo.
echo ===== Demo complete =====

endlocal
