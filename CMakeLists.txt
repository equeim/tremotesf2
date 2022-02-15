if (WIN32)
    cmake_minimum_required(VERSION 3.21)
else()
    cmake_minimum_required(VERSION 3.12)
endif()
cmake_policy(VERSION ${CMAKE_MINIMUM_REQUIRED_VERSION}..3.22)

project(tremotesf VERSION 1.11.0 LANGUAGES CXX)

option(TREMOTESF_BUILD_TESTS "Build tests" ON)
option(TREMOTESF_SAILFISHOS "Build for Sailfish OS" OFF)

include(GNUInstallDirs)

if (TREMOTESF_BUILD_TESTS)
    enable_testing()
endif()

if (TREMOTESF_SAILFISHOS)
    set(TREMOTESF_NAME "harbour-tremotesf")
else()
    set(TREMOTESF_NAME "tremotesf")
endif()

set(DATA_PATH "${CMAKE_INSTALL_FULL_DATADIR}/${TREMOTESF_NAME}")

find_package(Qt5 5.6 REQUIRED COMPONENTS Core)

set(QRC_FILES "")
add_subdirectory("data")
add_subdirectory("translations")
add_subdirectory("qml")
add_subdirectory("src")
add_subdirectory("packaging")
