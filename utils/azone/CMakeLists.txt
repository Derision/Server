CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

IF(NOT MSVC)
    ADD_DEFINITIONS(-fpermissive)
ENDIF(NOT MSVC)

#azone common
SET(azone_common_sources
    dat.cpp
    file.cpp
    global.cpp
    pfs.cpp
    ter.cpp
    wld.cpp
    zon.cpp
    zonv4.cpp
)

SET(azone_common_headers
    3d.hpp
    3d_base.hpp
    archive.hpp
    dat.hpp
    file.hpp
    file_loader.hpp
    global.hpp
    octree.hpp
    pfs.hpp
    s3d.h
    ter.hpp
    types.h
    wld.hpp
    wld_structs.hpp
    zon.hpp
    zonv4.hpp
)

ADD_LIBRARY(azone_common ${azone_common_sources} ${azone_common_headers})
TARGET_LINK_LIBRARIES(azone_common ${ZLIB_LIBRARY})
SET(LIBRARY_OUTPUT_PATH ../../Bin)

#azone2
SET(azone2_sources
    azone2.cpp
)

SET(azone2_headers
    azone2.h
)

ADD_EXECUTABLE(azone2 ${azone2_sources} ${azone2_headers})
TARGET_LINK_LIBRARIES(azone2 azone_common)
IF(MSVC)
    SET_TARGET_PROPERTIES(azone2 PROPERTIES LINK_FLAGS_RELEASE "/OPT:REF /OPT:ICF")
    TARGET_LINK_LIBRARIES(azone2 "Ws2_32.lib")
ENDIF(MSVC)

IF(UNIX)
    TARGET_LINK_LIBRARIES(azone2 "z")
ENDIF(UNIX)

SET(EXECUTABLE_OUTPUT_PATH ../../Bin)

#azone3
SET(azone3_sources
    azone3.cpp
    ../../zone/RaycastMesh.cpp
)

SET(azone3_headers
    azone3.h
)

ADD_EXECUTABLE(azone3 ${azone3_sources} ${azone3_headers})
TARGET_LINK_LIBRARIES(azone3 azone_common)
IF(MSVC)
    SET_TARGET_PROPERTIES(azone3 PROPERTIES LINK_FLAGS_RELEASE "/OPT:REF /OPT:ICF")
    TARGET_LINK_LIBRARIES(azone3 "Ws2_32.lib")
ENDIF(MSVC)

IF(UNIX)
    TARGET_LINK_LIBRARIES(azone3 "z")
ENDIF(UNIX)

SET(EXECUTABLE_OUTPUT_PATH ../../Bin)

#awater
SET(awater_sources
    awater.cpp
)

SET(awater_headers
    awater.h
)

ADD_EXECUTABLE(awater ${awater_sources} ${awater_headers})
TARGET_LINK_LIBRARIES(awater azone_common debug ${MySQL_LIBRARY_DEBUG} optimized ${MySQL_LIBRARY_RELEASE})
IF(MSVC)
    SET_TARGET_PROPERTIES(awater PROPERTIES LINK_FLAGS_RELEASE "/OPT:REF /OPT:ICF")
    TARGET_LINK_LIBRARIES(awater "Ws2_32.lib")
ENDIF(MSVC)

IF(UNIX)
    TARGET_LINK_LIBRARIES(awater "z")
ENDIF(UNIX)

SET(EXECUTABLE_OUTPUT_PATH ../../Bin)

#listobj
SET(listobj_sources
    listobj.cpp
)

SET(listobj_headers
)

ADD_EXECUTABLE(listobj ${listobj_sources} ${listobj_headers})
TARGET_LINK_LIBRARIES(listobj azone_common debug ${MySQL_LIBRARY_DEBUG} optimized ${MySQL_LIBRARY_RELEASE})
IF(MSVC)
    SET_TARGET_PROPERTIES(listobj PROPERTIES LINK_FLAGS_RELEASE "/OPT:REF /OPT:ICF")
    TARGET_LINK_LIBRARIES(listobj "Ws2_32.lib")
ENDIF(MSVC)

IF(UNIX)
    TARGET_LINK_LIBRARIES(listobj "z")
ENDIF(UNIX)

#on windows: glmodelviewer
IF(MSVC)
    SET(glmodelviewer_sources
        GLModelViewer.cpp
    )
    
    SET(glmodelviewer_headers
        GLModelViewer.h
    )
    
    ADD_EXECUTABLE(glmodelviewer WIN32 ${glmodelviewer_sources} ${glmodelviewer_headers})
    TARGET_LINK_LIBRARIES(glmodelviewer azone_common debug ${MySQL_LIBRARY_DEBUG} optimized ${MySQL_LIBRARY_RELEASE} "opengl32.lib" "GLU32.lib" "Ws2_32.lib")
    SET_TARGET_PROPERTIES(glmodelviewer PROPERTIES LINK_FLAGS_RELEASE "/OPT:REF /OPT:ICF")
    
    SET(EXECUTABLE_OUTPUT_PATH ../../Bin)
ENDIF(MSVC)
