cmake_minimum_required(VERSION 3.21)
project(MinecraftLauncher VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC_COMPILER_PREDEFS OFF)

find_package(Qt6 COMPONENTS Widgets Network Multimedia QUIET)
if(Qt6_FOUND)
    set(QT_LIBS Qt6::Widgets Qt6::Network Qt6::Multimedia)
else()
    find_package(Qt5 COMPONENTS Widgets Network Multimedia REQUIRED)
    set(QT_LIBS Qt5::Widgets Qt5::Network Qt5::Multimedia)
endif()

# Используем inline JSON парсер вместо внешней библиотеки
# или скачиваем header-only версию

set(SOURCES
    src/main.cpp
    src/core/config_manager.cpp
    src/core/downloader.cpp
    src/core/java_manager.cpp
    src/core/launcher_core.cpp
    src/ui/main_window.cpp
    src/ui/register_dialog.cpp
    src/ui/add_build_dialog.cpp
    src/ui/catalog_dialog.cpp
    src/ui/i18n.cpp
    src/ui/language_dialog.cpp
    src/ui/settings_dialog.cpp
    src/ui/notification.cpp
    src/ui/flow_layout.cpp
    resources/resources.qrc
)

if(WIN32)
    list(APPEND SOURCES resources/app_icon.obj)
    set_source_files_properties(resources/app_icon.obj PROPERTIES EXTERNAL_OBJECT TRUE GENERATED TRUE)
endif()

set(HEADERS
    src/core/config_manager.h
    src/core/downloader.h
    src/core/java_manager.h
    src/core/launcher_core.h
    src/core/build_info.h
    src/ui/main_window.h
    src/ui/register_dialog.h
    src/ui/add_build_dialog.h
    src/ui/catalog_dialog.h
    src/ui/i18n.h
    src/ui/language_dialog.h
    src/ui/settings_dialog.h
    src/ui/notification.h
    src/ui/flow_layout.h
)

add_executable(MinecraftLauncher WIN32 ${SOURCES} ${HEADERS})
set_target_properties(MinecraftLauncher PROPERTIES AUTOMOC_COMPILER_PREDEFS OFF)
set_target_properties(MinecraftLauncher PROPERTIES OUTPUT_NAME "LDL")

set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(SRC_DIR_SHORT "")
if(WIN32)
    execute_process(
        COMMAND cmd /c for %I in ("${SRC_DIR}") do @echo %~sI
        OUTPUT_VARIABLE SRC_DIR_SHORT
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()
if(SRC_DIR_SHORT STREQUAL "")
    set(SRC_DIR_SHORT "${SRC_DIR}")
endif()
target_include_directories(MinecraftLauncher PRIVATE "${SRC_DIR_SHORT}")
target_link_libraries(MinecraftLauncher PRIVATE ${QT_LIBS})

# Для Windows
if(WIN32)
    target_link_libraries(MinecraftLauncher PRIVATE ws2_32 wininet bcrypt)
    if(NOT MSVC)
        target_link_options(MinecraftLauncher PRIVATE -mwindows)
    endif()
    if(MSVC)
        target_compile_options(MinecraftLauncher PRIVATE /utf-8)
    endif()
endif()
