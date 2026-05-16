#!/usr/bin/env pwsh
<#
.SYNOPSIS
    从 Keil MDK .uvprojx 工程文件自动生成 VS Code IntelliSense 配置。

.DESCRIPTION
    解析项目目录下的 .uvprojx 文件，提取 IncludePath 和 Define，
    同时生成以下文件:
      - .clangd                          (clangd 扩展)
      - .vscode/c_cpp_properties.json    (Microsoft C/C++ 扩展)

    用法:
        双击 run.bat，或在项目根目录执行:
        pwsh -File tools/gen-clangd/gen-clangd.ps1

    自动检测:
        - 项目根目录（向上搜索 .uvprojx）
        - ARM GCC / Clang / MinGW 编译器路径

    适配: STM32 / Keil MDK-ARM (.uvprojx) 项目
#>

#Requires -PSEdition Core

set-strictmode -version latest
$ErrorActionPreference = 'Stop'

# ================================================================
#  项目根目录检测
# ================================================================

# 策略1: 从脚本自身位置向上搜索，找到含 .uvprojx 的目录
$scriptDir = $PSScriptRoot
if (-not $scriptDir) { $scriptDir = $PWD.Path }

$searchDir = $scriptDir
$projectRoot = $null

for ($i = 0; $i -lt 10; $i++) {
    $found = Get-ChildItem -LiteralPath $searchDir -Filter '*.uvprojx' -Recurse -Depth 2 -ErrorAction SilentlyContinue
    if ($found) {
        $projectRoot = $searchDir
        break
    }
    $parent = [System.IO.Path]::GetDirectoryName($searchDir)
    if ($parent -eq $searchDir) { break }  # 到达根目录
    $searchDir = $parent
}

# 策略2: fallback 到当前工作目录
if (-not $projectRoot) {
    $projectRoot = $PWD.Path
    Write-Host "[gen-clangd] 未自动检测到项目根，使用当前目录: $projectRoot" -ForegroundColor Yellow
} else {
    Write-Host "[gen-clangd] 项目根目录: $projectRoot"
}

# ================================================================
#  找到 .uvprojx
# ================================================================

$uvprojxFiles = @(Get-ChildItem -LiteralPath $projectRoot -Recurse -Filter '*.uvprojx' -File)

if ($uvprojxFiles.Count -eq 0) {
    Write-Error "[gen-clangd] 未找到 .uvprojx 文件。请在 Keil 项目根目录运行此脚本。"
    exit 1
}

# 优先选 MDK-ARM/ 下的文件
$uvprojx = $uvprojxFiles | Where-Object { $_.DirectoryName -match 'MDK-ARM' } | Select-Object -First 1
if (-not $uvprojx) {
    $uvprojx = $uvprojxFiles[0]
}

$uvprojxDir = $uvprojx.DirectoryName
Write-Host "[gen-clangd] 使用工程文件: $($uvprojx.FullName)"

# ================================================================
#  解析 XML
# ================================================================

[xml]$xml = Get-Content -LiteralPath $uvprojx.FullName -Raw

$targets = $xml.Project.Targets.Target

# 选 Define 最多的 target（主工程，而非 bootloader）
$bestTarget = $null
$bestDefineCount = -1

foreach ($tgt in @($targets)) {
    $ads = $tgt.TargetOption.TargetArmAds
    if (-not $ads) { continue }

    $defineText = $ads.Cads.VariousControls.Define
    if ($null -eq $defineText) { $defineText = '' }

    $count = ($defineText -split ',').Where({ $_ -ne '' }).Count
    if ($count -gt $bestDefineCount) {
        $bestDefineCount = $count
        $bestTarget = $tgt
    }
}

if (-not $bestTarget) {
    $bestTarget = $targets[0]
}

$ads = $bestTarget.TargetOption.TargetArmAds
$rawDefines = $ads.Cads.VariousControls.Define
$rawIncludes = $ads.Cads.VariousControls.IncludePath

# ================================================================
#  处理 Define
# ================================================================

$defines = @()
if ($rawDefines) {
    $defines = ($rawDefines -split '[,;]').Where({ $_ -ne '' }) | ForEach-Object { $_.Trim() }
}

# clangd 使用 clang 引擎，CMSIS 要求识别编译器。
# __GNUC__ 让 CMSIS 走 GCC 分支，clang 天然兼容。
# 不能用 __CC_ARM（ARMCC 专有关键字如 __value_in_regs 会导致 parse error）。
if ($defines -notcontains '__GNUC__') {
    $defines += '__GNUC__'
}

# ================================================================
#  处理 IncludePath
# ================================================================

$includeAbsPaths = @()
if ($rawIncludes) {
    $paths = ($rawIncludes -split ';').Where({ $_ -ne '' })
    foreach ($p in $paths) {
        $trimmed = $p.Trim()
        $absPath = [System.IO.Path]::GetFullPath(
            [System.IO.Path]::Combine($uvprojxDir, $trimmed)
        )
        if (Test-Path -LiteralPath $absPath -PathType Container) {
            $includeAbsPaths += $absPath -replace '\\', '/'
        } else {
            Write-Host "[gen-clangd] WARNING: include 路径不存在: $absPath (来自 $trimmed)" -ForegroundColor Yellow
        }
    }
}

# ================================================================
#  自动检测编译器路径
# ================================================================

$compilerPath = ''
$intelliSenseMode = 'msvc-x86'

# 搜索优先级: ARM GCC > Clang > MinGW GCC
$devEnv = 'E:\Tool\DevEnv'

$armGcc = Get-ChildItem -LiteralPath $devEnv -Recurse -Filter 'arm-none-eabi-gcc.exe' `
    -ErrorAction SilentlyContinue | Select-Object -First 1
$clang = if (Test-Path "$devEnv\llvm\bin\clang.exe") { Get-Item "$devEnv\llvm\bin\clang.exe" } else { $null }
$mingwGcc = Get-ChildItem -LiteralPath $devEnv -Recurse -Filter 'gcc.exe' `
    -ErrorAction SilentlyContinue | Where-Object { $_.DirectoryName -match 'mingw' } | Select-Object -First 1

if ($armGcc) {
    $compilerPath = $armGcc.FullName -replace '\\', '/'
    $intelliSenseMode = 'gcc-arm'
    Write-Host "[gen-clangd] 检测到 ARM GCC: $compilerPath" -ForegroundColor Green
} elseif ($clang) {
    $compilerPath = $clang.FullName -replace '\\', '/'
    $intelliSenseMode = 'clang-x86'
    Write-Host "[gen-clangd] 检测到 Clang: $compilerPath" -ForegroundColor Green
} elseif ($mingwGcc) {
    $compilerPath = $mingwGcc.FullName -replace '\\', '/'
    $intelliSenseMode = 'gcc-x86'
    Write-Host "[gen-clangd] 检测到 MinGW GCC: $compilerPath" -ForegroundColor Green
} else {
    Write-Host "[gen-clangd] WARNING: 未检测到编译器，stdint.h 等系统头文件可能找不到" -ForegroundColor Yellow
}

# ================================================================
#  生成 .clangd（clangd 扩展用）
# ================================================================

$sb = [System.Text.StringBuilder]::new()

[void]$sb.AppendLine("CompileFlags:")
[void]$sb.AppendLine("  Add:")

foreach ($def in $defines) {
    [void]$sb.AppendLine("    - -D$def")
}

foreach ($inc in $includeAbsPaths) {
    [void]$sb.AppendLine("    - -I$inc")
}

[void]$sb.AppendLine()
[void]$sb.AppendLine("Diagnostics:")
[void]$sb.AppendLine("  Suppress:")

@(
    'pp_file_not_found'
    'drv_unknown_argument'
    'unknown_warning_option'
    'unsigned-enum-constexpr-conversion'
) | ForEach-Object {
    [void]$sb.AppendLine("    - $_")
}

$clangdOutput = ($sb.ToString()) -replace "`r`n", "`n"
$clangdPath = Join-Path $projectRoot '.clangd'
[System.IO.File]::WriteAllText($clangdPath, $clangdOutput, [System.Text.UTF8Encoding]::new($false))

# ================================================================
#  生成 .vscode/c_cpp_properties.json（Microsoft C/C++ 扩展用）
# ================================================================

$includeRelPaths = $includeAbsPaths | ForEach-Object {
    $rel = [System.IO.Path]::GetRelativePath($projectRoot, ($_ -replace '/', '\')) -replace '\\', '/'
    "`${workspaceFolder}/$rel"
}

$cppJson = @{
    configurations = @(
        @{
            name            = 'Keil MDK-ARM'
            includePath     = $includeRelPaths
            defines         = $defines
            compilerPath    = $compilerPath
            cStandard       = 'c99'
            intelliSenseMode = $intelliSenseMode
        }
    )
    version = 4
}

$vscodeDir = Join-Path $projectRoot '.vscode'
if (-not (Test-Path -LiteralPath $vscodeDir)) {
    New-Item -ItemType Directory -Path $vscodeDir -Force | Out-Null
}

$cppJsonPath = Join-Path $vscodeDir 'c_cpp_properties.json'
$cppJsonStr = ($cppJson | ConvertTo-Json -Depth 5) -replace "`r`n", "`n"
[System.IO.File]::WriteAllText($cppJsonPath, $cppJsonStr, [System.Text.UTF8Encoding]::new($false))

# ================================================================
#  输出结果
# ================================================================

Write-Host ""
Write-Host "=============================================" -ForegroundColor Green
Write-Host "  gen-clangd 配置生成完毕" -ForegroundColor Green
Write-Host "=============================================" -ForegroundColor Green
Write-Host ""
Write-Host "  .clangd                            (clangd)" -ForegroundColor Cyan
Write-Host "  .vscode/c_cpp_properties.json      (C/C++)" -ForegroundColor Cyan
Write-Host ""
Write-Host "  Defines : $($defines -join ', ')" -ForegroundColor Cyan
Write-Host "  Includes: $($includeAbsPaths.Count) 个目录" -ForegroundColor Cyan
Write-Host "  Compiler: $compilerPath" -ForegroundColor Cyan
Write-Host ""
Write-Host "  如 VS Code 仍报错:" -ForegroundColor Yellow
Write-Host "  Ctrl+Shift+P -> Developer: Reload Window" -ForegroundColor Yellow
Write-Host ""
