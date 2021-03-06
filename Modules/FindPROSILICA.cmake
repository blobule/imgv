# - Find PROSILICA
# Find the native PROSILICA (Vimba drivers) includes and library
# This module defines
#  PROSILICA_INCLUDE_DIR, where to find jpeglib.h, etc.
#  PROSILICA_LIBRARIES, the libraries needed to use PROSILICA.
#  PROSILICA_FOUND, If false, do not try to use PROSILICA.
# also defined, but not for general use are
#  PROSILICA_LIBRARY, where to find the PROSILICA library.

FIND_PATH(PROSILICA_INCLUDE_VIMBA VimbaC.h
    PATHS /usr/local/include /usr/local PATH_SUFFIXES VimbaC/Include/)

FIND_PATH(PROSILICA_INCLUDE_IMAGE VmbTransform.h
    PATHS /usr/local/include /usr/local PATH_SUFFIXES VimbaImageTransform/Include/)

FIND_LIBRARY(PROSILICA_LIBRARY_VIMBA VimbaC)
FIND_LIBRARY(PROSILICA_LIBRARY_IMAGE VimbaImageTransform)

IF (PROSILICA_LIBRARY_VIMBA AND PROSILICA_LIBRARY_IMAGE AND PROSILICA_INCLUDE_VIMBA AND PROSILICA_INCLUDE_IMAGE)
  SET(PROSILICA_LIBS ${PROSILICA_LIBRARY_VIMBA} ${PROSILICA_LIBRARY_IMAGE})
  SET(PROSILICA_INCLUDE_DIR ${PROSILICA_INCLUDE_VIMBA} ${PROSILICA_INCLUDE_IMAGE})
  SET(PROSILICA_FOUND "YES")
ELSE()
  SET(PROSILICA_FOUND "NO")
ENDIF()

IF (PROSILICA_FOUND)
   IF (NOT PROSILICA_FIND_QUIETLY)
      MESSAGE(STATUS "Found PROSILICA: ${PROSILICA_INCLUDE_VIMBA}")
   ENDIF (NOT PROSILICA_FIND_QUIETLY)
ELSE (PROSILICA_FOUND)
   IF (PROSILICA_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "Could not find PROSILICA library")
   ENDIF (PROSILICA_FIND_REQUIRED)
ENDIF (PROSILICA_FOUND)

MARK_AS_ADVANCED(
  PROSILICA_LIBRARY_VIMBA
  PROSILICA_LIBRARY_IMAGE
  PROSILICA_INCLUDE_DIR
  )

