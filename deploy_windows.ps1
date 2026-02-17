# ============================================
# iPhone Stock Management - Windows Deploy Script
# ============================================
#
# Usage:
# 1. Open PowerShell as Administrator
# 2. If you encounter execution policy issues, run:
#    Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
# 3. Run this script: .\deploy_windows.ps1
#
# Parameters:
#   -QtPath       Qt install path (e.g. C:\Qt\6.10.1\msvc2022_64)
#   -BuildType    Build type (Release or Debug, default: Release)
#   -CleanBuild   Whether to clean old build (default: $true)
#   -CreateZip    Whether to create ZIP archive (default: $true)
#
# Examples:
#   .\deploy_windows.ps1 -QtPath "C:\Qt\6.10.1\msvc2022_64"
#   .\deploy_windows.ps1 -BuildType Debug -CreateZip $false
# ============================================

param(
    [string]$QtPath = "",
    [string]$BuildType = "Release",
    [bool]$CleanBuild = $true,
    [bool]$CreateZip = $true
)

# Stop on error
$ErrorActionPreference = "Stop"

# Color output functions
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
    Write-ColorOutput Yellow "[Step $StepNumber/$TotalSteps] $Message"
}

function Write-Success($Message) {
    Write-ColorOutput Green "[OK] $Message"
}

function Write-Error-Custom($Message) {
    Write-ColorOutput Red "[ERROR] $Message"
}

function Write-Info($Message) {
    Write-ColorOutput White "[INFO] $Message"
}

# Start build
Write-Header "iPhone Stock Management - Windows Build"

# Detect Qt path
if ($QtPath -eq "") {
    Write-Info "Qt path not specified, auto-detecting..."

    # Try to get from qmake
    $qmakePath = Get-Command qmake -ErrorAction SilentlyContinue
    if ($qmakePath) {
        $QtPath = Split-Path (Split-Path $qmakePath.Source -Parent) -Parent
        Write-Success "Detected Qt path: $QtPath"
    } else {
        # Try common Qt install locations
        $commonPaths = @(
            "C:\Qt\6.10.1\msvc2022_64",
            "C:\Qt\6.10.0\msvc2022_64",
            "C:\Qt\6.9.0\msvc2022_64",
            "C:\Qt\6.8.0\msvc2022_64",
            "C:\Qt\6.7.0\msvc2022_64",
            "C:\Qt\6.6.0\msvc2019_64",
            "C:\Qt\6.5.0\msvc2019_64"
        )

        foreach ($path in $commonPaths) {
            if (Test-Path $path) {
                $QtPath = $path
                Write-Success "Found Qt installation: $QtPath"
                break
            }
        }

        if ($QtPath -eq "") {
            Write-Error-Custom "Qt installation not found. Please specify with -QtPath"
            Write-Info "Example: .\deploy_windows.ps1 -QtPath 'C:\Qt\6.10.1\msvc2022_64'"
            exit 1
        }
    }
}

# Validate Qt path
if (-not (Test-Path "$QtPath\bin\qmake.exe")) {
    Write-Error-Custom "Invalid Qt path: $QtPath"
    Write-Info "Please ensure the path contains bin\qmake.exe"
    exit 1
}

# Set environment variables
$env:PATH = "$QtPath\bin;$env:PATH"
$env:Qt6_DIR = "$QtPath"

Write-Info "Qt path: $QtPath"
Write-Info "Build type: $BuildType"

# Check required tools
Write-Step 1 6 "Checking build tools..."

$tools = @{
    "qmake" = "Qt qmake"
    "cmake" = "CMake"
    "nmake" = "NMake (Visual Studio)"
}

foreach ($tool in $tools.Keys) {
    $command = Get-Command $tool -ErrorAction SilentlyContinue
    if (-not $command) {
        Write-Error-Custom "Not found: $($tools[$tool])"
        if ($tool -eq "nmake") {
            Write-Info "Please run this script in 'x64 Native Tools Command Prompt for VS'"
        }
        exit 1
    }
    Write-Success "Found $($tools[$tool]): $($command.Source)"
}

# Show version info
Write-Info "`nTool versions:"
& qmake -v | Select-Object -First 2
& cmake --version | Select-Object -First 1
Write-Output ""

# Clean old build
if ($CleanBuild -and (Test-Path "build-windows")) {
    Write-Step 2 6 "Cleaning old build directory..."
    Remove-Item -Recurse -Force "build-windows"
    Write-Success "Clean complete"
}

# Create build directory
Write-Step 3 6 "Creating build directory..."
New-Item -ItemType Directory -Force -Path "build-windows" | Out-Null
Set-Location "build-windows"

# Configure CMake
Write-Step 4 6 "Configuring CMake..."
& cmake .. -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=$BuildType -DCMAKE_PREFIX_PATH=$QtPath
if ($LASTEXITCODE -ne 0) {
    Write-Error-Custom "CMake configuration failed"
    Set-Location ..
    exit 1
}
Write-Success "CMake configuration complete"

# Build
Write-Step 5 6 "Building project..."
& cmake --build . --config $BuildType
if ($LASTEXITCODE -ne 0) {
    Write-Error-Custom "Build failed"
    Set-Location ..
    exit 1
}
Write-Success "Build complete"

# Deploy
Write-Step 6 6 "Deploying application..."

# Create deploy directory
$deployDir = "deploy"
if (Test-Path $deployDir) {
    Remove-Item -Recurse -Force $deployDir
}
New-Item -ItemType Directory -Force -Path $deployDir | Out-Null

# Copy executable
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
        Write-Success "Copied executable: $exePath"
        break
    }
}

if (-not $exeCopied) {
    Write-Error-Custom "Executable not found"
    Set-Location ..
    exit 1
}

# Run windeployqt
Set-Location $deployDir
Write-Info "Running windeployqt..."
& windeployqt $exeName --release --no-translations --no-system-d3d-compiler --no-opengl-sw
if ($LASTEXITCODE -ne 0) {
    Write-ColorOutput Yellow "[WARN] windeployqt returned non-zero exit code, but may have partially completed"
}

# Copy SQL driver
Write-Info "Copying SQL driver..."
$pluginsDir = & qmake -query QT_INSTALL_PLUGINS
$sqlDriversSource = Join-Path $pluginsDir "sqldrivers"
if (Test-Path $sqlDriversSource) {
    $sqlDriversDest = "sqldrivers"
    New-Item -ItemType Directory -Force -Path $sqlDriversDest | Out-Null
    Copy-Item "$sqlDriversSource\qsqlite.dll" -Destination $sqlDriversDest -ErrorAction SilentlyContinue
    Write-Success "SQL driver copied"
}

Set-Location ..\..

# Create ZIP archive
if ($CreateZip) {
    Write-Info "Creating ZIP archive..."
    $version = "0.1.0"
    $zipName = "iPhoneStockManagement_v${version}_Windows_x64.zip"

    if (Test-Path $zipName) {
        Remove-Item $zipName
    }

    Compress-Archive -Path "build-windows\$deployDir\*" -DestinationPath $zipName -CompressionLevel Optimal
    Write-Success "ZIP archive created: $zipName"

    $zipSize = (Get-Item $zipName).Length / 1MB
    Write-Info ("File size: {0:N2} MB" -f $zipSize)
}

# Done
Write-Header "Build Complete!"
Write-Success "Output directory: build-windows\$deployDir\"
Write-Success "Executable: build-windows\$deployDir\$exeName"

if ($CreateZip) {
    Write-Success "ZIP archive: $zipName"
}

Write-Info "You can run the program directly or distribute the deploy folder"
Write-Info "Double-click $exeName to run"
