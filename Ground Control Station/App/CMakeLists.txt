cmake_minimum_required(VERSION 3.16)

project(App VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# This application is intended to build against Qt6 exclusively.
# Using find_package with Qt5 fallback can accidentally select an older
# Qt version, so we explicitly require Qt6 here.
find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
)

qt_add_executable(App
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
)

target_link_libraries(App PRIVATE Qt6::Widgets)

qt_finalize_executable(App)
