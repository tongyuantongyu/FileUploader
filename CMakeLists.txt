cmake_minimum_required(VERSION 3.10)
project(filetransfer)

set(CMAKE_CXX_STANDARD 17)

# switch it to on to compile test code
set(BUILD_TEST OFF)

if(CMAKE_BUILD_TYPE STREQUAL Release)
    message("Release mode.")
    add_definitions(-DELPP_DISABLE_DEBUG_LOGS)
    add_definitions(-DELPP_DISABLE_TRACE_LOGS)
else()
    message("Debug mode.")
endif()

set(NEED_SOURCES
        third_party/easyloggingpp/src/easylogging++.cc
        protocol.cpp
        file.cpp
        encrypt.cpp
        )

# auto detect cryptopp prebuilt static library
if(EXISTS ${CMAKE_SOURCE_DIR}/third_party/cryptopp/libcryptopp.a)
    set(cryptopplib ${CMAKE_SOURCE_DIR}/third_party/cryptopp/libcryptopp.a)
    message("Using your own built cryptopp library.")
else()
    if(MINGW AND EXISTS
       ${CMAKE_SOURCE_DIR}/third_party/cryptopp/.prebuilt/libcryptopp-mingw.a)
        message("Detect MINGW Environment")
        set(cryptopplib
            ${CMAKE_SOURCE_DIR}/third_party/cryptopp/.prebuilt/libcryptopp-mingw.a)
        message("Using prebuilt Mingw cryptopp library.")
    elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux"
           AND EXISTS
           ${CMAKE_SOURCE_DIR}/third_party/cryptopp/.prebuilt/libcryptopp-g++.a)
        message("Detect LINUX Environment")
        set(cryptopplib
            ${CMAKE_SOURCE_DIR}/third_party/cryptopp/.prebuilt/libcryptopp-g++.a)
        message("Using prebuilt G++ cryptopp library.")
    else()
        message(FATAL_ERROR
                "Neither own built nor prebuilt cryptopp library found. Please build the third party library first.")
    endif(MINGW AND EXISTS
          ${CMAKE_SOURCE_DIR}/third_party/cryptopp/.prebuilt/libcryptopp-mingw.a)
endif(EXISTS ${CMAKE_SOURCE_DIR}/third_party/cryptopp/libcryptopp.a)


# define required library by environment
if(MINGW)
    set(NEED_LIBS
            ws2_32.lib
            stdc++fs
            boost_random-mt
            ${cryptopplib}
            )

elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(NEED_LIBS
            stdc++fs
            pthread
            boost_random
            boost_system
            ${cryptopplib}
            )

# MSVC Support is under development

#elseif(MSVC)
#    message("Detect MSVC Environment")
#    SET(BOOST_ROOT "D:/Windows Kits/boost4vs")
#
#    SET(Boost_FIND_REQUIRED TRUE)
#    SET(Boost_FIND_QUIETLY TRUE)
#    SET(Boost_DEBUG FALSE)
#    SET(Boost_USE_MULTITHREADED TRUE)
#    SET(Boost_USE_STATIC_LIBS TRUE)
#
#    FIND_PACKAGE(Boost COMPONENTS random REQUIRED)
#    INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
#    LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})
#
#    set(NEED_LIBS
#            ws2_32.lib
#            ${Boost_LIBRARY_DIRS}/libboost_random-vc142-mt-x64-1_70.lib
#            ${CMAKE_SOURCE_DIR}/third_party/cryptopp/.prebuilt/cryptlib-release.lib
#            )

else()
    message(FATAL_ERROR "This project is not checked on your platform. Please configure required library in CMakeLists.txt yourself first.")
endif(MINGW)


# add executable
add_executable(server server.cpp
        ${NEED_SOURCES})

add_executable(client client.cpp
        ${NEED_SOURCES})

if(BUILD_TEST)
    add_executable(test_file
            debug_program/test_file.cpp
            third_party/easyloggingpp/src/easylogging++.cc
            file.cpp
            )
    add_executable(test_aes
            debug_program/test_aes.cpp
            third_party/easyloggingpp/src/easylogging++.cc
            encrypt.cpp
            )
    add_executable(test_asio
            debug_program/test_asio.cpp
            )
endif(BUILD_TEST)


# set need libraries
target_link_libraries(client ${NEED_LIBS})

if(WIN32)
    target_link_libraries(server ${NEED_LIBS} wsock32)
else()
    target_link_libraries(server ${NEED_LIBS})
endif(WIN32)

if(BUILD_TEST)
    if(WIN32)
        target_link_libraries(test_asio wsock32 ws2_32)
    endif(WIN32)

    target_link_libraries(test_aes ${cryptopplib})

    target_link_libraries(test_file stdc++fs)
endif(BUILD_TEST)