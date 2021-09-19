cmake_minimum_required(VERSION 3.10)
project(debugio-example CXX)

if(WIN32)
    set(ADD_LINK_LIBRARIES Advapi32)
else()
    set(ADD_LINK_LIBRARIES pthread rt)
endif()

set(READER_SOURCE reader.cpp)
set(WRITER_SOURCE writer.cpp)

add_executable(example-reader ${READER_SOURCE})
target_link_libraries(example-reader ${ADD_LINK_LIBRARIES})

add_executable(example-writer ${WRITER_SOURCE})
target_link_libraries(example-writer ${ADD_LINK_LIBRARIES})
