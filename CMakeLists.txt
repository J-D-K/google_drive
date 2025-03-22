cmake_minimum_required(VERSION 3.30)

project(google_drive)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE
               source/curl.cpp
               source/GoogleDrive.cpp
               source/Item.cpp
               source/Local.cpp
               source/logger.cpp
               source/main.cpp
               source/Storage.cpp
               source/stringutil.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE include)

target_compile_options(${PROJECT_NAME} PRIVATE -O2)

target_link_options(${PROJECT_NAME} PRIVATE -s)
target_link_libraries(${PROJECT_NAME} PRIVATE -ljson-c -lcurl)
