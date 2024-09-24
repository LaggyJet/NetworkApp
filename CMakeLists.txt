cmake_minimum_required(VERSION 3.16)

project(NetworkApp VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

set(PROJECT_SOURCES
    main.cpp
    reglog.cpp
    reglog.h
    reglog.ui
    clientuser.h
    clientuser.cpp
    hostuser.h
    hostuser.cpp
    resources.qrc
    appscreen.h
    appscreen.cpp
    appscreen.ui
    hostsettingpopup.h
    hostsettingpopup.cpp
    hostsettingpopup.ui
    connectjoinpopup.h
    connectjoinpopup.cpp
    connectjoinpopup.ui
    connectsettingpopup.h
    connectsettingpopup.cpp
    connectsettingpopup.ui
    logger.h
    logger.cpp
)

qt_add_executable(NetworkApp
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
)

qt_add_resources(NetworkApp "resources"
    ${CMAKE_CURRENT_SOURCE_DIR}/resources.qrc
)

target_link_libraries(NetworkApp PRIVATE Qt6::Widgets Qt6::Network)

qt_finalize_executable(NetworkApp)

if(WIN32)
    set_target_properties(NetworkApp PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
    target_link_libraries(NetworkApp PRIVATE Ws2_32)
endif()

include(GNUInstallDirs)
install(TARGETS NetworkApp
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
