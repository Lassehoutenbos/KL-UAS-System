cmake_minimum_required(VERSION 3.16)
project(PICODE VERSION 0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS
    Quick
    QuickControls2
    SerialPort
    Multimedia
    Positioning
    Location
    Network
)

qt_standard_project_setup(REQUIRES 6.10)

set_source_files_properties(Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_executable(appPICODE
    main.cpp
    backend/gcsstate.h   backend/gcsstate.cpp
    backend/picolink.h   backend/picolink.cpp
    backend/mavlinklink.h backend/mavlinklink.cpp
)

qt_add_qml_module(appPICODE
    URI PICODE
    QML_FILES
        Main.qml
        Theme.qml
        components/StatusBar.qml
        components/NavBar.qml
        components/DataCard.qml
        components/BigButton.qml
        components/SliderRow.qml
        components/StatusDot.qml
        components/ConfirmOverlay.qml
        components/QuickPanel.qml
        components/CaseView.qml
        pages/DashPage.qml
        pages/DronePage.qml
        pages/CameraPage.qml
        pages/MapPage.qml
        pages/MissionPage.qml
        pages/ParamsPage.qml
        pages/PayloadPage.qml
        pages/PeriphPage.qml
        pages/CasePage.qml
)

set_target_properties(appPICODE PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE $<NOT:$<CONFIG:Debug>>
)

target_link_libraries(appPICODE PRIVATE
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::SerialPort
    Qt6::Multimedia
    Qt6::Positioning
    Qt6::Location
    Qt6::Network
)

install(TARGETS appPICODE
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
