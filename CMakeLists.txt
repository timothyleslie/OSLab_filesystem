cmake_minimum_required(VERSION 3.10)

include_directories(./include)

aux_source_directory(./src DIR_SRCS)
add_executable(main ${DIR_SRCS})
target_link_libraries(main)

SET(EXECUTABLE_OUTPUT_PATH ../src)