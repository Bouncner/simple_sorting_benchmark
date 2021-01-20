cmake_minimum_required(VERSION 3.10)

project(SortBenchmark)

set(CMAKE_CXX_STANDARD 17)

FIND_PACKAGE( Boost 1.40 COMPONENTS program_options REQUIRED )
INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(CMAKE_BUILD_TYPE "Release")

add_executable(multi_client_sort_benchmark multi_client_sort_benchmark.cpp)

TARGET_LINK_LIBRARIES( multi_client_sort_benchmark LINK_PUBLIC ${Boost_LIBRARIES} )
TARGET_LINK_LIBRARIES(multi_client_sort_benchmark PRIVATE Threads::Threads)
