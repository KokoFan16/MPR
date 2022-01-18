
MESSAGE("Preparing external ZFP library.")
#add_subdirectory(zfp)
SET(ZFP_DIR "${CMAKE_SOURCE_DIR}/external/zfp")
SET(ZFP_INCLUDE_DIR  "${ZFP_DIR}/include" PARENT_SCOPE)

INCLUDE_DIRECTORIES("${ZFP_DIR}/include")
FILE(GLOB ZFP_SRC ${ZFP_DIR}/include/*.h ${ZFP_DIR}/src/*.c)

ADD_LIBRARY(zfp "${ZFP_SRC}")

INSTALL(DIRECTORY "${ZFP_DIR}/include" DESTINATION ./)
INSTALL(TARGETS zfp DESTINATION lib)