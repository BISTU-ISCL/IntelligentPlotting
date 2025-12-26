cmake_minimum_required(VERSION 3.16)
project(IntelligentPlotting VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(BUILD_DEMO "是否构建Qt演示程序" ON)

add_library(plotting_middleware SHARED
    src/PlottingMiddleware.cpp
)

target_include_directories(plotting_middleware
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

if(WIN32)
    target_compile_definitions(plotting_middleware PRIVATE PLOTTING_MIDDLEWARE_EXPORT)
else()
    target_compile_definitions(plotting_middleware PRIVATE PLOTTING_MIDDLEWARE_EXPORT __LINUX__)
endif()

find_package(Threads REQUIRED)
target_link_libraries(plotting_middleware PRIVATE Threads::Threads)

if(BUILD_DEMO)
    find_package(Qt6 COMPONENTS Widgets QUIET)
    if(NOT Qt6_FOUND)
        find_package(Qt5 COMPONENTS Widgets QUIET)
    endif()

    if(Qt6_FOUND)
        message(STATUS "启用Qt6演示程序构建")
        set(QT_LIBS Qt6::Widgets)
    elseif(Qt5_FOUND)
        message(STATUS "启用Qt5演示程序构建")
        set(QT_LIBS Qt5::Widgets)
    else()
        message(WARNING "未找到Qt5/Qt6，跳过演示程序构建")
    endif()

    if(QT_LIBS)
        set(CMAKE_AUTOMOC ON)
        set(CMAKE_AUTORCC ON)
        set(CMAKE_AUTOUIC ON)

        add_executable(chat_demo
            demo/main.cpp
            demo/ChatWindow.cpp
            demo/ChatWindow.h
        )
        target_link_libraries(chat_demo PRIVATE plotting_middleware ${QT_LIBS})
    endif()
endif()
