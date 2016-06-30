cmake_minimum_required(VERSION 2.8)
project(GeoFenceBeC)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
set (CMAKE_C_FLAGS "--std=gnu11 ${CMAKE_C_FLAGS}")

include(FindBson.cmake)
include(FindMongoC.cmake)
include_directories(${BSON_INCLUDE_DIR})
include_directories(${MONGOC_INCLUDE_DIR})

set(LIBS ${LIBS} ${BSON_LIBRARIES} ${MONGOC_LIBRARIES})

set(SOURCE_FILES main.c database.c database.h location.c location.h)
add_executable(GeoFenceBeC ${SOURCE_FILES})

target_link_libraries(GeoFenceBeC m microhttpd mongoc-1.0 ${LIBS})