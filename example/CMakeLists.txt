cmake_minimum_required(VERSION 3.10)
project(debugio-example CXX)

if(WIN32)
    set(ADD_LINK_LIBRARIES Advapi32)
else()
    set(ADD_LINK_LIBRARIES pthread rt)
endif()

set(READER_SOURCE dcat.cpp)
set(WRITER_SOURCE dtime.cpp)

add_executable(dcat ${READER_SOURCE})
target_link_libraries(dcat ${ADD_LINK_LIBRARIES})

add_executable(dtime ${WRITER_SOURCE})
target_link_libraries(dtime ${ADD_LINK_LIBRARIES})
