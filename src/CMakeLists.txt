cmake_minimum_required(VERSION 3.4)
project(btree CXX)
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O3 -Wall -lefence -g -ggdb")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")
set(CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)

# set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -pthread -lpmem -lpmemobj" )



add_executable(btree main.cpp btree.h spinlock.h mempool.cc nvm-common.cc nodepref.h Bitmap.h betree_single.h  betree_single.cpp)
# target_include_directories(btree PUBLIC ${LIBPMEMOBJ_INCLUDE_DIRS} . .. "third_party")


# target_link_libraries(btree pmem pmemobj ${CMAKE_THREAD_LIBS_INIT})
