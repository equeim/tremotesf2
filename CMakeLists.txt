cmake_minimum_required(VERSION 3.10)
cmake_policy(VERSION ${CMAKE_MINIMUM_REQUIRED_VERSION}..3.20)

project(tremotesf VERSION 1.10.0 LANGUAGES CXX)

option(TEST_BUILD "Test build" OFF)
option(TREMOTESF_SAILFISHOS "Build for Sailfish OS" OFF)
option(TREMOTESF_BUILD_TESTS "Build tests" OFF)

if (TEST_BUILD AND TREMOTESF_SAILFISHOS)
    message(FATAL_ERROR "TEST_BUILD and TREMOTESF_SAILFISHOS can't be used together")
endif()

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
