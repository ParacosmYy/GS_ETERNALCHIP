@echo off
chcp 65001 >nul 2>&1
setlocal enabledelayedexpansion

echo ============================================
echo   STM32 clangd Auto-Configurator v2.0
echo   Reads Keil .uvprojx and generates:
echo     - compile_flags.txt
echo     - .clangd
echo ============================================
echo.

:: Save the directory where this script is located
:: (the STM32 project root, where Core/ Drivers/ etc. live)
set "PROJECT_ROOT=%~dp0"
:: Remove trailing backslash
if "%PROJECT_ROOT:~-1%"=="\" set "PROJECT_ROOT=%PROJECT_ROOT:~0,-1%"

cd /d "%PROJECT_ROOT%"

:: ---- 1. Find .uvprojx ----
set "UVPROJX="
for %%f in ("MDK-ARM\*.uvprojx") do (
    set "UVPROJX=%%f"
)
if "%UVPROJX%"=="" (
    for %%f in ("*.uvprojx") do set "UVPROJX=%%f"
)
if "%UVPROJX%"=="" (
    echo [ERROR] No .uvprojx found in MDK-ARM\ or current directory.
    echo         Place this script in the project root (where Core/ folder exists).
    pause
    exit /b 1
)
echo [INFO] Found Keil project: %UVPROJX%

:: ---- 2. Extract IncludePath (pick the LONGEST one — it's the main target) ----
set "BEST_INC="
set "BEST_LEN=0"
for /f "tokens=2 delims=<>" %%a in ('findstr /C:"<IncludePath>" "%UVPROJX%"') do (
    set "VAL=%%a"
    if not "!VAL!"=="" if not "!VAL!"=="/" (
        call :strlen INC_LEN "!VAL!"
        if !INC_LEN! GTR !BEST_LEN! (
            set "BEST_INC=!VAL!"
            set "BEST_LEN=!INC_LEN!"
        )
    )
)
set "INC_RAW=%BEST_INC%"
if "%INC_RAW%"=="" (
    echo [ERROR] No valid IncludePath found.
    pause
    exit /b 1
)
echo [INFO] Include paths extracted ^(longest entry, %BEST_LEN% chars^).

:: ---- 3. Extract Defines (pick the first non-empty) ----
set "DEF_RAW="
for /f "tokens=2 delims=<>" %%a in ('findstr /C:"<Define>" "%UVPROJX%"') do (
    set "VAL=%%a"
    if not "!VAL!"=="" (
        set "DEF_RAW=!VAL!"
        goto :got_def
    )
)
:got_def
if "%DEF_RAW%"=="" (
    echo [WARN] No defines found, using defaults.
    set "DEF_RAW=USE_HAL_DRIVER"
)
echo [INFO] Defines: %DEF_RAW%

:: ---- 4. Detect STM32 variant from defines ----
set "STM32_VARIANT="
for %%d in (%DEF_RAW:,= %) do (
    echo %%d | findstr /R "STM32[FGHLMW][0-9]" >nul && set "STM32_VARIANT=%%d"
)
if defined STM32_VARIANT (
    echo [INFO] Detected STM32 variant: %STM32_VARIANT%
) else (
    echo [WARN] Could not detect STM32 variant from defines.
)

:: ---- 5. Write compile_flags.txt ----
echo [INFO] Generating compile_flags.txt ...

:: Clear file
> "compile_flags.txt" echo.

:: Write include paths
call :write_includes "%INC_RAW%"

:: Detect CMSIS sub-structure and add extra paths
if exist "Drivers\CMSIS\Core\Include" (
    echo -IDrivers/CMSIS/Core/Include>> "compile_flags.txt"
)
if exist "Drivers\CMSIS\Include" (
    echo -IDrivers/CMSIS/Include>> "compile_flags.txt"
)
if exist "Middlewares\Third_Party" (
    echo -IMiddlewares/Third_Party>> "compile_flags.txt"
)
:: Shared directory (if exists at parent level)
if exist "..\Shared" (
    echo -I../Shared>> "compile_flags.txt"
)

:: Write defines
call :write_defines "%DEF_RAW%"

:: Write extra CMSIS/GCC compatibility defines
(
    echo -D__GNUC__
    echo -D__CORTEX_M4
    echo -D__FPU_PRESENT=1
    echo -DARM_MATH_CM4
    echo -std=gnu11
    echo -Wno-unknown-attributes
    echo -Wno-unknown-pragmas
    echo -Wno-pragma-once-outside-header
    echo -Wno-unknown-warning-option
    echo -Wno-implicit-function-declaration
    echo -Wno-int-conversion
    echo -Wno-incompatible-pointer-types
) >> "compile_flags.txt"

echo [OK] compile_flags.txt generated.

:: ---- 6. Write .clangd ----
echo [INFO] Generating .clangd ...
(
    echo CompileFlags:
    echo   CompilationDatabase: .
    echo.
    echo Diagnostics:
    echo   Suppress:
    echo     - pp_file_not_found
    echo     - drv_unknown_argument
    echo     - pp_macro_not_defined
    echo     - drv_int_to_void_pointer_cast
    echo   UnusedIncludes: None
    echo   MissingIncludes: None
    echo.
    echo InlayHints:
    echo   Enabled: No
) > ".clangd"
echo [OK] .clangd generated.

:: ---- 7. Summary ----
echo.
echo ============================================
echo   Done! Files generated:
echo     %PROJECT_ROOT%\compile_flags.txt
echo     %PROJECT_ROOT%\.clangd
echo.
echo   Next: Restart clangd in VSCode
echo     Ctrl+Shift+P -^> "clangd: Restart language server"
echo ============================================
echo.
pause
exit /b 0

:: =============================================
:: Subroutines
:: =============================================

:strlen
    set "STR=!%~2!"
    set /a "LEN=0"
    :strlen_loop
    if not "!STR:~%LEN%!"=="" (
        set /a "LEN+=1"
        goto strlen_loop
    )
    set "%~1=%LEN%"
    exit /b

:write_includes
    set "REST=%~1"
    :write_inc_loop
    for /f "tokens=1* delims=;" %%i in ("!REST!") do (
        set "IPATH=%%i"
        :: Convert backslashes to forward slashes
        set "IPATH=!IPATH:\=/!"
        :: Remove leading ..\ or ../ since paths are relative to project root
        set "IPATH=!IPATH:../=!"
        set "IPATH=!IPATH:..\\=!"
        echo -I!IPATH!>> "compile_flags.txt"
        if not "%%j"=="" (
            set "REST=%%j"
            goto write_inc_loop
        )
    )
    exit /b

:write_defines
    set "REST=%~1"
    :write_def_loop
    for /f "tokens=1* delims=," %%i in ("!REST!") do (
        echo -D%%i>> "compile_flags.txt"
        if not "%%j"=="" (
            set "REST=%%j"
            goto write_def_loop
        )
    )
    exit /b
