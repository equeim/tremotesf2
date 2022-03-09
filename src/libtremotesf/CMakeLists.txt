if (WIN32)
    cmake_minimum_required(VERSION 3.21)
else()
    cmake_minimum_required(VERSION 3.16)
endif()
cmake_policy(VERSION ${CMAKE_MINIMUM_REQUIRED_VERSION}..3.23)

project(libtremotesf CXX)

option(TREMOTESF_QT6 "Build with Qt 6" ON)
option(TREMOTESF_BUILD_TESTS "Build tests" ON)

if (TREMOTESF_BUILD_TESTS)
    enable_testing()
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

include(cmake/CommonOptions.cmake)

find_package(Qt${TREMOTESF_QT_VERSION_MAJOR} ${TREMOTESF_MINIMUM_QT_VERSION} REQUIRED COMPONENTS Core Concurrent Network)
find_package(fmt 7.0 REQUIRED)

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
    rpc.cpp
    rpc.h
    println.h
    serversettings.cpp
    serversettings.h
    serverstats.cpp
    serverstats.h
    stdutils.h
    torrent.cpp
    torrent.h
    torrentfile.cpp
    torrentfile.h
    tracker.cpp
    tracker.h
)

target_link_libraries(libtremotesf PRIVATE Qt::Network PUBLIC Qt::Core fmt::fmt)
target_include_directories(libtremotesf PRIVATE ${Qt${TREMOTESF_QT_VERSION_MAJOR}Concurrent_INCLUDE_DIRS})
target_compile_definitions(libtremotesf PUBLIC FMT_UNICODE=1)

if (TREMOTESF_BUILD_TESTS AND NOT ANDROID)
    find_package(Qt${TREMOTESF_QT_VERSION_MAJOR} ${TREMOTESF_MINIMUM_QT_VERSION} REQUIRED COMPONENTS Test)
    add_executable(itemlistupdater_test itemlistupdater_test.cpp)
    add_test(NAME itemlistupdater_test COMMAND itemlistupdater_test)
    target_link_libraries(itemlistupdater_test libtremotesf Qt::Test)

    add_executable(println_test println_test.cpp)
    add_test(NAME println_test COMMAND println_test)
    target_link_libraries(println_test libtremotesf Qt::Test)
endif()

set_common_options_on_targets()
