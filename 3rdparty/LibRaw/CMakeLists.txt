cmake_minimum_required(VERSION 3.16)
project(libraw)

file(GLOB LIBRAW_SOURCES 
    "src/*.cpp"
    "src/decoders/*.cpp"
    "src/demosaic/*.cpp"
    "src/integration/*.cpp"
    "src/metadata/*.cpp"
    "src/postprocessing/*.cpp"
    "src/preprocessing/*.cpp"
    "src/tables/*.cpp"
    "src/decompressors/*.cpp"
    "src/utils/*.cpp"
    "src/write/*.cpp"
    "src/x3f/*.cpp"
)

# Exclude some files if necessary (e.g. demos)
# But GLOB should be fine for the library itself.
list(REMOVE_ITEM LIBRAW_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/postprocessing/postprocessing_ph.cpp")

add_library(libraw_static STATIC ${LIBRAW_SOURCES})

# Include directories
target_include_directories(libraw_static PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/libraw
)

target_compile_definitions(libraw_static PUBLIC LIBRAW_NODLL)

if(WIN32)
    target_link_libraries(libraw_static ws2_32)
endif()
