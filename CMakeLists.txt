cmake_minimum_required(VERSION 3.19)

project(untitled LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Network)

qt_standard_project_setup()

qt_add_executable(untitled
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    common/protocol.h
)

target_link_libraries(untitled
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Network
)

target_include_directories(untitled
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

qt_add_executable(untitled_server
    server/main.cpp
    common/protocol.h
)

target_link_libraries(untitled_server
    PRIVATE
        Qt::Core
        Qt::Network
)

target_include_directories(untitled_server
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

qt_add_executable(untitled_client
    client/main.cpp
)

target_link_libraries(untitled_client
    PRIVATE
        Qt::Core
        Qt::Network
)