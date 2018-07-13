cmake_minimum_required(VERSION 3.1.0)

project(tremotesf)
set(TREMOTESF_VERSION 1.4.0)

option(SAILFISHOS "Build for Sailfish OS" OFF)

include(GNUInstallDirs)

if (SAILFISHOS)
    set(TREMOTESF_NAME "harbour-tremotesf")
else()
    set(TREMOTESF_NAME "tremotesf")
endif()
file(TO_CMAKE_PATH "${CMAKE_INSTALL_DATADIR}/${TREMOTESF_NAME}" DATA_PATH)

add_subdirectory("src")
add_subdirectory("translations")
add_subdirectory("data")

if (SAILFISHOS)
    install(DIRECTORY "qml" DESTINATION "${DATA_PATH}")
endif()
