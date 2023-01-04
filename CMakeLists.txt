cmake_minimum_required(VERSION 3.17)
project(libav_tutorial)

# set out directory
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# set ffmpeg root directory
if(NOT FFMPEG_DEV_ROOT)
    message(FATAL_ERROR "set FFMPEG_DEV_ROOT to use ffmpeg libraries")
endif()

# set ffmpeg develop environment
include_directories(${FFMPEG_DEV_ROOT}/include)
link_directories(${FFMPEG_DEV_ROOT}/lib)
link_libraries(
    avcodec
    avformat
    avfilter
    avdevice
    swresample
    swscale
    avutil
)

# copy dlls 
file(GLOB ffmpeg_shared_libries ${FFMPEG_DEV_ROOT}/bin/*dll)
file(COPY ${ffmpeg_shared_libries} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

# copy test file
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/small_bunny_1080p_60fps.mp4 DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})


# add library
set(debug_src ${CMAKE_CURRENT_SOURCE_DIR}/video_debugging.c)
add_library(video_debug ${debug_src})
link_libraries(video_debug)

# add project/executables
file(GLOB srcs *.c)
list(REMOVE_ITEM srcs ${debug_src})
foreach(src  ${srcs})
    get_filename_component(TARGET ${src} NAME)
    add_executable(${TARGET} ${src})
    message(STATUS "${TARGET} added")
endforeach()


