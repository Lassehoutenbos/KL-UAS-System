cmake_minimum_required(VERSION 3.16)
project(GroundControlStation LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets SerialPort)
find_package(PkgConfig REQUIRED)
pkg_check_modules(HIDAPI REQUIRED hidapi-hidraw)

add_executable(gcs
    src/main.cpp
    src/MainWindow.cpp
    src/SerialHandler.cpp
    src/HIDHandler.cpp
)

target_include_directories(gcs PRIVATE src ${HIDAPI_INCLUDE_DIRS})
target_link_libraries(gcs PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::SerialPort ${HIDAPI_LIBRARIES})
