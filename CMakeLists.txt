cmake_minimum_required(VERSION 3.21)
project(poisson_fuzzy VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(PF_BUILD_GUI      "Build Qt GUI demo"     OFF)
option(PF_BUILD_EXAMPLES "Build console example" ON)

# ---------- Core library ----------
add_library(poisson_fuzzy
    src/fuzzy.cpp
    src/poisson.cpp
    src/clustering.cpp
)
add_library(poisson_fuzzy::poisson_fuzzy ALIAS poisson_fuzzy)

target_include_directories(poisson_fuzzy
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(poisson_fuzzy PUBLIC cxx_std_17)

# ---------- Install ----------
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS poisson_fuzzy
    EXPORT poisson_fuzzyTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT poisson_fuzzyTargets
    FILE poisson_fuzzyTargets.cmake
    NAMESPACE poisson_fuzzy::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/poisson_fuzzy
)

configure_package_config_file(
    cmake/poisson_fuzzyConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/poisson_fuzzyConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/poisson_fuzzy
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/poisson_fuzzyConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/poisson_fuzzyConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/poisson_fuzzyConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/poisson_fuzzy
)

# ---------- Examples ----------
if(PF_BUILD_EXAMPLES)
    add_executable(pf_console_demo examples/console_demo.cpp)
    target_link_libraries(pf_console_demo PRIVATE poisson_fuzzy)
endif()

# ---------- GUI ----------
if(PF_BUILD_GUI)
    find_package(Qt6 REQUIRED COMPONENTS Widgets Charts Concurrent)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTORCC ON)
    add_executable(pf_gui
        gui/main.cpp
        gui/mainwindow.h
        gui/mainwindow.cpp
    )
    target_link_libraries(pf_gui PRIVATE
        poisson_fuzzy Qt6::Widgets Qt6::Charts Qt6::Concurrent)
endif()