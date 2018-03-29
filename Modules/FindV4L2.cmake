# - Find V4L2
# Find the native V4L2 includes and library
# This module defines
#  V4L2_INCLUDE_DIR, where to find jpeglib.h, etc.
#  V4L2_FOUND, If false, do not try to use V4L2.

#should we also check for linux/videodev.h?
FIND_PATH(V4L2_INCLUDE_DIR linux/videodev2.h
	/usr/local/include
	/usr/include
)

IF (V4L2_INCLUDE_DIR)
    SET(V4L2_FOUND "YES")
ELSE (V4L2_INCLUDE_DIR)
    SET(V4L2_FOUND "NO")
ENDIF (V4L2_INCLUDE_DIR)


IF (V4L2_FOUND)
   IF (NOT V4L2_FIND_QUIETLY)
      MESSAGE(STATUS "Found V4L: linux/videodev2.h in ${V4L2_INCLUDE_DIR}")
   ENDIF (NOT V4L2_FIND_QUIETLY)
ELSE (V4L2_FOUND)
   IF (V4L2_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "Could not find V4L library")
   ENDIF (V4L2_FIND_REQUIRED)
ENDIF (V4L2_FOUND)

MARK_AS_ADVANCED(
   V4L2_INCLUDE_DIR
)

