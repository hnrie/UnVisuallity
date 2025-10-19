cmake_minimum_required(VERSION 3.28)
project(lz4)

set(CMAKE_CXX_STANDARD 23)

# disable and enable shi for performance :P
if(MSVC)
    add_compile_options(
            #/MT # not supported by our mapper atm... :(
            /MP
            /cgthreads8
            /arch:AVX2
    )
endif()

add_library(${PROJECT_NAME} STATIC lz4.c
	lz4.h
	lz4file.c
	lz4file.h
	lz4frame.c
	lz4frame.h
	lz4frame_static.h
	lz4hc.c
	lz4hc.h)
