if (WIN32)
    cmake_minimum_required(VERSION 3.22)
else()
    cmake_minimum_required(VERSION 3.12)
endif()
cmake_policy(VERSION ${CMAKE_MINIMUM_REQUIRED_VERSION}..3.22)

project(libtremotesf CXX)

option(TREMOTESF_QT6 "Build with Qt 6" ON)
option(TREMOTESF_BUILD_TESTS "Build tests" ON)
option(TREMOTESF_SAILFISHOS "Build for Sailfish OS" OFF)

if (TREMOTESF_BUILD_TESTS)
    enable_testing()
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

include(cmake/CommonOptions.cmake)

find_package(Qt${TREMOTESF_QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Concurrent Network)
if (TREMOTESF_SAILFISHOS)
    find_package(Qt${TREMOTESF_QT_VERSION_MAJOR} REQUIRED COMPONENTS Qml)
endif()

if (ANDROID)
    set(library_type STATIC)
else()
    set(library_type OBJECT)
endif()

add_library(
    libtremotesf
    ${library_type}
    itemlistupdater.h
    peer.cpp
    peer.h
    qtsupport.h
    rpc.cpp
    rpc.h
    serversettings.cpp
    serversettings.h
    serverstats.cpp
    serverstats.h
    stdutils.h
    torrent.cpp
    torrent.h
    torrent_qdebug.h
    torrentfile.cpp
    torrentfile.h
    tracker.cpp
    tracker.h
)

target_link_libraries(libtremotesf Qt${TREMOTESF_QT_VERSION_MAJOR}::Core Qt${TREMOTESF_QT_VERSION_MAJOR}::Network)
if (TREMOTESF_SAILFISHOS)
    target_link_libraries(libtremotesf Qt${TREMOTESF_QT_VERSION_MAJOR}::Qml)
endif()
target_include_directories(libtremotesf PRIVATE ${Qt${TREMOTESF_QT_VERSION_MAJOR}Concurrent_INCLUDE_DIRS})

if (TREMOTESF_BUILD_TESTS AND NOT ANDROID)
    find_package(Qt${TREMOTESF_QT_VERSION_MAJOR} REQUIRED COMPONENTS Test)
    add_executable(itemlistupdater_test itemlistupdater_test.cpp)
    add_test(NAME itemlistupdater_test COMMAND itemlistupdater_test)
    target_link_libraries(itemlistupdater_test libtremotesf Qt${TREMOTESF_QT_VERSION_MAJOR}::Test)
endif()

set_common_options_on_targets()
