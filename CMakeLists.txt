cmake_minimum_required(VERSION 3.5)

project(RayTracer CXX)

option(OMP_DISABLE "Disable OpenMP" OFF)
option(USE_SDL "Use SDL" ON)

# Allow support for C++11
if(CMAKE_VERSION VERSION_LESS "3.1")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
else()
    set(CMAKE_CXX_STANDARD 11)
endif()

if(NOT OMP_DISABLE)
    find_package(OpenMP REQUIRED)
    if(OPENMP_FOUND)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    endif()
endif()

if(USE_SDL)
  find_package(SDL2 REQUIRED)
  add_definitions(-DUSE_SDL)
endif()

###################################################
# Compile Flag Options:
#
# -> UNLIT: whether or not the scene is unlit
# -> BENCHMIN: benchmark easy mode (~1 min)
# -> BENCHMID: benchmark medium mode (~6 min)
# -> BENCHMAX: benchmark hard mode (~15 min)
###################################################
#add_definitions(-DUNLIT)
#add_definitions(-DBENCHMIN)
#add_definitions(-DBENCHMID)
#add_definitions(-DBENCHMAX)

# Set OMP target flags
set(OMP_TARGETS "x86_64-unknown-linux-spark")
set(OMP_OFFLOAD_FLAGS "-omptargets=${OMP_TARGETS}")

option(OMP_OFFLOAD "Offload to accelerator" ON)
if(OMP_OFFLOAD AND OPENMP_FOUND)
    # Override C and CXX compiler flags to enable OpenMP offloading
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OMP_OFFLOAD_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OMP_OFFLOAD_FLAGS}")
endif()

set(SRC_DIR ${PROJECT_SOURCE_DIR})

set(SRC_FILES
  ${SRC_DIR}/main.cpp
  ${SRC_DIR}/maths.cpp
  ${SRC_DIR}/renderer.cpp
)

include_directories(
  ${SRC_DIR}
  ${SDL2_INCLUDE_DIRS}
)

string(STRIP "${SDL2_LIBRARIES}" SDL2_LIBRARIES)

add_executable(raytracer ${SRC_FILES})
target_link_libraries(raytracer m ${SDL2_LIBRARIES})
