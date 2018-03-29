# - Find oscpack
# Find the native RQUEUE includes and library
# This module defines
#  RQUEUE_INCLUDE_DIR, where to find jpeglib.h, etc.
#  RQUEUE_LIBRARIES, the libraries needed to use RQUEUE.
#  RQUEUE_FOUND, If false, do not try to use RQUEUE.
# also defined, but not for general use are
#  RQUEUE_LIBRARY, where to find the RQUEUE library.

FIND_PATH(oscpack_INCLUDE_DIR oscpack/osc/OscTypes.h
	/usr/local/include
	/usr/include
)

FIND_LIBRARY(oscpack_LIBRARY
  NAMES oscpack
  )
##  PATHS /usr/lib /usr/local/lib

#message("oscpack: LIB is ${oscpack_LIBRARY}")
#message("oscpack: INC is ${oscpack_INCLUDE_DIR}")

IF (oscpack_LIBRARY AND oscpack_INCLUDE_DIR)
    #SET(oscpack_LIBRARIES ${oscpack_LIBRARY} pthread)
    SET(oscpack_FOUND "YES")
ELSE (oscpack_LIBRARY AND oscpack_INCLUDE_DIR)
  SET(oscpack_FOUND "NO")
ENDIF (oscpack_LIBRARY AND oscpack_INCLUDE_DIR)


IF (oscpack_FOUND)
   IF (NOT oscpack_FIND_QUIETLY)
      MESSAGE(STATUS "Found oscpack: ${oscpack_LIBRARY}")
   ENDIF (NOT oscpack_FIND_QUIETLY)
ELSE (oscpack_FOUND)
   IF (oscpack_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "Could not find oscpack library")
   ENDIF (oscpack_FIND_REQUIRED)
ENDIF (oscpack_FOUND)

# Deprecated declarations.
#SET (NATIVE_oscpack_INCLUDE_PATH ${oscpack_INCLUDE_DIR} )
#GET_FILENAME_COMPONENT (NATIVE_oscpack_LIB_PATH ${oscpack_LIBRARY} PATH)


