
MESSAGE("Preparing external ZFP library.")
#add_subdirectory(zfp)
SET(ZFP_DIR "${CMAKE_SOURCE_DIR}/external/zfp")
SET(ZFP_INCLUDE_DIR  "${ZFP_DIR}/include" PARENT_SCOPE)

INCLUDE_DIRECTORIES("${ZFP_DIR}/include")
FILE(GLOB ZFP_SRC ${ZFP_DIR}/include/*.h ${ZFP_DIR}/src/*.c)

ADD_LIBRARY(zfp "${ZFP_SRC}")

INSTALL(DIRECTORY "${ZFP_DIR}/include" DESTINATION ./)
INSTALL(TARGETS zfp DESTINATION lib)


MESSAGE("Preparing external VIS library.")
#add_subdirectory(profiler)
SET(VIS_DIR "${CMAKE_SOURCE_DIR}/external/profiler")
SET(VIS_INCLUDE_DIR  "${VIS_DIR}/src" PARENT_SCOPE)

INCLUDE_DIRECTORIES("${VIS_DIR}/include")
FILE(GLOB VIS_SRC ${VIS_DIR}/src/*.hpp ${VIS_DIR}/src/*.cpp)

ADD_LIBRARY(profiler "${VIS_SRC}")

INSTALL(DIRECTORY "${VIS_DIR}/src" DESTINATION ./)
INSTALL(TARGETS profiler DESTINATION lib)

