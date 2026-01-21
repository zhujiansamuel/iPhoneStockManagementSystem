cmake_minimum_required(VERSION 3.16)

project(iPhoneStockManagementSystem VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- Windows 特定设置 ----
if(WIN32)
    set(CMAKE_WIN32_EXECUTABLE ON)  # 创建 Windows GUI 应用（不显示控制台）
    # 设置输出目录
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
endif()

# ---- Qt 6 组件：Widgets + SvgWidgets + Sql + Multimedia + Network ----
find_package(Qt6 REQUIRED COMPONENTS Widgets SvgWidgets Sql Multimedia Network)

qt_standard_project_setup()

# ---- 源文件 ----
set(UI_FILES
    Mainwindow.ui         # 如果你的文件叫 MainWindow.ui，就改成 MainWindow.ui
)

set(SRC_FILES
    main.cpp
    mainwindow.cpp
)

set(HDR_FILES
    mainwindow.h
)

# ---- Windows 资源文件 ----
if(WIN32)
    #set(APP_ICON_RC "${CMAKE_CURRENT_SOURCE_DIR}/app.rc")
    enable_language(RC)
endif()

# ---- 可执行程序 + 资源 ----
if(WIN32)
    qt_add_executable(${PROJECT_NAME} WIN32
        ${SRC_FILES}
        ${HDR_FILES}
        ${UI_FILES}
        ${APP_ICON_RC}
    )
else()
    qt_add_executable(${PROJECT_NAME}
        ${SRC_FILES}
        ${HDR_FILES}
        ${UI_FILES}
    )
endif()

qt_add_resources(${PROJECT_NAME} "app-resources"
    PREFIX "/"
    BASE "pic"
    FILES
        pic/Reset.svg
        pic/Search.svg
        pic/Stock_Registration.svg
        pic/Temporary_Registration.svg
        pic/Temporary_Registration_Write.svg
        pic/View_Summary.svg
        pic/Excel_Output.svg
        pic/Excel_Display.svg
)

# ---- 音频资源（需要时取消注释）----
# 注意：请先在 sounds/ 目录下添加 WAV 音频文件，然后取消下面的注释
# 详见 sounds/README.md 了解如何生成音频文件
qt_add_resources(${PROJECT_NAME} "sound-resources"
    PREFIX "/sounds"
    BASE "sounds"
    FILES
        sounds/success.wav
        sounds/jan_error.wav
        sounds/jan_not_found.wav
        sounds/imei_error.wav
        sounds/imei_duplicate.wav
        sounds/count_reset.wav
)

# ---- 链接 Qt ----
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Widgets
    Qt6::SvgWidgets
    Qt6::Sql
    Qt6::Multimedia
    Qt6::Network
)

# ==================== 引入 QXlsx（FetchContent）====================
add_subdirectory(QXlsx-master/QXlsx)
target_link_libraries(${PROJECT_NAME} PRIVATE QXlsx::QXlsx)

# ==================== Windows 部署配置 ====================
if(WIN32)
    # 设置应用程序属性
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
        OUTPUT_NAME "iPhoneStockManagement"
    )

    # 使用 windeployqt 自动部署 Qt 依赖
    # 在安装阶段，windeployqt 会被打包脚本调用
    install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION .
    )
endif()
