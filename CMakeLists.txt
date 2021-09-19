cmake_minimum_required(VERSION 3.10)
project(debugio CXX)

enable_testing()
#SET (CMAKE_VERBOSE_MAKEFILE 1)

set(CMAKE_CXX_STANDARD 20)

set(EXAMPLE_READER_SOURCES example/reader.cpp)
set(EXAMPLE_WRITER_SOURCES example/writer.cpp)

if(WIN32)
    set(EXAMPLE_READER_SOURCES
        ${EXAMPLE_READER_SOURCES}
        windows/debugio_impl.cpp
    )
    set(EXAMPLE_WRITER_SOURCES
        ${EXAMPLE_READER_SOURCES}
        windows/debugio_impl.cpp
    )
else()
    set(ADD_LINK_LIBRARIES pthread rt)
    set(EXAMPLE_READER_SOURCES
        ${EXAMPLE_READER_SOURCES}
        linux/debugio_impl.cpp
    )
    set(EXAMPLE_WRITER_SOURCES
        ${EXAMPLE_READER_SOURCES}
        linux/debugio_impl.cpp
    )
endif()

add_executable(example_reader ${EXAMPLE_READER_SOURCES})
target_link_libraries(example_reader ${EXAMPLE_READER_LIBRARIES})

add_executable(example_writer ${EXAMPLE_WRITER_SOURCES})
target_link_libraries(example_writer ${EXAMPLE_WRITER_LIBRARIES})
