cmake_minimum_required(VERSION 3.20)
project(analyze CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(analyze
  main.cpp
  world_wrapper.cpp
  ../external/World/tools/audioio.cpp
)
target_link_libraries(analyze PRIVATE world util)
target_include_directories(analyze PRIVATE
  ../external/World/src
)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -stdlib=libc++")
    target_compile_options(analyze PRIVATE
        -Wall -Weverything
        -Wno-c++98-compat -Wno-padded -Wno-exit-time-destructors -Wno-global-constructors
        -Wno-sign-conversion
)
endif()
