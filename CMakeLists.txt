CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

FIND_PACKAGE(PNG)

SET(SRC
    cmdLine.h
    disparity.cpp disparity.h
    filters.cpp
    image.cpp image.h
    io_png.c io_png.h
    main.cpp
    occlusion.cpp occlusion.h)

SET(SRC2
    cmdLine.h
    image.cpp image.h
    io_png.c io_png.h
    main2.cpp
    show_weights.cpp)

ADD_EXECUTABLE(stereoAdaptiveWeights ${SRC})
TARGET_LINK_LIBRARIES(stereoAdaptiveWeights ${PNG_LIBRARIES})

ADD_EXECUTABLE(show_weights ${SRC2})
TARGET_LINK_LIBRARIES(show_weights ${PNG_LIBRARIES})

SET(COMBINE_WEIGHTS mult CACHE STRING "Weights combination (should be mult, plus, min, max or left")
IF(COMBINE_WEIGHTS STREQUAL "mult")
  ADD_DEFINITIONS(-DCOMB_MULT)
ELSEIF(COMBINE_WEIGHTS STREQUAL "plus")
  ADD_DEFINITIONS(-DCOMB_PLUS)
ELSEIF(COMBINE_WEIGHTS STREQUAL "min")
  ADD_DEFINITIONS(-DCOMB_MIN)
ELSEIF(COMBINE_WEIGHTS STREQUAL "max")
  ADD_DEFINITIONS(-DCOMB_MAX)
ELSEIF(COMBINE_WEIGHTS STREQUAL "left")
  ADD_DEFINITIONS(-DCOMB_LEFT)
ELSE(COMBINE_WEIGHTS STREQUAL "mult")
  MESSAGE(SEND_ERROR "Invalid variable COMBINE_WEIGHTS (see allowed values)")
ENDIF(COMBINE_WEIGHTS STREQUAL "mult")

FIND_PACKAGE(OpenMP)
IF(OPENMP_FOUND)
    SET_TARGET_PROPERTIES(stereoAdaptiveWeights PROPERTIES
                          COMPILE_FLAGS ${OpenMP_CXX_FLAGS})
ENDIF(OPENMP_FOUND)

IF(UNIX)
    SET_TARGET_PROPERTIES(stereoAdaptiveWeights PROPERTIES
                          COMPILE_FLAGS "-Wall -Wextra -Werror ${OpenMP_CXX_FLAGS}")
    IF(OPENMP_FOUND)
        SET(CMAKE_EXE_LINKER_FLAGS  ${OpenMP_CXX_FLAGS})
    ENDIF(OPENMP_FOUND)
ENDIF(UNIX)
