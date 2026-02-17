# Windows 构建指南 (最新版)

本文档详细介绍了在 Windows 上构建和打包 iPhone 库存管理系统的最新流程。

推荐使用 **PowerShell 自动化脚本 (`deploy_windows.ps1`)** 进行构建，该脚本包含自动环境检测、编译、依赖部署 (windeployqt) 和 ZIP 打包功能。

## 📋 环境准备

### 必需软件

1.  **Visual Studio 2019 或 2022**
    *   安装时勾选 "**使用 C++ 的桌面开发**" 工作负载。
    *   确保包含 MSVC 编译器。
2.  **Qt 6.4 或更高版本** (推荐 Qt 6.5 LTS)
    *   安装 MSVC 对应的版本（例如 `msvc2019_64` 或 `msvc2022_64`）。
    *   **注意**: 必须与 Visual Studio 版本兼容。
3.  **CMake 3.16 或更高版本**
    *   安装时选择 "Add CMake to the system PATH"。

---

## 🚀 快速构建 (推荐方法)

使用 `deploy_windows.ps1` 脚本可以一键完成所有步骤。

### 第 1 步：打开 Visual Studio 开发命令行

**重要**: 必须在 VS 的开发环境中运行，否则会找不到 `nmake` 编译器。

1.  点击 Windows 开始菜单。
2.  搜索并运行 **"x64 Native Tools Command Prompt for VS 2022"** (或 VS 2019)。
    *   *提示: 建议右键 "以管理员身份运行" 以避免权限问题。*

### 第 2 步：进入项目目录

```cmd
cd /d C:\Users\samue\Downloads\iPhoneStock\iPhone
```
*(请根据实际路径调整)*

### 第 3 步：设置 PowerShell 权限 (仅首次需要)

如果遇到 "禁止运行脚本" 的错误，请执行：
```powershell
powershell Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
```

### 第 4 步：运行构建脚本

直接运行脚本，它会自动尝试检测 Qt 路径并开始构建：

```powershell
powershell .\deploy_windows.ps1
```

**如果脚本无法自动找到 Qt**，请手动指定路径：

```powershell
powershell .\deploy_windows.ps1 -QtPath "C:\Qt\6.5.0\msvc2019_64"
```

### 常用参数说明

| 参数 | 说明 | 示例 |
| :--- | :--- | :--- |
| `-QtPath` | 指定 Qt 安装目录 (包含 bin\qmake.exe 的父目录) | `-QtPath "C:\Qt\6.5.3\msvc2019_64"` |
| `-BuildType` | 构建类型: `Release` (默认) 或 `Debug` | `-BuildType Debug` |
| `-CleanBuild` | 是否先清理旧的构建目录 ($true/$false) | `-CleanBuild $false` |
| `-CreateZip` | 是否在完成后创建 ZIP 压缩包 | `-CreateZip $false` |

**示例命令：**
```powershell
# 强制重新构建并指定 Qt 路径
powershell .\deploy_windows.ps1 -QtPath "C:\Qt\6.5.0\msvc2019_64" -CleanBuild $true
```

## 📦 构建结果

脚本运行完成后，您可以在以下位置找到文件：

1.  **可执行程序目录**: `build-windows\deploy\`
    *   包含 `.exe` 和所有依赖的 `.dll` 文件，可直接运行。
2.  **发布压缩包**: `iPhoneStockManagement_v0.1.0_Windows_x64.zip`
    *   位于项目根目录下，可直接分发给用户。

---

## 🛠️ 备用方法

### 方法 2：批处理脚本 (`build_windows.bat`)

如果你无法使用 PowerShell，可以使用批处理脚本：
1.  打开 **x64 Native Tools Command Prompt**。
2.  运行 `build_windows.bat`。
3.  *注意: 此脚本不会自动创建 ZIP 包，且 Qt 路径检测能力较弱。*

### 方法 3：手动构建 (高级)

如果脚本均无法工作，可以手动逐行执行命令：

```cmd
:: 1.设置环境变量 (根据实际情况修改)
set PATH=C:\Qt\6.5.0\msvc2019_64\bin;%PATH%

:: 2. 创建构建目录
mkdir build-windows
cd build-windows

:: 3. 配置 CMake (生成 NMake Makefiles)
cmake .. -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release

:: 4. 编译
cmake --build . --config Release

:: 5. 部署依赖
mkdir deploy
copy iPhoneStockManagement.exe deploy\
cd deploy
windeployqt iPhoneStockManagement.exe --release --no-translations

:: 6. 复制 SQL 驱动 (通常 windeployqt 会处理，但为了保险)
:: (需要从 C:\Qt\...\plugins\sqldrivers 复制 qsqlite.dll 到 deploy\sqldrivers)
```

---

## ❓ 常见问题排查

**Q: 出现 "‘nmake’ 不是内部或外部命令"？**
A: 你没有在 VS 的 **Native Tools Command Prompt** 中运行。普通的 CMD 或 PowerShell 无法识别 `nmake`。

**Q: 出现中文乱码？**
A: 请在执行脚本前，在命令行中输入 `chcp 65001` 切换编码。

**Q: 提示 "Qt installation not found"？**
A: 使用 `-QtPath` 参数明确指定你的 Qt MSVC 版本路径。

**Q: windeployqt 运行后仍然缺少 DLL？**
A: 检查是否安装了对应的 C++ 运行时 (VC Redistributable)。大部分情况下脚本生成的 `deploy` 文件夹已包含所需文件。
