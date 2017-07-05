cmake_minimum_required(VERSION 2.8)

find_package(PNG)
find_package(TIFF)

if(NOT PNG_FOUND OR NOT TIFF_FOUND)
    find_package(ZLIB) # zlib is needed by libPNG and by libTIFF
    if(NOT ZLIB_FOUND)
        add_subdirectory(third_party/zlib-1.2.11)
        set(ZLIB_FOUND TRUE)
    endif()
    include_directories(${ZLIB_INCLUDE_DIRS})

    if(NOT PNG_FOUND)
        set(SKIP_INSTALL_ALL 1)
        add_subdirectory(third_party/libpng-1.6.30)
    endif()

    if(NOT TIFF_FOUND)
        find_package(JPEG)
        if(NOT JPEG_FOUND)
            add_subdirectory(third_party/jpeg-9b)
            set(JPEG_FOUND TRUE)
        endif()
        include_directories(${JPEG_INCLUDE_DIR})
        add_subdirectory(third_party/tiff-4.0.8)
    endif()
endif()

add_definitions(${PNG_DEFINITIONS})
include_directories(${PNG_INCLUDE_DIRS})
include_directories(${TIFF_INCLUDE_DIR})

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
    if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
        set(CMAKE_EXE_LINKER_FLAGS ${OpenMP_CXX_FLAGS})
    endif(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
endif(OPENMP_FOUND)

if(UNIX)
    set_target_properties(stereoAdaptiveWeights PROPERTIES
                          COMPILE_FLAGS "-Wall -Wextra -Werror ${OpenMP_CXX_FLAGS}")
    set_target_properties(show_weights PROPERTIES
                          COMPILE_FLAGS "-Wall -Wextra -Werror")
    set_target_properties(tiff2png PROPERTIES
                          COMPILE_FLAGS "-Wall -Wextra -Werror")
endif(UNIX)
if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS)
endif(MSVC)
