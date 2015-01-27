cmake_minimum_required(VERSION 2.6)

# zlib is needed by libPNG and also by libTIFF, even though the cmake module for
# libTIFF does not specify that
find_package(ZLIB)
if(NOT ZLIB_FOUND)
  add_subdirectory(third_party/zlib-1.2.8)
endif(NOT ZLIB_FOUND)
include_directories(${ZLIB_INCLUDE_DIRS})

find_package(PNG)
find_package(TIFF)

if(NOT PNG_FOUND)
    add_subdirectory(third_party/libpng-1.6.16)
    include_directories(${PNG_INCLUDE_DIRS})
endif(NOT PNG_FOUND)

if(NOT TIFF_FOUND)
    find_package(JPEG)
    if(NOT JPEG_FOUND)
        add_subdirectory(third_party/jpeg-9a)
    endif(NOT JPEG_FOUND)
    include_directories(${JPEG_INCLUDE_DIR})
    add_subdirectory(third_party/tiff-4.0.3)
    include_directories(${TIFF_INCLUDE_DIR})
endif(NOT TIFF_FOUND)

set(SRC
    cmdLine.h
    disparity.cpp disparity.h
    filters.cpp
    image.cpp image.h
    io_png.c io_png.h
    io_tiff.c io_tiff.h
    main.cpp
    nan.h
    occlusion.cpp occlusion.h)

set(SRC2
    cmdLine.h
    image.cpp image.h
    io_png.c io_png.h
    io_tiff.c io_tiff.h
    nan.h
    show_weights.cpp)

set(SRC3
    cmdLine.h
    io_png.c io_png.h
    io_tiff.c io_tiff.h
    nan.h
    tiff2png.cpp)

add_executable(stereoAdaptiveWeights ${SRC})
target_link_libraries(stereoAdaptiveWeights ${TIFF_LIBRARIES} ${PNG_LIBRARIES})

add_executable(show_weights ${SRC2})
target_link_libraries(show_weights ${TIFF_LIBRARIES} ${PNG_LIBRARIES})

add_executable(tiff2png ${SRC3})
target_link_libraries(tiff2png ${TIFF_LIBRARIES} ${PNG_LIBRARIES})

set(COMBINE_WEIGHTS mult CACHE STRING
    "Weights combination (should be mult, plus, min, max or left")
add_definitions(-DCOMB="${COMBINE_WEIGHTS}")
if(    COMBINE_WEIGHTS STREQUAL "left")
  add_definitions(-DCOMB_LEFT)
elseif(COMBINE_WEIGHTS STREQUAL "max")
  add_definitions(-DCOMB_MAX)
elseif(COMBINE_WEIGHTS STREQUAL "min")
  add_definitions(-DCOMB_MIN)
elseif(COMBINE_WEIGHTS STREQUAL "mult")
  add_definitions(-DCOMB_MULT)
elseif(COMBINE_WEIGHTS STREQUAL "plus")
  add_definitions(-DCOMB_PLUS)
else(  COMBINE_WEIGHTS STREQUAL "left")
  message(SEND_ERROR "Invalid variable COMBINE_WEIGHTS (see allowed values)")
endif( COMBINE_WEIGHTS STREQUAL "left")

find_package(OpenMP)
if(OPENMP_FOUND)
    set_target_properties(stereoAdaptiveWeights PROPERTIES
                          COMPILE_FLAGS ${OpenMP_CXX_FLAGS})
endif(OPENMP_FOUND)

if(UNIX)
    set_target_properties(stereoAdaptiveWeights PROPERTIES
                          COMPILE_FLAGS "-Wall -Wextra -Werror ${OpenMP_CXX_FLAGS}")
    if(OPENMP_FOUND)
        set(CMAKE_EXE_LINKER_FLAGS ${OpenMP_CXX_FLAGS})
    endif(OPENMP_FOUND)
    set_target_properties(show_weights PROPERTIES
                          COMPILE_FLAGS "-Wall -Wextra -Werror")
    set_target_properties(tiff2png PROPERTIES
                          COMPILE_FLAGS "-Wall -Wextra -Werror")
endif(UNIX)
if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS)
endif(MSVC)
