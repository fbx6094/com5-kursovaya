cmake_minimum_required(VERSION 3.19)

project(untitled LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets Network)

qt_standard_project_setup()


qt_add_executable(untitled_server
    server/main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    common/protocol.h
)

target_link_libraries(untitled_server
    PRIVATE
        Qt::Core
        Qt::Gui
        Qt::Widgets
        Qt::Network
)

target_include_directories(untitled_server
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

qt_add_executable(untitled_client
    client/main.cpp
    client/clientwindow.cpp
    client/clientwindow.h
)

target_link_libraries(untitled_client
    PRIVATE
        Qt::Core
        Qt::Gui
        Qt::Network
        Qt::Widgets
)