# ============================================
# iPhone 库存管理系统 - Windows 高级打包脚本
# ============================================
#
# 使用说明：
# 1. 以管理员身份打开 PowerShell
# 2. 如果遇到执行策略问题，运行: Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
# 3. 运行此脚本: .\deploy_windows.ps1
#
# 可选参数：
#   -QtPath       Qt 安装路径 (例如: C:\Qt\6.5.0\msvc2019_64)
#   -BuildType    构建类型 (Release 或 Debug，默认: Release)
#   -CleanBuild   是否清理旧的构建 (默认: $true)
#   -CreateZip    是否创建 ZIP 压缩包 (默认: $true)
#
# 示例：
#   .\deploy_windows.ps1 -QtPath "C:\Qt\6.5.0\msvc2019_64"
#   .\deploy_windows.ps1 -BuildType Debug -CreateZip $false
# ============================================

param(
    [string]$QtPath = "",
    [string]$BuildType = "Release",
    [bool]$CleanBuild = $true,
    [bool]$CreateZip = $true
)

# 设置错误时停止
$ErrorActionPreference = "Stop"

# 颜色输出函数
function Write-ColorOutput($ForegroundColor, $Message) {
    $fc = $host.UI.RawUI.ForegroundColor
    $host.UI.RawUI.ForegroundColor = $ForegroundColor
    Write-Output $Message
    $host.UI.RawUI.ForegroundColor = $fc
}

function Write-Header($Message) {
    Write-ColorOutput Cyan "`n============================================"
    Write-ColorOutput Cyan "  $Message"
    Write-ColorOutput Cyan "============================================`n"
}

function Write-Step($StepNumber, $TotalSteps, $Message) {
    Write-ColorOutput Yellow "[步骤 $StepNumber/$TotalSteps] $Message"
}

function Write-Success($Message) {
    Write-ColorOutput Green "[成功] $Message"
}

function Write-Error-Custom($Message) {
    Write-ColorOutput Red "[错误] $Message"
}

function Write-Info($Message) {
    Write-ColorOutput White "[信息] $Message"
}

# 开始构建
Write-Header "iPhone 库存管理系统 - Windows 构建"

# 检测 Qt 路径
if ($QtPath -eq "") {
    Write-Info "未指定 Qt 路径，尝试自动检测..."

    # 尝试从 qmake 获取
    $qmakePath = Get-Command qmake -ErrorAction SilentlyContinue
    if ($qmakePath) {
        $QtPath = Split-Path (Split-Path $qmakePath.Source -Parent) -Parent
        Write-Success "检测到 Qt 路径: $QtPath"
    } else {
        # 尝试常见的 Qt 安装位置
        $commonPaths = @(
            "C:\Qt\6.8.0\msvc2022_64",
            "C:\Qt\6.7.0\msvc2022_64",
            "C:\Qt\6.6.0\msvc2019_64",
            "C:\Qt\6.5.0\msvc2019_64",
            "C:\Qt\6.4.0\msvc2019_64"
        )

        foreach ($path in $commonPaths) {
            if (Test-Path $path) {
                $QtPath = $path
                Write-Success "找到 Qt 安装: $QtPath"
                break
            }
        }

        if ($QtPath -eq "") {
            Write-Error-Custom "未找到 Qt 安装，请使用 -QtPath 参数指定"
            Write-Info "示例: .\deploy_windows.ps1 -QtPath 'C:\Qt\6.5.0\msvc2019_64'"
            exit 1
        }
    }
}

# 验证 Qt 路径
if (-not (Test-Path "$QtPath\bin\qmake.exe")) {
    Write-Error-Custom "无效的 Qt 路径: $QtPath"
    Write-Info "请确保路径包含 bin\qmake.exe"
    exit 1
}

# 设置环境变量
$env:PATH = "$QtPath\bin;$env:PATH"
$env:Qt6_DIR = "$QtPath"

Write-Info "Qt 路径: $QtPath"
Write-Info "构建类型: $BuildType"

# 检查必需工具
Write-Step 1 6 "检查构建工具..."

$tools = @{
    "qmake" = "Qt qmake"
    "cmake" = "CMake"
    "nmake" = "NMake (Visual Studio)"
}

foreach ($tool in $tools.Keys) {
    $command = Get-Command $tool -ErrorAction SilentlyContinue
    if (-not $command) {
        Write-Error-Custom "未找到 $($tools[$tool])"
        if ($tool -eq "nmake") {
            Write-Info "请在 'x64 Native Tools Command Prompt for VS' 中运行此脚本"
        }
        exit 1
    }
    Write-Success "找到 $($tools[$tool]): $($command.Source)"
}

# 显示版本信息
Write-Info "`n工具版本:"
& qmake -v | Select-Object -First 2
& cmake --version | Select-Object -First 1
Write-Output ""

# 清理旧构建
if ($CleanBuild -and (Test-Path "build-windows")) {
    Write-Step 2 6 "清理旧的构建目录..."
    Remove-Item -Recurse -Force "build-windows"
    Write-Success "清理完成"
}

# 创建构建目录
Write-Step 3 6 "创建构建目录..."
New-Item -ItemType Directory -Force -Path "build-windows" | Out-Null
Set-Location "build-windows"

# 配置 CMake
Write-Step 4 6 "配置 CMake..."
& cmake .. -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=$BuildType -DCMAKE_PREFIX_PATH=$QtPath
if ($LASTEXITCODE -ne 0) {
    Write-Error-Custom "CMake 配置失败"
    Set-Location ..
    exit 1
}
Write-Success "CMake 配置完成"

# 编译
Write-Step 5 6 "编译项目..."
& cmake --build . --config $BuildType
if ($LASTEXITCODE -ne 0) {
    Write-Error-Custom "编译失败"
    Set-Location ..
    exit 1
}
Write-Success "编译完成"

# 部署
Write-Step 6 6 "部署应用程序..."

# 创建部署目录
$deployDir = "deploy"
if (Test-Path $deployDir) {
    Remove-Item -Recurse -Force $deployDir
}
New-Item -ItemType Directory -Force -Path $deployDir | Out-Null

# 复制可执行文件
$exeName = "iPhoneStockManagement.exe"
$exePaths = @(
    "bin\$exeName",
    $exeName
)

$exeCopied = $false
foreach ($exePath in $exePaths) {
    if (Test-Path $exePath) {
        Copy-Item $exePath -Destination $deployDir
        $exeCopied = $true
        Write-Success "复制可执行文件: $exePath"
        break
    }
}

if (-not $exeCopied) {
    Write-Error-Custom "未找到可执行文件"
    Set-Location ..
    exit 1
}

# 运行 windeployqt
Set-Location $deployDir
Write-Info "运行 windeployqt..."
& windeployqt $exeName --release --no-translations --no-system-d3d-compiler --no-opengl-sw
if ($LASTEXITCODE -ne 0) {
    Write-ColorOutput Yellow "[警告] windeployqt 返回非零退出码，但可能已部分完成"
}

# 复制 SQL 驱动
Write-Info "复制 SQL 驱动..."
$pluginsDir = & qmake -query QT_INSTALL_PLUGINS
$sqlDriversSource = Join-Path $pluginsDir "sqldrivers"
if (Test-Path $sqlDriversSource) {
    $sqlDriversDest = "sqldrivers"
    New-Item -ItemType Directory -Force -Path $sqlDriversDest | Out-Null
    Copy-Item "$sqlDriversSource\qsqlite.dll" -Destination $sqlDriversDest -ErrorAction SilentlyContinue
    Write-Success "SQL 驱动已复制"
}

Set-Location ..\..

# 创建 ZIP 包
if ($CreateZip) {
    Write-Info "`n创建 ZIP 压缩包..."
    $version = "0.1.0"
    $zipName = "iPhoneStockManagement_v${version}_Windows_x64.zip"

    if (Test-Path $zipName) {
        Remove-Item $zipName
    }

    Compress-Archive -Path "build-windows\$deployDir\*" -DestinationPath $zipName -CompressionLevel Optimal
    Write-Success "ZIP 包已创建: $zipName"

    $zipSize = (Get-Item $zipName).Length / 1MB
    Write-Info ("文件大小: {0:N2} MB" -f $zipSize)
}

# 完成
Write-Header "构建完成！"
Write-Success "输出目录: build-windows\$deployDir\"
Write-Success "主程序: build-windows\$deployDir\$exeName"

if ($CreateZip) {
    Write-Success "ZIP 包: $zipName"
}

Write-Info "`n可以直接运行程序或分发 deploy 文件夹"
Write-Info "双击 $exeName 即可运行"
