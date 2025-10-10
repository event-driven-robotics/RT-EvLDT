# Copyright: (C) 2024 EDPR - Istituto Italiano di Tecnologia
# Authors: Mikihiro Ikura, Arren Glover

# requires minimum cmake version
cmake_minimum_required(VERSION 3.16.0)

# Project definition
project(ledge_example)

find_package(OpenCV REQUIRED)
find_package(YARP COMPONENTS os sig cv REQUIRED)
find_package(event-driven REQUIRED)
find_package(ledge REQUIRED)
find_package(Threads REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -mtune=native -march=native -mfpmath=both")
set(CMAKE_BUILD_TYPE Release)

add_executable(ledge-offline-par ledge_offline_parallel.cpp)
target_link_libraries(ledge-offline-par ledge::ledge Threads::Threads)

add_executable(ledge-offline-seq ledge_offline_sequential.cpp)
target_link_libraries(ledge-offline-seq ledge::ledge Threads::Threads)

add_executable(ledge-online ledge_online.cpp)
target_link_libraries(ledge-online ledge::ledge Threads::Threads)

add_executable(ledge-online-laptopcam ledge_online_laptopcam.cpp)
target_link_libraries(ledge-online-laptopcam ledge::ledge Threads::Threads)

add_executable(ledge-offline-par-process-time ledge_offline_parallel_processing_time.cpp)
target_link_libraries(ledge-offline-par-process-time ledge::ledge Threads::Threads)