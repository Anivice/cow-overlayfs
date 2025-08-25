cmake_minimum_required(VERSION 3.30)

project(cppCowOverlay C CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_C_STANDARD 23)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    add_compile_definitions(DEBUG=1)
    add_compile_definitions(VERBOSE=0)
    set(OPTIMIZERS
            -O0 -g3 -fsanitize=address -fsanitize=undefined -lubsan -lasan)
    add_compile_options(${OPTIMIZERS})
    add_link_options(${OPTIMIZERS})
else ()
    add_compile_definitions(DEBUG=0)
    add_compile_definitions(VERBOSE=0)
    set(OPTIMIZERS
            -O3
#            -march=native
#            -mtune=native
            -flto -fomit-frame-pointer
            -ffast-math -fstrict-aliasing
            -fdata-sections -ffunction-sections
            -D_FORTIFY_SOURCE=2 -fno-stack-protector
            -Wl,-z,relro -Wl,-z,now
            -fno-rtti
    )
    add_compile_options(${OPTIMIZERS})
    add_link_options(${OPTIMIZERS})
endif ()

execute_process(
        COMMAND bash -c "echo -n $(dd if=/dev/random bs=32 count=1 2>/dev/null | sha512sum | head -c 8)"
        OUTPUT_VARIABLE BUILD_ID_PREFIX
)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    set(BUILD_ID_PREFIX "${BUILD_ID_PREFIX}DEBUG")
endif ()

execute_process(
        COMMAND bash -c "echo -n $(date '+%s')h$(date '+%Y%m%d%H%M%S')"
        OUTPUT_VARIABLE BUILD_ID_TIMESTAMP
)

execute_process(
        COMMAND bash -c "echo -n $(git log | head -n 1 | awk '{print $2}' | head -c 12)"
        OUTPUT_VARIABLE BUILD_ID_GIT_HEAD
)

execute_process(
        COMMAND bash -c "echo -n $(date '+%Y-%m-%d %H:%M:%S')"
        OUTPUT_VARIABLE BUILD_TIME
)

message(STATUS "Build on ${BUILD_TIME}, of ID dran${BUILD_ID_PREFIX}time${BUILD_ID_TIMESTAMP}git${BUILD_ID_GIT_HEAD}")
add_compile_definitions(
        BUILD_ID="${BUILD_ID_PREFIX}time${BUILD_ID_TIMESTAMP}git${BUILD_ID_GIT_HEAD}"
        BUILD_TIME="${BUILD_TIME}"
        VERSION="0.0.1"
)

include_directories(src/include)
add_executable(cppCowOverlay            src/main.cpp
        src/debug/log.cpp               src/include/log.hpp
        src/debug/color.cpp             src/include/color.h
        src/debug/error.cpp             src/include/error.h
        src/debug/execute_command.cpp   src/include/execute_command.h
        src/utils/lz4.c                 src/include/lz4.h
        src/utils/configuration.cpp     src/include/configuration.h
        src/utils/rstring.cpp
        src/include/rstring.h
        src/include/layer_info.h
        src/blocks/block.cpp
        src/include/block.h
)
