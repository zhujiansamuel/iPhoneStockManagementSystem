# Windows 打包说明

本文档介绍如何在 Windows 上构建和打包 iPhone 库存管理系统。

## 📋 前置要求

### 必需软件

1. **Qt 6.4 或更高版本**
   - 推荐：Qt 6.5.0 或更高
   - 下载地址：https://www.qt.io/download-qt-installer
   - 安装时选择 MSVC 版本（例如：msvc2019_64 或 msvc2022_64）

2. **CMake 3.16 或更高版本**
   - 下载地址：https://cmake.org/download/
   - 安装时选择"Add CMake to the system PATH"

3. **Visual Studio 2019 或更高版本**
   - 下载地址：https://visualstudio.microsoft.com/zh-hans/downloads/
   - 必须安装"使用 C++ 的桌面开发"工作负载
   - Community 版本免费且足够使用

### 可选软件

- **7-Zip** 或其他压缩工具（用于打包分发）
- **NSIS** 或 **Inno Setup**（用于制作安装程序）

## 🚀 快速开始

### 方法 1：使用 PowerShell 脚本（推荐）

1. **打开正确的命令提示符**
   - 在开始菜单中搜索"x64 Native Tools Command Prompt for VS 2019"或"VS 2022"
   - 以管理员身份运行

2. **设置 PowerShell 执行策略**（首次使用时）
   ```powershell
   Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
   ```

3. **进入项目目录**
   ```powershell
   cd path\to\iPhoneStockManagementSystem
   ```

4. **运行打包脚本**

   自动检测 Qt 路径：
   ```powershell
   .\deploy_windows.ps1
   ```

   或手动指定 Qt 路径：
   ```powershell
   .\deploy_windows.ps1 -QtPath "C:\Qt\6.5.0\msvc2019_64"
   ```

   其他选项：
   ```powershell
   # Debug 构建
   .\deploy_windows.ps1 -BuildType Debug

   # 不创建 ZIP 包
   .\deploy_windows.ps1 -CreateZip $false

   # 不清理旧构建
   .\deploy_windows.ps1 -CleanBuild $false
   ```

5. **完成**
   - 可执行文件位于：`build-windows\deploy\`
   - ZIP 包：`iPhoneStockManagement_v0.1.0_Windows_x64.zip`

### 方法 2：使用批处理脚本

1. **打开 Visual Studio 命令提示符**
   - 在开始菜单中搜索"x64 Native Tools Command Prompt for VS 2019"或"VS 2022"
   - 运行该命令提示符

2. **设置 Qt 环境变量**（如果 Qt 不在 PATH 中）
   ```cmd
   set PATH=C:\Qt\6.5.0\msvc2019_64\bin;%PATH%
   ```

3. **运行构建脚本**
   ```cmd
   cd path\to\iPhoneStockManagementSystem
   build_windows.bat
   ```

4. **完成**
   - 可执行文件位于：`build-windows\deploy\iPhoneStockManagement.exe`

### 方法 3：手动构建

1. **打开 Visual Studio 命令提示符**
   ```cmd
   # 设置 Qt 路径（如需要）
   set PATH=C:\Qt\6.5.0\msvc2019_64\bin;%PATH%
   set Qt6_DIR=C:\Qt\6.5.0\msvc2019_64
   ```

2. **创建并进入构建目录**
   ```cmd
   mkdir build-windows
   cd build-windows
   ```

3. **配置 CMake**
   ```cmd
   cmake .. -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release
   ```

4. **编译**
   ```cmd
   cmake --build . --config Release
   ```

5. **部署**
   ```cmd
   mkdir deploy
   copy iPhoneStockManagement.exe deploy\
   cd deploy
   windeployqt iPhoneStockManagement.exe --release --no-translations
   ```

6. **复制 SQL 驱动**
   ```cmd
   mkdir sqldrivers
   copy C:\Qt\6.5.0\msvc2019_64\plugins\sqldrivers\qsqlite.dll sqldrivers\
   ```

## 📦 打包和分发

### 创建 ZIP 压缩包

```powershell
# 使用 PowerShell
Compress-Archive -Path build-windows\deploy\* -DestinationPath iPhoneStockManagement_v0.1.0_Windows_x64.zip
```

或使用 7-Zip：
```cmd
7z a -tzip iPhoneStockManagement_v0.1.0_Windows_x64.zip build-windows\deploy\*
```

### 制作安装程序（可选）

可以使用以下工具创建专业的安装程序：

1. **NSIS**（Nullsoft Scriptable Install System）
   - 轻量级，脚本驱动
   - 下载：https://nsis.sourceforge.io/

2. **Inno Setup**
   - 功能强大，易于使用
   - 下载：https://jrsoftware.org/isinfo.php

3. **Qt Installer Framework**
   - Qt 官方工具
   - 集成度高

## 🔧 自定义配置

### 修改应用程序图标

1. 准备一个 256x256 或更大的 PNG 图片
2. 转换为 .ico 格式：
   - 在线工具：https://convertico.com/
   - ImageMagick：`convert icon.png -define icon:auto-resize=256,128,64,48,32,16 app.ico`
   - GIMP：打开 PNG，导出为 .ico
3. 将 `app.ico` 放在项目根目录
4. 重新编译

### 修改版本信息

编辑 `app.rc` 文件中的版本信息：
```rc
FILEVERSION     0,1,0,0
PRODUCTVERSION  0,1,0,0
...
VALUE "FileVersion", "0.1.0.0\0"
VALUE "ProductVersion", "0.1.0.0\0"
```

编辑 `CMakeLists.txt` 中的版本号：
```cmake
project(iPhoneStockManagementSystem VERSION 0.1 LANGUAGES CXX)
```

## 🐛 常见问题

### Q: 运行脚本时提示"无法加载文件，因为在此系统上禁止运行脚本"

**A:** 需要修改 PowerShell 执行策略：
```powershell
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
```

### Q: 找不到 qmake 或 cmake

**A:** 确保 Qt 和 CMake 已添加到系统 PATH，或者：
```cmd
set PATH=C:\Qt\6.5.0\msvc2019_64\bin;C:\Program Files\CMake\bin;%PATH%
```

### Q: 编译时出现"找不到 nmake"

**A:** 必须在 Visual Studio 的命令提示符中运行：
- 开始菜单 → Visual Studio 2019/2022 → x64 Native Tools Command Prompt

### Q: windeployqt 后程序仍然缺少 DLL

**A:** 可能需要手动复制缺少的 DLL：
1. 查看错误消息中提示的 DLL 名称
2. 在 Qt 安装目录中搜索该 DLL
3. 复制到 `deploy` 文件夹

常见的额外依赖：
- `vcruntime140.dll`、`msvcp140.dll`：Visual C++ 运行时（通常由 windeployqt 处理）
- SQL 驱动：`sqldrivers\qsqlite.dll`

### Q: 程序运行时数据库无法打开

**A:** 确保 SQL 驱动已正确部署：
```
deploy/
├── iPhoneStockManagement.exe
├── sqldrivers/
│   └── qsqlite.dll
└── ... (其他 Qt DLLs)
```

### Q: 想要生成控制台版本（显示调试输出）

**A:** 修改 `CMakeLists.txt`：
```cmake
# 注释掉这一行：
# set(CMAKE_WIN32_EXECUTABLE ON)

# 或者在 qt_add_executable 中移除 WIN32 标志
```

## 📝 文件结构

构建完成后的目录结构：

```
iPhoneStockManagementSystem/
├── CMakeLists.txt              # 构建配置文件（已修改）
├── app.rc                      # Windows 资源文件（新增）
├── app.ico                     # 应用图标（需自行添加）
├── build_windows.bat           # 批处理构建脚本（新增）
├── deploy_windows.ps1          # PowerShell 构建脚本（新增）
├── WINDOWS_BUILD.md            # 本说明文档（新增）
├── app_icon_instructions.txt   # 图标制作说明（新增）
├── build-windows/              # 构建目录（自动生成）
│   ├── deploy/                 # 部署目录
│   │   ├── iPhoneStockManagement.exe
│   │   ├── Qt6Core.dll
│   │   ├── Qt6Gui.dll
│   │   ├── Qt6Widgets.dll
│   │   ├── Qt6Sql.dll
│   │   ├── Qt6Svg.dll
│   │   ├── sqldrivers/
│   │   │   └── qsqlite.dll
│   │   └── ... (其他 Qt DLLs)
│   └── ...
└── iPhoneStockManagement_v0.1.0_Windows_x64.zip  # ZIP 包
```

## 🔄 更新构建

如果代码有更新，重新构建：

```powershell
# 完全重新构建
.\deploy_windows.ps1 -CleanBuild $true

# 或者只重新编译
cd build-windows
cmake --build . --config Release
cd deploy
windeployqt iPhoneStockManagement.exe --release
```

## 📞 技术支持

如遇到问题，请检查：
1. 所有前置软件是否正确安装
2. 是否在正确的命令提示符中运行（VS Native Tools Command Prompt）
3. Qt 路径是否正确
4. 错误日志中的具体错误信息

## 📄 许可证

本项目的许可证信息请参考主项目文档。

---

**祝构建顺利！** 🎉
