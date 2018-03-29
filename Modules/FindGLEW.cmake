# Copyright (c) 2009 Boudewijn Rempt <boud@valdyas.org>                                                                                          
#                                                                                                                                                
# Redistribution and use is allowed according to the terms of the BSD license.                                                                   
# For details see the accompanying COPYING-CMAKE-SCRIPTS file. 
# 
# - try to find glew library and include files
#  GLEW_INCLUDE_DIR, where to find GL/glew.h, etc.
#  GLEW_LIBRARIES, the libraries to link against
#  GLEW_FOUND, If false, do not try to use GLEW.
# Also defined, but not for general use are:
#  GLEW_GLEW_LIBRARY = the full path to the glew library.


FIND_PATH(GLEW_INCLUDE_DIR glew.h
    PATHS
      /usr/include
	  /usr/local/include
      /usr/openwin/share/include
      /usr/openwin/include
      /usr/X11R6/include
      /usr/include/X11
      /opt/graphics/OpenGL/include
      /opt/graphics/OpenGL/contrib/libglew
      /opt/local/include/GL
	PATH_SUFFIXES
	  GL
    )

FIND_LIBRARY( GLEW_LIBRARY NAMES GLEW glew32
    PATHS
      /usr/openwin/lib
	  /usr/local/lib
      /usr/X11R6/lib
      /opt/local/lib
    )
	
IF(GLEW_INCLUDE_DIR AND GLEW_LIBRARY)
    SET( GLEW_FOUND "YES" )
    # Is -lXi and -lXmu required on all platforms that have it?
    # If not, we need some way to figure out what platform we are on.
    SET( GLEW_LIBS ${GLEW_LIBRARY} )
ELSE()
    SET( GLEW_FOUND "NO" )
ENDIF()

IF(GLEW_FOUND)
  IF(NOT GLEW_FIND_QUIETLY)
    MESSAGE(STATUS "Found Glew: ${GLEW_LIBS}")
  ENDIF()
ELSE()
  IF(GLEW_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "Could not find Glew")
  ENDIF()
ENDIF()

MARK_AS_ADVANCED(
  GLEW_INCLUDE_DIR
  GLEW_GLEW_LIBRARY
  GLEW_Xmu_LIBRARY
  GLEW_Xi_LIBRARY
)
