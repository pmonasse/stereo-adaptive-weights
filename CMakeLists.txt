cmake_minimum_required(VERSION 2.6)

find_package(PNG)

set(SRC
    cmdLine.h
    disparity.cpp disparity.h
    filters.cpp
    image.cpp image.h
    io_png.c io_png.h
    main.cpp
    occlusion.cpp occlusion.h)

set(SRC2
    cmdLine.h
    image.cpp image.h
    io_png.c io_png.h
    main2.cpp
    show_weights.cpp)

add_executable(stereoAdaptiveWeights ${SRC})
target_link_libraries(stereoAdaptiveWeights ${PNG_LIBRARIES})

add_executable(show_weights ${SRC2})
target_link_libraries(show_weights ${PNG_LIBRARIES})

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
        set(CMAKE_EXE_LINKER_FLAGS  ${OpenMP_CXX_FLAGS})
    endif(OPENMP_FOUND)
endif(UNIX)
