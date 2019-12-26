cmake_minimum_required(VERSION 3.0.0)

project(tremotesf VERSION 1.6.1 LANGUAGES CXX)

option(TEST_BUILD "Test build" OFF)
option(SAILFISHOS "Build for Sailfish OS" OFF)

if (TEST_BUILD AND SAILFISHOS)
    message(FATAL_ERROR "TEST_BUILD and SAILFISHOS can't be used together")
endif()

include(GNUInstallDirs)

if (SAILFISHOS)
    set(TREMOTESF_NAME "harbour-tremotesf")
else()
    set(TREMOTESF_NAME "tremotesf")
endif()

set(DATA_PATH "${CMAKE_INSTALL_FULL_DATADIR}/${TREMOTESF_NAME}")

find_package(Qt5 5.6 REQUIRED COMPONENTS Core)

add_subdirectory("data")
add_subdirectory("translations")
add_subdirectory("src")

if (SAILFISHOS)
    install(DIRECTORY "qml" DESTINATION "${DATA_PATH}")
endif()
