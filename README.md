# iPhone 库存管理系统

一套基于 Qt 6 的桌面端库存管理工具，用于 iPhone 设备的入库、暂存、查询与 Excel 导出。应用内置 iPhone 型号目录与颜色标识，结合扫描枪输入，快速完成入库登记与追踪记录。

## ✨ 功能概览

- **入荷登记**：支持扫描 13 位 JAN 与 15 位 IMEI 并写入 SQLite。
- **仮登记**：临时录入后可一键写入入库记录，并对重复 IMEI 进行标红提示。
- **检索**：按扫码规则快速定位设备信息与记录。
- **会话记录**：启动时可选择继续上次会话或创建新会话，保留历史记录。
- **Excel 导出**：支持导出当前会话到 Excel（QXlsx），并记录最近一次导出路径。

## 🧰 技术栈

- **Qt 6 / C++**：桌面 UI 与逻辑实现。
- **SQLite**：本地数据库存储。
- **QXlsx**：Excel 生成与导出。

## 📁 项目结构

- `mainwindow.cpp / mainwindow.h / Mainwindow.ui`：主窗口 UI 与业务逻辑。
- `QXlsx-master/`：Excel 导出依赖。
- `sounds/`：提示音资源。
- `pic/`：图标与图片资源。
- `WINDOWS_BUILD.md`：Windows 端打包与部署说明。

## 🚀 构建与运行

项目以 Windows 端部署为主，完整流程请参考 [WINDOWS_BUILD.md](WINDOWS_BUILD.md)。以下是简化版指引：

### 前置要求

- Qt 6.4+（建议 6.5+）
- CMake 3.16+
- Visual Studio 2019+（含“使用 C++ 的桌面开发”工作负载）

### 方式一：PowerShell 一键打包（推荐）

```powershell
# 进入项目目录
cd path\to\iPhoneStockManagementSystem

# 自动检测 Qt 路径并构建
.\deploy_windows.ps1
```

### 方式二：手动构建（概要）

```cmd
mkdir build-windows
cd build-windows
cmake .. -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

## 🗃️ 数据说明

- 本地数据库使用 SQLite，自动初始化必要的表结构。
- 内置 iPhone 目录用于 JAN 码查询与颜色映射。

## 📦 导出说明

- 导出 Excel 依赖 QXlsx。
- 导出文件保存在本机，界面可直接打开最近一次导出。

## 🤝 贡献

欢迎提交问题与建议。如需扩展型号目录或导出模板，请在需求说明中注明。

## 📄 许可

如需明确许可协议，请补充 LICENSE 文件。
