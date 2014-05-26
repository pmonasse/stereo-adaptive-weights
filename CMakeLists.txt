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

SET(DATA
    data/tsukuba0.png data/tsukuba1.png
    data/disparity.png
    data/disparity_postprocessed.png)

SET(CPACK_PACKAGE_FILE_NAME "stereoAdaptiveWeights")
SET(CPACK_GENERATOR "TGZ")
INCLUDE(CPack)
INSTALL(FILES CMakeLists.txt ${SRC} main2.cpp show_weights.cpp README.txt license.txt
        DESTINATION .)
INSTALL(FILES ${DATA}
        DESTINATION ./data)
