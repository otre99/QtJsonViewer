cmake_minimum_required(VERSION 3.19)
project(QtJsonViewer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)
if(NOT WIN32)
    find_package(TBB REQUIRED)
endif()

find_package(simdjson CONFIG QUIET)
if(TARGET simdjson::simdjson AND NOT TARGET simdjson)
    add_library(simdjson ALIAS simdjson::simdjson)
endif()
if(NOT TARGET simdjson)
    include(FetchContent)
    set(SIMDJSON_JUST_LIBRARY ON CACHE INTERNAL "")
    set(SIMDJSON_BUILD_STATIC ON CACHE INTERNAL "")
    FetchContent_Declare(
        simdjson
        GIT_REPOSITORY https://github.com/simdjson/simdjson.git
        GIT_TAG v4.6.2
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(simdjson)
endif()

qt_standard_project_setup()

qt_add_executable(QtJsonViewer
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    json_nodes.h json_nodes.cpp
    json_treeview_model.h json_treeview_model.cpp
    list_view_search_model.h list_view_search_model.cpp
    text_matcher.h text_matcher.cpp
)



target_link_libraries(QtJsonViewer
    PRIVATE
        Qt::Core
        Qt::Widgets
        simdjson
)

if(NOT WIN32)
    target_link_libraries(QtJsonViewer PRIVATE TBB::tbb)
endif()

include(GNUInstallDirs)

install(TARGETS QtJsonViewer
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET QtJsonViewer
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
