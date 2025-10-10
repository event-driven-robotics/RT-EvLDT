# Copyright: (C) 2024 EDPR - Istituto Italiano di Tecnologia
# Authors: Mikihiro Ikura, Arren Glover

# requires minimum cmake version
cmake_minimum_required(VERSION 3.16.0)

# Project definition
project(ledge_unit_test)

set(CMAKE_BUILD_TYPE Debug)

find_package(OpenCV REQUIRED)
find_package(YARP COMPONENTS os sig cv REQUIRED)
find_package(event-driven REQUIRED)
find_package(ledge REQUIRED)
find_package(GTest REQUIRED)

add_executable(test_ledge_core test_ledge_core.cpp fixture.h)
target_link_libraries(test_ledge_core GTest::GTest GTest::Main ledge::ledge)

add_executable(test_ledge_tracking test_ledge_tracking.cpp fixture.h)
target_link_libraries(test_ledge_tracking GTest::GTest GTest::Main ledge::ledge)

add_executable(test_ledge_manager test_ledge_manager.cpp fixture.h)
target_link_libraries(test_ledge_manager GTest::GTest GTest::Main ledge::ledge)