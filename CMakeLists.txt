cmake_minimum_required(VERSION 3.4)

project(lodge CXX)

SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O3 -Wall -g -ggdb -mrtm -msse4.1 -mavx2")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall -mrtm -msse4.1 -mavx2")

set(CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -std=c++11  -pthread -lpmem -lpmemobj" )

add_executable(btree test/main.cpp src/btree.h src/mempool.cc src/nvm-common.cc src/nodepref.h src/betree_single.h  src/betree_single.cpp)
add_executable(lodge test/lodge_test.cpp src/lodge.h src/mempool.cc src/nvm-common.cc src/nodepref.h src/betree_single.h src/status.h  src/betree_single.cpp)
add_executable(betree test/test.cpp src/mempool.cc src/mempool.cc src/nvm-common.cc src/nodepref.h src/betree_single.h  src/betree_single.cpp)
add_executable(workload test/workload.cpp)

target_include_directories(btree PUBLIC ${LIBPMEMOBJ_INCLUDE_DIRS} . .. "third_party")
target_include_directories(betree PUBLIC ${LIBPMEMOBJ_INCLUDE_DIRS} . .. "third_party")
target_include_directories(lodge PUBLIC ${LIBPMEMOBJ_INCLUDE_DIRS} . .. "third_party")


target_link_libraries(btree pmem pmemobj pthread ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(betree pmem pmemobj pthread ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(lodge pmem pmemobj pthread ${CMAKE_THREAD_LIBS_INIT})