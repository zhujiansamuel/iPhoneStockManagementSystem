cmake_minimum_required(VERSION 3.16)

project(iPhoneStockManagementSystem VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- Qt 6 组件：Widgets + SvgWidgets + Sql（你代码里都用到了）----
find_package(Qt6 REQUIRED COMPONENTS Widgets SvgWidgets Sql)

qt_standard_project_setup()

# ---- 源文件 ----
set(UI_FILES
    mainwindow.ui         # 如果你的文件叫 MainWindow.ui，就改成 MainWindow.ui
)

set(SRC_FILES
    main.cpp
    mainwindow.cpp
)

set(HDR_FILES
    mainwindow.h
)

# ---- 可执行程序 + 资源 ----
qt_add_executable(${PROJECT_NAME}
    ${SRC_FILES}
    ${HDR_FILES}
    ${UI_FILES}
)

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

# ---- 链接 Qt ----
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Widgets
    Qt6::SvgWidgets
    Qt6::Sql
)

# ==================== 引入 QXlsx（FetchContent）====================
add_subdirectory(QXlsx-master/QXlsx)
target_link_libraries(${PROJECT_NAME} PRIVATE QXlsx::QXlsx)
